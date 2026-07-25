#!/bin/sh
set -eu

source_directory=/workspace/source
preset=container-release-contracts
build_target=
test_regex=
test_label=
build_profile=
sanitizer=none
artifact_root="/workspace/out/${SIMDLIB_COMPILER_ID:-unknown}"
inspect_environment=0
run_benchmarks=0

## @brief Prints the supported container-runner arguments.
print_usage()
{
	cat <<'EOF'
Usage: simdlib-container [options]
  --preset NAME          CMake configure preset (default: container-release-contracts)
  --build-target NAME    Build only the named target
  --test-regex REGEX     Run only matching CTest tests
  --test-label REGEX     Run only tests with matching labels
  --build-profile NAME   Release or Debug; must agree with the selected preset
  --sanitizer MODE       none or asan-ubsan
  --artifact-root PATH   Writable compiler-specific artifact root
  --inspect-environment  Print provenance and validate the environment only
  --run-benchmarks       Run the runtime-derived Register benchmark after validation
  --help                 Show this help
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--preset) preset=$2; shift 2 ;;
		--build-target) build_target=$2; shift 2 ;;
		--test-regex) test_regex=$2; shift 2 ;;
		--test-label) test_label=$2; shift 2 ;;
		--build-profile) build_profile=$2; shift 2 ;;
		--sanitizer) sanitizer=$2; shift 2 ;;
		--artifact-root) artifact_root=$2; shift 2 ;;
		--inspect-environment) inspect_environment=1; shift ;;
		--run-benchmarks) run_benchmarks=1; shift ;;
		--help) print_usage; exit 0 ;;
		*) echo "Unknown argument: $1" >&2; print_usage >&2; exit 2 ;;
	esac
done

case "$artifact_root" in
	/workspace/out/*) ;;
	*) echo "Artifact root must be below /workspace/out: $artifact_root" >&2; exit 2 ;;
esac

case "$sanitizer" in
	none|asan-ubsan) ;;
	*) echo "Unsupported sanitizer mode: $sanitizer" >&2; exit 2 ;;
esac

case "$preset" in
	*debug*) expected_build_profile=Debug ;;
	*) expected_build_profile=Release ;;
esac
[ -n "$build_profile" ] || build_profile=$expected_build_profile
[ "$build_profile" = "$expected_build_profile" ] || {
	echo "Build profile $build_profile does not match preset $preset ($expected_build_profile)" >&2
	exit 2
}

result_directory="$artifact_root/$preset"
mkdir -p "$result_directory"
provenance_file="$result_directory/provenance.txt"

{
	echo "compiler_id=${SIMDLIB_COMPILER_ID:-unknown}"
	echo "build_profile=$build_profile"
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
	13.*|14.*|22.*) ;;
	*) echo "Unexpected compiler version from $CXX: $($CXX -dumpfullversion -dumpversion)" >&2; exit 3 ;;
esac

test "$(cmake --version | sed -n '1s/.* //p')" = 4.4.0 || {
	echo "Container requires exactly CMake 4.4.0" >&2
	exit 3
}

case "$preset" in
	*release-exhaustive|*debug-diagnostics|*debug-asan-ubsan)
	flags=" $(sed -n 's/^flags[[:space:]]*: //p' /proc/cpuinfo | head -n 1) "
	for required_flag in sse4_2 avx2 fma bmi1 bmi2; do
		case "$flags" in
		*" $required_flag "*) ;;
		*) echo "Host CPU does not expose required flag: $required_flag" >&2; exit 4 ;;
		esac
	done
	;;
esac

[ "$inspect_environment" -eq 0 ] || exit 0

export SIMDLIB_BUILD_ROOT="$artifact_root/build"
build_directory="$SIMDLIB_BUILD_ROOT/$preset"
cxx_flags=${SIMDLIB_REQUIRED_CXX_FLAGS:-}
linker_flags=${SIMDLIB_REQUIRED_LINKER_FLAGS:-}

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

set -- --test-dir "$build_directory" --output-on-failure --output-junit "$result_directory/ctest.xml"
[ -z "$test_regex" ] || set -- "$@" --tests-regex "$test_regex"
[ -z "$test_label" ] || set -- "$@" --label-regex "$test_label"
ctest "$@"

consumer_directory="$artifact_root/consumer/$preset"
register_consumer=ON
[ "${SIMDLIB_COMPILER_ID:-unknown}" != gcc13 ] || register_consumer=OFF
set -- -S "$source_directory/tests/consumer" -B "$consumer_directory" -G Ninja \
	-DCMAKE_BUILD_TYPE="$build_profile" \
	-DSIMDLIB_SOURCE_DIR="$source_directory" \
	-DSIMDLIB_BUILD_REGISTER_CONSUMER="$register_consumer" \
	-DCMAKE_CXX_FLAGS="$cxx_flags" \
	-DCMAKE_EXE_LINKER_FLAGS="$linker_flags"
cmake "$@"
cmake --build "$consumer_directory" --parallel
ctest --test-dir "$consumer_directory" --output-on-failure \
	--output-junit "$result_directory/consumer-ctest.xml"

if [ "$run_benchmarks" -eq 1 ]; then
	"$build_directory/Benchmarks" '[simdlib][benchmark][register]' --benchmark-samples 25
fi
