#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
signal_base="/eos/user/h/hsiaoche/Signal"

usage() {
    echo "Produce parameterized DSCB signal workspaces."
    echo
    echo "Usage:"
    echo "  $0 -f COUPLING -m MASS"
    echo "  $0 -f COUPLING"
    echo "  $0 -all"
    echo
    echo "Options:"
    echo "  -f COUPLING  Coupling label, for example f0p1, f0p5, or f1p0."
    echo "  -m MASS      Mass label, accepted as M1200 or 1200."
    echo "  -all         Process every completed analysis sample sequentially."
    echo "  --all, -a    Aliases for -all."
    echo "  -help        Show this help message and exit."
    echo "  --help, -h   Aliases for -help."
    echo
    echo "Examples:"
    echo "  $0 -f f0p1 -m M1200    Produce one workspace."
    echo "  $0 -f f0p1             Produce all existing f0p1 workspaces."
    echo "  $0 -all                 Produce all existing workspaces."
    echo
    echo "Each sample must already contain CstarToGJ.root. Fit ranges come from"
    echo "$script_dir/fit_ranges.csv. The workspace and fit plots are written"
    echo "into the sample's EOS directory, replacing files with the same names."
}

if [[ ${1:-} == "-help" || ${1:-} == "--help" || ${1:-} == "-h" ]]; then
    (( $# == 1 )) || { usage >&2; exit 2; }
    usage
    exit 0
fi

run_discovered_outputs() {
    local requested_coupling="${1:-}"
    local -a selected=()
    local input sample mass found_coupling

    while IFS= read -r input; do
        sample="$(basename "$(dirname "$input")")"
        [[ "$sample" =~ ^CstarToGJ_M([0-9]+)_(f[0-9]+p[0-9]+)_13TeV_NANOAOD$ ]] || continue
        mass="${BASH_REMATCH[1]}"
        found_coupling="${BASH_REMATCH[2]}"
        if [[ -z "$requested_coupling" || "$found_coupling" == "$requested_coupling" ]]; then
            selected+=("$mass,$found_coupling")
        fi
    done < <(find "$signal_base" -mindepth 2 -maxdepth 2 -type f \
        -name CstarToGJ.root | sort -V)

    if (( ${#selected[@]} == 0 )); then
        echo "No matching CstarToGJ.root files found under $signal_base" >&2
        exit 1
    fi

    echo "Producing workspaces for ${#selected[@]} signal samples sequentially"
    for sample in "${selected[@]}"; do
        IFS=, read -r mass found_coupling <<< "$sample"
        echo "===== M$mass $found_coupling ====="
        "$script_dir/runWorkspace.sh" -f "$found_coupling" -m "M$mass"
    done
}

if [[ ${1:-} == "-all" || ${1:-} == "--all" || ${1:-} == "-a" ]]; then
    (( $# == 1 )) || { usage >&2; exit 2; }
    run_discovered_outputs
    exit 0
fi

coupling=""
mass_arg=""
while getopts ":f:m:" option; do
    case "$option" in
        f) coupling="$OPTARG" ;;
        m) mass_arg="$OPTARG" ;;
        :) echo "Option -$OPTARG requires an argument" >&2; usage >&2; exit 2 ;;
        \?) echo "Unknown option: -$OPTARG" >&2; usage >&2; exit 2 ;;
    esac
done

if [[ -z "$coupling" || $OPTIND -le $# ]]; then
    usage >&2
    exit 2
fi

if [[ ! "$coupling" =~ ^f[0-9]+p[0-9]+$ ]]; then
    echo "Invalid coupling: $coupling" >&2
    usage >&2
    exit 2
fi
if [[ -z "$mass_arg" ]]; then
    run_discovered_outputs "$coupling"
    exit 0
fi

mass="${mass_arg#M}"
if [[ ! "$mass" =~ ^[0-9]+$ ]]; then
    echo "Invalid mass or coupling: mass=$mass_arg coupling=$coupling" >&2
    usage >&2
    exit 2
fi

sample="CstarToGJ_M${mass}_${coupling}_13TeV_NANOAOD"
sample_dir="$signal_base/$sample"
input_file="$sample_dir/CstarToGJ.root"
output_file="$sample_dir/signal_DSCB_workspace_paramSyst.root"

if [[ ! -f "$input_file" ]]; then
    echo "Analysis output does not exist: $input_file" >&2
    echo "Run: $script_dir/runAna.sh -f $coupling -m M$mass" >&2
    exit 1
fi

range="$(awk -F, -v mass="$mass" '
    $1 == mass { print $2 "," $3 "," $4 "," $5 }
' "$script_dir/fit_ranges.csv")"
if [[ -z "$range" ]]; then
    echo "No fit range configured for M$mass in $script_dir/fit_ranges.csv" >&2
    exit 1
fi

IFS=, read -r x_min x_max fit_min fit_max <<< "$range"
echo "Sample: mass=M$mass coupling=$coupling fit_range=[$fit_min,$fit_max] GeV"

source /cvmfs/cms.cern.ch/cmsset_default.sh
cd /eos/user/h/hsiaoche/CMSSW_13_3_0
eval "$(scram runtime -sh)"

root -l -b -q \
    "$script_dir/produce_signal_DSCB_workspace_paramSyst.C(\"$input_file\",\"$output_file\",$x_min,$x_max,$fit_min,$fit_max)"
