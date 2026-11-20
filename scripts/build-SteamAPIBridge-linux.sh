#!/bin/sh
# Usage: build-SteamAPIBridge-linux.sh <x86|x64> <Debug|Release> [CMake options]
set -e
case "$1" in x86) bits=32 ;; x64) bits=64 ;; *) exit 2 ;; esac
case "$2" in Debug|Release) ;; *) exit 2 ;; esac
arch=$1
configuration=$2
shift 2
source_dir=$(cd "$(dirname "$0")/.." && pwd)
build_dir="$source_dir/build/linux-$arch/$configuration"
cmake -S "$source_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE="$configuration" \
    -DCMAKE_CXX_FLAGS="-m$bits" -DCMAKE_SHARED_LINKER_FLAGS="-m$bits" -DCMAKE_EXE_LINKER_FLAGS="-m$bits" \
    -DCMAKE_INSTALL_PREFIX="$source_dir/install/linux-$arch/$configuration" "$@"
cmake --build "$build_dir" --parallel
ctest --test-dir "$build_dir" --output-on-failure
cmake --install "$build_dir"
