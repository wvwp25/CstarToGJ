#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
signal_base="/eos/user/h/hsiaoche/Signal"

usage() {
    echo "Run the common C-star event analysis."
    echo
    echo "Usage:"
    echo "  $0 -f COUPLING -m MASS"
    echo "  $0 -f COUPLING"
    echo "  $0 -all"
    echo "  $0 INPUT_ROOT_FILE [OUTPUT_ROOT_FILE]"
    echo
    echo "Options:"
    echo "  -f COUPLING  Coupling label, for example f0p1, f0p5, or f1p0."
    echo "  -m MASS      Mass label, accepted as M1200 or 1200."
    echo "  -all         Run every discovered mass and coupling sequentially."
    echo "  --all, -a    Aliases for -all."
    echo "  -help        Show this help message and exit."
    echo "  --help, -h   Aliases for -help."
    echo
    echo "Examples:"
    echo "  $0 -f f0p1 -m M1200    Run one sample."
    echo "  $0 -f f0p1             Run all existing f0p1 masses."
    echo "  $0 -all                 Run all existing samples."
    echo
    echo "The script discovers inputs under $signal_base, looks up the cross"
    echo "section in $script_dir/cross_sections.csv, and writes CstarToGJ.root,"
    echo "plots, and text summaries into each sample's EOS directory. Existing"
    echo "outputs with the same names are overwritten."
}

if [[ ${1:-} == "-help" || ${1:-} == "--help" || ${1:-} == "-h" ]]; then
    (( $# == 1 )) || { usage >&2; exit 2; }
    usage
    exit 0
fi

run_discovered_inputs() {
    local requested_coupling="${1:-}"
    local -a inputs=()
    local input sample mass found_coupling

    while IFS= read -r input; do
        inputs+=("$input")
    done < <(find "$signal_base" -mindepth 2 -maxdepth 2 -type f \
        -name 'CstarToGJ_M*_f*p*_13TeV_NANOAOD.root' | sort -V)

    local -a selected=()
    for input in "${inputs[@]}"; do
        sample="$(basename "$input" .root)"
        [[ "$sample" =~ ^CstarToGJ_M([0-9]+)_(f[0-9]+p[0-9]+)_13TeV_NANOAOD$ ]] || continue
        mass="${BASH_REMATCH[1]}"
        found_coupling="${BASH_REMATCH[2]}"
        if [[ -z "$requested_coupling" || "$found_coupling" == "$requested_coupling" ]]; then
            selected+=("$mass,$found_coupling")
        fi
    done

    if (( ${#selected[@]} == 0 )); then
        echo "No matching C-star input samples found under $signal_base" >&2
        exit 1
    fi

    echo "Running ${#selected[@]} signal samples sequentially"
    for sample in "${selected[@]}"; do
        IFS=, read -r mass found_coupling <<< "$sample"
        echo "===== M$mass $found_coupling ====="
        "$script_dir/runAna.sh" -f "$found_coupling" -m "M$mass"
    done
}

if [[ ${1:-} == "-all" || ${1:-} == "--all" || ${1:-} == "-a" ]]; then
    (( $# == 1 )) || { usage >&2; exit 2; }
    run_discovered_inputs
    exit 0
fi

if [[ ${1:-} == -* ]]; then
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
        run_discovered_inputs "$coupling"
        exit 0
    fi
    mass="${mass_arg#M}"
    if [[ ! "$mass" =~ ^[0-9]+$ ]]; then
        echo "Invalid mass or coupling: mass=$mass_arg coupling=$coupling" >&2
        usage >&2
        exit 2
    fi
    sample_name="CstarToGJ_M${mass}_${coupling}_13TeV_NANOAOD"
    sample_dir="$signal_base/$sample_name"
    input_file="$sample_dir/$sample_name.root"
    output_file="$sample_dir/CstarToGJ.root"
else
    if (( $# < 1 || $# > 2 )); then
        usage >&2
        exit 2
    fi
    input_file="$(readlink -f "$1")"
    if (( $# == 2 )); then
        output_file="$2"
    else
        output_file="$(dirname "$input_file")/CstarToGJ.root"
    fi
fi

if [[ ! -f "$input_file" ]]; then
    echo "Input ROOT file does not exist: $input_file" >&2
    exit 1
fi

output_dir="$(dirname "$output_file")"
mkdir -p "$output_dir"
output_dir="$(readlink -f "$output_dir")"
output_file="$output_dir/$(basename "$output_file")"

sample_name="$(basename "$input_file" .root)"
if [[ ! "$sample_name" =~ ^CstarToGJ_M([0-9]+)_(f[0-9]+p[0-9]+)_13TeV_NANOAOD$ ]]; then
    echo "Cannot infer mass and coupling from input filename: $sample_name" >&2
    echo "Expected: CstarToGJ_M<MASS>_f<COUPLING>_13TeV_NANOAOD.root" >&2
    exit 1
fi

mass="${BASH_REMATCH[1]}"
coupling="${BASH_REMATCH[2]}"
cross_section_file="$script_dir/cross_sections.csv"
cross_section="$(awk -F, -v mass="$mass" -v coupling="$coupling" '
    $1 == "CstarToGJ" && $2 == mass && $3 == coupling { print $4 }
' "$cross_section_file")"

if [[ -z "$cross_section" ]]; then
    echo "No cross section configured for mass=$mass coupling=$coupling" >&2
    echo "Add it to $cross_section_file" >&2
    exit 1
fi

echo "Sample: mass=$mass GeV coupling=$coupling cross_section=$cross_section pb"

source /cvmfs/cms.cern.ch/cmsset_default.sh
cd /eos/user/h/hsiaoche/CMSSW_13_3_0
eval "$(scram runtime -sh)"
cd "$output_dir"

export LD_LIBRARY_PATH=/afs/cern.ch/user/h/hsiaoche/.local/lib/python3.9/site-packages/correctionlib/lib:${LD_LIBRARY_PATH:-}

root -l -b -q \
    "$script_dir/loadAna.C(\"$input_file\",\"$output_file\",\"$script_dir\",$cross_section)"
