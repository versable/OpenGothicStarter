#!/usr/bin/env bash
set -euo pipefail

pot_file="i18n/opengothicstarter.pot"

if ! command -v msgmerge >/dev/null 2>&1; then
  echo "msgmerge is required to update .po catalogs." >&2
  exit 1
fi

if [ ! -f "${pot_file}" ]; then
  echo "Expected POT file not found: ${pot_file}" >&2
  exit 1
fi

if [ "$#" -gt 0 ]; then
  po_files=("$@")
else
  mapfile -t po_files < <(find i18n -type f -name '*.po' | sort)
fi

if [ "${#po_files[@]}" -eq 0 ]; then
  echo "No localization catalog files found under i18n/."
  exit 0
fi

for po_file in "${po_files[@]}"; do
  if [ ! -f "${po_file}" ]; then
    echo "Localization catalog file not found: ${po_file}" >&2
    exit 1
  fi

  echo "Updating ${po_file}"
  msgmerge --update --backup=none --no-fuzzy-matching "${po_file}" "${pot_file}" >/dev/null
done

echo "Updated ${#po_files[@]} localization catalog(s) from ${pot_file}."
