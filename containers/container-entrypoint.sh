#!/bin/sh
set -eu

source_directory=/workspace/source
operation=
preset=container-release-contracts
test_regex=
test_label=
build_profile=
sanitizer=none
codegen_mode=OFF
aggregate=ExhaustiveArtifacts
consumer_scope=none
artifact_root="/workspace/out/${SIMDLIB_COMPILER_ID:-unknown}"
fingerprint_sha256=

## @brief Prints the supported container operation arguments.
print_usage()
{
	cat <<'EOF'
Usage: simdlib-container --operation OPERATION [options]
  --operation NAME       build-validation, test, test-compiler-contracts, record-codegen,
                         build-benchmarks, run-benchmarks, or inspect-environment
  --preset NAME          Owning CMake configure preset
  --test-regex REGEX     Run only matching CTest tests during test
  --test-label REGEX     Run only matching CTest labels during test
  --build-profile NAME   Release or Debug; must agree with the selected preset
  --sanitizer MODE       none or asan-ubsan
  --codegen-mode MODE    OFF, ENFORCE, or RECORD
  --aggregate NAME       Scoped CMake aggregate owned by this operation
  --consumer-scope SCOPE none or compiler-release
  --artifact-root PATH   Writable compiler-specific artifact root
  --fingerprint-sha256   Full SHA256 of the canonical build-cell fingerprint
  --help                 Show this help
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--operation) operation=$2; shift 2 ;;
		--preset) preset=$2; shift 2 ;;
		--test-regex) test_regex=$2; shift 2 ;;
		--test-label) test_label=$2; shift 2 ;;
		--build-profile) build_profile=$2; shift 2 ;;
		--sanitizer) sanitizer=$2; shift 2 ;;
		--codegen-mode) codegen_mode=$2; shift 2 ;;
		--aggregate) aggregate=$2; shift 2 ;;
		--consumer-scope) consumer_scope=$2; shift 2 ;;
		--artifact-root) artifact_root=$2; shift 2 ;;
		--fingerprint-sha256) fingerprint_sha256=$2; shift 2 ;;
		--help) print_usage; exit 0 ;;
		*) echo "Unknown argument: $1" >&2; print_usage >&2; exit 2 ;;
	esac
done

case "$operation" in
	build-validation|test|test-compiler-contracts|record-codegen|build-benchmarks|run-benchmarks|inspect-environment) ;;
	*) echo "A supported --operation is required: ${operation:-<missing>}" >&2; exit 2 ;;
esac
case "$artifact_root" in
	/workspace/out/*) ;;
	*) echo "Artifact root must be below /workspace/out: $artifact_root" >&2; exit 2 ;;
esac
case "$fingerprint_sha256" in
	*[!0-9a-f]*|'') echo "A lowercase 64-character --fingerprint-sha256 is required" >&2; exit 2 ;;
esac
[ "${#fingerprint_sha256}" -eq 64 ] || {
	echo "A lowercase 64-character --fingerprint-sha256 is required" >&2
	exit 2
}
fingerprint_prefix=$(printf '%s' "$fingerprint_sha256" | cut -c 1-16)
case "${artifact_root##*/}" in
	*-$fingerprint_prefix) ;;
	*) echo "Artifact root does not match fingerprint prefix: $artifact_root" >&2; exit 2 ;;
esac
case "$sanitizer" in
	none|asan-ubsan) ;;
	*) echo "Unsupported sanitizer mode: $sanitizer" >&2; exit 2 ;;
esac
case "$codegen_mode" in
	OFF|ENFORCE|RECORD) ;;
	*) echo "Unsupported codegen mode: $codegen_mode" >&2; exit 2 ;;
esac
case "$aggregate" in
	ExhaustiveArtifacts|SimdLibCompilerContractArtifacts|SimdLibDebugDiagnosticArtifacts) ;;
	*) echo "Unsupported scoped aggregate: $aggregate" >&2; exit 2 ;;
esac
case "$consumer_scope" in
	none|compiler-release) ;;
	*) echo "Unsupported consumer scope: $consumer_scope" >&2; exit 2 ;;
esac
if [ "$operation" = record-codegen ] && [ "$codegen_mode" != RECORD ]; then
	echo "The record-codegen operation requires --codegen-mode RECORD" >&2
	exit 2
fi
case "$preset" in
	*debug*|*asan-ubsan-codegen-diagnostic) expected_build_profile=Debug ;;
	*) expected_build_profile=Release ;;
