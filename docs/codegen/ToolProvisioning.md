# Development inspection tools

The standalone extraction harness explicitly selects FileCheck, llvm-objdump,
and llvm-readobj using `cmake/development/CodegenTools.cmake`. Supply
`SIMDLIB_CODEGEN_LLVM_ROOT` with a `bin` directory, or set `SIMDLIB_FILECHECK`,
`SIMDLIB_LLVM_OBJDUMP`, and `SIMDLIB_LLVM_READOBJ` to absolute executables.
The root may also be supplied by the environment. Discovery does not search
PATH. Missing tools, unsupported versions, or mismatched releases fail.
`codegen-tools.txt` records selected paths, versions and SHA256 identities.

These tools belong to development validation. They are not linked to the
library or included in its installed package. Existing qualification gates keep
their current ownership until the instruction-contract integration is selected.

## Windows: local, native CI and Windows containers

From the repository root, with CMake 3.31+ and a VS2022 C++ workload installed:

```powershell
cmake "-DPREFIX=$PWD/out/codegen-tools" -P cmake/codegen/ProvisionWindowsTools.cmake
```

The provisioner pins the official LLVM 22.1.8 Windows SDK archive by SHA256 and
upstream FileCheck source by commit and SHA256. LLVM's Windows binary packages
omit FileCheck. This builds only that command against the SDK's FileCheck,
Support and Demangle libraries using the static MSVC runtime. It does not
configure the LLVM source tree or require Python/lit. The first run downloads
approximately 862 MB; matching cached downloads are reused.

The prefix owns its cache, selected SDK files, build tree and `bin` directory.
It does not alter the system installation or PATH. `provisioning.txt` retains
distribution/source identity and executable hashes. The inspection-tool release
is independent of the compiler being inspected, including clang-cl 20.
The native CI jobs and Windows container invoke this same provisioner.

## Alpine validation containers

`containers/provision-codegen-tools.sh` provisions the three commands into
`/opt/codegen/bin`, selected by `SIMDLIB_CODEGEN_LLVM_ROOT` in each image.

| Compiler environment | Inspection-tool packages |
| --- | --- |
| GCC 13 / Alpine 3.20 | LLVM 18.1.8-r1 |
| GCC 14 / Alpine 3.22 | LLVM 20.1.8-r0 |
| Clang 22 / Alpine 3.24 | LLVM 22.1.3-r0 |

LLVM runtime-library packages and libcurl are installed. The latter resolves
from the image's configured Alpine release repository, like its existing
OpenSSL runtime dependencies. The LLVM and test-utils APKs
are fetched and verified, and the three named binaries are extracted directly.
This avoids the umbrella packages' Python/lit dependencies. Package hashes,
binary hashes, version output and the installed runtime-package inventory are
retained under `/opt/codegen`. Unavailable package pins fail provisioning.

See [function inspection](FunctionExtraction.md) and the
[harness prerequisites](../../tests/codegen/harness/README.md) for execution.

## Validation and scope

The tool tasks in [the tasklist](../RegisterCodegenPolicy.todo) inherit the
proposal's [tool responsibilities](../RegisterCodegenPolicyProposal.md#cmake-ctest-and-filecheck-responsibilities)
and the [integration inventory](IntegrationInventory.md) environment owners.
Selection must fail explicitly; tools must remain development-only; local and
container provisioning must retain identities without introducing Python/lit.

| Environment | Observed evidence |
| --- | --- |
| Windows VS2022 | Exact tracked SDK provisioner executed successfully; selected prefix used by the standalone harness. `out/codegen-tools-provisioned/provisioning.txt` and `provisioning.log` retain receipts. |
| GCC 13 / LLVM 18 | Targeted derived validation image provisioned; explicit selector, FileCheck positive/negative checks and six freshly compiled object checks passed. |
| GCC 14 / LLVM 20 | Same provisioning, selector, FileCheck and fresh-object checks passed independently. |
| Clang 22 / LLVM 22 | Same provisioning, selector, FileCheck and fresh-object checks passed independently. |

Linux logs and receipts live under
`out/codegen-container-evidence/{gcc13,gcc14,clang22}`. Each directory retains
compiler/tool versions, `codegen-tools.txt`, package/binary SHA256 receipts,
runtime package inventory, FileCheck mismatch diagnostics and per-function
extraction artifacts. Container validation asserted that Python and lit were
absent. Derived-image build logs are retained under
`out/codegen-extraction-probe/docker`.

The Linux validation used existing pinned compiler images plus the new tracked
provisioner. It did not rebuild their CMake/Catch2 layers or run the full library
matrix. The Windows recipe ran locally; the Windows container and CI wiring were
source-reviewed, not executed here. Full workflow/cutover qualification remains
separate from these tool and extraction results.
