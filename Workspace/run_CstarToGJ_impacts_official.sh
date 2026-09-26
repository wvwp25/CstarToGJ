#!/usr/bin/env bash
set -uo pipefail

usage() {
  cat <<'USAGE'
Usage:
  ./run_CstarToGJ_impacts.sh [options]

Options:
  -f, --coupling VALUE   Coupling to process: f1p0, f0p5, f0p1, f*, or all. Default: all
  -w, --work-dir PATH    Directory with workspace ROOT files. Default: current directory
  -c, --cmssw-dir PATH   CMSSW directory used for cmsenv. Default: /eos/user/h/hsiaoche/CMSSW_13_3_0
  --mass-list LIST       Space/comma separated masses, e.g. "1000 1200" or 1000,1200
  --parallel N           Number of parallel jobs for --doFits. Default: 8
  --expect-signal VALUE  Expected signal for toys. Default: 0
  --r-min VALUE          Minimum r value. Default: -5
  --r-max VALUE          Maximum r value. Default: 5
  --dry-run              Print commands without executing them
  -h, --help             Show this help

Examples:
  ./run_CstarToGJ_impacts.sh -f f1p0
  ./run_CstarToGJ_impacts.sh -f f0p5 --parallel 12
  ./run_CstarToGJ_impacts.sh -f f0p1 --mass-list "1000 1200 1400"
  ./run_CstarToGJ_impacts.sh -f all
  ./run_CstarToGJ_impacts.sh -f 'f*'

Manual command for one mass point / coupling, e.g. M1000 f1p0:

  mkdir -p higgsCombine_initialFit_f1p0
  cd higgsCombine_initialFit_f1p0

  combineTool.py -M Impacts \
    -d /path/to/workspace_M1000_f1p0.root \
    -m 1000 \
    --doInitialFit \
    --robustFit 1

  combineTool.py -M Impacts \
    -d /path/to/workspace_M1000_f1p0.root \
    -m 1000 \
    --doFits \
    --robustFit 1 \
    --rMin 0 \
    --rMax 50 \
    --parallel 8

  combineTool.py -M Impacts \
    -d /path/to/workspace_M1000_f1p0.root \
    -m 1000 \
    -o impacts_M1000_f1p0.json

  plotImpacts.py \
    -i impacts_M1000_f1p0.json \
    -o impacts_M1000_f1p0
USAGE
}

coupling="all"
work_dir="$(pwd)"
cmssw_dir="/eos/user/h/hsiaoche/CMSSW_13_3_0"
mass_list=""
parallel_jobs=8
expect_signal=0
r_min=-5
r_max=5
dry_run=0

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
    --parallel)
      parallel_jobs="${2:-}"
      shift 2
      ;;
    --expect-signal)
      expect_signal="${2:-}"
      shift 2
      ;;
    --r-min)
      r_min="${2:-}"
      shift 2
      ;;
    --r-max)
      r_max="${2:-}"
      shift 2
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
  f1p0|f0p5|f0p1|all|f\*) ;;
  *)
    echo "Unsupported coupling: $coupling" >&2
    echo "Use one of: f1p0, f0p5, f0p1, f*, all" >&2
    exit 2
    ;;
esac

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

workspaces=()
if [ -n "$mass_list" ]; then
  mass_list="${mass_list//,/ }"
  if [ "$coupling" = "all" ] || [ "$coupling" = "f*" ]; then
    couplings=(f1p0 f0p5 f0p1)
  else
    couplings=("$coupling")
  fi
  for mass in $mass_list; do
    for coup in "${couplings[@]}"; do
      workspaces+=("workspace_M${mass}_${coup}.root")
    done
  done
elif [ "$coupling" = "all" ] || [ "$coupling" = "f*" ]; then
  workspaces=(workspace_M*_f1p0.root workspace_M*_f0p5.root workspace_M*_f0p1.root)
else
  workspaces=(workspace_M*_"${coupling}".root)
fi

status=0
processed=0
skipped=0
failed=0

run_step() {
  if [ "$dry_run" -eq 1 ]; then
    printf '  '
    printf '%q ' "$@"
    printf '\n'
    return 0
  fi

  "$@"
}

for ws in "${workspaces[@]}"; do
  if [ ! -f "$ws" ]; then
    echo "SKIP missing workspace: $ws"
    skipped=$((skipped + 1))
    continue
  fi

  base="${ws%.root}"
  label="${base#workspace_}"
  mass="${label%%_*}"
  mass="${mass#M}"
  coup="${label#*_}"
  output_dir="higgsCombine_initialFit_${coup}"
  ws_path="${work_dir}/${ws}"
  output_json="impacts_${label}.json"
  output_plot="impacts_${label}"

  echo "BEGIN impacts ${label}"

  if [ "$dry_run" -eq 1 ]; then
    printf '  mkdir -p %q\n' "$output_dir"
    printf '  cd %q\n' "$output_dir"
  else
    mkdir -p "$output_dir"
    cd "$output_dir" || exit 1
  fi

  run_step combineTool.py -M Impacts \
    -d "$ws_path" \
    -m "$mass" \
    --doInitialFit \
    --robustFit 1 \
    --rMin 0 \
    --rMax 50
  rc=$?
  if [ "$dry_run" -eq 0 ]; then
    cd "$work_dir" || exit 1
  fi
  if [ "$rc" -ne 0 ]; then
    echo "END impacts ${label} failed at initial fit rc=$rc"
    failed=$((failed + 1))
    status=$rc
    continue
  fi
  if [ "$dry_run" -eq 0 ]; then
    cd "$output_dir" || exit 1
  fi

  run_step combineTool.py -M Impacts \
    -d "$ws_path" \
    -m "$mass" \
    --doFits \
    --robustFit 1 \
    --rMin 0 \
    --rMax 50 \
    --parallel "$parallel_jobs"
  rc=$?
  if [ "$dry_run" -eq 0 ]; then
    cd "$work_dir" || exit 1
  fi
  if [ "$rc" -ne 0 ]; then
    echo "END impacts ${label} failed at nuisance fits rc=$rc"
    failed=$((failed + 1))
    status=$rc
    continue
  fi
  if [ "$dry_run" -eq 0 ]; then
    cd "$output_dir" || exit 1
  fi

  run_step combineTool.py -M Impacts \
    -d "$ws_path" \
    -m "$mass" \
    -o "$output_json" \
    --rMin 0 \
    --rMax 50
  rc=$?
  if [ "$dry_run" -eq 0 ]; then
    cd "$work_dir" || exit 1
  fi
  if [ "$rc" -ne 0 ]; then
    echo "END impacts ${label} failed at json output rc=$rc"
    failed=$((failed + 1))
    status=$rc
    continue
  fi
  if [ "$dry_run" -eq 0 ]; then
    cd "$output_dir" || exit 1
  fi

  run_step plotImpacts.py \
    -i "$output_json" \
    -o "$output_plot"
  rc=$?
  if [ "$dry_run" -eq 0 ]; then
    cd "$work_dir" || exit 1
  fi
  if [ "$rc" -ne 0 ]; then
    echo "END impacts ${label} failed at plotting rc=$rc"
    failed=$((failed + 1))
    status=$rc
    continue
  fi

  echo "END impacts ${label} rc=0"
  processed=$((processed + 1))
done

echo "Summary: coupling=$coupling processed=$processed skipped=$skipped failed=$failed"
exit "$status"
