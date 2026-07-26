cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
	WRAPPER_OBJECT RAW_OBJECT OBJDUMP ARTIFACT_DIRECTORY COMPILER_ID
	COMPILER_VERSION COMPILER_PATH SYSTEM_NAME SYSTEM_PROCESSOR CONFIGURATION REGISTER_WIDTH
	ISA_PROFILE VECTORCALL_ENABLED STACK_PROTECTOR_MODE)
	if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
		message(FATAL_ERROR "CompareRegisterCodegen requires ${required_variable}")
	endif()
endforeach()
if(NOT DEFINED SYMBOL_PATTERN OR "${SYMBOL_PATTERN}" STREQUAL "")
	set(SYMBOL_PATTERN "simdlib_codegen_")
endif()
if(NOT DEFINED CODEGEN_PROFILE OR "${CODEGEN_PROFILE}" STREQUAL "")
	set(CODEGEN_PROFILE "default")
endif()
if(NOT DEFINED FMA_EXPECTATION OR "${FMA_EXPECTATION}" STREQUAL "")
	set(FMA_EXPECTATION "none")
endif()
if(NOT DEFINED RECORD_ONLY OR "${RECORD_ONLY}" STREQUAL "")
	set(RECORD_ONLY OFF)
endif()
if(NOT DEFINED RECORD_FILE OR "${RECORD_FILE}" STREQUAL "")
	set(RECORD_FILE "${ARTIFACT_DIRECTORY}/comparison.record.json")
endif()
if(NOT FMA_EXPECTATION MATCHES "^(none|enabled|disabled)$")
	message(FATAL_ERROR "Unsupported FMA_EXPECTATION: ${FMA_EXPECTATION}")
endif()
file(REMOVE "${RECORD_FILE}" "${RECORD_FILE}.tmp")

# @brief Escapes a string for inclusion as a JSON string value.
# @param input_text Unescaped text.
# @param output_variable Variable that receives escaped text.
function(simdlib_escape_json input_text output_variable)
	set(escaped "${input_text}")
	string(REPLACE "\\" "\\\\" escaped "${escaped}")
	string(REPLACE "\"" "\\\"" escaped "${escaped}")
	string(REPLACE "\r" "\\r" escaped "${escaped}")
	string(REPLACE "\n" "\\n" escaped "${escaped}")
	string(REPLACE "\t" "\\t" escaped "${escaped}")
	set(${output_variable} "${escaped}" PARENT_SCOPE)
endfunction()

# @brief Disassembles one generated-code fixture object.
# @param object_file Compiled object containing the fixture functions.
# @param output_variable Variable that receives the disassembly.
function(simdlib_disassemble object_file output_variable)
	execute_process(
		COMMAND "${OBJDUMP}" -d "${object_file}"
		RESULT_VARIABLE disassembly_result
		OUTPUT_VARIABLE disassembly
		ERROR_VARIABLE disassembly_error)
	if(NOT disassembly_result EQUAL 0)
		message(FATAL_ERROR "Unable to disassemble ${object_file}: ${disassembly_error}")
	endif()
	set(${output_variable} "${disassembly}" PARENT_SCOPE)
endfunction()

