#include <algorithm>
#include <cmath>
#include <iostream>

#include "TCanvas.h"
#include "TFile.h"
#include "TF1.h"
#include "TFitResult.h"
#include "TFitResultPtr.h"
#include "TGraphErrors.h"
#include "TH1.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMath.h"
#include "TParameter.h"
#include "TPad.h"
#include "TPaveText.h"
#include "TLatex.h"
#include "TROOT.h"
#include "TStyle.h"
#include "Math/MinimizerOptions.h"
#include "RooAbsReal.h"
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

void drawPrivateLabel(double luminosityFb) {
  TLatex label;
  label.SetNDC();
  label.SetTextFont(52);
  label.SetTextSize(0.035);
  label.DrawLatex(0.14, 0.91, "Private work (CMS simulation)");
  label.SetTextFont(42);
  label.SetTextAlign(31);
  label.SetTextSize(0.035);
  label.DrawLatex(0.94, 0.91,
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
      "CstarToGJ_M%d_f1p0_13TeV_NANOAOD/"
      "signal_DSCB_workspace_paramSyst.root",
      signalMass);
  TFile signalFile(signalFileName, "READ");
  RooWorkspace *signalWorkspace =
      dynamic_cast<RooWorkspace *>(signalFile.Get("ws"));
  if (signalFile.IsZombie() || !signalWorkspace) {
    Error("Bkg_model",
          "Cannot read workspace ws from %s",
          signalFileName.Data());
    return;
  }

  // Define the nominal signal window with all shape-systematic nuisance
  // parameters fixed at their central values.
  for (const char *thetaName : {"theta_JES", "theta_JER", "theta_PES"}) {
    RooRealVar *theta = signalWorkspace->var(thetaName);
    if (!theta) {
      Error("Bkg_model", "Cannot read %s from %s", thetaName,
            signalFileName.Data());
      return;
    }
    theta->setVal(0.0);
  }

  RooAbsReal *x0Syst = signalWorkspace->function("x0_syst");
  RooAbsReal *sigmaLSyst = signalWorkspace->function("sigmaL_syst");
  RooAbsReal *sigmaRSyst = signalWorkspace->function("sigmaR_syst");
  if (!x0Syst || !sigmaLSyst || !sigmaRSyst) {
    Error("Bkg_model",
          "Cannot read x0_syst, sigmaL_syst, and sigmaR_syst from %s",
          signalFileName.Data());
    return;
  }

  const double windowCenter = x0Syst->getVal();
  const double sigmaLeft = sigmaLSyst->getVal();
  const double sigmaRight = sigmaRSyst->getVal();
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

  // Figure 59 uses roughly 50 visible points across the full mass range.  The
  // source histogram has 4 GeV bins, so combine 20 bins (80 GeV) for display
  // only.  Dividing by the group size keeps the original events-per-4-GeV
  // convention and therefore the normalization of backgroundFit unchanged.
  constexpr int displayRebin = 20;
  TH1 *displayData = dynamic_cast<TH1 *>(data->Clone("hM_display"));
  displayData->SetDirectory(nullptr);
  displayData->Rebin(displayRebin);
  displayData->Scale(1.0 / displayRebin);
  for (int bin = 1; bin <= displayData->GetNbinsX(); ++bin) {
    const double lowEdge = displayData->GetXaxis()->GetBinLowEdge(bin);
    const double highEdge = displayData->GetXaxis()->GetBinUpEdge(bin);
    if (highEdge > excludedLow && lowEdge < excludedHigh) {
      displayData->SetBinContent(bin, 0.0);
      displayData->SetBinError(bin, 0.0);
    }
  }

  // Fractional residual used in the lower panel: (Data - Fit) / Fit.  Use a
  // graph so bins outside the fit range and inside the excluded signal window
  // are genuinely absent rather than drawn as artificial zeroes.
  TGraphErrors residual;
  residual.SetName("fractionalResidual");
  int residualPoint = 0;
  for (int bin = 1; bin <= displayData->GetNbinsX(); ++bin) {
    const double mass = displayData->GetBinCenter(bin);
    const double lowEdge = displayData->GetXaxis()->GetBinLowEdge(bin);
    const double highEdge = displayData->GetXaxis()->GetBinUpEdge(bin);
    if (mass < fitMin || mass > fitMax ||
        (highEdge > excludedLow && lowEdge < excludedHigh)) continue;

    const double prediction =
        backgroundFit.Integral(lowEdge, highEdge) / (highEdge - lowEdge);
    if (prediction <= 0.0) continue;

    const double value = displayData->GetBinContent(bin);
    const double uncertainty = displayData->GetBinError(bin);
    residual.SetPoint(residualPoint, mass, (value - prediction) / prediction);
    residual.SetPointError(residualPoint, 0.0, uncertainty / prediction);
    ++residualPoint;
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
  TCanvas canvas("cSimultaneousSidebands", "Simultaneous sideband fit", 700, 700);
  TPad topPad("topPad", "data and fit", 0.0, 0.30, 1.0, 1.0);
  TPad residualPad("residualPad", "fractional residual", 0.0, 0.0, 1.0, 0.30);
  topPad.SetLeftMargin(0.12);
  topPad.SetRightMargin(0.05);
  topPad.SetTopMargin(0.11);
  topPad.SetBottomMargin(0.025);
  topPad.SetLogy();
  residualPad.SetLeftMargin(0.12);
  residualPad.SetRightMargin(0.05);
  residualPad.SetTopMargin(0.03);
  residualPad.SetBottomMargin(0.32);
  topPad.Draw();
  residualPad.Draw();

  topPad.cd();
  // Draw an explicit frame because the input histogram currently ends at
  // 3 TeV; SetRangeUser cannot extend a histogram beyond its native axis.
  // The frame lets every fit curve be shown through 4 TeV.
  const double yMinimum = 0.3;
  const double yMaximum = std::max(10.0, 1.8 * displayData->GetMaximum());
  TH1 *frame = topPad.DrawFrame(fitMin, yMinimum, plotMax, yMaximum);
  frame->SetTitle("");
  frame->GetYaxis()->SetTitle("Events");
  frame->GetXaxis()->SetLabelSize(0.0);
  displayData->SetMarkerStyle(20);
  displayData->SetMarkerSize(0.75);
  displayData->SetMarkerColor(kBlack);
  displayData->SetLineColor(kBlack);
  displayData->Draw("E1 SAME");
  backgroundFit.SetLineColor(kRed + 1);
  backgroundFit.SetLineWidth(2);
  backgroundFit.Draw("SAME");

  topPad.Update();
  TLine lowLine(excludedLow, yMinimum, excludedLow, yMaximum);
  TLine highLine(excludedHigh, yMinimum, excludedHigh, yMaximum);
  lowLine.SetLineStyle(2);
  highLine.SetLineStyle(2);
  lowLine.Draw();
  highLine.Draw();
  drawPrivateLabel(luminosityFb);

  TLegend legend(0.63, 0.68, 0.89, 0.84);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.SetTextSize(0.035);
  legend.AddEntry(displayData, "Data (sidebands)", "lep");
  legend.AddEntry(&backgroundFit, "4-parameter fit", "l");
  legend.AddEntry(&lowLine, "Excluded signal window", "l");
  legend.Draw();

  TPaveText fitLabel(0.63, 0.58, 0.89, 0.67, "NDC");
  fitLabel.SetBorderSize(0);
  fitLabel.SetFillStyle(0);
  fitLabel.SetTextSize(0.03);
  fitLabel.AddText(Form("#chi^{2}/ndf = %.2f", chi2 / ndf));
  fitLabel.Draw();

  residualPad.cd();
  TH1 *residualFrame = residualPad.DrawFrame(fitMin, -2.0, plotMax, 2.0);
  residualFrame->SetTitle("");
  residualFrame->GetXaxis()->SetTitle("m_{#gamma+jet} [GeV]");
  residualFrame->GetYaxis()->SetTitle("(Data-Fit)/Fit");
  residualFrame->GetXaxis()->SetTitleSize(0.12);
  residualFrame->GetXaxis()->SetLabelSize(0.10);
  residualFrame->GetXaxis()->SetTitleOffset(1.05);
  residualFrame->GetYaxis()->SetTitleSize(0.10);
  residualFrame->GetYaxis()->SetLabelSize(0.08);
  residualFrame->GetYaxis()->SetTitleOffset(0.48);
  residualFrame->GetYaxis()->SetNdivisions(505);

  residual.SetMarkerStyle(20);
  residual.SetMarkerSize(0.65);
  residual.SetMarkerColor(kBlack);
  residual.SetLineColor(kBlack);
  residual.Draw("P SAME");

  TLine zeroLine(fitMin, 0.0, plotMax, 0.0);
  zeroLine.SetLineColor(kRed + 1);
  zeroLine.SetLineWidth(2);
  zeroLine.Draw();
  TLine residualLowLine(excludedLow, -2.0, excludedLow, 2.0);
  TLine residualHighLine(excludedHigh, -2.0, excludedHigh, 2.0);
  residualLowLine.SetLineStyle(2);
  residualHighLine.SetLineStyle(2);
  residualLowLine.Draw();
  residualHighLine.Draw();

  canvas.cd();
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
  residual.Write("fractionalResidual");
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
  delete displayData;
  delete sidebandData;
  delete data;
}
