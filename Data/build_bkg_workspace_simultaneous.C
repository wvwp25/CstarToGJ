#include <iostream>
#include <vector>

#include "TFile.h"
#include "TF1.h"
#include "TH1.h"
#include "TParameter.h"
#include "TString.h"
#include "TSystem.h"

#include "RooArgList.h"
#include "RooArgSet.h"
#include "RooDataHist.h"
#include "RooGenericPdf.h"
#include "RooRealVar.h"
#include "RooWorkspace.h"

namespace {

struct MassRange {
  int mass;
  double xmin;
  double xmax;
};

const std::vector<MassRange> kMassRanges = {
    {1000, 500.0, 1400.0},  {1200, 700.0, 1600.0},
    {1400, 850.0, 1800.0},  {1600, 1000.0, 2050.0},
    {1800, 1150.0, 2300.0}, {2000, 1300.0, 2500.0},
    {2200, 1450.0, 2700.0}, {2400, 1600.0, 2900.0},
    {2600, 1750.0, 3000.0}, {2800, 1900.0, 3300.0},
    {3000, 2050.0, 3500.0},
};

bool buildOne(const MassRange &range) {
  const TString inputName =
      Form("Run2017BCDEF_BG_M%d.root", range.mass);
  if (gSystem->AccessPathName(inputName)) {
    std::cerr << "Missing input: " << inputName << std::endl;
    return false;
  }

  TFile input(inputName, "READ");
  TF1 *fit = dynamic_cast<TF1 *>(input.Get("bkgFit"));
  TH1 *histogram = dynamic_cast<TH1 *>(input.Get("hM"));
  TParameter<int> *storedMass =
      dynamic_cast<TParameter<int> *>(input.Get("signal_mass"));
  if (input.IsZombie() || !fit || !histogram) {
    std::cerr << inputName << " must contain bkgFit and hM" << std::endl;
    return false;
  }
  if (storedMass && storedMass->GetVal() != range.mass) {
    std::cerr << inputName << " stores signal_mass=" << storedMass->GetVal()
              << ", expected " << range.mass << std::endl;
    return false;
  }

  RooRealVar x("x", "m_{#gamma j}", range.xmin, range.xmax, "GeV");
  RooRealVar p1("P1", "P1", fit->GetParameter(1));
  RooRealVar p2("P2", "P2", fit->GetParameter(2));
  RooRealVar p3("P3", "P3", fit->GetParameter(3));
  p1.setConstant(false);
  p2.setConstant(false);
  p3.setConstant(false);

  // This is a shape-only PDF.  Its event yield is supplied by the datacard
  // rate and bkg_norm rateParam, not by a RooExtendPdf in the workspace.
  RooGenericPdf background(
      "bkg", "pow(1-x/13000.,P1)/pow(x/13000.,P2+P3*log(x/13000.))",
      RooArgList(x, p1, p2, p3));
  RooDataHist data("data_obs", "data_obs", RooArgList(x), histogram);

  RooWorkspace workspace("ws", "background workspace");
  workspace.import(background);
  workspace.import(data);

  // All signal couplings at a given mass use this same background workspace.
  const TString outputName =
      Form("bkg_workspace_M%d_f1p0.root", range.mass);
  TFile output(outputName, "RECREATE");
  if (output.IsZombie()) {
    std::cerr << "Cannot create " << outputName << std::endl;
    return false;
  }
  workspace.Write("ws");
  output.Close();

  const int firstBin = histogram->GetXaxis()->FindBin(range.xmin);
  const int lastBin = histogram->GetXaxis()->FindBin(range.xmax);
  std::cout << "Wrote " << outputName << " from " << inputName << ": range ["
            << range.xmin << ", " << range.xmax << "], data yield "
            << histogram->Integral(firstBin, lastBin) << std::endl;
  return true;
}

}  // namespace

void build_bkg_workspace_simultaneous(int signalMass = 0) {
  int built = 0;
  int failed = 0;
  bool matched = false;

  for (const MassRange &range : kMassRanges) {
    if (signalMass != 0 && signalMass != range.mass) continue;
    matched = true;
    if (buildOne(range))
      ++built;
    else
      ++failed;
  }

  if (!matched) {
    std::cerr << "Unsupported mass " << signalMass << std::endl;
    return;
  }
  std::cout << "Built " << built << " workspace(s); " << failed
            << " failed." << std::endl;
}