# @brief Removes the exact accepted MSVC from-array security-cookie sequence.
# @param input_text Allocation-independent wrapper instruction profile.
# @param output_variable Variable that receives the comparable wrapper profile.
# @param accepted_variable Variable that reports whether the exact exception was found.
function(simdlib_accept_msvc_from_array_cookie input_text output_variable accepted_variable)
	set(${output_variable} "${input_text}" PARENT_SCOPE)
	set(${accepted_variable} OFF PARENT_SCOPE)
	if(NOT COMPILER_ID STREQUAL "MSVC" OR
		NOT SYSTEM_NAME STREQUAL "Windows" OR
		NOT VECTORCALL_ENABLED STREQUAL "1" OR
		NOT SYMBOL_PATTERN STREQUAL "simdlib_type_matrix_" OR
		NOT REGISTER_WIDTH STREQUAL "128")
		return()
	endif()

	set(cookie_profile [=[<symbol>:
subq	$0x28, %rsp
movq	(%rip), %rax            # 0x<target>
xorq	%rsp, %rax
movq	%rax, 0x10(%rsp)
movq	(%rcx), %rax
movq	%rax, (%rsp)
movq	0x8(%rcx), %rax
movq	%rax, 0x8(%rsp)
vmovdqu	(%rsp), %vreg
movq	0x10(%rsp), %rcx
xorq	%rsp, %rcx
callq	0x<target>
addq	$0x28, %rsp
retq]=])
	set(raw_profile [=[<symbol>:
subq	$0x18, %rsp
movq	(%rcx), %rax
movq	%rax, (%rsp)
movq	0x8(%rcx), %rax
movq	%rax, 0x8(%rsp)
vmovdqu	(%rsp), %vreg
addq	$0x18, %rsp
retq]=])
	if(ISA_PROFILE STREQUAL "SSE42")
		string(REPLACE "vmovdqu" "movdqu" cookie_profile "${cookie_profile}")
		string(REPLACE "vmovdqu" "movdqu" raw_profile "${raw_profile}")
	endif()
	string(FIND "${input_text}" "${cookie_profile}" cookie_index)
	if(cookie_index LESS 0)
		return()
	endif()
	string(LENGTH "${cookie_profile}" cookie_length)
	math(EXPR cookie_tail_index "${cookie_index} + ${cookie_length}")
	string(SUBSTRING "${input_text}" ${cookie_tail_index} -1 cookie_tail)
	string(FIND "${cookie_tail}" "${cookie_profile}" second_cookie_relative_index)
	if(second_cookie_relative_index LESS 0)
		return()
	endif()
	math(EXPR second_cookie_index "${cookie_tail_index} + ${second_cookie_relative_index}")
	math(EXPR second_cookie_tail_index "${second_cookie_index} + ${cookie_length}")
	string(SUBSTRING "${input_text}" 0 ${second_cookie_index} comparable_prefix)
	string(SUBSTRING "${input_text}" ${second_cookie_tail_index} -1 comparable_suffix)
	set(comparable_profile "${comparable_prefix}${raw_profile}${comparable_suffix}")
	set(${output_variable} "${comparable_profile}" PARENT_SCOPE)
	set(${accepted_variable} ON PARENT_SCOPE)
endfunction()

# @brief Removes object identity, instruction addresses, and encoded bytes while retaining instructions.
# @param input_text Raw object disassembly.
# @param output_variable Variable that receives normalized disassembly.
function(simdlib_normalize_disassembly input_text output_variable)
	set(normalized "${input_text}")
	string(REPLACE "\r\n" "\n" normalized "${normalized}")
	string(REPLACE "\n" ";" disassembly_lines "${normalized}")
	set(fixture_only "")
	set(in_fixture OFF)
	foreach(disassembly_line IN LISTS disassembly_lines)
		if(disassembly_line MATCHES "<[^>]*${SYMBOL_PATTERN}[^>]*>:")
			set(in_fixture ON)
			string(APPEND fixture_only "<symbol>:\n")
		elseif(disassembly_line MATCHES "^[ \t]*[0-9A-Fa-f]+[ \t]+<[^>]+>:")
			set(in_fixture OFF)
		elseif(in_fixture AND NOT disassembly_line MATCHES "^Disassembly of section")
			string(APPEND fixture_only "${disassembly_line}\n")
			if(disassembly_line MATCHES "[ \t]ret[qwl]?([ \t]|$)")
				set(in_fixture OFF)
			endif()
		endif()
	endforeach()
	set(normalized "${fixture_only}")
	string(REGEX REPLACE "[^\n]*file format[^\n]*\n" "" normalized "${normalized}")
	string(REGEX REPLACE "(^|\n)[ \t]*[0-9A-Fa-f]+[ \t]+<" "\\1<" normalized "${normalized}")
	string(REGEX REPLACE "(^|\n)[ \t]*[0-9A-Fa-f]+:[ \t]+([0-9A-Fa-f][0-9A-Fa-f][ \t]+)+" "\\1" normalized "${normalized}")
	string(REGEX REPLACE "<[^>]+>" "<symbol>" normalized "${normalized}")
	string(REGEX REPLACE "[0-9A-Fa-f]+[ \t]+<symbol>" "<target>" normalized "${normalized}")
	string(REGEX REPLACE "[ \t]+\n" "\n" normalized "${normalized}")
	string(REGEX REPLACE "\n+" "\n" normalized "${normalized}")
	string(STRIP "${normalized}" normalized)
	set(${output_variable} "${normalized}" PARENT_SCOPE)
