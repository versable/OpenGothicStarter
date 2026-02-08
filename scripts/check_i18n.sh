#!/usr/bin/env bash
set -euo pipefail

if ! command -v msgfmt >/dev/null 2>&1; then
  echo "msgfmt is required to validate localization catalogs." >&2
  exit 1
fi

if [ "$#" -gt 0 ]; then
  catalog_files=("$@")
else
  mapfile -t catalog_files < <(find i18n -type f -name '*.po' | sort)
fi

if [ "${#catalog_files[@]}" -eq 0 ]; then
  echo "No localization catalog files found under i18n/."
  exit 0
fi

for catalog in "${catalog_files[@]}"; do
  if [ ! -f "${catalog}" ]; then
    echo "Localization catalog file not found: ${catalog}" >&2
    exit 1
  fi

  echo "Checking ${catalog}"
  if ! output=$(msgfmt --check --check-format -o /dev/null "${catalog}" 2>&1); then
    echo "${output}" >&2
    exit 1
  fi
done

echo "Localization catalogs validated successfully."
