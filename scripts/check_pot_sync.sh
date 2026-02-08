#!/usr/bin/env bash
set -euo pipefail

pot_file="i18n/opengothicstarter.pot"

if ! command -v xgettext >/dev/null 2>&1; then
  echo "xgettext is required to validate ${pot_file}." >&2
  exit 1
fi

if ! command -v msgcat >/dev/null 2>&1; then
  echo "msgcat is required to validate ${pot_file}." >&2
  exit 1
fi

if [ ! -f "${pot_file}" ]; then
  echo "Expected POT file not found: ${pot_file}" >&2
  exit 1
fi

mapfile -t source_files < <(find src -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' -o -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o -name '*.hxx' \) | sort)

if [ "${#source_files[@]}" -eq 0 ]; then
  echo "No source files found under src/."
  exit 0
fi

expected_tmp=$(mktemp)
current_tmp=$(mktemp)
expected_normalized_tmp=$(mktemp)
current_normalized_tmp=$(mktemp)
cleanup() {
  rm -f "${expected_tmp}" "${current_tmp}" "${expected_normalized_tmp}" "${current_normalized_tmp}"
}
trap cleanup EXIT

xgettext \
  --from-code=UTF-8 \
  --language=C++ \
  --keyword=_ \
  --omit-header \
  --no-location \
  -o "${expected_tmp}" \
  "${source_files[@]}"

msgcat --sort-output --no-location -o "${expected_tmp}" "${expected_tmp}"
msgcat --sort-output --no-location -o "${current_tmp}" "${pot_file}"

strip_header_entry() {
  local input_file="$1"
  local output_file="$2"
  awk '
  BEGIN { skipping_preamble = 1; skipping_header = 0; }
  {
    if (skipping_preamble) {
      if ($0 ~ /^#/ || $0 == "") {
        next;
      }

      if ($0 ~ /^msgid ""$/) {
        skipping_preamble = 0;
        skipping_header = 1;
        next;
      }

      skipping_preamble = 0;
    }

    if (skipping_header) {
      if ($0 == "") {
        skipping_header = 0;
      }
      next;
    }

    if ($0 ~ /^#/) {
      next;
    }

    if ($0 ~ /^msgid ""$/) {
      skipping_header = 1;
      next;
    }

    print;
  }' "${input_file}" > "${output_file}"
}

strip_header_entry "${expected_tmp}" "${expected_normalized_tmp}"
strip_header_entry "${current_tmp}" "${current_normalized_tmp}"

if ! diff -u "${current_normalized_tmp}" "${expected_normalized_tmp}" >/dev/null; then
  echo "${pot_file} is out of sync with translatable source strings." >&2
  echo "Regenerate ${pot_file} from current sources before committing." >&2
  diff -u "${current_normalized_tmp}" "${expected_normalized_tmp}" || true
  exit 1
fi

echo "POT template is in sync with source strings."
