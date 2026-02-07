#!/usr/bin/env bash
set -euo pipefail

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format is required but was not found in PATH." >&2
  exit 1
fi

if [ "$#" -gt 0 ]; then
  files=("$@")
else
  mapfile -t files < <(
    git ls-files '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx'
  )
fi

if [ "${#files[@]}" -eq 0 ]; then
  exit 0
fi

clang-format -i "${files[@]}"
