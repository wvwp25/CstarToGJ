#include <algorithm>
#include <cmath>
#include <iostream>

#include "TCanvas.h"
#include "TFile.h"
#include "TF1.h"
#include "TFitResult.h"
#include "TFitResultPtr.h"
#include "TH1.h"
#include "TLine.h"
#include "TMath.h"
#include "TParameter.h"
#include "TPaveText.h"
#include "TLatex.h"
#include "TROOT.h"
#include "TStyle.h"
#include "Math/MinimizerOptions.h"
#include "RooRealVar.h"
#include "RooWorkspace.h"

namespace {

// The fit callback uses these boundaries to mask the signal region during one
// minimization over the complete [fitMin, fitMax] interval.
double excludedLow = 0.0;
double excludedHigh = 0.0;
bool rejectSignalWindow = true;

Double_t maskedBackgroundFunction(Double_t *xx, Double_t *par) {
  const double mass = xx[0];

  if (rejectSignalWindow && mass > excludedLow && mass < excludedHigh) {
    TF1::RejectPoint();
    return 0.0;
  }

  const double z = mass / 13000.0;
  if (z <= 0.0 || z >= 1.0) return 0.0;

  return TMath::Exp(par[0]) * TMath::Power(1.0 - z, par[1]) /
         TMath::Power(z, par[2] + par[3] * TMath::Log(z));
}

// An unmasked copy is used for plotting and for interpolation through the
// excluded window after the simultaneous sideband fit is complete.
Double_t backgroundFunction(Double_t *xx, Double_t *par) {
  const double mass = xx[0];
  const double z = mass / 13000.0;
  if (z <= 0.0 || z >= 1.0) return 0.0;

  return par[0] * TMath::Power(1.0 - z, par[1]) /
         TMath::Power(z, par[2] + par[3] * TMath::Log(z));
}

bool inSidebands(double mass, double fitMin, double fitMax) {
  return mass >= fitMin && mass <= fitMax &&
         !(mass > excludedLow && mass < excludedHigh);
}

void drawCmsLabel(double luminosityFb) {
  TLatex label;
  label.SetNDC();
  label.SetTextFont(62);
  label.SetTextSize(0.045);
  label.DrawLatex(0.13, 0.90, "CMS");
  label.SetTextFont(52);
  label.SetTextSize(0.035);
  label.DrawLatex(0.23, 0.90, "Preliminary");
  label.SetTextFont(42);
  label.SetTextAlign(31);
  label.SetTextSize(0.035);
  label.DrawLatex(0.94, 0.90,
                  Form("%.1f fb^{-1} (13 TeV)", luminosityFb));
}

}  // namespace

