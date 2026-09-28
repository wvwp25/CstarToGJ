#!/usr/bin/env python3
"""Build the charge-inclusive c*/anti-c* efficiency report from ROOT outputs.

Unlike the older diagnostic cutflow, this script reads the histograms produced by
the actual signal event loop.  Consequently the generator matching, nominal
JEC/JER object selection, pileup reweighting, and charm-tagging scale factor are
the same ones used by the signal templates.
"""

from __future__ import annotations

import csv
import os
import re
from pathlib import Path

import ROOT  # type: ignore


ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)

BASE = Path(__file__).resolve().parent
SIGNAL_BASE = Path(os.environ.get("CSTAR_SIGNAL_BASE", "/eos/user/h/hsiaoche/Signal"))
OUT = BASE / "correct_signal_efficiency_report"
ANALYSIS_SOURCE = BASE / "CstarToGJ_analysis.C"
CROSS_SECTIONS = BASE / "cross_sections.csv"
SAMPLE = re.compile(
    r"CstarToGJ_M(?P<mass>\d+)_(?P<coupling>f(?:0p1|0p5|1p0))_13TeV_NANOAOD"
)
CHARGE_INCLUSIVE_CSTAR = re.compile(
    r"(?:std::)?abs\s*\(\s*GenPart_pdgId\s*\[\s*i\s*\]\s*\)\s*==\s*4000004"
)
CHARGE_INCLUSIVE_CHARM = re.compile(
    r"(?:std::)?abs\s*\(\s*GenPart_pdgId\s*\[\s*i\s*\]\s*\)\s*==\s*4"
)
LUMI_PB = 41800.0


def integral(hist) -> float:
    """Return the same in-range integral used by the template workflow."""
    return float(hist.Integral())


def read_cross_sections() -> dict[tuple[int, str], float]:
    values = {}
    with CROSS_SECTIONS.open(newline="") as handle:
        for row in csv.reader(line for line in handle if not line.startswith("#")):
            model, mass, coupling, cross_section, _ = row
            if model == "CstarToGJ":
                values[(int(mass), coupling)] = float(cross_section)
    return values


def read_sample(directory: Path) -> dict[str, float | int | str]:
    match = SAMPLE.fullmatch(directory.name)
    if match is None:
        raise ValueError(directory)

    output = directory / "CstarToGJ.root"
    nano = directory / f"{directory.name}.root"
    source_text = ANALYSIS_SOURCE.read_text()
    if not (
        CHARGE_INCLUSIVE_CSTAR.search(source_text)
        and CHARGE_INCLUSIVE_CHARM.search(source_text)
    ):
        raise RuntimeError(
            f"{ANALYSIS_SOURCE} is charge-specific; both particle charges are required."
        )
    if output.stat().st_mtime < ANALYSIS_SOURCE.stat().st_mtime:
        raise RuntimeError(
            f"{output} predates the charge-inclusive analysis source; rerun this sample."
        )
    key = (int(match["mass"]), match["coupling"])
    try:
        xsec_pb = read_cross_sections()[key]
    except KeyError as error:
        raise RuntimeError(f"No cross section for mass/coupling {key} in {CROSS_SECTIONS}") from error
    produced = LUMI_PB * xsec_pb

    root_file = ROOT.TFile.Open(str(output))
    if not root_file or root_file.IsZombie():
        raise RuntimeError(f"Cannot open {output}")

    values = {}
    for name in ("h_m_cstar", "hM_gen", "hM_reco_selected", "sig"):
        hist = root_file.Get(name)
        if not hist:
            raise RuntimeError(f"Missing {name} in {output}")
        values[name] = integral(hist)

    # Count the true denominator independently of histogram ranges/weights.
    total = int(ROOT.RDataFrame("Events", str(nano)).Count().GetValue())
    root_file.Close()

    return {
        "coupling": match["coupling"],
        "mass_GeV": int(match["mass"]),
        "generated_events": total,
        "xsec_pb": xsec_pb,
        "gen_decay_percent": 100.0 * values["h_m_cstar"] / produced,
        "gen_match_percent": 100.0 * values["hM_gen"] / produced,
        # hM_reco_selected is filled with L*sigma*genWeight/sum(genWeight).
        "selection_efficiency_percent": 100.0 * values["hM_reco_selected"] / produced,
        # sig additionally contains nominal PU and c-tagging SF weights.
        "corrected_effective_efficiency_percent": 100.0 * values["sig"] / produced,
        "expected_yield": values["sig"],
    }


