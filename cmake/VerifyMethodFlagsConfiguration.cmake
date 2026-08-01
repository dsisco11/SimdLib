cmake_minimum_required(VERSION 3.31)

foreach(required_variable IN ITEMS
	SIMDLIB_METHOD_FLAGS_COMPILER
	SIMDLIB_METHOD_FLAGS_COMPILER_ID
	SIMDLIB_METHOD_FLAGS_MSVC_STYLE
	SIMDLIB_METHOD_FLAGS_SOURCE_DIR
	SIMDLIB_METHOD_FLAGS_BINARY_DIR)
	if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
		message(FATAL_ERROR "${required_variable} is required")
	endif()
endforeach()

set(probe_directory "${SIMDLIB_METHOD_FLAGS_BINARY_DIR}/method-flags-configuration")
file(MAKE_DIRECTORY "${probe_directory}")
set(probe_source "${probe_directory}/MethodFlagsConfigurationProbe.cpp")
set(actual_output "${probe_directory}/MethodFlagsConfigurationActual.txt")
set(method_flags_compiler_options)
if(DEFINED SIMDLIB_METHOD_FLAGS_COMPILER_OPTIONS
	AND NOT "${SIMDLIB_METHOD_FLAGS_COMPILER_OPTIONS}" STREQUAL "")
	separate_arguments(method_flags_compiler_options NATIVE_COMMAND
		"${SIMDLIB_METHOD_FLAGS_COMPILER_OPTIONS}")
endif()

file(WRITE "${probe_source}"
	"#define SIMDLIB_METHOD_FLAGS_HAS_VECTORCALL 1\n"
	"#define SIMDLIB_METHOD_FLAGS_HAS_SAFE_BUFFERS 1\n"
	"#define SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE 1\n"
	"#define SIMDLIB_METHOD_FLAGS_HAS_FLATTEN 1\n"
	"#define SIMDLIB_METHOD_FLAGS_VECTORCALL SIMDLIB_CONFIG_VECTORCALL\n"
	"#define SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS SIMDLIB_CONFIG_SAFE_BUFFERS\n"
	"#define SIMDLIB_METHOD_FLAGS_FORCE_INLINE SIMDLIB_CONFIG_FORCE_INLINE\n"
	"#define SIMDLIB_METHOD_FLAGS_FLATTEN SIMDLIB_CONFIG_FLATTEN\n"
	"#define SIMDLIB_PRECONDITION(condition, message)\n"
	"#include <SimdLib/Config.h>\n"
	"SIMDLIB_CONFIG_CAP_VECTORCALL SIMDLIB_METHOD_FLAGS_HAS_VECTORCALL\n"
	"SIMDLIB_CONFIG_CAP_SAFE_BUFFERS SIMDLIB_METHOD_FLAGS_HAS_SAFE_BUFFERS\n"
	"SIMDLIB_CONFIG_CAP_FORCE_INLINE SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE\n"
	"SIMDLIB_CONFIG_CAP_FLATTEN SIMDLIB_METHOD_FLAGS_HAS_FLATTEN\n"
	"SIMDLIB_CONFIG_CASE_NEITHER SIMD_FLAGS(Neither)\n"
	"SIMDLIB_CONFIG_CASE_IN SIMD_FLAGS(In)\n"
	"SIMDLIB_CONFIG_CASE_OUT SIMD_FLAGS(Out)\n"
	"SIMDLIB_CONFIG_CASE_INOUT SIMD_FLAGS(InOut)\n"
	"SIMDLIB_CONFIG_CASE_REGISTER_ONLY SIMD_FLAGS(Neither, RegisterOnly)\n"
	"SIMDLIB_CONFIG_CASE_FORCE_INLINE SIMD_FLAGS(Neither, ForceInline)\n"
	"SIMDLIB_CONFIG_CASE_FLATTEN SIMD_FLAGS(Neither, Flatten)\n"
	"SIMDLIB_CONFIG_CASE_ALL SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)\n")

