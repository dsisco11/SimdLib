if(NOT DEFINED SOURCE_FILE OR SOURCE_FILE STREQUAL "")
	message(FATAL_ERROR "SOURCE_FILE is required")
endif()

file(READ "${SOURCE_FILE}" source_text)
string(REGEX REPLACE "[ \t\r\n]+" " " normalized_source "${source_text}")

string(REGEX MATCHALL "(class|struct)[ ]+[A-Za-z_][A-Za-z0-9_]*" declared_types
	"${normalized_source}")
foreach(declared_type IN LISTS declared_types)
	string(REGEX REPLACE "^(class|struct)[ ]+" "" type_name "${declared_type}")
	if(normalized_source MATCHES
		"SIMD_FLAGS\\([^)]*\\)[ ]+${type_name}[ ]*\\(")
		message(FATAL_ERROR
			"SIMDLIB_METHOD_FLAGS_PROHIBITED_CONSTRUCTOR: ${SOURCE_FILE}")
	endif()
endforeach()

if(normalized_source MATCHES "\\[[^]]*\\][ ]*SIMD_FLAGS\\(")
	message(FATAL_ERROR
		"SIMDLIB_METHOD_FLAGS_PROHIBITED_LAMBDA: ${SOURCE_FILE}")
endif()

if(normalized_source MATCHES "consteval[^;{}]*SIMD_FLAGS|SIMD_FLAGS[^;{}]*consteval")
	message(FATAL_ERROR
		"SIMDLIB_METHOD_FLAGS_PROHIBITED_CONSTEVAL: ${SOURCE_FILE}")
endif()

if(normalized_source MATCHES "SIMD_FLAGS\\([^)]*\\)[^;{}]*\\(\\*")
	message(FATAL_ERROR
		"SIMDLIB_METHOD_FLAGS_PROHIBITED_FUNCTION_POINTER: ${SOURCE_FILE}")
endif()

if(normalized_source MATCHES "virtual[^;{}]*SIMD_FLAGS|SIMD_FLAGS[^;{}]*override")
	message(FATAL_ERROR
		"SIMDLIB_METHOD_FLAGS_PROHIBITED_VIRTUAL: ${SOURCE_FILE}")
endif()

if(normalized_source MATCHES "extern[ ]+\"C\"[^;{}]*SIMD_FLAGS")
	message(FATAL_ERROR
		"SIMDLIB_METHOD_FLAGS_PROHIBITED_EXTERN_C: ${SOURCE_FILE}")
endif()

if(normalized_source MATCHES "SIMD_FLAGS\\([^)]*\\)[^;{}]*\\.\\.\\.")
	message(FATAL_ERROR
		"SIMDLIB_METHOD_FLAGS_PROHIBITED_VARIADIC: ${SOURCE_FILE}")
endif()

if(normalized_source MATCHES "SIMD_FLAGS\\([^)]*\\)[^;{}]*operator[ ]+(new|delete)")
	message(FATAL_ERROR
		"SIMDLIB_METHOD_FLAGS_PROHIBITED_ALLOCATION: ${SOURCE_FILE}")
endif()

if(normalized_source MATCHES
	"SIMD_FLAGS\\([^)]*\\)[^;{}]*operator[ ]+[A-Za-z_:][A-Za-z0-9_:<>]*[ ]*\\(")
	message(FATAL_ERROR
		"SIMDLIB_METHOD_FLAGS_PROHIBITED_CONVERSION: ${SOURCE_FILE}")
endif()

if(normalized_source MATCHES "SIMD_FLAGS\\([^)]*\\)[^;{}]*=[ ]*(default|delete)")
	message(FATAL_ERROR
		"SIMDLIB_METHOD_FLAGS_PROHIBITED_DEFAULTED_OR_DELETED: ${SOURCE_FILE}")
endif()

if(normalized_source MATCHES "SIMD_FLAGS\\([^)]*\\)[^{]*\\{[^}]*co_(await|yield|return)")
	message(FATAL_ERROR
		"SIMDLIB_METHOD_FLAGS_PROHIBITED_COROUTINE: ${SOURCE_FILE}")
endif()