esac
[ -n "$build_profile" ] || build_profile=$expected_build_profile
[ "$build_profile" = "$expected_build_profile" ] || {
	echo "Build profile $build_profile does not match preset $preset ($expected_build_profile)" >&2
	exit 2
}
[ "$consumer_scope" != compiler-release ] || [ "$build_profile" = Release ] || {
	echo "Compiler Release consumer scope requires a Release profile" >&2
	exit 2
}
case "$operation" in
	build-benchmarks|run-benchmarks)
		[ "$build_profile" = Release ] || {
			echo "Benchmark operations require a Release fingerprint: $preset" >&2
			exit 2
		}
		;;
esac

build_directory="$artifact_root/build"
consumer_directory="$artifact_root/consumer"
report_directory="$artifact_root/reports"
provenance_directory="$artifact_root/provenance"
fingerprint_document="$provenance_directory/fingerprint.json"
validation_manifest="$provenance_directory/validation-build.manifest"
benchmark_manifest="$provenance_directory/benchmark-build.manifest"
main_inventory="$provenance_directory/main-test-artifacts.inventory"
consumer_inventory="$provenance_directory/consumer-test-artifacts.inventory"
codegen_record_index="$provenance_directory/codegen-records.index"
target_inventory="$build_directory/development-profile-targets.txt"
codegen_diagnostic_provenance="$provenance_directory/codegen-diagnostic.json"
mkdir -p "$report_directory" "$provenance_directory"
[ -f "$fingerprint_document" ] || {
	echo "Canonical fingerprint document is missing: $fingerprint_document" >&2
	exit 2
}
[ "$(sha256sum "$fingerprint_document" | cut -d ' ' -f 1)" = "$fingerprint_sha256" ] || {
	echo "Canonical fingerprint document does not match --fingerprint-sha256" >&2
	exit 2
}

## @brief Runs a test-only operation under process tracing and rejects build processes.
run_traced_test_operation()
{
	trace_temporary="$report_directory/test-only.execve.trace.tmp"
	trace_file="$report_directory/test-only.execve.trace"
	rm -f "$trace_temporary"
	set -- --operation "$operation" --preset "$preset" --build-profile "$build_profile" \
		--sanitizer "$sanitizer" --artifact-root "$artifact_root" \
		--codegen-mode "$codegen_mode" --aggregate "$aggregate" \
		--consumer-scope "$consumer_scope" \
		--fingerprint-sha256 "$fingerprint_sha256"
	[ -z "$test_regex" ] || set -- "$@" --test-regex "$test_regex"
	[ -z "$test_label" ] || set -- "$@" --test-label "$test_label"
	set +e
	strace -f -qq -e trace=execve -o "$trace_temporary" \
		env SIMDLIB_TEST_TRACE_ACTIVE=1 "$0" "$@"
	test_status=$?
	set -e
	if grep -E 'execve\("([^"]*/)?cmake(\.exe)?", \[[^]]*"(--build|--preset)"' \
		"$trace_temporary" >/dev/null ||
		grep -E 'execve\("([^"]*/)?cmake(\.exe)?", \[[^]]*"-S", "/workspace/source"' \
			"$trace_temporary" >/dev/null ||
		grep -E 'execve\("([^"]*/)?(ninja|make|msbuild)(\.exe)?"' "$trace_temporary" |
			grep -v -- '"--version"' >/dev/null; then
		echo "Test-only process trace contains an artifact-tree configure or build invocation" >&2
		test_status=5
	fi
	mv "$trace_temporary" "$trace_file"
	exit "$test_status"
}

# LeakSanitizer refuses to execute under ptrace. Sanitizer cells retain the same
# inner test-only operation without tracing; ordinary cells own the trace gate.
if { [ "$operation" = test ] || [ "$operation" = test-compiler-contracts ]; } &&
	[ -z "${SIMDLIB_TEST_TRACE_ACTIVE:-}" ] &&
	[ "$sanitizer" != asan-ubsan ]; then
	run_traced_test_operation
fi

## @brief Computes a stable digest of source inputs that affect configured artifacts.
compute_source_digest()
{
	{
		for source_file in CMakeLists.txt CMakePresets.json compose.yml .clang-format; do
			[ ! -f "$source_directory/$source_file" ] || printf '%s\n' "$source_directory/$source_file"
		done
		for source_tree in include cmake tests examples benchmarks containers tools; do
			[ ! -d "$source_directory/$source_tree" ] ||
				find "$source_directory/$source_tree" -type f
		done
	} | LC_ALL=C sort | while IFS= read -r source_file; do
		relative_file=${source_file#"$source_directory/"}
		printf '%s\0' "$relative_file"
		printf '%s\n' "$(sha256sum "$source_file" | cut -d ' ' -f 1)"
	done | sha256sum | cut -d ' ' -f 1
}

## @brief Returns the runtime CPU features required by the selected fingerprint.
required_cpu_features()
{
	case "$preset" in
		*release-contracts) printf '%s\n' sse4_2 ;;
		*release-exhaustive|*debug-diagnostics|*debug-asan-ubsan)
			printf '%s\n' "sse4_2 avx2 fma bmi1 bmi2"
			;;
		*) printf '%s\n' "" ;;
	esac
}

