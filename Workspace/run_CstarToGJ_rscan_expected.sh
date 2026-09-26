#!/usr/bin/env bash
set -uo pipefail

usage() {
  cat <<'USAGE'
Usage:
  ./run_CstarToGJ_rscan_expected.sh [options]

Options:
  -f, --coupling VALUE   Coupling to process: f1p0, f0p5, f0p1. Default: f1p0
  -w, --work-dir PATH    Directory with generated workspace ROOT files.
                         Default: /eos/user/h/hsiaoche/workspace
  -c, --cmssw-dir PATH   CMSSW directory used for cmsenv. Default: /eos/user/h/hsiaoche/CMSSW_13_3_0
  --mass-list LIST       Space/comma separated masses, e.g. "1000 1200" or 1000,1200
  --expect-signal VALUE  Signal strength injected into the Asimov data. Default: 1
  --points VALUE         Number of grid scan points. Default: 100
  --r-min VALUE          Minimum r value for the scan. Default: 0
  --r-max VALUE          Maximum r value for the scan. Default: 10
  --dry-run              Print commands without executing them
  -h, --help             Show this help

Examples:
  ./run_CstarToGJ_rscan_expected.sh -f f1p0
  ./run_CstarToGJ_rscan_expected.sh -f f0p5 --mass-list "1000 1600"
  ./run_CstarToGJ_rscan_expected.sh -f f0p1 --points 200 --r-min -1 --r-max 5

Manual command for one mass point / coupling, e.g. M1000 f1p0:

  mkdir -p higgsCombine_initialFit_f1p0
  cd higgsCombine_initialFit_f1p0

  combine -M MultiDimFit /path/to/workspace_M1000_f1p0.root \
    -m 1000 \
    -n _scan_expected_r1_M1000_f1p0 \
    -t -1 \
    --expectSignal 1 \
    --algo grid --points 100 \
    --setParameterRanges 'P1=0,20:P2=0,15:P3=0,2' \
    --rMin 0 --rMax 10

  plot1DScan.py higgsCombine_scan_expected_r1_M1000_f1p0.MultiDimFit.mH1000.root \
    --main-label "M_{c*}=1000 GeV, f=1.0" \
    --POI r \
    -o scan_M1000_f1p0
USAGE
}

coupling="f1p0"
work_dir="/eos/user/h/hsiaoche/workspace"
cmssw_dir="/eos/user/h/hsiaoche/CMSSW_13_3_0"
mass_list=""
expect_signal=1
points=100
r_min=0
r_max=10
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
    --expect-signal)
      expect_signal="${2:-}"
      shift 2
      ;;
    --points)
      points="${2:-}"
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
  f1p0|f0p5|f0p1) ;;
  *)
    echo "Unsupported coupling: $coupling" >&2
    echo "Use one of: f1p0, f0p5, f0p1" >&2
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
  for mass in $mass_list; do
    workspaces+=("workspace_M${mass}_${coupling}.root")
  done
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

coupling_label() {
  case "$1" in
    f1p0) printf '1.0' ;;
    f0p5) printf '0.5' ;;
    f0p1) printf '0.1' ;;
    *) printf '%s' "$1" ;;
  esac
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
  output_dir="higgsCombine_rscan_expected_r${expect_signal}_${coupling}"
  ws_path="${work_dir}/${ws}"
  expected_label="expected_r${expect_signal}_${label}"
  scan_root="higgsCombine_scan_${expected_label}.MultiDimFit.mH${mass}.root"
  scan_plot="scan_${label}"
  main_label="Expected r=${expect_signal}: M_{c*}=${mass} GeV, f=$(coupling_label "$coupling")"

  echo "BEGIN r-scan ${label}"

  if [ "$dry_run" -eq 1 ]; then
    printf '  mkdir -p %q\n' "$output_dir"
    printf '  cd %q\n' "$output_dir"
  else
    mkdir -p "$output_dir"
    cd "$output_dir" || exit 1
  fi

  run_step combine -M MultiDimFit "$ws_path" \
    -m "$mass" \
    -n "_scan_${expected_label}" \
    -t -1 \
    --expectSignal "$expect_signal" \
    --algo grid --points "$points" \
    --setParameterRanges 'P1=0,20:P2=0,15:P3=0,2' \
    --rMin "$r_min" --rMax "$r_max"
  rc=$?
  if [ "$rc" -ne 0 ]; then
    echo "END r-scan ${label} failed at combine rc=$rc"
    failed=$((failed + 1))
    status=$rc
    if [ "$dry_run" -eq 0 ]; then
      cd "$work_dir" || exit 1
    fi
    continue
  fi

  run_step plot1DScan.py "$scan_root" \
    --main-label "$main_label" \
    --POI r \
    -o "$scan_plot"
  rc=$?
  if [ "$dry_run" -eq 0 ]; then
    cd "$work_dir" || exit 1
  fi
  if [ "$rc" -ne 0 ]; then
    echo "END r-scan ${label} failed at plotting rc=$rc"
    failed=$((failed + 1))
    status=$rc
    continue
  fi

  processed=$((processed + 1))
  echo "END r-scan ${label} rc=0"
done

echo "Summary: coupling=$coupling expect_signal=$expect_signal processed=$processed skipped=$skipped failed=$failed output_dir=higgsCombine_rscan_expected_r${expect_signal}_${coupling}"
exit "$status"
