#!/usr/bin/env bash
set -euo pipefail

pot_file="i18n/opengothicstarter.pot"

if ! command -v xgettext >/dev/null 2>&1; then
  echo "xgettext is required to update ${pot_file}." >&2
  exit 1
fi

if ! command -v msgcat >/dev/null 2>&1; then
  echo "msgcat is required to update ${pot_file}." >&2
  exit 1
fi

if [ ! -f "${pot_file}" ]; then
  echo "Expected POT file not found: ${pot_file}" >&2
  exit 1
fi

mapfile -t source_files < <(find src -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' -o -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o -name '*.hxx' \) | sort)

if [ "${#source_files[@]}" -eq 0 ]; then
  echo "No source files found under src/." >&2
  exit 1
fi

tmp_generated=$(mktemp)
tmp_generated_no_header=$(mktemp)
tmp_header=$(mktemp)
cleanup() {
  rm -f "${tmp_generated}" "${tmp_generated_no_header}" "${tmp_header}"
}
trap cleanup EXIT

xgettext \
  --from-code=UTF-8 \
  --language=C++ \
  --keyword=_ \
  --omit-header \
  --no-location \
  -o "${tmp_generated}" \
  "${source_files[@]}"

msgcat --sort-output --no-location -o "${tmp_generated_no_header}" "${tmp_generated}"

# Preserve current POT header metadata/comments.
awk '
BEGIN { in_header = 1; saw_header_msgid = 0; }
{
  if (!in_header) {
    exit;
  }

  print;

  if ($0 ~ /^msgid ""$/) {
    saw_header_msgid = 1;
    next;
  }

  if (saw_header_msgid && $0 == "") {
    in_header = 0;
  }
}' "${pot_file}" > "${tmp_header}"

if [ ! -s "${tmp_header}" ]; then
  echo "Failed to read header from ${pot_file}." >&2
  exit 1
fi

cat "${tmp_header}" "${tmp_generated_no_header}" > "${pot_file}"

echo "Updated ${pot_file} from current source strings (header preserved)."