## @brief Validates every required host CPU feature with an exact diagnostic.
validate_cpu_features()
{
	cpuinfo_file=${SIMDLIB_CPUINFO_PATH:-/proc/cpuinfo}
	[ -r "$cpuinfo_file" ] || {
		echo "Host CPU feature inventory is unavailable: $cpuinfo_file" >&2
		exit 4
	}
	flags=" $(sed -n 's/^flags[[:space:]]*: //p' "$cpuinfo_file" | head -n 1) "
	for required_flag in $(required_cpu_features); do
		case "$flags" in
			*" $required_flag "*) ;;
			*) echo "Host CPU does not expose required flag: $required_flag" >&2; exit 4 ;;
		esac
	done
}

## @brief Reads one exact key from an owned build manifest.
manifest_value()
{
	manifest_file=$1
	manifest_key=$2
	sed -n "s/^${manifest_key}=//p" "$manifest_file"
}

## @brief Verifies shared toolchain and compiler invariants.
validate_environment()
{
	command -v "$CXX" >/dev/null 2>&1 || {
		echo "Configured C++ compiler is unavailable: $CXX" >&2
		exit 3
	}
	case "$($CXX -dumpversion)" in
		13.*|14.*|22.*) ;;
		*) echo "Unexpected compiler version from $CXX: $($CXX -dumpfullversion -dumpversion)" >&2; exit 3 ;;
	esac
	test "$(cmake --version | sed -n '1s/.* //p')" = 4.4.0 || {
		echo "Container requires exactly CMake 4.4.0" >&2
		exit 3
	}
}

## @brief Writes shared compiler, image, host, and operation provenance.
write_provenance()
{
	provenance_file="$provenance_directory/environment.txt"
	{
		echo "compiler_id=${SIMDLIB_COMPILER_ID:-unknown}"
		echo "operation=$operation"
		echo "build_profile=$build_profile"
		echo "preset=$preset"
		echo "sanitizer=$sanitizer"
		echo "codegen_mode=$codegen_mode"
		echo "aggregate=$aggregate"
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
}

## @brief Runs a command into a report while preserving and displaying its failure.
run_reported()
{
	report_file=$1
	shift
	if "$@" >"$report_file" 2>&1; then
		cat "$report_file"
	else
		command_status=$?
		cat "$report_file" >&2
		return "$command_status"
	fi
}

## @brief Configures the owning main-project tree, applying CI freshness only here.
configure_main_project()
{
	export SIMDLIB_BUILD_DIRECTORY="$build_directory"
	cxx_flags=${SIMDLIB_REQUIRED_CXX_FLAGS:-}
	linker_flags=${SIMDLIB_REQUIRED_LINKER_FLAGS:-}
	set -- --preset "$preset" -S "$source_directory" \
		-DFETCHCONTENT_SOURCE_DIR_CATCH2="$SIMDLIB_CATCH2_SOURCE" \
		-DCMAKE_CXX_FLAGS="$cxx_flags" \
		-DCMAKE_EXE_LINKER_FLAGS="$linker_flags"
	for ci_indicator in \
		"${CI:-}" "${GITHUB_ACTIONS:-}" "${GITLAB_CI:-}" "${TF_BUILD:-}" \
		"${BUILDKITE:-}" "${CIRCLECI:-}" "${JENKINS_URL:-}" "${TEAMCITY_VERSION:-}"
	do
		[ -z "$ci_indicator" ] || {
			set -- --fresh "$@"
			break
		}
	done
	run_reported "$report_directory/main-configure.log" cmake "$@"
}

## @brief Resolves the concrete consumer inventory owned by this compiler cell.
resolve_external_consumer_scope()
{
	[ "$consumer_scope" != none ] || {
		printf '%s\n' none
		return
	}
	capability_file="$build_directory/external-consumer-targets.txt"
	[ -f "$capability_file" ] || {
		echo "External-consumer capability inventory is missing: $capability_file" >&2
		exit 6
	}
	consumer_targets=$(LC_ALL=C sort -u "$capability_file" | tr '\n' '|')
	case "$consumer_targets" in
		CoreConsumerSmoke\|) printf '%s\n' core ;;
		CoreConsumerSmoke\|RegisterConsumerSmoke\|)
			printf '%s\n' core-register
			;;
		*)
			echo "Unsupported external-consumer capability inventory: $consumer_targets" >&2
			exit 6
			;;
	esac
}

