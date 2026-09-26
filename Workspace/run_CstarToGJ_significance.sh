#!/usr/bin/env bash
set -uo pipefail

usage() {
  cat <<'USAGE'
Usage:
  ./run_CstarToGJ_significance.sh [options]

Options:
  --card PATH            Single datacard/workspace to process. If omitted, use datacard_M*_${coupling}.txt
  -f, --coupling VALUE   Coupling to process: f1p0, f0p5, f0p1. Default: f1p0
  -w, --work-dir PATH    Directory for generated workspaces/results.
                         Default: /eos/user/h/hsiaoche/workspace
  -c, --cmssw-dir PATH   CMSSW directory used for cmsenv. Default: /eos/user/h/hsiaoche/CMSSW_13_3_0
  --mass-list LIST       Space/comma separated masses, e.g. "1000 1200" or 1000,1200
  --expected             Run expected Asimov significance with -t -1 --expectSignal VALUE
  --pvalue               Run Significance in p-value mode by appending Combine --pvalue
  --expect-signal VALUE  Expected signal used with --expected. Default: 1
  --name-tag VALUE       Suffix for combine output name. Default: significance
  --extra-args "ARGS"    Extra arguments appended to combine command
  --dry-run              Print commands without executing them
  -h, --help             Show this help

Examples:
  ./run_CstarToGJ_significance.sh --card datacard_M2400_f1p0.txt
  ./run_CstarToGJ_significance.sh -f f1p0 --mass-list "1600 2800"
  ./run_CstarToGJ_significance.sh -f f1p0 --mass-list 2400 --expected --expect-signal 1
  ./run_CstarToGJ_significance.sh --card datacard_M2400_f1p0.txt --pvalue
  ./run_CstarToGJ_significance.sh --card workspace_M2400_f1p0.root --extra-args "--rMin 0 --rMax 50"

Manual command:
  combine -M Significance datacard.txt -m MASS -n _significance_LABEL
USAGE
}

card=""
coupling="f1p0"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
card_dir="$script_dir"
work_dir="/eos/user/h/hsiaoche/workspace"
cmssw_dir="/eos/user/h/hsiaoche/CMSSW_13_3_0"
mass_list=""
expected=0
pvalue=0
expect_signal=1
name_tag="significance"
extra_args=""
dry_run=0

while [ "$#" -gt 0 ]; do
  case "$1" in
    --card)
      card="${2:-}"
      shift 2
      ;;
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
    --expected)
      expected=1
      shift
      ;;
    --pvalue)
      pvalue=1
      shift
      ;;
    --expect-signal)
      expect_signal="${2:-}"
      shift 2
      ;;
    --name-tag)
      name_tag="${2:-}"
      shift 2
      ;;
    --extra-args)
      extra_args="${2:-}"
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
  echo "Work directory not found: $work_dir" >&2
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
inputs=()
if [ -n "$card" ]; then
  inputs+=("$card")
elif [ -n "$mass_list" ]; then
  mass_list="${mass_list//,/ }"
  for mass in $mass_list; do
    inputs+=("datacard_M${mass}_${coupling}.txt")
  done
else
  shopt -s nullglob
  # shellcheck disable=SC2206
  inputs=("$card_dir"/datacard_M*_"$coupling".txt)
  shopt -u nullglob
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

mass_from_input() {
  local input="$1"
  local base
  base="$(basename "$input")"

  case "$base" in
    datacard_M*_${coupling}.txt)
      base="${base#datacard_M}"
      printf '%s\n' "${base%_"${coupling}".txt}"
      ;;
    workspace_M*_${coupling}.root)
      base="${base#workspace_M}"
      printf '%s\n' "${base%_"${coupling}".root}"
      ;;
    *)
      printf '125\n'
      ;;
  esac
}

label_from_input() {
  local input="$1"
  local base
  base="$(basename "$input")"
  base="${base%.txt}"
  base="${base%.root}"
  base="${base#datacard_}"
  base="${base#workspace_}"
  printf '%s\n' "$base"
}

for input in "${inputs[@]}"; do
  if [[ "$input" != /* && -f "$card_dir/$input" ]]; then
    input="$card_dir/$input"
  fi
  if [ ! -f "$input" ]; then
    echo "SKIP missing input: $input"
    skipped=$((skipped + 1))
    continue
  fi

  mass="$(mass_from_input "$input")"
  label="$(label_from_input "$input")"
  output_dir="higgsCombine_initialFit_${coupling}"
  input_path="$input"
  case "$input_path" in
    /*) ;;
    *) input_path="${work_dir}/${input_path}" ;;
  esac
  name="_${name_tag}_${label}"
  cmd=(combine -M Significance "$input_path" -m "$mass" -n "$name")

  if [ "$expected" -eq 1 ]; then
    cmd+=(-t -1 --expectSignal "$expect_signal")
  fi

  if [ "$pvalue" -eq 1 ]; then
    cmd+=(--pvalue)
  fi
  if [ -n "$extra_args" ]; then
    # shellcheck disable=SC2206
    extra_array=($extra_args)
    cmd+=("${extra_array[@]}")
  fi

  echo "BEGIN Significance ${label}"

  if [ "$dry_run" -eq 1 ]; then
    printf '  mkdir -p %q\n' "$output_dir"
    printf '  cd %q\n' "$output_dir"
  else
    mkdir -p "$output_dir"
    cd "$output_dir" || exit 1
  fi

  run_step "${cmd[@]}"
  rc=$?
  if [ "$dry_run" -eq 0 ]; then
    cd "$work_dir" || exit 1
  fi
  echo "END Significance ${label} rc=$rc"

  if [ "$rc" -ne 0 ]; then
    failed=$((failed + 1))
    status=$rc
    continue
  fi

  processed=$((processed + 1))
done

echo "Summary: coupling=$coupling expected=$expected pvalue=$pvalue processed=$processed skipped=$skipped failed=$failed output_dir=higgsCombine_initialFit_${coupling}"
exit "$status"
