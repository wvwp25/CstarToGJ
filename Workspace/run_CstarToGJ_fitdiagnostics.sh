#!/usr/bin/env bash
set -uo pipefail

usage() {
  cat <<'USAGE'
Usage:
  ./run_CstarToGJ_fitdiagnostics.sh [options]

Options:
  -f, --coupling VALUE   Coupling to process: f1p0, f0p5, f0p1. Default: f1p0
  -w, --work-dir PATH    Directory with workspace ROOT files. Default: current directory
  -c, --cmssw-dir PATH   CMSSW directory used for cmsenv. Default: /eos/user/h/hsiaoche/CMSSW_13_3_0
  --mass-list LIST       Space/comma separated masses, e.g. "1000 1200" or 1000,1200
  --r-min VALUE          Minimum r value for FitDiagnostics. Default: 0
  --r-max VALUE          Maximum r value for FitDiagnostics. Default: 50
  --dry-run              Print commands without executing them
  -h, --help             Show this help

Examples:
  ./run_CstarToGJ_fitdiagnostics.sh -f f1p0
  ./run_CstarToGJ_fitdiagnostics.sh -f f0p5 --mass-list "1000 1200"
  ./run_CstarToGJ_fitdiagnostics.sh -f f0p1 --dry-run

Manual command for one mass point / coupling, e.g. M1000 f1p0:

  mkdir -p higgsCombine_initialFit_f1p0
  cd higgsCombine_initialFit_f1p0

  combine -M FitDiagnostics /path/to/workspace_M1000_f1p0.root \
    -m 1000 \
    -n _M1000_f1p0 \
    --rMin 0 --rMax 50 \
    --robustFit 1 \
    --saveShapes \
    --saveWithUncertainties \
    --saveNormalizations \
    --plots

  python3 $CMSSW_BASE/src/HiggsAnalysis/CombinedLimit/test/diffNuisances.py \
    fitDiagnostics_M1000_f1p0.root \
    --all --abs \
    > diffNuisances_M1000_f1p0.txt

  python3 $CMSSW_BASE/src/HiggsAnalysis/CombinedLimit/test/diffNuisances.py \
    fitDiagnostics_M1000_f1p0.root \
    --all \
    --pullDef relDiffAsymErrs \
    -g pulls_M1000_f1p0.root
USAGE
}

coupling="f1p0"
work_dir="$(pwd)"
cmssw_dir="/eos/user/h/hsiaoche/CMSSW_13_3_0"
mass_list=""
r_min=0
r_max=50
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

run_step_to_file() {
  output_file="$1"
  shift

  if [ "$dry_run" -eq 1 ]; then
    printf '  '
    printf '%q ' "$@"
    printf '> %q\n' "$output_file"
    return 0
  fi

  "$@" > "$output_file"
}

rename_plot_if_present() {
  src="$1"
  dst="$2"

  if [ "$dry_run" -eq 1 ]; then
    printf '  [ -f %q ] && mv -f %q %q\n' "$src" "$src" "$dst"
    return 0
  fi

  if [ -f "$src" ]; then
    mv -f "$src" "$dst"
  fi
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
  output_dir="higgsCombine_initialFit_${coupling}"
  ws_path="${work_dir}/${ws}"
  fit_file="fitDiagnostics_${label}.root"
  diff_file="diffNuisances_${label}.txt"
  pulls_file="pulls_${label}.root"

  echo "BEGIN FitDiagnostics ${label}"

  if [ "$dry_run" -eq 1 ]; then
    printf '  mkdir -p %q\n' "$output_dir"
    printf '  cd %q\n' "$output_dir"
  else
    mkdir -p "$output_dir"
    cd "$output_dir" || exit 1
  fi

  run_step combine -M FitDiagnostics "$ws_path" \
    -m "$mass" \
    -n "_${label}" \
    --rMin "$r_min" --rMax "$r_max" \
    --robustFit 1 \
    --saveShapes \
    --saveWithUncertainties \
    --saveNormalizations \
    --plots
  rc=$?
  if [ "$rc" -ne 0 ]; then
    echo "END FitDiagnostics ${label} failed at combine rc=$rc"
    failed=$((failed + 1))
    status=$rc
    if [ "$dry_run" -eq 0 ]; then
      cd "$work_dir" || exit 1
    fi
    continue
  fi

  rename_plot_if_present bin1_x_prefit.png "bin1_x_prefit_${label}.png"
  rename_plot_if_present bin1_x_prefit_logy.png "bin1_x_prefit_logy_${label}.png"
  rename_plot_if_present bin1_x_fit_b.png "bin1_x_fit_b_${label}.png"
  rename_plot_if_present bin1_x_fit_b_logy.png "bin1_x_fit_b_logy_${label}.png"
  rename_plot_if_present bin1_x_fit_s.png "bin1_x_fit_s_${label}.png"
  rename_plot_if_present bin1_x_fit_s_logy.png "bin1_x_fit_s_logy_${label}.png"
  rename_plot_if_present covariance_fit_b.png "covariance_fit_b_${label}.png"
  rename_plot_if_present covariance_fit_s.png "covariance_fit_s_${label}.png"

  run_step_to_file "$diff_file" \
    python3 "$CMSSW_BASE/src/HiggsAnalysis/CombinedLimit/test/diffNuisances.py" \
    "$fit_file" \
    --all --abs
  rc=$?
  if [ "$rc" -ne 0 ]; then
    echo "END FitDiagnostics ${label} failed at diffNuisances text rc=$rc"
    failed=$((failed + 1))
    status=$rc
    if [ "$dry_run" -eq 0 ]; then
      cd "$work_dir" || exit 1
    fi
    continue
  fi

  run_step python3 "$CMSSW_BASE/src/HiggsAnalysis/CombinedLimit/test/diffNuisances.py" \
    "$fit_file" \
    --all \
    --pullDef relDiffAsymErrs \
    -g "$pulls_file"
  rc=$?
  if [ "$dry_run" -eq 0 ]; then
    cd "$work_dir" || exit 1
  fi
  if [ "$rc" -ne 0 ]; then
    echo "END FitDiagnostics ${label} failed at pulls root rc=$rc"
    failed=$((failed + 1))
    status=$rc
    continue
  fi

  echo "END FitDiagnostics ${label} rc=0"
  processed=$((processed + 1))
done

echo "Summary: coupling=$coupling processed=$processed skipped=$skipped failed=$failed output_dir=higgsCombine_initialFit_${coupling}"
exit "$status"