## @brief Configures and builds the assigned external-consumer tree.
build_external_consumer()
{
	concrete_scope=$1
	cxx_flags=${SIMDLIB_REQUIRED_CXX_FLAGS:-}
	linker_flags=${SIMDLIB_REQUIRED_LINKER_FLAGS:-}
	register_consumer=OFF
	[ "$concrete_scope" != core-register ] || register_consumer=ON
	run_reported "$report_directory/consumer-configure.log" cmake \
		-S "$source_directory/tests/consumer" -B "$consumer_directory" -G Ninja \
		-DCMAKE_BUILD_TYPE="$build_profile" \
		-DSIMDLIB_SOURCE_DIR="$source_directory" \
		-DSIMDLIB_BUILD_REGISTER_CONSUMER="$register_consumer" \
		-DCMAKE_CXX_FLAGS="$cxx_flags" \
		-DCMAKE_EXE_LINKER_FLAGS="$linker_flags"
	run_reported "$report_directory/consumer-build.log" \
		cmake --build "$consumer_directory" --parallel
}

## @brief Records the built CTest executables for pre-test staleness checks.
record_test_inventory()
{
	test_directory=$1
	inventory_file=$2
	cmake -DMODE=RECORD \
		-DTEST_DIRECTORY="$test_directory" \
		-DINVENTORY_FILE="$inventory_file" \
		-DCMAKE_CTEST_COMMAND="$(command -v ctest)" \
		-P "$source_directory/cmake/RecordTestInventory.cmake"
}

## @brief Writes the aggregate generated-code record index from CMake-owned indexes.
write_codegen_record_index()
{
	{
		for owner_index in \
			"$build_directory/method-flags-codegen/all-records.txt" \
			"$build_directory/register-codegen/sse42/128/all-records.txt" \
			"$build_directory/register-codegen/avx2/128/all-records.txt" \
			"$build_directory/register-codegen/avx2/256/all-records.txt"; do
			[ ! -f "$owner_index" ] || cat "$owner_index"
		done
	} | sed '/^[[:space:]]*$/d' | LC_ALL=C sort -u >"$codegen_record_index"
	if [ "$codegen_mode" != OFF ] && [ ! -s "$codegen_record_index" ]; then
		echo "No CMake-owned generated-code records were found under $build_directory" >&2
		exit 6
	fi
}

## @brief Validates a recorded CTest executable inventory before running tests.
validate_test_inventory()
{
	test_directory=$1
	inventory_file=$2
	cmake -DMODE=VALIDATE \
		-DTEST_DIRECTORY="$test_directory" \
		-DINVENTORY_FILE="$inventory_file" \
		-DCMAKE_CTEST_COMMAND="$(command -v ctest)" \
		-P "$source_directory/cmake/RecordTestInventory.cmake"
}

## @brief Verifies mandatory runtime-test labels and families before execution.
audit_runtime_test_inventory()
{
	register_required=ON
	[ "${SIMDLIB_COMPILER_ID:-}" != gcc13 ] || register_required=OFF
	cmake -DTEST_DIRECTORY="$build_directory" \
		-DCMAKE_CTEST_COMMAND="$(command -v ctest)" \
		-DAUDIT_FILE="$report_directory/runtime-test-inventory.audit.txt" \
		-DREGISTER_REQUIRED="$register_required" \
		-P "$source_directory/cmake/VerifyRuntimeTestInventory.cmake"
}

