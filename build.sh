#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
bin_dir="${root_dir}/bin"

if [[ ! -d "${bin_dir}" ]]; then
  mkdir -p "${bin_dir}"
fi

set -x

clang -Wall -Wextra -arch arm64e -dynamiclib \
    -o "${bin_dir}/libscreen_sharing.dylib" \
    "${root_dir}/src/screen_sharing.c"
