cmake_minimum_required(VERSION 3.31)
# Keep the proven freshness and complete LLVM selection unchanged for the runner
# comparison. Catch2 owns check invocation, aggregation and result reporting.
include("${SCRIPTS}/InputReceipt.cmake")
simdlib_verify_codegen_build("${INPUT_MANIFEST}" "${OBJECT_FILE}" "${BUILD_RECEIPT}")
include("${SCRIPTS}/ExtractFunction.cmake")