## @brief Records an atomic completed-operation manifest after all assigned builds succeed.
write_completed_manifest()
{
	manifest_file=$1
	manifest_operation=$2
	source_digest=$3
	concrete_consumer_scope=$(resolve_external_consumer_scope)
	cache_hash=$(sha256sum "$build_directory/CMakeCache.txt" | cut -d ' ' -f 1)
	source_revision=${SIMDLIB_BUILD_REVISION:-unknown}
	if [ "$source_revision" = unknown ]; then
		source_revision=$(git -C "$source_directory" rev-parse HEAD 2>/dev/null || printf '%s' unknown)
	fi
	manifest_aggregate=$aggregate
	[ "$manifest_operation" != build-benchmarks ] || manifest_aggregate=BenchmarkArtifacts
	target_inventory_hash=none
	main_inventory_hash=none
	consumer_inventory_hash=none
	codegen_record_index_hash=none
	main_ctest_metadata_hash=none
	consumer_ctest_metadata_hash=none
	[ ! -f "$target_inventory" ] ||
		target_inventory_hash=$(sha256sum "$target_inventory" | cut -d ' ' -f 1)
	[ ! -f "$main_inventory" ] ||
		main_inventory_hash=$(sha256sum "$main_inventory" | cut -d ' ' -f 1)
	[ ! -f "$consumer_inventory" ] ||
		consumer_inventory_hash=$(sha256sum "$consumer_inventory" | cut -d ' ' -f 1)
	[ ! -f "$codegen_record_index" ] ||
		codegen_record_index_hash=$(sha256sum "$codegen_record_index" | cut -d ' ' -f 1)
	[ ! -f "$build_directory/CTestTestfile.cmake" ] ||
		main_ctest_metadata_hash=$(sha256sum "$build_directory/CTestTestfile.cmake" | cut -d ' ' -f 1)
	[ ! -f "$consumer_directory/CTestTestfile.cmake" ] ||
		consumer_ctest_metadata_hash=$(sha256sum "$consumer_directory/CTestTestfile.cmake" | cut -d ' ' -f 1)
	temporary_manifest="${manifest_file}.tmp"
	rm -f "$manifest_file" "$temporary_manifest"
	{
		echo "schema=simdlib.build-manifest.v1"
		echo "operation=$manifest_operation"
		echo "status=complete"
		echo "source_revision=$source_revision"
		echo "source_digest=$source_digest"
		echo "fingerprint_sha256=$fingerprint_sha256"
		echo "fingerprint_document=$fingerprint_document"
		echo "compiler_id=${SIMDLIB_COMPILER_ID:-unknown}"
		echo "compiler=$($CXX --version | head -n 1)"
		echo "base_image=${SIMDLIB_BASE_IMAGE:-unknown}"
		echo "preset=$preset"
		echo "build_profile=$build_profile"
		echo "sanitizer=$sanitizer"
		echo "codegen_mode=$codegen_mode"
		echo "aggregate=$manifest_aggregate"
		echo "consumer_owner=$consumer_scope"
		echo "target_inventory=$target_inventory"
		echo "target_inventory_sha256=$target_inventory_hash"
		echo "consumer_scope=$concrete_consumer_scope"
		echo "build_directory=$build_directory"
		echo "consumer_directory=$consumer_directory"
		echo "cmake_cache_sha256=$cache_hash"
		echo "required_cpu_features=$(required_cpu_features | tr ' ' ',')"
		echo "main_test_inventory=$main_inventory"
		echo "main_test_inventory_sha256=$main_inventory_hash"
		echo "main_ctest_metadata_sha256=$main_ctest_metadata_hash"
		echo "consumer_test_inventory=$consumer_inventory"
		echo "consumer_test_inventory_sha256=$consumer_inventory_hash"
		echo "consumer_ctest_metadata_sha256=$consumer_ctest_metadata_hash"
		echo "codegen_record_index=$codegen_record_index"
		echo "codegen_record_index_sha256=$codegen_record_index_hash"
	} >"$temporary_manifest"
	mv "$temporary_manifest" "$manifest_file"
}