void Bkg_model(int signalMass = 1000,
                            double fitMin = 700.0,
                            double fitMax = 3500.0) {
  const double luminosityFb = 41.8;  // Keep current input convention.
  // Use one consistent 4 TeV display range for every mass hypothesis.  This
  // changes only the displayed/extrapolated range; the fit remains limited by
  // fitMax.
  const double plotMax = std::max(fitMax, 4000.0);

  const TString signalFileName = Form(
      "/eos/user/h/hsiaoche/Signal/"
      "CstarToGJ_M%d_f1p0_13TeV_NANOAOD/signal_DSCB_workspace.root",
      signalMass);
  TFile signalFile(signalFileName, "READ");
  RooWorkspace *signalWorkspace =
      dynamic_cast<RooWorkspace *>(signalFile.Get("ws"));
  if (signalFile.IsZombie() || !signalWorkspace ||
      !signalWorkspace->var("x0") || !signalWorkspace->var("sigmaL") ||
      !signalWorkspace->var("sigmaR")) {
    Error("Bkg_model",
          "Cannot read x0, sigmaL, and sigmaR from %s",
          signalFileName.Data());
    return;
  }

  const double windowCenter = signalWorkspace->var("x0")->getVal();
  const double sigmaLeft = signalWorkspace->var("sigmaL")->getVal();
  const double sigmaRight = signalWorkspace->var("sigmaR")->getVal();
  signalFile.Close();

  excludedLow = windowCenter - 3.0 * sigmaLeft;
  excludedHigh = windowCenter + 3.0 * sigmaRight;

  if (fitMin >= excludedLow || fitMax <= excludedHigh) {
    Error("Bkg_model",
          "Fit range [%.1f, %.1f] must contain both sidebands around [%.1f, %.1f]",
          fitMin, fitMax, excludedLow, excludedHigh);
    return;
  }

  // The observed background spectrum is common to every signal hypothesis.
  // signalMass is used only above to select the signal workspace and derive
  // the hypothesis-dependent excluded window; keep one shared data input.
  const TString inputName = "Run2017BCDEF_BG.root";
  TFile input(inputName, "READ");
  if (input.IsZombie()) {
    Error("Bkg_model", "Cannot open %s", inputName.Data());
    return;
  }

  TH1 *inputHistogram = dynamic_cast<TH1 *>(input.Get("hM"));
  if (!inputHistogram) {
    Error("Bkg_model", "%s does not contain hM", inputName.Data());
    return;
  }

  TH1 *data = dynamic_cast<TH1 *>(inputHistogram->Clone("hM"));
  data->SetDirectory(nullptr);

  double initial[4] = {1.1, 17.5, 3.5, 0.16};
  if (TF1 *storedFit = dynamic_cast<TF1 *>(input.Get("bkgFit"))) {
    for (int parameter = 0; parameter < 4; ++parameter) {
      initial[parameter] = storedFit->GetParameter(parameter);
    }
  }
  input.Close();

  TF1 maskedFit("maskedFit", maskedBackgroundFunction, fitMin, fitMax, 4);
  maskedFit.SetParNames("lnP0", "P1", "P2", "P3");
  maskedFit.SetParameters(std::log(std::max(initial[0], 1.e-6)),
                          initial[1], initial[2], initial[3]);
  maskedFit.SetParLimits(0, std::log(1.e-6), std::log(1.e8));
  maskedFit.SetParLimits(1, 1.0, 30.0);
  maskedFit.SetParLimits(2, 0.1, 10.0);
  maskedFit.SetParLimits(3, -5.0, 5.0);

  ROOT::Math::MinimizerOptions::SetDefaultMaxFunctionCalls(100000);
  ROOT::Math::MinimizerOptions::SetDefaultStrategy(2);

  // This is the only Fit call. TF1::RejectPoint masks the signal-window bins,
  // so both sidebands contribute to the same objective function and covariance.
  // "I" compares each bin with the function integral over that bin. "R",
  // "S", and "0" select the range, save the result, and suppress automatic
  // drawing, respectively.
  TFitResultPtr result = data->Fit(&maskedFit, "IRS0", "", fitMin, fitMax);
  if (!result.Get()) {
    Error("Bkg_model", "The fit did not return a result");
    delete data;
    return;
  }

  // Copy the common best-fit parameters to an ordinary, unmasked function.
  TF1 backgroundFit("bkgFit", backgroundFunction, fitMin, plotMax, 4);
  backgroundFit.SetParNames("P0", "P1", "P2", "P3");
  backgroundFit.SetParameter(0, std::exp(maskedFit.GetParameter(0)));
  backgroundFit.SetParError(
      0, backgroundFit.GetParameter(0) * maskedFit.GetParError(0));
  for (int parameter = 1; parameter < 4; ++parameter) {
    backgroundFit.SetParameter(parameter, maskedFit.GetParameter(parameter));
    backgroundFit.SetParError(parameter, maskedFit.GetParError(parameter));
  }

  // Compute diagnostics using the same bin-content convention as the fit.
  double pullChi2 = 0.0;
  int binsUsed = 0;
  TH1 *pull = dynamic_cast<TH1 *>(data->Clone("hPull"));
  pull->Reset("ICES");
  pull->SetDirectory(nullptr);

  for (int bin = 1; bin <= data->GetNbinsX(); ++bin) {
    const double mass = data->GetBinCenter(bin);
    if (!inSidebands(mass, fitMin, fitMax)) continue;

    const double uncertainty = data->GetBinError(bin);
    if (uncertainty <= 0.0) continue;

    const double lowEdge = data->GetXaxis()->GetBinLowEdge(bin);
    const double highEdge = data->GetXaxis()->GetBinUpEdge(bin);
    const double prediction =
        backgroundFit.Integral(lowEdge, highEdge) / (highEdge - lowEdge);
    const double value = (data->GetBinContent(bin) - prediction) / uncertainty;
    pull->SetBinContent(bin, value);
    pull->SetBinError(bin, 0.0);
    pullChi2 += value * value;
    ++binsUsed;
  }
  const double chi2 = result->Chi2();
  const int ndf = std::max(1, static_cast<int>(result->Ndf()));

  // Use a separate histogram for display so the histogram represents exactly
  // the data that constrained the fit.  The fitted curve remains continuous
  // through the intentionally blank signal window.
  TH1 *sidebandData = dynamic_cast<TH1 *>(data->Clone("hM_sidebands"));
  sidebandData->SetDirectory(nullptr);
  for (int bin = 1; bin <= sidebandData->GetNbinsX(); ++bin) {
    const double mass = sidebandData->GetBinCenter(bin);
    if (mass > excludedLow && mass < excludedHigh) {
      sidebandData->SetBinContent(bin, 0.0);
      sidebandData->SetBinError(bin, 0.0);
    }
  }

  std::cout << "=== Simultaneous two-sideband background fit ===\n"
            << "Fit status: " << result->Status() << "\n"
            << "Covariance quality: " << result->CovMatrixStatus() << "\n"
            << "Mass hypothesis: " << signalMass << " GeV\n"
            << "f1p0 DSCB peak and widths: x0=" << windowCenter
            << ", sigmaL=" << sigmaLeft << ", sigmaR=" << sigmaRight << " GeV\n"
            << "Excluded window: [" << excludedLow << ", " << excludedHigh
            << "] GeV\n"
            << "Bins used: " << binsUsed << "\n"
            << "chi2/ndf: " << chi2 << "/" << ndf << " = " << chi2 / ndf
            << "\nPull diagnostic sum: " << pullChi2 << "\n";
  for (int parameter = 0; parameter < 4; ++parameter) {
    std::cout << backgroundFit.GetParName(parameter) << " = "
              << backgroundFit.GetParameter(parameter) << " +/- "
              << backgroundFit.GetParError(parameter) << "\n";
  }

  gStyle->SetOptStat(0);
  gROOT->SetBatch(kTRUE);
  TCanvas canvas("cSimultaneousNoPullSidebandsOnly", "Simultaneous sideband fit", 700, 650);
  canvas.SetLogy();
  canvas.SetLeftMargin(0.12);
  canvas.SetRightMargin(0.05);
  canvas.SetTopMargin(0.11);
  canvas.SetBottomMargin(0.12);
  // Draw an explicit frame because the input histogram currently ends at
  // 3 TeV; SetRangeUser cannot extend a histogram beyond its native axis.
  // The frame lets every fit curve be shown through 4 TeV.
  const double yMinimum = 0.3;
  const double yMaximum = std::max(10.0, 1.8 * sidebandData->GetMaximum());
  TH1 *frame = canvas.DrawFrame(fitMin, yMinimum, plotMax, yMaximum);
  frame->SetTitle("");
  frame->GetXaxis()->SetTitle("m_{#gamma+jet} [GeV]");
  frame->GetYaxis()->SetTitle("Events");
  sidebandData->Draw("E SAME");
  backgroundFit.SetLineColor(kRed + 1);
  backgroundFit.SetLineWidth(2);
  backgroundFit.Draw("SAME");

  canvas.Update();
  TLine lowLine(excludedLow, canvas.GetUymin(), excludedLow, canvas.GetUymax());
  TLine highLine(excludedHigh, canvas.GetUymin(), excludedHigh, canvas.GetUymax());
  lowLine.SetLineStyle(2);
  highLine.SetLineStyle(2);
  lowLine.Draw();
  highLine.Draw();
  canvas.cd();
  drawCmsLabel(luminosityFb);

  TPaveText fitLabel(0.57, 0.67, 0.88, 0.82, "NDC");
  fitLabel.SetBorderSize(0);
  fitLabel.SetFillStyle(0);
  fitLabel.SetTextSize(0.022);
  //fitLabel.AddText("Simultaneous sideband fit");
  fitLabel.AddText(Form("#chi^{2}/ndf = %.2f", chi2 / ndf));
  fitLabel.Draw();
  canvas.Modified();
  canvas.Update();

  const TString plotName =
      Form("Invariant_Mass_gJet_M%d.png", signalMass);
  canvas.SaveAs(plotName);

  const TString outputName =
      Form("Run2017BCDEF_BG_M%d.root", signalMass);
  TFile output(outputName, "RECREATE");
  data->Write("hM");
  sidebandData->Write("hM_sidebands");
  backgroundFit.Write("bkgFit");
  pull->Write("hPull");
  result->Write("fitResult");
  TParameter<double>("fit_min", fitMin).Write();
  TParameter<double>("fit_max", fitMax).Write();
  TParameter<int>("signal_mass", signalMass).Write();
  TParameter<double>("signal_x0_f1p0", windowCenter).Write();
  TParameter<double>("sigmaL", sigmaLeft).Write();
  TParameter<double>("sigmaR", sigmaRight).Write();
  TParameter<double>("excluded_low", excludedLow).Write();
  TParameter<double>("excluded_high", excludedHigh).Write();
  TParameter<double>("chi2_sb", chi2).Write();
  TParameter<int>("ndof_sb", ndf).Write();
  TParameter<int>("fit_status", result->Status()).Write();
  TParameter<int>("covariance_quality", result->CovMatrixStatus()).Write();
  output.Close();

  delete pull;
  delete sidebandData;
  delete data;
}
