#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: ./scripts/verify.sh [--staged|--full]

Modes:
  --staged  Run staged-file verification (pre-commit mode).
  --full    Run full local verification gate.

Default mode is --full.
EOF
}

run_staged() {
  mapfile -t files < <(
    git diff --cached --name-only --diff-filter=ACMR |
      grep -E '\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$' || true
  )

  mapfile -t po_files < <(
    git diff --cached --name-only --diff-filter=ACMR |
      grep -E '^i18n/.+\.po$' || true
  )

  mapfile -t i18n_sync_trigger_files < <(
    git diff --cached --name-only --diff-filter=ACMR |
      grep -E '^src/.*\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$|^i18n/opengothicstarter\.pot$' || true
  )

  if [ "${#files[@]}" -gt 0 ]; then
    ./scripts/format.sh "${files[@]}"
    git add "${files[@]}"
    echo "Formatted staged C/C++ files with clang-format."
  fi

  if [ "${#po_files[@]}" -gt 0 ]; then
    ./scripts/check_i18n.sh "${po_files[@]}"
  fi

  if [ "${#i18n_sync_trigger_files[@]}" -gt 0 ]; then
    ./scripts/update_pot.sh
    ./scripts/update_po.sh

    git add i18n/opengothicstarter.pot
    git add i18n/*/LC_MESSAGES/opengothicstarter.po

    ./scripts/check_pot_sync.sh
    ./scripts/check_i18n.sh
    echo "Updated and staged POT/PO catalogs."
  fi
}

run_full() {
  cmake -S . -B build -DOGS_WARNINGS_AS_ERRORS=ON -DOGS_EXTRA_WARNINGS=ON
  cmake --build build --parallel

  cmake -S . -B build-tidy -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DOGS_WARNINGS_AS_ERRORS=ON -DOGS_EXTRA_WARNINGS=ON
  ./scripts/tidy.sh build-tidy

  ./scripts/check_pot_sync.sh
  ./scripts/check_i18n.sh
}

mode="full"
if [ "$#" -gt 1 ]; then
  usage >&2
  exit 1
fi

if [ "$#" -eq 1 ]; then
  case "$1" in
    --staged)
      mode="staged"
      ;;
    --full)
      mode="full"
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage >&2
      exit 1
      ;;
  esac
fi

if [ "${mode}" = "staged" ]; then
  run_staged
else
  run_full
fi
