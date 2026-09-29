#!/usr/bin/env python3
"""Generate thesis-ready cutflows from the current analysis.

The production analysis source and ROOT outputs are never modified.  This tool
creates a temporary instrumented copy of CstarToGJ_analysis.C, runs that copy on
the requested EOS NanoAOD samples, and writes per-sample CSV files plus one
Markdown report.  By default it processes the representative M1800, f=1.0
sample; pass --all to process every configured mass and coupling.
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


BASE = Path(__file__).resolve().parent
DEFAULT_SIGNAL_BASE = Path("/eos/user/h/hsiaoche/Signal")
SAMPLE = re.compile(r"CstarToGJ_M(?P<mass>\d+)_(?P<coupling>f\d+p\d+)_13TeV_NANOAOD")


DECLARATIONS = r'''
    // Charge-inclusive cumulative cutflow (instrumented copy only).
    static const int nCutStages = 13;
    const char *cutLabels[nCutStages] = {
        "Total generated",
        "NanoAOD object availability",
        "Generated c*",
        "Generated c* to c + gamma",
        "Gen charm to gen-jet match (DeltaR < 0.2)",
        "Medium photon ID plus electron veto",
        "Photon pT > 240 GeV and |eta| < 1.4442",
        "Corrected jet pT > 170 GeV",
        "Jet |eta| < 2.4",
        "Tight jet ID",
        "Medium DeepJet charm tag",
        "Photon-jet DeltaR > 0.4",
        "Final selected (nominal scale factors)"
    };
    unsigned long long cutCount[nCutStages] = {};
    double cutWeighted[nCutStages] = {};
'''


PHOTON_COUNTERS = r'''
        bool cutPassPhotonID = false;
        bool cutPassPhotonKinematics = false;
        for (UInt_t cutPhoton = 0; cutPhoton < nPhoton; ++cutPhoton) {
            const bool passID = Photon_cutBased[cutPhoton] >= 2 &&
                                Photon_electronVeto[cutPhoton] == 1;
            if (passID) cutPassPhotonID = true;
            if (passID && Photon_pt[cutPhoton] >= 240.0 &&
                std::fabs(Photon_eta[cutPhoton]) < 1.4442)
                cutPassPhotonKinematics = true;
        }
        if (cutPassPhotonID) {
            ++cutCount[5]; cutWeighted[5] += weight_raw;
        }
        if (cutPassPhotonKinematics) {
            ++cutCount[6]; cutWeighted[6] += weight_raw;
        }
'''


JET_DECLARATIONS = r'''
        bool cutPassJetPt = false;
        bool cutPassJetEta = false;
        bool cutPassJetID = false;
        bool cutPassCTag = false;
        bool cutPassDeltaR = false;
'''


JET_TESTS = r'''
            const bool cutThisJetPt = ptNom >= 170.0;
            const bool cutThisJetEta = cutThisJetPt && std::fabs(Jet_eta[i]) < 2.4;
            const bool cutThisJetID = cutThisJetEta && Jet_jetId[i] >= 6;
            const bool cutThisJetCTag = cutThisJetID &&
                Jet_btagDeepFlavCvB[i] >= 0.340 &&
                Jet_btagDeepFlavCvL[i] >= 0.085;
            TLorentzVector cutJetRaw, cutJetNom;
            cutJetRaw.SetPtEtaPhiM(Jet_pt[i], Jet_eta[i], Jet_phi[i], Jet_mass[i]);
            cutJetNom.SetPtEtaPhiM(ptNom, Jet_eta[i], Jet_phi[i], massNom);
            const bool cutThisJetDeltaR = cutThisJetCTag &&
                g_nom.DeltaR(cutJetRaw) > 0.4 &&
                g_nom.DeltaR(cutJetNom) > 0.4;
            cutPassJetPt = cutPassJetPt || cutThisJetPt;
            cutPassJetEta = cutPassJetEta || cutThisJetEta;
            cutPassJetID = cutPassJetID || cutThisJetID;
            cutPassCTag = cutPassCTag || cutThisJetCTag;
            cutPassDeltaR = cutPassDeltaR || cutThisJetDeltaR;
'''


JET_COUNTERS = r'''
        if (cutPassJetPt) { ++cutCount[7]; cutWeighted[7] += weight_raw; }
        if (cutPassJetEta) { ++cutCount[8]; cutWeighted[8] += weight_raw; }
        if (cutPassJetID) { ++cutCount[9]; cutWeighted[9] += weight_raw; }
        if (cutPassCTag) { ++cutCount[10]; cutWeighted[10] += weight_raw; }
        if (cutPassDeltaR) { ++cutCount[11]; cutWeighted[11] += weight_raw; }
'''


WRITE_COUNTERS = r'''
    std::ofstream cutflowOut(artifactPath("cutflow.csv").Data());
    cutflowOut << "stage,events,weighted_yield,cumulative_raw_percent,"
                  "cumulative_weighted_percent\n";
    for (int cut = 0; cut < nCutStages; ++cut) {
        const double rawEfficiency = cutCount[0] > 0
            ? 100.0 * cutCount[cut] / cutCount[0] : 0.0;
        const double weightedEfficiency = cutWeighted[0] != 0.0
            ? 100.0 * cutWeighted[cut] / cutWeighted[0] : 0.0;
        cutflowOut << '"' << cutLabels[cut] << '"' << ","
                   << cutCount[cut] << "," << cutWeighted[cut] << ","
                   << rawEfficiency << "," << weightedEfficiency << "\n";
    }
    cutflowOut.close();
'''


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"expected one {label} insertion point, found {count}")
    return text.replace(old, new, 1)


def instrument(source: str) -> str:
    if "std::abs(GenPart_pdgId[i]) == 4000004" not in source:
        raise RuntimeError("analysis is not charge-inclusive for c*/anti-c*")
    if "std::abs(GenPart_pdgId[i]) == 4" not in source:
        raise RuntimeError("analysis is not charge-inclusive for c/anti-c")
    source = replace_once(
        source,
        "    Long64_t nbytes = 0, nb = 0;",
        DECLARATIONS + "\n    Long64_t nbytes = 0, nb = 0;",
        "cutflow declarations",
    )
    source = replace_once(
        source,
        "        double weight_raw = lumi_pb * xsec * genWeight / sum_genWeight;",
        "        double weight_raw = lumi_pb * xsec * genWeight / sum_genWeight;\n"
        "        ++cutCount[0]; cutWeighted[0] += weight_raw;",
        "generated-event counter",
    )
    source = replace_once(
        source,
        "        if (nPhoton < 1 || nJet < 1 || nGenPart <= 0 || nGenJet <= 0)   continue;",
        "        if (nPhoton < 1 || nJet < 1 || nGenPart <= 0 || nGenJet <= 0) continue;\n"
        "        ++cutCount[1]; cutWeighted[1] += weight_raw;",
        "object-availability counter",
    )
    source = replace_once(
        source,
        "        if (cstar.size() == 0) continue;",
        "        if (cstar.size() == 0) continue;\n"
        "        ++cutCount[2]; cutWeighted[2] += weight_raw;",
        "resonance counter",
    )
    source = replace_once(
        source,
        "        if (cstarIdx < 0) continue;",
        "        if (cstarIdx < 0) continue;\n"
        "        ++cutCount[3]; cutWeighted[3] += weight_raw;",
        "decay counter",
    )
    source = replace_once(
        source,
        "        if (best_deltaR_cJet > 0.2) continue;",
        "        if (best_deltaR_cJet > 0.2) continue;\n"
        "        ++cutCount[4]; cutWeighted[4] += weight_raw;",
        "generator-match counter",
    )
    source = replace_once(
        source,
        "        // Photon selection\n        int goodPhotonIdx = -1;",
        "        // Photon selection\n" + PHOTON_COUNTERS +
        "\n        int goodPhotonIdx = -1;",
        "photon counters",
    )
    source = replace_once(
        source,
        "        std::vector<int> goodJetOriginalIdx;",
        "        std::vector<int> goodJetOriginalIdx;\n" + JET_DECLARATIONS,
        "jet-stage declarations",
    )
    source = replace_once(
        source,
        "            double massNom = massAfterJes * jerNom;",
        "            double massNom = massAfterJes * jerNom;\n" + JET_TESTS,
        "jet-stage tests",
    )
    source = replace_once(
        source,
        "        if (goodJetP4s_nom.size() == 0) continue;",
        JET_COUNTERS + "\n        if (goodJetP4s_nom.size() == 0) continue;",
        "jet-stage counters",
    )
    source = replace_once(
        source,
        "        double weight_nominal   = weight_CTag_nom * photonIDSF;",
        "        double weight_nominal   = weight_CTag_nom * photonIDSF;\n"
        "        ++cutCount[12]; cutWeighted[12] += weight_nominal;",
        "final corrected counter",
    )
    source = replace_once(
        source,
        "    SetCMSStyle();",
        WRITE_COUNTERS + "\n    SetCMSStyle();",
        "cutflow output",
    )
    return source


def read_cross_sections(path: Path) -> dict[tuple[int, str], float]:
    values = {}
    with path.open(newline="") as handle:
        for row in csv.reader(line for line in handle if not line.lstrip().startswith("#")):
            if row and row[0] == "CstarToGJ":
                values[(int(row[1]), row[2])] = float(row[3])
    return values


def arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-m", "--mass", type=int, default=1800)
    parser.add_argument("-f", "--coupling", default="f1p0")
    parser.add_argument("--all", action="store_true", help="process all available samples")
    parser.add_argument("--signal-base", type=Path, default=DEFAULT_SIGNAL_BASE)
    parser.add_argument(
        "--cmssw-dir",
        type=Path,
        default=Path("/eos/user/h/hsiaoche/CMSSW_13_3_0"),
        help="CMSSW runtime used to execute the instrumented analysis",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_SIGNAL_BASE / "cutflow_report",
    )
    parser.add_argument("--keep-work", action="store_true")
    parser.add_argument(
        "--prepare-only",
        action="store_true",
        help="validate instrumentation without running ROOT",
    )
    return parser.parse_args()


def discover_samples(args: argparse.Namespace) -> list[tuple[int, str, Path]]:
    found = []
    for directory in args.signal_base.glob("CstarToGJ_M*_f*_13TeV_NANOAOD"):
        match = SAMPLE.fullmatch(directory.name)
        if not match:
            continue
        mass, coupling = int(match["mass"]), match["coupling"]
        if not args.all and (mass != args.mass or coupling != args.coupling):
            continue
        nano = directory / f"{directory.name}.root"
        if nano.is_file():
            found.append((mass, coupling, nano))
    return sorted(found, key=lambda item: (item[1], item[0]))


def write_markdown(output_dir: Path, csv_files: list[Path]) -> None:
    lines = [
        "# Charge-inclusive c* signal cutflow",
        "",
        "Both c* -> c gamma and anti-c* -> anti-c gamma are included.",
        "Efficiencies are cumulative relative to all generated events.",
        "",
    ]
    for path in csv_files:
        match = SAMPLE.search(path.stem)
        title = match.group(0) if match else path.stem
        lines += [f"## {title}", "", "| Selection | Events | Weighted yield | Raw efficiency (%) | Weighted efficiency (%) |", "|---|---:|---:|---:|---:|"]
        with path.open(newline="") as handle:
            for row in csv.DictReader(handle):
                lines.append(
                    f"| {row['stage']} | {row['events']} | {float(row['weighted_yield']):.6g} | "
                    f"{float(row['cumulative_raw_percent']):.3f} | "
                    f"{float(row['cumulative_weighted_percent']):.3f} |"
                )
        lines.append("")
    (output_dir / "cutflow.md").write_text("\n".join(lines))


def main() -> int:
    args = arguments()
    try:
        instrument((BASE / "CstarToGJ_analysis.C").read_text())
        samples = discover_samples(args)
        cross_sections = read_cross_sections(BASE / "cross_sections.csv")
    except (OSError, RuntimeError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    if not samples:
        print("ERROR: no matching EOS signal samples", file=sys.stderr)
        return 1
    if args.prepare_only:
        print(f"Instrumentation validated for {len(samples)} selected sample(s).")
        return 0

    args.output_dir.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="cstar-cutflow-", dir="/tmp"))
    csv_files = []
    try:
        (work / "CstarToGJ_analysis.C").write_text(
            instrument((BASE / "CstarToGJ_analysis.C").read_text())
        )
        shutil.copy2(BASE / "CstarToGJ_analysis.h", work / "CstarToGJ_analysis.h")
        for index, (mass, coupling, nano) in enumerate(samples, 1):
            key = (mass, coupling)
            if key not in cross_sections:
                raise RuntimeError(f"missing cross section for M{mass} {coupling}")
            sample = nano.stem
            sample_work = work / sample
            sample_work.mkdir()
            output_root = sample_work / "cutflow_diagnostic.root"
            command = (
                f'{BASE / "loadAna.C"}("{nano}","{output_root}",'
                f'"{work}",{cross_sections[key]:.17g})'
            )
            print(f"[{index}/{len(samples)}] {sample}", flush=True)
            environment = os.environ.copy()
            correction_lib = "/afs/cern.ch/user/h/hsiaoche/.local/lib/python3.9/site-packages/correctionlib/lib"
            environment["LD_LIBRARY_PATH"] = correction_lib + ":" + environment.get("LD_LIBRARY_PATH", "")
            root_command = shlex.join(["root", "-l", "-b", "-q", command])
            shell_command = "\n".join(
                [
                    "set -euo pipefail",
                    "source /cvmfs/cms.cern.ch/cmsset_default.sh",
                    f"cd {shlex.quote(str(args.cmssw_dir))}",
                    'eval "$(scram runtime -sh)"',
                    f"cd {shlex.quote(str(sample_work))}",
                    f"export LD_LIBRARY_PATH={shlex.quote(correction_lib)}:${{LD_LIBRARY_PATH:-}}",
                    f"exec {root_command}",
                ]
            )
            result = subprocess.run(
                ["bash", "-c", shell_command],
                env=environment,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True,
            )
            produced = sample_work / "cutflow.csv"
            if result.returncode or not produced.is_file():
                raise RuntimeError(
                    f"ROOT failed for {sample} (exit {result.returncode}):\n"
                    + result.stderr[-3000:]
                )
            destination = args.output_dir / f"{sample}_cutflow.csv"
            shutil.copy2(produced, destination)
            csv_files.append(destination)
        write_markdown(args.output_dir, csv_files)
        print(f"Wrote {len(csv_files)} cutflow CSV file(s) and Markdown table to {args.output_dir}")
    except (OSError, RuntimeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    finally:
        if args.keep_work:
            print(f"Kept instrumented work directory: {work}")
        else:
            shutil.rmtree(work, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
