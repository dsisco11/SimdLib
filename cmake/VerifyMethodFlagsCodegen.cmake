cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
	FLAGGED_OBJECT RAW_OBJECT OBJDUMP COMPILER_ID STACK_PROTECTOR_MODE OUTPUT_FILE)
	if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
		message(FATAL_ERROR "VerifyMethodFlagsCodegen requires ${required_variable}")
	endif()
endforeach()

# @brief Extracts one function and its relocation lines from an object disassembly.
# @param disassembly Complete disassembly text.
# @param symbol_fragment Stable fragment of the function name.
# @param output_variable Variable that receives the selected function body.
function(simdlib_extract_method_flags_symbol disassembly symbol_fragment output_variable)
	string(REPLACE "\r\n" "\n" normalized "${disassembly}")
	string(REPLACE "\n" ";" disassembly_lines "${normalized}")
	set(selected "")
	set(in_symbol OFF)
	foreach(disassembly_line IN LISTS disassembly_lines)
		if(disassembly_line MATCHES "<[^>]*${symbol_fragment}[^>]*>:")
			set(in_symbol ON)
			string(APPEND selected "${disassembly_line}\n")
		elseif(in_symbol AND disassembly_line MATCHES "^[ \t]*[0-9A-Fa-f]+[ \t]+<[^>]+>:")
			set(in_symbol OFF)
		elseif(in_symbol)
			string(APPEND selected "${disassembly_line}\n")
		endif()
	endforeach()
	if(selected STREQUAL "")
		message(FATAL_ERROR "Unable to find generated-code symbol ${symbol_fragment}")
	endif()
	set(${output_variable} "${selected}" PARENT_SCOPE)
endfunction()

set(register_only_symbols
	simdlib_method_flags_codegen_unary
	simdlib_method_flags_codegen_binary
	simdlib_method_flags_codegen_ternary
	simdlib_method_flags_codegen_scalar_result
	simdlib_method_flags_codegen_register_result
	simdlib_method_flags_codegen_load
	simdlib_method_flags_codegen_forceinline
	simdlib_method_flags_codegen_flatten)

foreach(object_file IN ITEMS "${FLAGGED_OBJECT}" "${RAW_OBJECT}")
	execute_process(
		COMMAND "${OBJDUMP}" -dr "${object_file}"
		RESULT_VARIABLE disassembly_result
		OUTPUT_VARIABLE disassembly
		ERROR_VARIABLE disassembly_error)
	if(NOT disassembly_result EQUAL 0)
		message(FATAL_ERROR "Unable to disassemble ${object_file}: ${disassembly_error}")
	endif()

	foreach(symbol_name IN LISTS register_only_symbols)
		simdlib_extract_method_flags_symbol("${disassembly}" "${symbol_name}" symbol_body)
		if(symbol_body MATCHES "security_(cookie|check_cookie)|stack_chk_(fail|guard)")
			message(FATAL_ERROR "${symbol_name} acquired stack-cookie code in ${object_file}")
		endif()
	endforeach()

	if(disassembly MATCHES "call[^\n]*(\n[^\n]*)?simdlib_method_flags_force_leaf")
		message(FATAL_ERROR "ForceInline did not inline its dedicated leaf in ${object_file}")
	endif()
	if(disassembly MATCHES "call[^\n]*(\n[^\n]*)?simdlib_method_flags_flatten_leaf")
		message(FATAL_ERROR "Flatten did not inline its dedicated leaf in ${object_file}")
	endif()

endforeach()

if(COMPILER_ID STREQUAL "MSVC" AND NOT STACK_PROTECTOR_MODE STREQUAL "msvc-gs")
	message(FATAL_ERROR "MSVC method-flags codegen requires /GS stack protection")
endif()
if(NOT STACK_PROTECTOR_MODE MATCHES "^(strong|msvc-gs)$")
	message(FATAL_ERROR "Method-flags codegen requires an explicit stack-protection mode")
endif()

file(WRITE "${OUTPUT_FILE}"
	"method_flags_codegen=verified\n"
	"compiler_id=${COMPILER_ID}\n"
	"stack_protector_mode=${STACK_PROTECTOR_MODE}\n")