## @brief Validates the owning completed build and all pre-test artifacts.
validate_validation_manifest()
{
	[ -f "$validation_manifest" ] || {
		echo "Required validation build manifest is missing: $validation_manifest" >&2
		exit 6
	}
	[ "$(manifest_value "$validation_manifest" schema)" = simdlib.build-manifest.v1 ] &&
		[ "$(manifest_value "$validation_manifest" operation)" = build-validation ] &&
		[ "$(manifest_value "$validation_manifest" status)" = complete ] ||
		{
			echo "Validation build manifest is incomplete or incompatible: $validation_manifest" >&2
			exit 6
		}
	[ "$(manifest_value "$validation_manifest" preset)" = "$preset" ] &&
		[ "$(manifest_value "$validation_manifest" fingerprint_sha256)" = "$fingerprint_sha256" ] &&
		[ "$(manifest_value "$validation_manifest" fingerprint_document)" = "$fingerprint_document" ] &&
		[ "$(manifest_value "$validation_manifest" build_profile)" = "$build_profile" ] &&
		[ "$(manifest_value "$validation_manifest" sanitizer)" = "$sanitizer" ] &&
		[ "$(manifest_value "$validation_manifest" codegen_mode)" = "$codegen_mode" ] &&
		[ "$(manifest_value "$validation_manifest" aggregate)" = "$aggregate" ] &&
		[ "$(manifest_value "$validation_manifest" consumer_owner)" = "$consumer_scope" ] &&
		[ "$(manifest_value "$validation_manifest" compiler_id)" = "${SIMDLIB_COMPILER_ID:-unknown}" ] &&
		[ "$(manifest_value "$validation_manifest" base_image)" = "${SIMDLIB_BASE_IMAGE:-unknown}" ] ||
		{
			echo "Validation build manifest does not match the requested fingerprint: $validation_manifest" >&2
			exit 6
		}
	[ -f "$build_directory/CMakeCache.txt" ] || {
		echo "Required CMake cache is missing: $build_directory/CMakeCache.txt" >&2
		exit 6
	}
	current_source_digest=$(compute_source_digest)
	[ "$(manifest_value "$validation_manifest" source_digest)" = "$current_source_digest" ] || {
		echo "Validation build manifest is stale for the current source inputs: $validation_manifest" >&2
		exit 6
	}
	current_cache_hash=$(sha256sum "$build_directory/CMakeCache.txt" | cut -d ' ' -f 1)
	[ "$(manifest_value "$validation_manifest" cmake_cache_sha256)" = "$current_cache_hash" ] || {
		echo "Validation build manifest is stale for the current CMake cache: $validation_manifest" >&2
		exit 6
	}
	concrete_consumer_scope=$(resolve_external_consumer_scope)
	[ "$(manifest_value "$validation_manifest" consumer_scope)" = "$concrete_consumer_scope" ] || {
		echo "Validation consumer scope does not match compiler capabilities" >&2
		exit 6
	}
	[ "$(manifest_value "$validation_manifest" target_inventory_sha256)" = \
		"$(sha256sum "$target_inventory" | cut -d ' ' -f 1)" ] &&
		[ "$(manifest_value "$validation_manifest" main_test_inventory_sha256)" = \
		"$(sha256sum "$main_inventory" | cut -d ' ' -f 1)" ] &&
		[ "$(manifest_value "$validation_manifest" consumer_test_inventory_sha256)" = \
			"$(sha256sum "$consumer_inventory" | cut -d ' ' -f 1)" ] &&
		[ "$(manifest_value "$validation_manifest" codegen_record_index_sha256)" = \
			"$(sha256sum "$codegen_record_index" | cut -d ' ' -f 1)" ] ||
		{
			echo "Validation artifact indexes are missing or stale: $validation_manifest" >&2
			exit 6
		}
	[ "$(manifest_value "$validation_manifest" main_ctest_metadata_sha256)" = \
		"$(sha256sum "$build_directory/CTestTestfile.cmake" | cut -d ' ' -f 1)" ] || {
		echo "Generated main CTest metadata is missing or stale: $validation_manifest" >&2
		exit 6
	}
	validate_test_inventory "$build_directory" "$main_inventory"
	if [ "$concrete_consumer_scope" = none ]; then
		[ "$(manifest_value "$validation_manifest" consumer_ctest_metadata_sha256)" = none ] &&
			[ ! -f "$consumer_directory/CTestTestfile.cmake" ] &&
			[ ! -s "$consumer_inventory" ] || {
				echo "Consumer-free cell contains external-consumer artifacts" >&2
				exit 6
			}
	else
		[ "$(manifest_value "$validation_manifest" consumer_ctest_metadata_sha256)" = \
			"$(sha256sum "$consumer_directory/CTestTestfile.cmake" | cut -d ' ' -f 1)" ] || {
			echo "Generated consumer CTest metadata is missing or stale" >&2
			exit 6
		}
		validate_test_inventory "$consumer_directory" "$consumer_inventory"
	fi
	cmake -DRECORD_INDEX="$codegen_record_index" \
		-P "$source_directory/cmake/ValidateCodegenRecords.cmake"
}

