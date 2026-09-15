cmake_minimum_required(VERSION 3.31)

# Explicit opt-in development provisioning. PREFIX owns cache/build/bin outputs;
# no system installation or PATH mutation occurs. A VS2022 C++ workload is needed.
if(NOT WIN32 OR NOT IS_ABSOLUTE "${PREFIX}")
    message(FATAL_ERROR "Set PREFIX to an absolute Windows development-tools directory")
endif()
set(version 22.1.8)
set(sdk_name "clang+llvm-${version}-x86_64-pc-windows-msvc")
set(archive "${PREFIX}/cache/llvm${version}-windows.tar.xz")
set(archive_hash d96c2cc1736f4eb7fa43cb9bbdf56d93551a9ae0a9aadb9c99c3c3b2b712a234)
set(source "${PREFIX}/cache/FileCheck.cpp")
file(MAKE_DIRECTORY "${PREFIX}/cache" "${PREFIX}/sdk" "${PREFIX}/bin")
file(DOWNLOAD
    "https://github.com/llvm/llvm-project/releases/download/llvmorg-${version}/clang%2Bllvm-${version}-x86_64-pc-windows-msvc.tar.xz"
    "${archive}" EXPECTED_HASH "SHA256=${archive_hash}" TLS_VERIFY ON)
file(DOWNLOAD
    "https://raw.githubusercontent.com/llvm/llvm-project/ca7933e47d3a3451d81e72ac174dcb5aa28b59d1/llvm/utils/FileCheck/FileCheck.cpp"
    "${source}" EXPECTED_HASH SHA256=394d3744e5b88e72e08693ae35510a2cea8a7bae209dbd31e08c2d288ef15b17 TLS_VERIFY ON)
# Extract only the headers, three static libraries, and two inspection tools.
file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${PREFIX}/sdk" PATTERNS
    "${sdk_name}/include/*" "${sdk_name}/lib/LLVMFileCheck.lib"
    "${sdk_name}/lib/LLVMSupport.lib" "${sdk_name}/lib/LLVMDemangle.lib"
    "${sdk_name}/bin/llvm-objdump.exe" "${sdk_name}/bin/llvm-readobj.exe")
set(sdk "${PREFIX}/sdk/${sdk_name}")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}/filecheck"
    -B "${PREFIX}/build" -G "Visual Studio 17 2022" -A x64
    "-DLLVM_SDK=${sdk}" "-DFILECHECK_SOURCE=${source}"
    "-DCMAKE_INSTALL_PREFIX=${PREFIX}" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${PREFIX}/build" --config Release
    COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${CMAKE_COMMAND}" --install "${PREFIX}/build" --config Release
    COMMAND_ERROR_IS_FATAL ANY)
file(COPY "${sdk}/bin/llvm-objdump.exe" "${sdk}/bin/llvm-readobj.exe" DESTINATION "${PREFIX}/bin")
include("${CMAKE_CURRENT_LIST_DIR}/ToolIdentity.cmake")
set(receipt "distribution=LLVM official Windows SDK\nversion=${version}\narchive_sha256=${archive_hash}\nsource_commit=ca7933e47d3a3451d81e72ac174dcb5aa28b59d1\n")
foreach(name IN ITEMS FileCheck llvm-objdump llvm-readobj)
    simdlib_codegen_tool_identity("${PREFIX}/bin/${name}.exe" "${name}" tool)
    string(APPEND receipt "${name}=${tool_PATH}\n${name}_version=${tool_VERSION}\n${name}_sha256=${tool_SHA256}\n")
endforeach()
file(WRITE "${PREFIX}/provisioning.txt" "${receipt}")
message(STATUS "Select -DSIMDLIB_CODEGEN_LLVM_ROOT=${PREFIX}")
