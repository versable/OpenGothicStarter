#!/usr/bin/env bash
set -euo pipefail

if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "clang-tidy is required but was not found in PATH." >&2
  exit 1
fi

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <build-dir> [files...]" >&2
  exit 1
fi

build_dir="$1"
shift

if [ ! -f "${build_dir}/compile_commands.json" ]; then
  echo "compile_commands.json not found in ${build_dir}." >&2
  exit 1
fi

if [ "$#" -gt 0 ]; then
  files=("$@")
else
  mapfile -t files < <(find src -type f -name '*.cpp' | sort)
fi

if [ "${#files[@]}" -eq 0 ]; then
  echo "No source files to lint." >&2
  exit 0
fi

clang-tidy -p "${build_dir}" "${files[@]}" --quiet