## @brief Validates the completed benchmark build without rebuilding it.
validate_benchmark_manifest()
{
	[ -f "$benchmark_manifest" ] || {
		echo "Required benchmark build manifest is missing: $benchmark_manifest" >&2
		exit 6
	}
	[ "$(manifest_value "$benchmark_manifest" operation)" = build-benchmarks ] &&
		[ "$(manifest_value "$benchmark_manifest" status)" = complete ] &&
		[ "$(manifest_value "$benchmark_manifest" preset)" = "$preset" ] &&
		[ "$(manifest_value "$benchmark_manifest" fingerprint_sha256)" = "$fingerprint_sha256" ] &&
		[ "$(manifest_value "$benchmark_manifest" fingerprint_document)" = "$fingerprint_document" ] &&
		[ "$(manifest_value "$benchmark_manifest" build_profile)" = "$build_profile" ] &&
		[ "$(manifest_value "$benchmark_manifest" sanitizer)" = "$sanitizer" ] &&
		[ "$(manifest_value "$benchmark_manifest" aggregate)" = BenchmarkArtifacts ] &&
		[ "$(manifest_value "$benchmark_manifest" target_inventory_sha256)" = \
			"$(sha256sum "$target_inventory" | cut -d ' ' -f 1)" ] &&
		[ "$(manifest_value "$benchmark_manifest" compiler_id)" = "${SIMDLIB_COMPILER_ID:-unknown}" ] &&
		[ "$(manifest_value "$benchmark_manifest" base_image)" = "${SIMDLIB_BASE_IMAGE:-unknown}" ] ||
		{
			echo "Benchmark build manifest is incomplete: $benchmark_manifest" >&2
			exit 6
		}
	[ "$(manifest_value "$benchmark_manifest" source_digest)" = "$(compute_source_digest)" ] || {
		echo "Benchmark build manifest is stale for the current source inputs: $benchmark_manifest" >&2
		exit 6
	}
	[ "$(manifest_value "$benchmark_manifest" cmake_cache_sha256)" = \
		"$(sha256sum "$build_directory/CMakeCache.txt" | cut -d ' ' -f 1)" ] || {
		echo "Benchmark build manifest is stale for the current CMake cache: $benchmark_manifest" >&2
		exit 6
	}
	[ -x "$build_directory/Benchmarks" ] || {
		echo "Required benchmark executable is missing: $build_directory/Benchmarks" >&2
		exit 6
	}
}

## @brief Reports whether the existing owning tree matches the completed validation build.
can_reuse_validation_configuration()
{
	[ -f "$validation_manifest" ] &&
		[ -f "$build_directory/CMakeCache.txt" ] &&
		[ "$(manifest_value "$validation_manifest" schema)" = simdlib.build-manifest.v1 ] &&
		[ "$(manifest_value "$validation_manifest" operation)" = build-validation ] &&
		[ "$(manifest_value "$validation_manifest" status)" = complete ] &&
		[ "$(manifest_value "$validation_manifest" fingerprint_sha256)" = "$fingerprint_sha256" ] &&
		[ "$(manifest_value "$validation_manifest" fingerprint_document)" = "$fingerprint_document" ] &&
		[ "$(manifest_value "$validation_manifest" preset)" = "$preset" ] &&
		[ "$(manifest_value "$validation_manifest" build_profile)" = "$build_profile" ] &&
		[ "$(manifest_value "$validation_manifest" sanitizer)" = "$sanitizer" ] &&
		[ "$(manifest_value "$validation_manifest" codegen_mode)" = "$codegen_mode" ] &&
		[ "$(manifest_value "$validation_manifest" aggregate)" = "$aggregate" ] &&
		[ "$(manifest_value "$validation_manifest" consumer_owner)" = "$consumer_scope" ] &&
		[ "$(manifest_value "$validation_manifest" compiler_id)" = "${SIMDLIB_COMPILER_ID:-unknown}" ] &&
		[ "$(manifest_value "$validation_manifest" base_image)" = "${SIMDLIB_BASE_IMAGE:-unknown}" ] &&
		[ "$(manifest_value "$validation_manifest" source_digest)" = "$(compute_source_digest)" ] &&
		[ "$(manifest_value "$validation_manifest" cmake_cache_sha256)" = \
			"$(sha256sum "$build_directory/CMakeCache.txt" | cut -d ' ' -f 1)" ]
}

validate_environment
write_provenance
[ "$operation" != inspect-environment ] || exit 0

