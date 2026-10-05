#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  ./run_makeBrazil.sh [f0p1|f0p5|f1p0|all]

Generates both thesis and poster versions in PDF format.
Default coupling selection: all

Examples:
  ./run_makeBrazil.sh
  ./run_makeBrazil.sh f0p1
  ./run_makeBrazil.sh f0p5
USAGE
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
coupling="${1:-all}"
plot_format="pdf"
if [ "$#" -gt 1 ]; then
  usage >&2
  exit 2
fi

run_one() {
  local coup="$1"
  local macro="$script_dir/makeBrazil_${coup}.C"

  if [ ! -f "$macro" ]; then
    echo "Missing macro: $macro" >&2
    return 1
  fi

  echo "BEGIN makeBrazil ${coup} (thesis)"
  root -l -b -q "${macro}(\"${plot_format}\",false)"
  echo "END makeBrazil ${coup} (thesis)"

  echo "BEGIN makeBrazil ${coup} (poster)"
  root -l -b -q "${macro}(\"${plot_format}\",true)"
  echo "END makeBrazil ${coup} (poster)"
}

case "$coupling" in
  f0p1|f0p5|f1p0)
    run_one "$coupling"
    ;;
  all)
    run_one f0p1
    run_one f0p5
    run_one f1p0
    ;;
  -h|--help)
    usage
    ;;
  *)
    echo "Unsupported coupling: $coupling" >&2
    usage >&2
    exit 2
    ;;
esac