endfunction()

# @brief Removes allocator-selected vector-register identities, including names repeated in disassembler comments.
# @param input_text Normalized fixture disassembly.
# @param output_variable Variable that receives the allocation-independent instruction profile.
function(simdlib_profile_disassembly input_text output_variable)
	set(profile "${input_text}")
	string(REGEX REPLACE "%[xyz]mm[0-9]+" "%vreg" profile "${profile}")
	string(REGEX REPLACE "[xyz]mm[0-9]+" "vreg" profile "${profile}")
	set(${output_variable} "${profile}" PARENT_SCOPE)
endfunction()

# @brief Removes the one accepted MSVC scalar-result security-cookie sequence.
# @param input_text Allocation-independent wrapper instruction profile.
# @param output_variable Variable that receives the comparable wrapper profile.
# @param accepted_variable Variable that reports whether the exact exception was found.
function(simdlib_accept_msvc_scalar_cookie input_text output_variable accepted_variable)
	set(${output_variable} "${input_text}" PARENT_SCOPE)
	set(${accepted_variable} OFF PARENT_SCOPE)
	if(NOT COMPILER_ID STREQUAL "MSVC" OR
		NOT SYSTEM_NAME STREQUAL "Windows" OR
		NOT VECTORCALL_ENABLED STREQUAL "1" OR
		NOT SYMBOL_PATTERN STREQUAL "simdlib_codegen_")
		return()
	endif()

	if(REGISTER_WIDTH STREQUAL "128")
		set(cookie_profile [=[<symbol>:
subq	$0x18, %rsp
movq	(%rip), %rax            # 0x<target>
xorq	%rsp, %rax
movq	%rax, (%rsp)
vpmovmskb	%vreg, %eax
movq	(%rsp), %rcx
xorq	%rsp, %rcx
callq	0x<target>
addq	$0x18, %rsp
retq]=])
		set(raw_scalar_profile [=[<symbol>:
vpmovmskb	%vreg, %eax
retq]=])
	elseif(REGISTER_WIDTH STREQUAL "256")
		set(cookie_profile [=[<symbol>:
subq	$0x18, %rsp
movq	(%rip), %rax            # 0x<target>
xorq	%rsp, %rax
movq	%rax, (%rsp)
vpmovmskb	%vreg, %eax
vzeroupper
movq	(%rsp), %rcx
xorq	%rsp, %rcx
callq	0x<target>
addq	$0x18, %rsp
retq]=])
		set(raw_scalar_profile [=[<symbol>:
vpmovmskb	%vreg, %eax
vzeroupper
retq]=])
	else()
		return()
	endif()

	string(FIND "${input_text}" "${cookie_profile}" cookie_index)
	if(cookie_index LESS 0)
		return()
	endif()
	string(LENGTH "${cookie_profile}" cookie_length)
	math(EXPR cookie_tail_index "${cookie_index} + ${cookie_length}")
	string(SUBSTRING "${input_text}" ${cookie_tail_index} -1 cookie_tail)
	string(FIND "${cookie_tail}" "${cookie_profile}" duplicate_cookie_index)
	if(NOT duplicate_cookie_index LESS 0)
		return()
	endif()

	string(REPLACE "${cookie_profile}" "${raw_scalar_profile}" comparable_profile "${input_text}")
	set(${output_variable} "${comparable_profile}" PARENT_SCOPE)
	set(${accepted_variable} ON PARENT_SCOPE)
