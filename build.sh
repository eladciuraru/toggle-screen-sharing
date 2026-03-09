#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
bin_dir="${root_dir}/bin"
common_flags="-Wall -Wextra -Werror -arch arm64e"

if [[ ! -d "${bin_dir}" ]]; then
    mkdir -p "${bin_dir}"
fi

set -x

# build shared library
clang $common_flags \
    -dynamiclib \
    -o "${bin_dir}/libscreen_sharing.dylib" \
    "${root_dir}/src/screen_sharing.c"

# build executable
clang $common_flags \
    -DSCREEN_SHARING_EXECUTABLE \
    -o "${bin_dir}/screen_sharing" \
    "${root_dir}/src/screen_sharing.c"

codesign --force --entitlements "${root_dir}/src/entitlements.plist" \
    --deep -s - "${bin_dir}/screen_sharing"
