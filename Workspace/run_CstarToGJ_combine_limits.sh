#!/usr/bin/env bash
set -uo pipefail

usage() {
  cat <<'USAGE'
Usage:
  ./run_CstarToGJ_combine_limits.sh [options]

Options:
  -f, --coupling VALUE   Coupling to process: f1p0, f0p5, f0p1. Default: f1p0
  -w, --work-dir PATH    Directory for generated workspaces/results.
                         Default: /eos/user/h/hsiaoche/workspace
  -c, --cmssw-dir PATH   CMSSW directory used for cmsenv. Default: /eos/user/h/hsiaoche/CMSSW_13_3_0
  --mass-list LIST       Space/comma separated masses, e.g. "1000 1200" or 1000,1200
  --skip-existing        Do not rerun text2workspace.py if the workspace ROOT file already exists
  --text2workspace-only  Only run text2workspace.py, skip combine
  --combine-only         Only run combine, using existing workspace ROOT files
  --dry-run              Print commands without executing them
  -h, --help             Show this help

Examples:
  ./run_CstarToGJ_combine_limits.sh -f f1p0
  ./run_CstarToGJ_combine_limits.sh -f f0p5 --dry-run
  ./run_CstarToGJ_combine_limits.sh -f f0p1 --mass-list "1000 1200 1400"
  ./run_CstarToGJ_combine_limits.sh -f f1p0 --skip-existing

Manual command for one mass point / coupling, e.g. M1000 f1p0:

  text2workspace.py datacard_M1000_f1p0.txt \
    -o workspace_M1000_f1p0.root

  combine -M AsymptoticLimits workspace_M1000_f1p0.root \
    -m 1000 \
    -n _f1p0 \
    --run blind
USAGE
}

coupling="f1p0"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
card_dir="$script_dir"
work_dir="/eos/user/h/hsiaoche/workspace"
cmssw_dir="/eos/user/h/hsiaoche/CMSSW_13_3_0"
mass_list=""
dry_run=0
skip_existing=0
text2workspace_only=0
combine_only=0

while [ "$#" -gt 0 ]; do
  case "$1" in
    -f|--coupling)
      coupling="${2:-}"
      shift 2
      ;;
    -w|--work-dir)
      work_dir="${2:-}"
      shift 2
      ;;
    -c|--cmssw-dir)
      cmssw_dir="${2:-}"
      shift 2
      ;;
    --mass-list)
      mass_list="${2:-}"
      shift 2
      ;;
    --skip-existing)
      skip_existing=1
      shift
      ;;
    --text2workspace-only)
      text2workspace_only=1
      shift
      ;;
    --combine-only)
      combine_only=1
      shift
      ;;
    --dry-run)
      dry_run=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

case "$coupling" in
  f1p0|f0p5|f0p1) ;;
  *)
    echo "Unsupported coupling: $coupling" >&2
    echo "Use one of: f1p0, f0p5, f0p1" >&2
    exit 2
    ;;
esac

if [ "$text2workspace_only" -eq 1 ] && [ "$combine_only" -eq 1 ]; then
  echo "Choose only one of --text2workspace-only or --combine-only." >&2
  exit 2
fi

if [ ! -d "$cmssw_dir" ]; then
  echo "CMSSW directory not found: $cmssw_dir" >&2
  exit 1
fi

if [ ! -d "$work_dir" ]; then
  echo "Workspace directory not found: $work_dir" >&2
  exit 1
fi

if [ "$dry_run" -eq 0 ]; then
  if [ -f /cvmfs/cms.cern.ch/cmsset_default.sh ]; then
    # shellcheck disable=SC1091
    source /cvmfs/cms.cern.ch/cmsset_default.sh
  fi

  cd "$cmssw_dir" || exit 1
  if command -v cmsenv >/dev/null 2>&1; then
    cmsenv
  elif command -v scram >/dev/null 2>&1; then
    eval "$(scram runtime -sh)"
  else
    echo "Neither cmsenv nor scram is available after CMS setup." >&2
    exit 1
  fi
fi

cd "$work_dir" || exit 1
work_dir="$(pwd -P)"

cards=()
if [ -n "$mass_list" ]; then
  mass_list="${mass_list//,/ }"
  for mass in $mass_list; do
    cards+=("datacard_M${mass}_${coupling}.txt")
  done
else
  shopt -s nullglob
  # shellcheck disable=SC2206
  cards=("$card_dir"/datacard_M*_"$coupling".txt)
  shopt -u nullglob
fi

status=0
converted=0
combined=0
skipped=0
failed=0

for card in "${cards[@]}"; do
  case "$card" in
    /*) ;;
    *) card="$card_dir/$card" ;;
  esac
  if [ ! -f "$card" ]; then
    echo "SKIP missing datacard: $card"
    skipped=$((skipped + 1))
    continue
  fi

  card_name="$(basename "$card")"
  mass="${card_name#datacard_M}"
  mass="${mass%_"${coupling}".txt}"
  ws="workspace_M${mass}_${coupling}.root"
  name="_${coupling}"

  if [ "$combine_only" -eq 0 ]; then
    if [ "$skip_existing" -eq 1 ] && [ -f "$ws" ]; then
      echo "SKIP existing workspace: $ws"
      skipped=$((skipped + 1))
    else
      echo "BEGIN text2workspace M${mass} ${coupling}"
      if [ "$dry_run" -eq 1 ]; then
        echo "  text2workspace.py $card -o $ws"
        rc=0
      else
        text2workspace.py "$card" -o "$ws"
        rc=$?
      fi
      echo "END text2workspace M${mass} rc=$rc"
      if [ "$rc" -ne 0 ]; then
        failed=$((failed + 1))
        status=$rc
        continue
      fi
      converted=$((converted + 1))
    fi
  fi

  if [ "$text2workspace_only" -eq 1 ]; then
    continue
  fi

  if [ ! -f "$ws" ] && [ "$dry_run" -eq 0 ]; then
    echo "SKIP missing workspace: $ws"
    skipped=$((skipped + 1))
    continue
  fi

  echo "BEGIN combine M${mass} ${coupling}"
  if [ "$dry_run" -eq 1 ]; then
    echo "  combine -M AsymptoticLimits $ws -m $mass -n $name --run blind"
    rc=0
  else
    combine -M AsymptoticLimits "$ws" -m "$mass" -n "$name" --run blind
    rc=$?
  fi
  echo "END combine M${mass} rc=$rc"
  if [ "$rc" -ne 0 ]; then
    failed=$((failed + 1))
    status=$rc
    continue
  fi
  combined=$((combined + 1))
done

echo "Summary: coupling=$coupling converted=$converted combined=$combined skipped=$skipped failed=$failed"
exit "$status"