def make_csv(rows: list[dict[str, float | int | str]]) -> None:
    fields = list(rows[0])
    with (OUT / "signal_efficiency.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def make_markdown(rows: list[dict[str, float | int | str]]) -> None:
    explanation = [
        "# Corrected c* + anti-c* signal efficiency report",
        "",
        "The report is charge-inclusive: both c* -> c gamma and anti-c* -> anti-c gamma are included.",
        "All efficiencies use the generated signal normalization as denominator.",
        "`Selection efficiency` is obtained from the nominal selected histogram before PU and c-tag SFs.",
        "`Corrected effective efficiency` is obtained from the nominal signal template after PU reweighting and the c-tag SF.",
        "",
        "The applied event weight is",
        "",
        "`L * cross section * genWeight / sum(genWeight) * PU weight * c-tag SF`.",
        "",
    ]
    for coupling in ("f0p1", "f0p5", "f1p0"):
        selected = [row for row in rows if row["coupling"] == coupling]
        explanation += [
            f"## Coupling {coupling[1:].replace('p', '.')}",
            "",
            "| Mass (GeV) | Generated events | c*/anti-c* -> c/anti-c + gamma found (%) | Gen c/anti-c jet matched (%) | Full selection (%) | PU+c-tag corrected Aeff (%) | Expected yield |",
            "|--:|--:|--:|--:|--:|--:|--:|",
        ]
        for row in selected:
            explanation.append(
                f"| {row['mass_GeV']} | {row['generated_events']} | "
                f"{row['gen_decay_percent']:.3f} | {row['gen_match_percent']:.3f} | "
                f"{row['selection_efficiency_percent']:.3f} | "
                f"{row['corrected_effective_efficiency_percent']:.3f} | "
                f"{row['expected_yield']:.5g} |"
            )
        explanation.append("")
    (OUT / "signal_efficiency.md").write_text("\n".join(explanation))


def make_plot(rows: list[dict[str, float | int | str]], corrected: bool) -> None:
    value_field = (
        "corrected_effective_efficiency_percent"
        if corrected
        else "selection_efficiency_percent"
    )
    canvas_name = "c_corrected_efficiency" if corrected else "c_selection_efficiency"
    canvas = ROOT.TCanvas(canvas_name, canvas_name, 900, 720)
    canvas.SetLeftMargin(0.13)
    canvas.SetRightMargin(0.05)
    canvas.SetBottomMargin(0.12)
    canvas.SetTopMargin(0.10)
    canvas.SetGrid()
    y_max = 1.15 * max(float(row[value_field]) / 100.0 for row in rows)
    frame = canvas.DrawFrame(900.0, 0.0, 3100.0, y_max)
    ytitle = "Corrected effective acceptance #times efficiency" if corrected else "Signal selection efficiency"
    frame.SetTitle(f";m_{{c*/#bar{{c}}*}} [GeV];{ytitle}")
    frame.GetXaxis().SetTitleSize(0.045)
    frame.GetYaxis().SetTitleSize(0.045)

    colors = {"f0p1": ROOT.kBlue + 1, "f0p5": ROOT.kGreen + 2, "f1p0": ROOT.kRed + 1}
    markers = {"f0p1": 20, "f0p5": 21, "f1p0": 22}
    graphs = []
    legend = ROOT.TLegend(0.62, 0.20, 0.87, 0.38)
    legend.SetBorderSize(0)
    legend.SetFillStyle(0)
    for coupling in ("f0p1", "f0p5", "f1p0"):
        selected = [row for row in rows if row["coupling"] == coupling]
        graph = ROOT.TGraph(len(selected))
        for index, row in enumerate(selected):
            graph.SetPoint(
                index,
                float(row["mass_GeV"]),
                float(row[value_field]) / 100.0,
            )
        graph.SetLineColor(colors[coupling])
        graph.SetMarkerColor(colors[coupling])
        graph.SetMarkerStyle(markers[coupling])
        graph.SetLineWidth(2)
        graph.Draw("LP SAME")
        legend.AddEntry(graph, f"f = {coupling[1:].replace('p', '.')}", "lp")
        graphs.append(graph)

    legend.Draw()
    label = ROOT.TLatex()
    label.SetNDC()
    label.SetTextFont(52)
    label.SetTextSize(0.040)
    label.DrawLatex(0.235, 0.93, "Private work (CMS simulation)")
    label.SetTextFont(42)
    label.SetTextAlign(31)
    label.DrawLatex(0.95, 0.93, "41.8 fb^{-1} (13 TeV)")
    stem = "signal_corrected_effective_efficiency_vs_mass" if corrected else "signal_selection_efficiency_vs_mass"
    canvas.Modified()
    canvas.Update()
    canvas.SaveAs(str(OUT / f"{stem}.pdf"))
    canvas.SaveAs(str(OUT / f"{stem}.png"))


def main() -> None:
    OUT.mkdir(exist_ok=True)
    directories = sorted(
        (
            path
            for path in SIGNAL_BASE.glob("CstarToGJ_M*_f*_13TeV_NANOAOD")
            if SAMPLE.fullmatch(path.name)
        ),
        key=lambda path: (SAMPLE.fullmatch(path.name)["coupling"], int(SAMPLE.fullmatch(path.name)["mass"])),
    )
    if not directories:
        raise RuntimeError(f"No signal sample directories found under {SIGNAL_BASE}")
    rows = [read_sample(directory) for directory in directories]
    make_csv(rows)
    make_markdown(rows)
    make_plot(rows, corrected=False)
    make_plot(rows, corrected=True)
    print(f"Wrote {len(rows)} signal points to {OUT}")


if __name__ == "__main__":
    main()