endfunction()

#[[
The MSVC compound-assignment exception is disabled with the compound-assignment
API. Reassignment avoids the mutable wrapper reference that triggers the
redundant security-cookie and 32-byte stack-alignment frame, so its codegen gate
requires exact parity. The former exception remains here for diagnostic history.
# @brief Removes the one accepted MSVC compound-assignment security-cookie sequence.
# @param input_text Allocation-independent wrapper instruction profile.
# @param output_variable Variable that receives the comparable wrapper profile.
# @param accepted_variable Variable that reports whether the exact exception was found.
function(simdlib_accept_msvc_compound_cookie input_text output_variable accepted_variable)
	set(${output_variable} "${input_text}" PARENT_SCOPE)
	set(${accepted_variable} OFF PARENT_SCOPE)
	if(NOT COMPILER_ID STREQUAL "MSVC" OR
		NOT SYSTEM_NAME STREQUAL "Windows" OR
		NOT VECTORCALL_ENABLED STREQUAL "1" OR
		NOT SYMBOL_PATTERN STREQUAL "simdlib_codegen_compound_arithmetic")
		return()
	endif()

	if(REGISTER_WIDTH STREQUAL "128")
		set(cookie_profile [=[<symbol>:
subq	$0x18, %rsp
movq	(%rip), %rax            # 0x<target>
xorq	%rsp, %rax
movq	%rax, (%rsp)
vaddps	%vreg, %vreg, %vreg
vmulps	%vreg, %vreg, %vreg
movq	(%rsp), %rcx
xorq	%rsp, %rcx
callq	0x<target>
addq	$0x18, %rsp
retq]=])
	elseif(REGISTER_WIDTH STREQUAL "256")
		set(cookie_profile [=[<symbol>:
pushq	%rbp
subq	$0x30, %rsp
leaq	0x20(%rsp), %rbp
andq	$-0x20, %rbp
movq	(%rip), %rax            # 0x<target>
xorq	%rsp, %rax
movq	%rax, (%rbp)
vaddps	%vreg, %vreg, %vreg
vmulps	%vreg, %vreg, %vreg
movq	(%rbp), %rcx
xorq	%rsp, %rcx
callq	0x<target>
addq	$0x30, %rsp
popq	%rbp
retq]=])
	else()
		return()
	endif()
	set(raw_profile [=[<symbol>:
vaddps	%vreg, %vreg, %vreg
vmulps	%vreg, %vreg, %vreg
retq]=])
	if(input_text STREQUAL cookie_profile)
		set(${output_variable} "${raw_profile}" PARENT_SCOPE)
		set(${accepted_variable} ON PARENT_SCOPE)
	endif()
endfunction()
]]

simdlib_disassemble("${WRAPPER_OBJECT}" wrapper_disassembly)
simdlib_disassemble("${RAW_OBJECT}" raw_disassembly)
simdlib_normalize_disassembly("${wrapper_disassembly}" wrapper_normalized)
simdlib_normalize_disassembly("${raw_disassembly}" raw_normalized)
simdlib_profile_disassembly("${wrapper_normalized}" wrapper_profile)
simdlib_profile_disassembly("${raw_normalized}" raw_profile)

string(FIND "${wrapper_profile}" "vfmadd" wrapper_fma_index)
string(FIND "${raw_profile}" "vfmadd" raw_fma_index)
if(NOT RECORD_ONLY AND FMA_EXPECTATION STREQUAL "enabled" AND (wrapper_fma_index LESS 0 OR raw_fma_index LESS 0))
	message(FATAL_ERROR "The FMA-enabled generated-code profile does not contain fused multiply-add instructions")
elseif(NOT RECORD_ONLY AND FMA_EXPECTATION STREQUAL "disabled" AND (NOT wrapper_fma_index LESS 0 OR NOT raw_fma_index LESS 0))
	message(FATAL_ERROR "The FMA-disabled generated-code profile unexpectedly contains fused multiply-add instructions")
endif()