if(SIMDLIB_METHOD_FLAGS_MSVC_STYLE)
	set(preprocess_arguments
		/nologo
		/std:c++20
		${method_flags_compiler_options}
		/EP
		/TP
		"/I${SIMDLIB_METHOD_FLAGS_SOURCE_DIR}/include"
		"${probe_source}")
else()
	set(preprocess_arguments
		-std=c++20
		${method_flags_compiler_options}
		-E
		-P
		-x c++
		"-I${SIMDLIB_METHOD_FLAGS_SOURCE_DIR}/include"
		"${probe_source}")
endif()

execute_process(
	COMMAND "${SIMDLIB_METHOD_FLAGS_COMPILER}" ${preprocess_arguments}
	RESULT_VARIABLE preprocess_result
	OUTPUT_VARIABLE preprocess_stdout
	ERROR_VARIABLE preprocess_stderr)
file(WRITE "${actual_output}" "${preprocess_stdout}")
if(NOT preprocess_result EQUAL 0)
	message(FATAL_ERROR
		"${SIMDLIB_METHOD_FLAGS_COMPILER_ID} configuration preprocessing failed:\n${preprocess_stderr}")
endif()

set(expected_lines
	"SIMDLIB_CONFIG_CAP_VECTORCALL 1"
	"SIMDLIB_CONFIG_CAP_SAFE_BUFFERS 1"
	"SIMDLIB_CONFIG_CAP_FORCE_INLINE 1"
	"SIMDLIB_CONFIG_CAP_FLATTEN 1"
	"SIMDLIB_CONFIG_CASE_NEITHER"
	"SIMDLIB_CONFIG_CASE_IN SIMDLIB_CONFIG_VECTORCALL"
	"SIMDLIB_CONFIG_CASE_OUT SIMDLIB_CONFIG_VECTORCALL"
	"SIMDLIB_CONFIG_CASE_INOUT SIMDLIB_CONFIG_VECTORCALL"
	"SIMDLIB_CONFIG_CASE_REGISTER_ONLY SIMDLIB_CONFIG_SAFE_BUFFERS"
	"SIMDLIB_CONFIG_CASE_FORCE_INLINE SIMDLIB_CONFIG_FORCE_INLINE"
	"SIMDLIB_CONFIG_CASE_FLATTEN SIMDLIB_CONFIG_FLATTEN"
	"SIMDLIB_CONFIG_CASE_ALL SIMDLIB_CONFIG_FLATTEN SIMDLIB_CONFIG_FORCE_INLINE SIMDLIB_CONFIG_SAFE_BUFFERS SIMDLIB_CONFIG_VECTORCALL")

string(REPLACE "\r\n" "\n" preprocess_stdout "${preprocess_stdout}")
string(REPLACE "\r" "\n" preprocess_stdout "${preprocess_stdout}")
string(REGEX MATCHALL "SIMDLIB_CONFIG_(CAP|CASE)_[A-Za-z0-9_]+[^\n]*" actual_lines
	"${preprocess_stdout}")
list(LENGTH expected_lines expected_count)
list(LENGTH actual_lines actual_count)
if(NOT actual_count EQUAL expected_count)
	message(FATAL_ERROR
		"${SIMDLIB_METHOD_FLAGS_COMPILER_ID} produced ${actual_count} configuration markers; expected ${expected_count}. See ${actual_output}")
endif()

math(EXPR final_index "${expected_count} - 1")
foreach(index RANGE 0 ${final_index})
	list(GET expected_lines ${index} expected_line)
	list(GET actual_lines ${index} actual_line)
	string(STRIP "${actual_line}" actual_line)
	string(REGEX REPLACE "[ \t]+" " " actual_line "${actual_line}")
	if(NOT actual_line STREQUAL expected_line)
		message(FATAL_ERROR
			"${SIMDLIB_METHOD_FLAGS_COMPILER_ID} configuration mismatch at marker ${index}:\n"
			"  expected: ${expected_line}\n"
			"  actual:   ${actual_line}\n"
			"See ${actual_output}")
	endif()
endforeach()

message(STATUS
	"${SIMDLIB_METHOD_FLAGS_COMPILER_ID}: verified ${expected_count} public method-flags adapter markers")
