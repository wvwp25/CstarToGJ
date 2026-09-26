#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
data_dir="/eos/user/h/hsiaoche/Data"
cmssw_dir="/eos/user/h/hsiaoche/CMSSW_13_3_0"
input_name="SinglePhoton_Run2017BCDEF-UL2017_MiniAODv2_NanoAODv9-v1_NANOAOD.root"
build_dir="$data_dir/.rootbuild"
masses=(1000 1200 1400 1600 1800 2000 2200 2400 2600 2800 3000)

usage() {
    echo "Run the 2017 data background workflow."
    echo
    echo "Usage:"
    echo "  $0 -make"
    echo "  $0 -m MASS"
    echo "  $0 -workspace [MASS]"
    echo "  $0 -all"
    echo
    echo "Options:"
    echo "  -make              Create Run2017BCDEF_BG.root from the combined NanoAOD."
    echo "  -m MASS            Fit one mass and build its background workspace."
    echo "                     MASS may be written as 1000 or M1000."
    echo "  -workspace [MASS]  Build workspace(s) from existing mass-fit files."
    echo "  -all               Recreate the histogram, fit every mass, and build all"
    echo "                     background workspaces."
    echo "  -help, --help, -h  Show this help message."
    echo
    echo "Macros are read from $script_dir. Inputs and outputs remain in $data_dir."
}

die() {
    echo "Error: $*" >&2
    exit 1
}

normalize_mass() {
    local mass="${1#M}"
    [[ "$mass" =~ ^[0-9]+$ ]] || die "Invalid mass: $1"

    local supported
    for supported in "${masses[@]}"; do
        if [[ "$mass" == "$supported" ]]; then
            echo "$mass"
            return
        fi
    done
    die "Unsupported mass: $1 (supported: ${masses[*]})"
}

root_macro() {
    local invocation="$1"
    root -l -b -q \
        -e "gSystem->SetBuildDir(\"$build_dir\", true);" \
        "$invocation"
}

make_histogram() {
    [[ -f "$data_dir/$input_name" ]] || \
        die "Missing input: $data_dir/$input_name"
    echo "===== Creating the common 2017 data histogram ====="
    root_macro \
        "$script_dir/make_Run2017BCDEF_BG.C+(\"$data_dir/$input_name\",\"$data_dir/Run2017BCDEF_BG.root\")"
    [[ -f "$data_dir/Run2017BCDEF_BG.root" ]] || \
        die "The histogram macro did not create Run2017BCDEF_BG.root"
}

fit_mass() {
    local mass="$1"
    [[ -f "$data_dir/Run2017BCDEF_BG.root" ]] || \
        die "Missing $data_dir/Run2017BCDEF_BG.root; run $0 -make first"
    [[ -f "/eos/user/h/hsiaoche/Signal/CstarToGJ_M${mass}_f1p0_13TeV_NANOAOD/signal_DSCB_workspace_paramSyst.root" ]] || \
        die "Missing f1p0 signal workspace for M$mass"
    echo "===== Fitting the M$mass background ====="
    root_macro "$script_dir/Bkg_model.C+($mass)"
    [[ -f "$data_dir/Run2017BCDEF_BG_M${mass}.root" ]] || \
        die "The fit macro did not create Run2017BCDEF_BG_M${mass}.root"
}

build_workspaces() {
    local mass="${1:-0}"
    if (( mass == 0 )); then
        echo "===== Building all background workspaces ====="
    else
        echo "===== Building the M$mass background workspace ====="
    fi
    root_macro "$script_dir/build_bkg_workspace_simultaneous.C+($mass)"
    if (( mass == 0 )); then
        local expected_mass
        for expected_mass in "${masses[@]}"; do
            [[ -f "$data_dir/bkg_workspace_M${expected_mass}_f1p0.root" ]] || \
                die "Missing output workspace for M$expected_mass"
        done
    else
        [[ -f "$data_dir/bkg_workspace_M${mass}_f1p0.root" ]] || \
            die "Missing output workspace for M$mass"
    fi
}

for required_file in make_Run2017BCDEF_BG.C Run2017BCDEF.h Bkg_model.C \
    build_bkg_workspace_simultaneous.C; do
    [[ -f "$script_dir/$required_file" ]] || \
        die "Missing source file: $script_dir/$required_file"
done

if (( $# == 0 )); then
    usage >&2
    exit 2
fi

case "$1" in
    -help|--help|-h)
        (( $# == 1 )) || { usage >&2; exit 2; }
        usage
        exit 0
        ;;
    -make)
        (( $# == 1 )) || { usage >&2; exit 2; }
        mode="make"
        ;;
    -m)
        (( $# == 2 )) || { usage >&2; exit 2; }
        mode="mass"
        requested_mass="$(normalize_mass "$2")"
        ;;
    -workspace)
        (( $# <= 2 )) || { usage >&2; exit 2; }
        mode="workspace"
        if (( $# == 2 )); then
            requested_mass="$(normalize_mass "$2")"
        else
            requested_mass=0
        fi
        ;;
    -all)
        (( $# == 1 )) || { usage >&2; exit 2; }
        mode="all"
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac

source /cvmfs/cms.cern.ch/cmsset_default.sh
cd "$cmssw_dir"
eval "$(scram runtime -sh)"
mkdir -p "$build_dir"
cd "$data_dir"

case "$mode" in
    make)
        make_histogram
        ;;
    mass)
        fit_mass "$requested_mass"
        build_workspaces "$requested_mass"
        ;;
    workspace)
        build_workspaces "$requested_mass"
        ;;
    all)
        make_histogram
        for mass in "${masses[@]}"; do
            fit_mass "$mass"
        done
        build_workspaces 0
        ;;
esac

echo "Data workflow completed successfully."
