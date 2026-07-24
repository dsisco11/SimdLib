cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
	WRAPPER_OBJECT RAW_OBJECT OBJDUMP ARTIFACT_DIRECTORY COMPILER_ID
	COMPILER_VERSION COMPILER_PATH SYSTEM_NAME SYSTEM_PROCESSOR CONFIGURATION REGISTER_WIDTH
	VECTORCALL_ENABLED STACK_PROTECTOR_MODE)
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
if(NOT FMA_EXPECTATION MATCHES "^(none|enabled|disabled)$")
	message(FATAL_ERROR "Unsupported FMA_EXPECTATION: ${FMA_EXPECTATION}")
endif()

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
if(FMA_EXPECTATION STREQUAL "enabled" AND (wrapper_fma_index LESS 0 OR raw_fma_index LESS 0))
	message(FATAL_ERROR "The FMA-enabled generated-code profile does not contain fused multiply-add instructions")
elseif(FMA_EXPECTATION STREQUAL "disabled" AND (NOT wrapper_fma_index LESS 0 OR NOT raw_fma_index LESS 0))
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
		set(comparison_result "failed")
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
	"vectorcall_enabled=${VECTORCALL_ENABLED}\n"
	"stack_protector_mode=${STACK_PROTECTOR_MODE}\n"
	"codegen_profile=${CODEGEN_PROFILE}\n"
	"fma_expectation=${FMA_EXPECTATION}\n"
	"comparison_result=${comparison_result}\n"
	"accepted_exception=${accepted_exception}\n"
	"wrapper_object=${WRAPPER_OBJECT}\n"
	"raw_object=${RAW_OBJECT}\n")

if(comparison_result STREQUAL "failed")
	message(FATAL_ERROR
		"Register wrapper generated code differs from the raw fixture; inspect ${ARTIFACT_DIRECTORY}")
elseif(comparison_result STREQUAL "accepted-compiler-exception")
	message(STATUS
		"Accepted the exact MSVC /GS security-cookie exception ${accepted_exception}; artifacts: ${ARTIFACT_DIRECTORY}")
endif()
