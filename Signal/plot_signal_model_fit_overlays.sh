#!/usr/bin/env bash
set -euo pipefail

input_dir="${1:-/eos/user/h/hsiaoche/Signal}"
output_dir="${2:-/eos/user/h/hsiaoche/Signal/signal_model_paramSyst_fit_overlays}"
workspace_file="${3:-signal_DSCB_workspace_paramSyst.root}"

if [[ ! -d "${input_dir}" ]]; then
    echo "ERROR: signal input directory does not exist: ${input_dir}" >&2
    exit 1
fi

# Keep plot products on EOS. A custom output is allowed only under /eos.
case "${output_dir}" in
    /eos/*) ;;
    *)
        echo "ERROR: output directory must be on EOS: ${output_dir}" >&2
        exit 1
        ;;
esac

mkdir -p "${output_dir}"

python3 - "${input_dir}" "${output_dir}" "${workspace_file}" <<'PY'
import glob
import os
import re
import sys
from array import array

import ROOT

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)
ROOT.gStyle.SetLineWidth(2)
# Suppress EOS's failed remote-redirect diagnostic; explicit file/object checks
# below still make genuine input and output failures fatal.
ROOT.gErrorIgnoreLevel = ROOT.kFatal + 1

input_dir, output_dir, workspace_file = sys.argv[1:4]
pattern = os.path.join(input_dir, "CstarToGJ_M*_f*_13TeV_NANOAOD", workspace_file)
name_re = re.compile(r"CstarToGJ_M(\d+)_f([0-9p]+)_13TeV_NANOAOD$")

def coupling_value(tag):
    return float(tag.replace("p", "."))

models = []
for path in glob.glob(pattern):
    directory = os.path.basename(os.path.dirname(path))
    match = name_re.match(directory)
    if not match:
        continue
    data_path = os.path.join(os.path.dirname(path), "CstarToGJ.root")
    if not os.path.isfile(data_path):
        print(f"WARNING: missing charge-inclusive analysis output {data_path}", file=sys.stderr)
        continue
    if os.path.getmtime(path) < os.path.getmtime(data_path):
        print(
            f"ERROR: stale signal model {path} is older than {data_path}; "
            "rerun Signal/runWorkspace.sh for this sample",
            file=sys.stderr,
        )
        raise SystemExit(1)

    root_file = ROOT.TFile.Open(path)
    if not root_file or root_file.IsZombie():
        print(f"WARNING: cannot open {path}", file=sys.stderr)
        continue
    ws = root_file.Get("ws")
    if not ws:
        print(f"WARNING: no RooWorkspace 'ws' in {path}", file=sys.stderr)
        root_file.Close()
        continue
    x = ws.var("x")
    pdf = ws.pdf("dscb")
    if not x or not pdf:
        print(f"WARNING: missing x or dscb in {path}", file=sys.stderr)
        root_file.Close()
        continue
    data_file = ROOT.TFile.Open(data_path)
    if not data_file or data_file.IsZombie():
        print(f"WARNING: cannot open {data_path}", file=sys.stderr)
        root_file.Close()
        continue
    # Use the corrected charge-inclusive nominal template (c* + anti-c*) that
    # supplies the datacard signal yield and nominal DSCB fit. It includes the
    # nominal PU, photon-ID, and c-tag weights.
    hist = data_file.Get("sig")
    if not hist:
        print(f"WARNING: no sig in {data_path}", file=sys.stderr)
        data_file.Close()
        root_file.Close()
        continue
    hist.SetDirectory(0)
    data_file.Close()
    models.append({
        "mass": int(match.group(1)),
        "coupling": coupling_value(match.group(2)),
        "file": root_file,
        "hist": hist,
        "x": x,
        "pdf": pdf,
    })

models.sort(key=lambda item: (item["mass"], item["coupling"]))
if not models:
    raise SystemExit(f"No workspaces found with pattern: {pattern}")

colors = {
    0.1: ROOT.kCyan + 2,
    0.5: ROOT.kOrange + 7,
    1.0: ROOT.kMagenta + 2,
}

def sampled_pdf(model, xmin, xmax, npoints=900):
    observable = model["x"]
    pdf = model["pdf"]
    # The saved workspace observable is restricted to the fit interval.
    # Expand it so the analytic DSCB tails reach the frame boundaries.
    observable.setMin(min(xmin, float(observable.getMin())))
    observable.setMax(max(xmax, float(observable.getMax())))
    plot_min = xmin
    plot_max = xmax
    norm_set = ROOT.RooArgSet(observable)
    xs = array("d")
    ys = array("d")
    for index in range(npoints):
        value = plot_min + (plot_max - plot_min) * index / (npoints - 1)
        observable.setVal(value)
        xs.append(value)
        ys.append(float(pdf.getVal(norm_set)))
    return ROOT.TGraph(len(xs), xs, ys)

def normalized_data(model, xmin, xmax):
    hist = model["hist"]
    total = float(hist.Integral())
    xs, ys, exs, eys = array("d"), array("d"), array("d"), array("d")
    # Match RooPlot::Bins(60) in fit_DSCB_RooFit.C.
    display_min, display_max, display_bins = 0.0, 3000.0, 60
    width = (display_max - display_min) / display_bins
    for output_bin in range(display_bins):
        low = display_min + output_bin * width
        high = low + width
        center = 0.5 * (low + high)
        if center < xmin or center > xmax:
            continue
        content = 0.0
        error2 = 0.0
        for index in range(1, hist.GetNbinsX() + 1):
            source_center = float(hist.GetBinCenter(index))
            if low <= source_center < high:
                content += float(hist.GetBinContent(index))
                error2 += float(hist.GetBinError(index)) ** 2
        scale = total * width
        xs.append(center)
        ys.append(content / scale if scale else 0.0)
        exs.append(0.0)
        eys.append(error2 ** 0.5 / scale if scale else 0.0)
    return ROOT.TGraphErrors(len(xs), xs, ys, exs, eys)

masses = sorted({model["mass"] for model in models})
for mass in masses:
    group = [model for model in models if model["mass"] == mass]
    xmin, xmax = 0.7 * mass, 1.3 * mass
    plots = [(model, normalized_data(model, xmin, xmax),
              sampled_pdf(model, xmin, xmax)) for model in group]
    # Size the frame for both the MC points (including their errors) and the
    # fitted curves.  Using only the points can clip a narrow DSCB peak.
    data_ymax = max(max(data.GetY()[index] + data.GetEY()[index]
                        for index in range(data.GetN()))
                    for _, data, _ in plots)
    curve_ymax = max(max(curve.GetY()[index]
                         for index in range(curve.GetN()))
                     for _, _, curve in plots)
    ymax = max(data_ymax, curve_ymax)

    canvas = ROOT.TCanvas(f"c_fit_overlay_{mass}", "", 900, 750)
    canvas.SetLeftMargin(0.13)
    canvas.SetRightMargin(0.05)
    canvas.SetBottomMargin(0.12)
    canvas.SetGridx(True)
    canvas.SetGridy(True)
    frame = canvas.DrawFrame(xmin, 0.0, xmax, 1.18 * ymax)
    frame.SetTitle(";m_{#gamma + jet} [GeV];Probability density [GeV^{-1}]")
    frame.GetXaxis().SetTitleSize(0.045)
    frame.GetYaxis().SetTitleSize(0.045)
    frame.GetYaxis().SetTitleOffset(1.35)

    legend = ROOT.TLegend(0.72, 0.69, 0.89, 0.86)
    legend.SetBorderSize(0)
    legend.SetFillStyle(0)
    legend.SetTextSize(0.035)
    marker_styles = {0.1: 20, 0.5: 21, 1.0: 22}
    for model, data, curve in plots:
        color = colors.get(model["coupling"], ROOT.kBlack)
        data.SetMarkerColor(color)
        data.SetLineColor(color)
        data.SetMarkerStyle(marker_styles.get(model["coupling"], 20))
        data.SetMarkerSize(0.85)
        curve.SetLineColor(color)
        curve.SetLineWidth(3)
        data.Draw("P E1 SAME")
        curve.Draw("L SAME")
        legend.AddEntry(data, f"f = {model['coupling']:g}", "lep")
    legend.Draw()

    mass_label = ROOT.TLatex()
    mass_label.SetNDC(True)
    mass_label.SetTextFont(42)
    mass_label.SetTextSize(0.042)
    mass_label.SetTextAlign(13)
    mass_label.DrawLatex(0.16, 0.86, f"c* = {mass / 1000.0:.1f} TeV")

    stem = os.path.join(output_dir, f"DSCB_fit_paramSyst_overlays_M{mass}")
    outputs = (stem + ".png", stem + ".pdf")
    for output in outputs:
        canvas.SaveAs(output)
        if not os.path.isfile(output) or os.path.getsize(output) == 0:
            raise RuntimeError(f"ROOT failed to write output plot: {output}")

for model in models:
    model["file"].Close()

print(f"Plotted charge-inclusive c* + anti-c* DSCB fit overlays for {len(masses)} masses")
print(f"EOS output: {output_dir}")
PY
