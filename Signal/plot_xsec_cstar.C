// Plot the c* production cross sections stored in cross_sections.csv.
// Usage from any directory:
//   root -l -b -q '/path/to/plot_xsec_cstar_bstar.C(1.0,true)'
//
// BR is applied as a common factor (default BR=1).  If y_in_fb=true,
// cross sections are converted from pb to fb.

// The historical file/function name is retained so existing commands continue
// to work, but no b* samples are read or plotted.

#include "TAxis.h"
#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct CrossSectionPoint {
  double massTeV;
  double valuePb;
  double uncertaintyPb;
};

std::vector<CrossSectionPoint> readCrossSections(const TString &csvFile,
                                                 const std::string &coupling)
{
  std::ifstream input(csvFile.Data());
  if (!input) {
    std::cerr << "ERROR: cannot open " << csvFile << std::endl;
    return {};
  }

  std::vector<CrossSectionPoint> points;
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;

    std::stringstream row(line);
    std::string model, mass, foundCoupling, value, uncertainty;
    if (!std::getline(row, model, ',') || !std::getline(row, mass, ',') ||
        !std::getline(row, foundCoupling, ',') ||
        !std::getline(row, value, ',') ||
        !std::getline(row, uncertainty, ',')) continue;
    if (model != "CstarToGJ" || foundCoupling != coupling) continue;

    points.push_back({std::stod(mass) / 1000.0, std::stod(value),
                      std::stod(uncertainty)});
  }
  std::sort(points.begin(), points.end(),
            [](const CrossSectionPoint &a, const CrossSectionPoint &b) {
              return a.massTeV < b.massTeV;
            });
  return points;
}

} // namespace

void plot_xsec_cstar_bstar(
    double BR = 1.0, bool y_in_fb = true, TString csvFile = "",
    TString outputDir = "/eos/user/h/hsiaoche/Signal")
{
  if (csvFile.IsNull())
    csvFile = TString(gSystem->DirName(__FILE__)) + "/cross_sections.csv";

  gStyle->SetOptStat(0);
  gStyle->SetTitle(0);
  gStyle->SetLineWidth(2);
  gStyle->SetFrameLineWidth(2);
  gStyle->SetPadLeftMargin(0.12);
  gStyle->SetPadBottomMargin(0.12);
  gStyle->SetPadRightMargin(0.05);
  gStyle->SetPadTopMargin(0.08);

  const std::vector<std::string> couplings = {"f0p1", "f0p5", "f1p0"};
  const int colors[] = {kGreen + 2, kOrange + 7, kAzure + 2};
  const int markers[] = {20, 22, 21};

  TCanvas *canvas = new TCanvas("c_xsec_cstar", "c* cross sections", 900, 700);
  canvas->SetLogy();

  TLegend *legend = new TLegend(0.60, 0.68, 0.88, 0.86);
  legend->SetBorderSize(0);
  legend->SetFillStyle(0);
  legend->SetTextSize(0.035);

  std::vector<TGraphErrors *> graphs;
  for (unsigned int curve = 0; curve < couplings.size(); ++curve) {
    const auto points = readCrossSections(csvFile, couplings[curve]);
    if (points.empty()) {
      std::cerr << "ERROR: no CstarToGJ points for " << couplings[curve]
                << " in " << csvFile << std::endl;
      continue;
    }

    const double unitScale = y_in_fb ? 1000.0 : 1.0;
    TGraphErrors *graph = new TGraphErrors(points.size());
    graph->SetName(Form("xsec_cstar_%s", couplings[curve].c_str()));
    for (unsigned int i = 0; i < points.size(); ++i) {
      graph->SetPoint(i, points[i].massTeV,
                      points[i].valuePb * BR * unitScale);
      graph->SetPointError(i, 0.0,
                           points[i].uncertaintyPb * BR * unitScale);
    }
    graph->SetMarkerStyle(markers[curve]);
    graph->SetMarkerSize(1.1);
    graph->SetLineWidth(2);
    graph->SetMarkerColor(colors[curve]);
    graph->SetLineColor(colors[curve]);
    graphs.push_back(graph);

    std::string couplingLabel = couplings[curve].substr(1);
    std::replace(couplingLabel.begin(), couplingLabel.end(), 'p', '.');
    legend->AddEntry(graph, Form("c*, f = %s", couplingLabel.c_str()), "lp");
  }

  if (graphs.empty()) {
    std::cerr << "ERROR: no c* cross sections were plotted" << std::endl;
    return;
  }

  TGraphErrors *frame = graphs.front();
  frame->SetTitle("");
  frame->GetXaxis()->SetTitle("m_{c*} [TeV]");
  frame->GetYaxis()->SetTitle(y_in_fb ? "#sigma B [fb]" : "#sigma B [pb]");
  frame->GetXaxis()->SetTitleSize(0.05);
  frame->GetYaxis()->SetTitleSize(0.05);
  frame->GetXaxis()->SetLabelSize(0.045);
  frame->GetYaxis()->SetLabelSize(0.045);
  frame->GetYaxis()->SetTitleOffset(1.0);
  frame->GetXaxis()->SetLimits(0.9, 3.1);
  frame->SetMinimum(y_in_fb ? 3e-3 : 3e-6);
  frame->SetMaximum(y_in_fb ? 3e3 : 3.0);
  frame->Draw("ALP");
  for (unsigned int i = 1; i < graphs.size(); ++i)
    graphs[i]->Draw("LP SAME");
  legend->Draw();

  TLatex label;
  label.SetNDC();
  label.SetTextFont(42);
  label.SetTextSize(0.04);
  label.SetTextAlign(31);
  label.DrawLatex(0.95, 0.93, "13 TeV");
  label.SetTextAlign(13);
  label.SetTextFont(52);
  label.SetTextSize(0.035);
  label.DrawLatex(0.14, 0.91, "Private work (CMS simulation)");

  if (gSystem->mkdir(outputDir, true) != 0 &&
      gSystem->AccessPathName(outputDir)) {
    std::cerr << "ERROR: cannot create output directory " << outputDir
              << std::endl;
    return;
  }
  const TString outputStem = outputDir + "/sigmaB_cstar";
  canvas->SaveAs(outputStem + ".png");
  canvas->SaveAs(outputStem + ".pdf");
  std::cout << "Wrote " << outputStem << ".png/.pdf" << std::endl;
}