set(comparable_wrapper_profile "${wrapper_profile}")
set(comparison_result "exact-parity")
set(accepted_exception "none")
if(NOT wrapper_profile STREQUAL raw_profile)
	simdlib_accept_msvc_scalar_cookie(
		"${wrapper_profile}" comparable_wrapper_profile accepted_msvc_scalar_cookie)
	if(accepted_msvc_scalar_cookie AND comparable_wrapper_profile STREQUAL raw_profile)
		set(comparison_result "accepted-compiler-exception")
		set(accepted_exception "msvc-gs-scalar-cookie")
	else()
		simdlib_accept_msvc_from_array_cookie(
			"${wrapper_profile}" comparable_wrapper_profile accepted_msvc_from_array_cookie)
		if(accepted_msvc_from_array_cookie AND comparable_wrapper_profile STREQUAL raw_profile)
			set(comparison_result "accepted-compiler-exception")
			set(accepted_exception "msvc-gs-from-array-cookie")
		else()
			set(comparison_result "failed")
		endif()
		#[[
		The compound-assignment exception branch is disabled with the public
		compound-assignment API. Reassignment must satisfy exact parity.
		simdlib_accept_msvc_compound_cookie(
			"${wrapper_profile}" comparable_wrapper_profile accepted_msvc_compound_cookie)
		if(accepted_msvc_compound_cookie AND comparable_wrapper_profile STREQUAL raw_profile)
			set(comparison_result "accepted-compiler-exception")
			set(accepted_exception "msvc-gs-compound-cookie")
		else()
			set(comparison_result "failed")
		endif()
		]]
	endif()
endif()

if(RECORD_ONLY AND comparison_result STREQUAL "failed")
	set(comparison_result "recorded-difference")
	set(accepted_exception "non-release-differential")
endif()

file(WRITE "${ARTIFACT_DIRECTORY}/wrapper.disassembly.txt" "${wrapper_disassembly}")
file(WRITE "${ARTIFACT_DIRECTORY}/raw.disassembly.txt" "${raw_disassembly}")
file(WRITE "${ARTIFACT_DIRECTORY}/wrapper.normalized.txt" "${wrapper_normalized}\n")
file(WRITE "${ARTIFACT_DIRECTORY}/raw.normalized.txt" "${raw_normalized}\n")
file(WRITE "${ARTIFACT_DIRECTORY}/wrapper.profile.txt" "${wrapper_profile}\n")
file(WRITE "${ARTIFACT_DIRECTORY}/raw.profile.txt" "${raw_profile}\n")
file(WRITE "${ARTIFACT_DIRECTORY}/wrapper.comparable.profile.txt" "${comparable_wrapper_profile}\n")
file(WRITE "${ARTIFACT_DIRECTORY}/comparison.txt"
	"result=${comparison_result}\n"
	"accepted_exception=${accepted_exception}\n")
file(WRITE "${ARTIFACT_DIRECTORY}/provenance.txt"
	"compiler_id=${COMPILER_ID}\n"
	"compiler_version=${COMPILER_VERSION}\n"
	"compiler_path=${COMPILER_PATH}\n"
	"system_name=${SYSTEM_NAME}\n"
	"system_processor=${SYSTEM_PROCESSOR}\n"
	"configuration=${CONFIGURATION}\n"
	"register_width=${REGISTER_WIDTH}\n"
	"isa_profile=${ISA_PROFILE}\n"
	"vectorcall_enabled=${VECTORCALL_ENABLED}\n"
	"stack_protector_mode=${STACK_PROTECTOR_MODE}\n"
	"codegen_profile=${CODEGEN_PROFILE}\n"
	"fma_expectation=${FMA_EXPECTATION}\n"
	"record_only=${RECORD_ONLY}\n"
	"comparison_result=${comparison_result}\n"
	"accepted_exception=${accepted_exception}\n"
	"wrapper_object=${WRAPPER_OBJECT}\n"
	"raw_object=${RAW_OBJECT}\n")

if(comparison_result STREQUAL "failed")
	message(FATAL_ERROR
		"Register wrapper generated code differs from the raw fixture; inspect ${ARTIFACT_DIRECTORY}")
