#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

source /cvmfs/cms.cern.ch/cmsset_default.sh
cd /eos/user/h/hsiaoche/CMSSW_13_3_0
eval "$(scram runtime -sh)"
cd "$script_dir"

export LD_LIBRARY_PATH=/afs/cern.ch/user/h/hsiaoche/.local/lib/python3.9/site-packages/correctionlib/lib:${LD_LIBRARY_PATH:-}

root -l -b -q loadAna.C