case "$operation" in
	build-validation)
		rm -f "$validation_manifest"
		source_digest=$(compute_source_digest)
		configure_main_project
		run_reported "$report_directory/main-build.log" \
			cmake --build "$build_directory" --parallel --target "$aggregate"
		concrete_consumer_scope=$(resolve_external_consumer_scope)
		if [ "$concrete_consumer_scope" != none ]; then
			build_external_consumer "$concrete_consumer_scope"
		elif [ -e "$consumer_directory" ]; then
			echo "Consumer-free cell contains an external-consumer tree" >&2
			exit 6
		fi
		record_test_inventory "$build_directory" "$main_inventory"
		if [ "$concrete_consumer_scope" = none ]; then
			: >"$consumer_inventory"
		else
			record_test_inventory "$consumer_directory" "$consumer_inventory"
		fi
		write_codegen_record_index
		write_completed_manifest "$validation_manifest" build-validation "$source_digest"
		;;
	record-codegen)
		source_digest=$(compute_source_digest)
		configure_main_project
		compilation_started=$(date +%s)
		run_reported "$report_directory/codegen-compilation.log" \
			cmake --build "$build_directory" --parallel --target RegisterCodegenFixtureObjects
		compilation_finished=$(date +%s)
		comparison_started=$(date +%s)
		run_reported "$report_directory/codegen-comparison.log" \
			cmake --build "$build_directory" --parallel --target SimdLibDebugDiagnosticArtifacts
		comparison_finished=$(date +%s)
		compilation_seconds=$((compilation_finished - compilation_started))
		comparison_seconds=$((comparison_finished - comparison_started))
		write_codegen_record_index
		cmake -DRECORD_INDEX="$codegen_record_index" \
			-DEXPECTED_POLICY_MODE=RECORD \
			-DEXPECTED_CONFIGURATION=Debug \
			-DREQUIRE_RECORDS=ON \
			-P "$source_directory/cmake/ValidateCodegenRecords.cmake"
		cmake -DBINARY_DIRECTORY="$build_directory" \
			-DOWNERSHIP_FILE="$build_directory/development-target-ownership.tsv" \
			-DPROFILE=CODEGEN_DIAGNOSTIC -DCODEGEN_MODE=RECORD \
			-P "$source_directory/cmake/VerifyCodegenProfileIsolation.cmake"
		cmake -DRECORD_INDEX="$codegen_record_index" \
			-DOUTPUT_FILE="$codegen_diagnostic_provenance" \
			-DCOMPILE_COMMANDS="$build_directory/compile_commands.json" \
			-DSOURCE_REVISION="${SIMDLIB_BUILD_REVISION:-unknown}" \
			-DSOURCE_DIGEST="$source_digest" \
			-DFINGERPRINT="$fingerprint_sha256" \
			-DCOMPILER_ID="${SIMDLIB_COMPILER_ID:-unknown}" \
			-DPRESET="$preset" -DCONFIGURATION="$build_profile" \
			-DSANITIZER="$sanitizer" \
			-DCOMPILATION_SECONDS="$compilation_seconds" \
			-DCOMPARISON_SECONDS="$comparison_seconds" \
			-P "$source_directory/cmake/SummarizeCodegenDiagnostic.cmake"
		printf 'Codegen diagnostic provenance: %s\n' "$codegen_diagnostic_provenance"
		;;
	build-benchmarks)
		rm -f "$benchmark_manifest"
		source_digest=$(compute_source_digest)
		if ! can_reuse_validation_configuration; then
			echo "Benchmark build requires a current validated Release configuration: $validation_manifest" >&2
			exit 6
		fi
		printf 'Reusing validated Release configuration: %s\n' "$build_directory" |
			tee "$report_directory/benchmark-configure.log"
		run_reported "$report_directory/benchmark-build.log" \
			cmake --build "$build_directory" --parallel --target BenchmarkArtifacts
		write_completed_manifest "$benchmark_manifest" build-benchmarks "$source_digest"
		;;
	test-compiler-contracts)
		validate_validation_manifest
		set -- --test-dir "$build_directory" --output-on-failure \
			--output-junit "$report_directory/compiler-contract-tests.xml"
		[ -z "$test_regex" ] || set -- "$@" --tests-regex "$test_regex"
		[ -z "$test_label" ] || set -- "$@" --label-regex "$test_label"
		ctest "$@"
		;;
	test)
		validate_validation_manifest
		validate_cpu_features
		audit_runtime_test_inventory
		set -- --test-dir "$build_directory" --output-on-failure \
			--output-junit "$report_directory/main-test.xml"
		[ -z "$test_regex" ] || set -- "$@" --tests-regex "$test_regex"
		[ -z "$test_label" ] || set -- "$@" --label-regex "$test_label"
		ctest "$@"
		if [ "$(manifest_value "$validation_manifest" consumer_scope)" != none ]; then
			ctest --test-dir "$consumer_directory" --output-on-failure \
				--output-junit "$report_directory/consumer-test.xml"
		fi
		;;
	run-benchmarks)
		validate_benchmark_manifest
		validate_cpu_features
		run_reported "$report_directory/benchmark-execution.txt" \
			"$build_directory/Benchmarks" '[simdlib][benchmark]' --benchmark-samples 25
		;;
esac