endif()

file(SHA256 "${WRAPPER_OBJECT}" wrapper_hash)
file(SHA256 "${RAW_OBJECT}" raw_hash)
file(SHA256 "${OBJDUMP}" tool_hash)
execute_process(
	COMMAND "${OBJDUMP}" --version
	RESULT_VARIABLE tool_version_result
	OUTPUT_VARIABLE tool_version_output
	ERROR_VARIABLE tool_version_error)
if(NOT tool_version_result EQUAL 0)
	message(FATAL_ERROR "Unable to identify generated-code comparison tool: ${tool_version_error}")
endif()
string(REGEX REPLACE "\r?\n.*" "" tool_version "${tool_version_output}")
if(RECORD_ONLY)
	set(policy_mode "RECORD")
else()
	set(policy_mode "ENFORCE")
endif()
foreach(json_value IN ITEMS
	WRAPPER_OBJECT RAW_OBJECT OBJDUMP tool_version COMPILER_ID COMPILER_VERSION
	COMPILER_PATH SYSTEM_NAME SYSTEM_PROCESSOR CONFIGURATION ISA_PROFILE
	STACK_PROTECTOR_MODE CODEGEN_PROFILE FMA_EXPECTATION SYMBOL_PATTERN
	comparison_result accepted_exception policy_mode)
	simdlib_escape_json("${${json_value}}" "${json_value}_json")
endforeach()
file(WRITE "${RECORD_FILE}.tmp"
	"{\n"
	"  \"schema\": \"simdlib.codegen-record.v1\",\n"
	"  \"kind\": \"comparison\",\n"
	"  \"result\": \"${comparison_result_json}\",\n"
	"  \"accepted_exception\": \"${accepted_exception_json}\",\n"
	"  \"inputs\": {\n"
	"    \"wrapper\": {\"path\": \"${WRAPPER_OBJECT_json}\", \"sha256\": \"${wrapper_hash}\"},\n"
	"    \"raw\": {\"path\": \"${RAW_OBJECT_json}\", \"sha256\": \"${raw_hash}\"}\n"
	"  },\n"
	"  \"tool\": {\"path\": \"${OBJDUMP_json}\", \"version\": \"${tool_version_json}\", \"sha256\": \"${tool_hash}\"},\n"
	"  \"policy\": {\"id\": \"register-codegen-comparison-v1\", \"mode\": \"${policy_mode_json}\", "
		"\"codegen_profile\": \"${CODEGEN_PROFILE_json}\", \"fma_expectation\": \"${FMA_EXPECTATION_json}\", "
		"\"symbol_pattern\": \"${SYMBOL_PATTERN_json}\"},\n"
	"  \"compiler\": {\"id\": \"${COMPILER_ID_json}\", \"version\": \"${COMPILER_VERSION_json}\", "
		"\"path\": \"${COMPILER_PATH_json}\"},\n"
	"  \"platform\": {\"system\": \"${SYSTEM_NAME_json}\", \"processor\": \"${SYSTEM_PROCESSOR_json}\"},\n"
	"  \"configuration\": \"${CONFIGURATION_json}\",\n"
	"  \"register_width\": ${REGISTER_WIDTH},\n"
	"  \"isa_profile\": \"${ISA_PROFILE_json}\",\n"
	"  \"vectorcall_enabled\": ${VECTORCALL_ENABLED},\n"
	"  \"stack_protector_mode\": \"${STACK_PROTECTOR_MODE_json}\"\n"
	"}\n")
file(RENAME "${RECORD_FILE}.tmp" "${RECORD_FILE}")

if(comparison_result STREQUAL "recorded-difference")
	message(STATUS
		"Recorded a non-Release Register wrapper/raw difference; artifacts: ${ARTIFACT_DIRECTORY}")
elseif(comparison_result STREQUAL "accepted-compiler-exception")
	message(STATUS
		"Accepted the exact MSVC /GS security-cookie exception ${accepted_exception}; artifacts: ${ARTIFACT_DIRECTORY}")
endif()
