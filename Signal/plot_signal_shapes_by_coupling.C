#include "RooAbsPdf.h"
#include "RooArgSet.h"
#include "RooRealVar.h"
#include "RooWorkspace.h"

#include "TCanvas.h"
#include "TColor.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1F.h"
#include "TLegend.h"
#include "TROOT.h"
#include "TString.h"
#include "TSystem.h"
#include "TLatex.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// The plotted workspaces are built from the charge-inclusive signal template,
// which contains c* -> c gamma and anti-c* -> anti-c gamma events.
namespace {

struct SignalPoint {
    int mass;
    TString directory;
};

std::vector<SignalPoint> findSignalPoints(const TString &baseDir,
                                          const TString &coupling)
{
    std::vector<SignalPoint> points;
    void *dir = gSystem->OpenDirectory(baseDir);
    if (!dir) {
        std::cerr << "ERROR: cannot open " << baseDir << std::endl;
        return points;
    }

    const char *entry = nullptr;
    while ((entry = gSystem->GetDirEntry(dir))) {
        int mass = 0;
        char foundCoupling[64] = {0};
        char extra = '\0';
        const int matched = std::sscanf(entry,
            "CstarToGJ_M%d_f%63[^_]_13TeV_NANOAOD%c",
            &mass, foundCoupling, &extra);
        if (matched != 2 || coupling != foundCoupling)
            continue;

        const TString sampleDir = baseDir + "/" + entry;
        const TString workspace = sampleDir + "/signal_DSCB_workspace_paramSyst.root";
        if (!gSystem->AccessPathName(workspace))
            points.push_back({mass, sampleDir});
    }
    gSystem->FreeDirectory(dir);

    std::sort(points.begin(), points.end(),
              [](const SignalPoint &a, const SignalPoint &b) {
                  return a.mass < b.mass;
              });
    return points;
}

int curveColor(unsigned int index, unsigned int count)
{
    // A perceptually ordered blue-to-red palette, readable for many mass points.
    double stops[] = {0.00, 0.25, 0.50, 0.75, 1.00};
    double red[]   = {0.10, 0.10, 0.15, 0.85, 0.65};
    double green[] = {0.25, 0.65, 0.75, 0.45, 0.05};
    double blue[]  = {0.75, 0.75, 0.20, 0.10, 0.10};
    static int first = TColor::CreateGradientColorTable(5, stops, red, green,
                                                         blue, 100);
    const double fraction = count > 1 ? double(index) / double(count - 1) : 0.5;
    return first + int(99.0 * fraction + 0.5);
}

void plotOneCoupling(const TString &baseDir, const TString &outputDir,
                     const TString &coupling, double xMin, double xMax,
                     int nSamples)
{
    const std::vector<SignalPoint> points = findSignalPoints(baseDir, coupling);
    if (points.empty()) {
        std::cerr << "WARNING: no usable workspaces for f=" << coupling << std::endl;
        return;
    }

    TCanvas canvas("canvas_" + coupling,
                   "Charge-inclusive c* and anti-c* signal shapes", 1000, 750);
    canvas.SetLeftMargin(0.13);
    canvas.SetRightMargin(0.04);
    canvas.SetBottomMargin(0.12);
    canvas.SetTopMargin(0.08);
    canvas.SetTicks(1, 1);

    TH1F frame("frame_" + coupling, "", 100, xMin, xMax);
    frame.SetDirectory(nullptr);
    frame.SetStats(false);
    frame.GetXaxis()->SetTitle("m_{#gamma+j} [GeV]");
    frame.GetYaxis()->SetTitle("Probability density [GeV^{-1}]");
    frame.GetXaxis()->SetTitleSize(0.035);
    frame.GetYaxis()->SetTitleSize(0.035);
    frame.GetXaxis()->SetLabelSize(0.040);
    frame.GetYaxis()->SetLabelSize(0.040);
    frame.GetXaxis()->SetTitleOffset(1.35);
    frame.GetYaxis()->SetTitleOffset(1.75);
    frame.SetMinimum(0.0);

    std::vector<std::unique_ptr<TGraph>> graphs;
    double yMax = 0.0;

    for (unsigned int i = 0; i < points.size(); ++i) {
        const TString fileName = points[i].directory
                               + "/signal_DSCB_workspace_paramSyst.root";
        std::unique_ptr<TFile> file(TFile::Open(fileName, "READ"));
        RooWorkspace *ws = file && !file->IsZombie()
                         ? dynamic_cast<RooWorkspace *>(file->Get("ws")) : nullptr;
        RooRealVar *x = ws ? ws->var("x") : nullptr;
        RooAbsPdf *pdf = ws ? ws->pdf("dscb") : nullptr;
        if (!x || !pdf) {
            std::cerr << "WARNING: missing ws/x/dscb in " << fileName << std::endl;
            continue;
        }

        // getVal normalizes the PDF over the fitted domain saved in this workspace.
        const double fitMin = x->getMin();
        const double fitMax = x->getMax();
        RooArgSet normSet(*x);
        std::unique_ptr<TGraph> graph(new TGraph(nSamples));
        graph->SetName(Form("shape_M%d_f%s", points[i].mass, coupling.Data()));
        for (int bin = 0; bin < nSamples; ++bin) {
            const double value = xMin + (xMax - xMin) * bin / (nSamples - 1.0);
            double density = 0.0;
            if (value >= fitMin && value <= fitMax) {
                x->setVal(value);
                density = pdf->getVal(&normSet);
            }
            graph->SetPoint(bin, value, density);
            yMax = std::max(yMax, density);
        }
        graph->SetLineColor(curveColor(i, points.size()));
        graph->SetLineWidth(3);
        graphs.push_back(std::move(graph));
    }

    if (graphs.empty())
        return;

    frame.SetMaximum(1.18 * yMax);
    frame.Draw("AXIS");
    for (const auto &graph : graphs)
        graph->Draw("L SAME");

    TLegend legend(0.54, 0.59, 0.87, 0.89);
    legend.SetNColumns(2);
    legend.SetBorderSize(0);
    legend.SetFillStyle(0);
    legend.SetTextSize(0.032);
    legend.SetTextFont(42);
    legend.SetColumnSeparation(0.18);
    unsigned int graphIndex = 0;
    for (const SignalPoint &point : points) {
        if (graphIndex >= graphs.size()) break;
        legend.AddEntry(graphs[graphIndex++].get(),
                        Form("m_{c*} = %.1f TeV",
                             point.mass / 1000.0), "l");
    }
    legend.Draw();

    TString couplingLabel = coupling;
    couplingLabel.ReplaceAll("p", ".");
    TLatex label;
    label.SetNDC();
    label.SetTextAlign(31);
    label.SetTextSize(0.035);
    label.SetTextFont(42);
    label.DrawLatex(0.94, 0.93, "41.8 fb^{-1} (13 TeV)");
    label.DrawLatex(0.25, 0.84, "f = " + couplingLabel);
    label.SetTextAlign(13);

    gSystem->mkdir(outputDir, true);
    const TString stem = outputDir + "/signal_shapes_f" + coupling;
        {
            gPad->Update();
            if (auto *statsBox = gPad->GetPrimitive("stats")) statsBox->Delete();
            TLatex privateWorkLabel;
            privateWorkLabel.SetNDC();
            privateWorkLabel.SetTextFont(52);
            privateWorkLabel.SetTextSize(0.035);
            privateWorkLabel.DrawLatex(0.14, 0.93, "Private work (CMS simulation)");
        }
    canvas.SaveAs(stem + ".png");
        {
            gPad->Update();
            if (auto *statsBox = gPad->GetPrimitive("stats")) statsBox->Delete();
            TLatex privateWorkLabel;
            privateWorkLabel.SetNDC();
            privateWorkLabel.SetTextFont(52);
            privateWorkLabel.SetTextSize(0.035);
            privateWorkLabel.DrawLatex(0.14, 0.93, "Private work (CMS simulation)");
        }
    canvas.SaveAs(stem + ".pdf");
    std::cout << "Wrote " << stem << ".png/.pdf (" << graphs.size()
              << " signal models)" << std::endl;
}

} // namespace

// Usage from any directory:
//   root -l -b -q '/path/to/plot_signal_shapes_by_coupling.C()'
// Optional arguments can override input/output directories and plot range.
void plot_signal_shapes_by_coupling(
    TString baseDir = "/eos/user/h/hsiaoche/Signal",
    TString outputDir = "/eos/user/h/hsiaoche/Signal/combined_signal_shapes",
    double xMin = 500.0, double xMax = 3500.0, int nSamples = 3001)
{
    gROOT->SetBatch(kTRUE);
    if (nSamples < 2) nSamples = 2;
    const std::vector<TString> couplings = {"0p1", "0p5", "1p0"};
    for (const TString &coupling : couplings)
        plotOneCoupling(baseDir, outputDir, coupling, xMin, xMax, nSamples);
}
