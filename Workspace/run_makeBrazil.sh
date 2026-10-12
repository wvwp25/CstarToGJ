#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  ./run_makeBrazil.sh [-f f0p1 f0p5 f1p0 | all]

Generates the Brazil plots in PDF format.
Default coupling selection: all

Examples:
  ./run_makeBrazil.sh
  ./run_makeBrazil.sh f0p1
  ./run_makeBrazil.sh f0p5
USAGE
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ "${1:-}" = "-f" ] || [ "${1:-}" = "--coupling" ]; then
  shift
fi
couplings=("$@")
if [ "${#couplings[@]}" -eq 0 ]; then
  couplings=(f0p1 f0p5 f1p0)
fi
plot_format="pdf"

run_one() {
  local coup="$1"
  local macro="$script_dir/makeBrazil_${coup}.C"

  if [ ! -f "$macro" ]; then
    echo "Missing macro: $macro" >&2
    return 1
  fi

  echo "BEGIN makeBrazil ${coup}"
  root -l -b -q "${macro}(\"${plot_format}\")"
  echo "END makeBrazil ${coup}"
}

for coupling in "${couplings[@]}"; do
  case "$coupling" in
    f0p1|f0p5|f1p0) run_one "$coupling" ;;
    all)
      run_one f0p1
      run_one f0p5
      run_one f1p0
      ;;
    -h|--help) usage ;;
    *)
      echo "Unsupported coupling: $coupling" >&2
      usage >&2
      exit 2
      ;;
  esac
done
