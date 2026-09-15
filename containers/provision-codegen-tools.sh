#!/bin/sh
# Provision exact development inspection tools without installing Python or lit.
# Usage: provision-codegen-tools.sh <LLVM-major> <APK-version> <absolute-prefix>
set -eu
major=${1:?LLVM major is required}
version=${2:?Exact APK version is required}
prefix=${3:?Absolute output prefix is required}
case "$major:$version" in
    18:18.1.8-r1|20:20.1.8-r0|22:22.1.3-r0) ;;
    *) echo "Unsupported LLVM package selection: $major:$version" >&2; exit 1 ;;
esac
case "$prefix" in /*) ;; *) echo 'Tool prefix must be absolute' >&2; exit 1 ;; esac
test "$(apk --print-arch)" = x86_64
mkdir -p "$prefix/bin" "$prefix/packages" "$prefix/unpacked"
# Only runtime libraries are installed. The umbrella llvm/test-utils packages
# pull in lit/Python, so fetch their signed archives and extract named binaries.
apk add --no-cache "llvm${major}-libs=$version" libcurl
# apk 2 fetch accepts names, not add-style version constraints. Use a fresh
# fetch directory and require the exact versioned filenames before extraction.
download=$(mktemp -d "$prefix/packages/fetch.XXXXXX")
apk fetch --no-cache --output "$download" "llvm${major}" "llvm${major}-test-utils"
for name in "llvm${major}" "llvm${major}-test-utils"; do
    test -f "$download/$name-$version.apk"
    cp "$download/$name-$version.apk" "$prefix/packages/"
done
for package in "$prefix/packages/llvm${major}-$version.apk" "$prefix/packages/llvm${major}-test-utils-$version.apk"; do
    apk verify "$package"
done
tar -xzf "$prefix/packages/llvm${major}-$version.apk" -C "$prefix/unpacked" \
    "usr/lib/llvm${major}/bin/llvm-objdump" "usr/lib/llvm${major}/bin/llvm-readobj"
tar -xzf "$prefix/packages/llvm${major}-test-utils-$version.apk" -C "$prefix/unpacked" \
    "usr/lib/llvm${major}/bin/FileCheck"
for name in FileCheck llvm-objdump llvm-readobj; do
    cp "$prefix/unpacked/usr/lib/llvm${major}/bin/$name" "$prefix/bin/$name"
    "$prefix/bin/$name" --version > "$prefix/$name.version.txt"
    grep -F "LLVM version ${version%-r*}" "$prefix/$name.version.txt"
done
sha256sum "$prefix"/packages/*.apk "$prefix"/bin/* > "$prefix/provisioning.sha256"
apk info -v > "$prefix/runtime-packages.txt"
