#!/bin/sh
set -eu

source_directory=/workspace/source
preset=container-full
build_target=
test_regex=
test_label=
configuration=Release
sanitizer=none
output_directory="/workspace/out/${SIMDLIB_COMPILER_ID:-unknown}"
doctor_only=0

## @brief Prints the supported container-runner arguments.
print_usage()
{
	cat <<'EOF'
Usage: simdlib-container [options]
  --preset NAME          CMake configure preset (default: container-full)
  --build-target NAME    Build only the named target
  --test-regex REGEX     Run only matching CTest tests
  --test-label REGEX     Run only tests with matching labels
  --configuration NAME   Build configuration recorded in provenance
  --sanitizer MODE       none or address-undefined
  --output-dir PATH      Writable compiler-specific output directory
  --doctor-only          Print provenance and validate the environment only
  --help                 Show this help
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--preset) preset=$2; shift 2 ;;
		--build-target) build_target=$2; shift 2 ;;
		--test-regex) test_regex=$2; shift 2 ;;
		--test-label) test_label=$2; shift 2 ;;
		--configuration) configuration=$2; shift 2 ;;
		--sanitizer) sanitizer=$2; shift 2 ;;
		--output-dir) output_directory=$2; shift 2 ;;
		--doctor-only) doctor_only=1; shift ;;
		--help) print_usage; exit 0 ;;
		*) echo "Unknown argument: $1" >&2; print_usage >&2; exit 2 ;;
	esac
done

case "$output_directory" in
	/workspace/out/*) ;;
	*) echo "Output directory must be below /workspace/out: $output_directory" >&2; exit 2 ;;
esac

case "$sanitizer" in
	none|address-undefined) ;;
	*) echo "Unsupported sanitizer mode: $sanitizer" >&2; exit 2 ;;
esac

mkdir -p "$output_directory"
provenance_file="$output_directory/provenance.txt"

{
	echo "compiler_id=${SIMDLIB_COMPILER_ID:-unknown}"
	echo "configuration=$configuration"
	echo "preset=$preset"
	echo "sanitizer=$sanitizer"
	echo "base_image=${SIMDLIB_BASE_IMAGE:-unknown}"
	echo "architecture=$(uname -m)"
	echo "os_release=$(tr '\n' ' ' </etc/os-release)"
	echo "compiler=$($CXX --version | head -n 1)"
	echo "cmake=$(cmake --version | head -n 1)"
	echo "ninja=$(ninja --version)"
	echo "libc=$(ldd --version 2>&1 | head -n 1)"
	echo "catch2_commit=2b60af89e23d28eefc081bc930831ee9d45ea58b"
	echo "packages=$(apk info -v 2>/dev/null | sort | tr '\n' ' ')"
	echo "cpu_flags=$(sed -n 's/^flags[[:space:]]*: //p' /proc/cpuinfo | head -n 1)"
} | tee "$provenance_file"

case "$($CXX -dumpversion)" in
	14.*|22.*) ;;
	*) echo "Unexpected compiler version from $CXX: $($CXX -dumpfullversion -dumpversion)" >&2; exit 3 ;;
esac

test "$(cmake --version | sed -n '1s/.* //p')" = 4.4.0 || {
	echo "Container requires exactly CMake 4.4.0" >&2
	exit 3
}

if [ "$preset" = container-full ] || [ "$preset" = container-sanitize ]; then
	flags=" $(sed -n 's/^flags[[:space:]]*: //p' /proc/cpuinfo | head -n 1) "
	for required_flag in sse4_2 avx2 fma bmi1 bmi2; do
		case "$flags" in
		*" $required_flag "*) ;;
		*) echo "Host CPU does not expose required flag: $required_flag" >&2; exit 4 ;;
		esac
	done
fi

[ "$doctor_only" -eq 0 ] || exit 0

export SIMDLIB_BUILD_ROOT="$output_directory/build"
build_directory="$SIMDLIB_BUILD_ROOT/$preset"
cxx_flags=${SIMDLIB_REQUIRED_CXX_FLAGS:-}
linker_flags=${SIMDLIB_REQUIRED_LINKER_FLAGS:-}

if [ "$sanitizer" = address-undefined ]; then
	cxx_flags="${cxx_flags:+$cxx_flags }-fsanitize=address,undefined -fno-omit-frame-pointer"
	linker_flags="${linker_flags:+$linker_flags }-fsanitize=address,undefined"
fi

set -- --preset "$preset" -S "$source_directory" \
	-DFETCHCONTENT_SOURCE_DIR_CATCH2="$SIMDLIB_CATCH2_SOURCE" \
	-DCMAKE_CXX_FLAGS="$cxx_flags" \
	-DCMAKE_EXE_LINKER_FLAGS="$linker_flags"

for ci_indicator in \
	"${CI:-}" \
	"${GITHUB_ACTIONS:-}" \
	"${GITLAB_CI:-}" \
	"${TF_BUILD:-}" \
	"${BUILDKITE:-}" \
	"${CIRCLECI:-}" \
	"${JENKINS_URL:-}" \
	"${TEAMCITY_VERSION:-}"
do
	[ -z "$ci_indicator" ] || {
		set -- --fresh "$@"
		break
	}
done

cmake "$@"

set -- --build "$build_directory" --parallel
[ -z "$build_target" ] || set -- "$@" --target "$build_target"
cmake "$@"

set -- --test-dir "$build_directory" --output-on-failure --output-junit "$output_directory/ctest.xml"
[ -z "$test_regex" ] || set -- "$@" --tests-regex "$test_regex"
[ -z "$test_label" ] || set -- "$@" --label-regex "$test_label"
ctest "$@"

consumer_directory="$output_directory/consumer"
set -- -S "$source_directory/tests/consumer" -B "$consumer_directory" -G Ninja \
	-DCMAKE_BUILD_TYPE="$configuration" \
	-DSIMDLIB_SOURCE_DIR="$source_directory" \
	-DSIMDLIB_BUILD_REGISTER_CONSUMER=ON \
	-DCMAKE_CXX_FLAGS="$cxx_flags" \
	-DCMAKE_EXE_LINKER_FLAGS="$linker_flags"
cmake "$@"
cmake --build "$consumer_directory" --parallel
ctest --test-dir "$consumer_directory" --output-on-failure \
	--output-junit "$output_directory/consumer-ctest.xml"
