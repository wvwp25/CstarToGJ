#include "RooCrystalBall.h"
#include "RooDataHist.h"
#include "RooFit.h"
#include "RooFitResult.h"
#include "RooFormulaVar.h"
#include "RooAbsData.h"
#include "RooPlot.h"
#include "RooRealVar.h"
#include "RooWorkspace.h"

#include "TFile.h"
#include "TCanvas.h"
#include "TAxis.h"
#include "TLegend.h"
#include "TPaveText.h"
#include "TH1.h"
#include "TROOT.h"

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace RooFit;

namespace {

struct TemplateSpec {
    const char *histName;
    const char *label;
};

struct FitParams {
    double yield = 0.0;
    double x0 = 0.0;
    double sigmaL = 0.0;
    double sigmaR = 0.0;
    double alphaL = 0.0;
    double nL = 0.0;
    double alphaR = 0.0;
    double nR = 0.0;
    int status = -999;
};

double integralInRange(const TH1 *hist, double xmin, double xmax)
{
    const int firstBin = hist->GetXaxis()->FindBin(xmin);
    const int lastBin = hist->GetXaxis()->FindBin(xmax);
    return hist->Integral(firstBin, lastBin);
}

double peakInRange(const TH1 *hist, double xmin, double xmax)
{
    const int firstBin = hist->GetXaxis()->FindBin(xmin);
    const int lastBin = hist->GetXaxis()->FindBin(xmax);

    int maxBin = firstBin;
    double maxContent = hist->GetBinContent(firstBin);
    for (int bin = firstBin + 1; bin <= lastBin; ++bin) {
        if (hist->GetBinContent(bin) > maxContent) {
            maxContent = hist->GetBinContent(bin);
            maxBin = bin;
        }
    }
    return hist->GetBinCenter(maxBin);
}

void saveFitPlot(RooRealVar &x,
                 RooDataHist &data,
                 RooCrystalBall &dscb,
                 const TemplateSpec &spec,
                 double xminFit,
                 double xmaxFit,
                 RooRealVar &mean,
                 RooRealVar &sigmaL,
                 RooRealVar &sigmaR,
                 RooRealVar &alphaL,
                 RooRealVar &nL,
                 RooRealVar &alphaR,
                 RooRealVar &nR)
{
    TCanvas canvas((std::string("c_fit_") + spec.histName).c_str(),
                   Form("DSCB Fit %s", spec.label), 800, 600);

    RooPlot *frame = x.frame(Range("fitRange"), Bins(60),
                             Title(Form("DSCB fit: %s", spec.label)));
    data.plotOn(frame, Name("data"),
                DataError(RooAbsData::SumW2),
                MarkerStyle(20),
                MarkerSize(0.8),
                MarkerColor(kBlack),
                LineColor(kBlack));

    dscb.plotOn(frame, Name("dscb"),
                Range("fitRange"),
                NormRange("fitRange"),
                LineColor(kRed),
                LineWidth(2));

    frame->GetXaxis()->SetTitle("m_{#gamma j} [GeV]");
    frame->GetYaxis()->SetTitle("Events");
    frame->GetXaxis()->SetRangeUser(xminFit, xmaxFit);
    frame->SetMinimum(0.0);
    frame->Draw();

    const double chi2 = frame->chiSquare("dscb", "data", 7);

    TLegend legend(0.60, 0.76, 0.88, 0.88);
    legend.SetBorderSize(0);
    legend.SetFillStyle(0);
    legend.AddEntry(frame->findObject("data"), "Signal (MC)", "lep");
    legend.AddEntry(frame->findObject("dscb"), "DSCB fit", "l");
    legend.Draw();

    TPaveText text(0.60, 0.43, 0.88, 0.74, "NDC");
    text.SetBorderSize(0);
    text.SetFillColor(0);
    text.SetTextAlign(12);
    text.SetTextSize(0.03);
    text.AddText(Form("Mean = %.1f #pm %.1f", mean.getVal(), mean.getError()));
    text.AddText(Form("#sigma_{L} = %.1f #pm %.1f", sigmaL.getVal(), sigmaL.getError()));
    text.AddText(Form("#sigma_{R} = %.1f #pm %.1f", sigmaR.getVal(), sigmaR.getError()));
    text.AddText(Form("#alpha_{L} = %.2f #pm %.2f", alphaL.getVal(), alphaL.getError()));
    text.AddText(Form("n_{L} = %.2f #pm %.2f", nL.getVal(), nL.getError()));
    text.AddText(Form("#alpha_{R} = %.2f #pm %.2f", alphaR.getVal(), alphaR.getError()));
    text.AddText(Form("n_{R} = %.2f #pm %.2f", nR.getVal(), nR.getError()));
    text.AddText(Form("#chi^{2}/N_{dof} = %.4f", chi2));
    text.Draw();

    const std::string plotName = std::string("DSCB_fit_paramSyst_") + spec.histName + ".png";
    canvas.SaveAs(plotName.c_str());
    delete frame;
}

FitParams fitTemplate(TFile &inputFile,
                      RooRealVar &x,
                      const TemplateSpec &spec,
                      double xminFit,
                      double xmaxFit)
{
    FitParams result;

    TH1 *hist = dynamic_cast<TH1*>(inputFile.Get(spec.histName));
    if (!hist) {
        std::cerr << "ERROR: missing histogram " << spec.histName << std::endl;
        return result;
    }

    result.yield = integralInRange(hist, xminFit, xmaxFit);
    if (result.yield <= 0.0) {
        std::cerr << "ERROR: non-positive fitted yield for " << spec.histName
                  << ": " << result.yield << std::endl;
        return result;
    }

    const double peak = peakInRange(hist, xminFit, xmaxFit);
    const std::string tag = spec.histName;

    RooDataHist data(("data_" + tag).c_str(), ("data_" + tag).c_str(),
                     RooArgList(x), hist);

    RooRealVar mean(("mean_" + tag).c_str(), "mean", peak, xminFit, xmaxFit);
    RooRealVar sigmaL(("sigmaL_" + tag).c_str(), "sigmaL", 50.0, 1.0, 250.0);
    RooRealVar sigmaR(("sigmaR_" + tag).c_str(), "sigmaR", 50.0, 1.0, 250.0);
    RooRealVar alphaL(("alphaL_" + tag).c_str(), "alphaL", 1.5, 0.05, 10.0);
    RooRealVar nL(("nL_" + tag).c_str(), "nL", 5.0, 0.05, 50.0);
    RooRealVar alphaR(("alphaR_" + tag).c_str(), "alphaR", 1.5, 0.05, 10.0);
    RooRealVar nR(("nR_" + tag).c_str(), "nR", 5.0, 0.05, 50.0);

    RooCrystalBall dscb(("dscb_" + tag).c_str(), "Double-Sided Crystal Ball",
                        x, mean, sigmaL, sigmaR,
                        alphaL, nL, alphaR, nR);

    RooFitResult *fitResult = dscb.fitTo(data,
                                         Range("fitRange"),
                                         SumW2Error(kTRUE),
                                         Save(kTRUE),
                                         PrintLevel(-1));

    result.status = fitResult ? fitResult->status() : -999;
    result.x0 = mean.getVal();
    result.sigmaL = sigmaL.getVal();
    result.sigmaR = sigmaR.getVal();
    result.alphaL = alphaL.getVal();
    result.nL = nL.getVal();
    result.alphaR = alphaR.getVal();
    result.nR = nR.getVal();

    saveFitPlot(x, data, dscb, spec, xminFit, xmaxFit,
                mean, sigmaL, sigmaR, alphaL, nL, alphaR, nR);

    delete fitResult;

    std::cout << spec.histName
              << " yield=" << result.yield
              << " mean=" << result.x0
              << " sigmaL=" << result.sigmaL
              << " sigmaR=" << result.sigmaR
              << " status=" << result.status << std::endl;

    return result;
}

double halfDiff(const std::map<std::string, FitParams> &params,
                const char *upName,
                const char *downName,
                double FitParams::*member)
{
    return (params.at(upName).*member - params.at(downName).*member) / 2.0;
}

RooFormulaVar makeSystParam(const char *name,
                            const char *title,
                            double nominal,
                            double deltaJES,
                            double deltaJER,
                            double deltaPES,
                            RooRealVar &thetaJES,
                            RooRealVar &thetaJER,
                            RooRealVar &thetaPES)
{
    const std::string formula = Form("%.17g + %.17g*@0 + %.17g*@1 + %.17g*@2",
                                     nominal, deltaJES, deltaJER, deltaPES);
    return RooFormulaVar(name, title, formula.c_str(),
                         RooArgList(thetaJES, thetaJER, thetaPES));
}

} // namespace

void produce_signal_DSCB_workspace_paramSyst()
{
    gROOT->SetBatch(kTRUE);

    const char *inputName = "CstarToGJ.root";
    const char *outputName = "signal_DSCB_workspace_paramSyst.root";
    const double xMin = 500.0;
    const double xMax = 1400.0;
    const double xminFit = 500.0;
    const double xmaxFit = 1400.0;

    TFile inputFile(inputName, "READ");
    if (inputFile.IsZombie()) {
        std::cerr << "ERROR: cannot open " << inputName << std::endl;
        return;
    }

    RooRealVar x("x", "M_{#gamma+jet} [GeV]", xMin, xMax);
    x.setRange("fitRange", xminFit, xmaxFit);

    const std::vector<TemplateSpec> templates = {
        {"sig", "Nominal"},
        {"sig_JESUp", "JESUp"},
        {"sig_JESDown", "JESDown"},
        {"sig_JERUp", "JERUp"},
        {"sig_JERDown", "JERDown"},
        {"sig_PESUp", "PESUp"},
        {"sig_PESDown", "PESDown"}
    };

    std::map<std::string, FitParams> params;
    bool allOk = true;
    for (const TemplateSpec &spec : templates) {
        FitParams fit = fitTemplate(inputFile, x, spec, xminFit, xmaxFit);
        params[spec.histName] = fit;
        allOk = allOk && (fit.status == 0) && (fit.yield > 0.0);
    }

    for (const TemplateSpec &spec : templates) {
        if (params.at(spec.histName).yield <= 0.0) {
            std::cerr << "ERROR: cannot build workspace because fit failed for "
                      << spec.histName << std::endl;
            return;
        }
    }

    const FitParams &nom = params.at("sig");

    const double dx0JES = halfDiff(params, "sig_JESUp", "sig_JESDown", &FitParams::x0);
    const double dsigmaLJES = halfDiff(params, "sig_JESUp", "sig_JESDown", &FitParams::sigmaL);
    const double dsigmaRJES = halfDiff(params, "sig_JESUp", "sig_JESDown", &FitParams::sigmaR);
    const double dalphaLJES = halfDiff(params, "sig_JESUp", "sig_JESDown", &FitParams::alphaL);
    const double dnLJES = halfDiff(params, "sig_JESUp", "sig_JESDown", &FitParams::nL);
    const double dalphaRJES = halfDiff(params, "sig_JESUp", "sig_JESDown", &FitParams::alphaR);
    const double dnRJES = halfDiff(params, "sig_JESUp", "sig_JESDown", &FitParams::nR);

    const double dx0JER = halfDiff(params, "sig_JERUp", "sig_JERDown", &FitParams::x0);
    const double dsigmaLJER = halfDiff(params, "sig_JERUp", "sig_JERDown", &FitParams::sigmaL);
    const double dsigmaRJER = halfDiff(params, "sig_JERUp", "sig_JERDown", &FitParams::sigmaR);
    const double dalphaLJER = halfDiff(params, "sig_JERUp", "sig_JERDown", &FitParams::alphaL);
    const double dnLJER = halfDiff(params, "sig_JERUp", "sig_JERDown", &FitParams::nL);
    const double dalphaRJER = halfDiff(params, "sig_JERUp", "sig_JERDown", &FitParams::alphaR);
    const double dnRJER = halfDiff(params, "sig_JERUp", "sig_JERDown", &FitParams::nR);

    const double dx0PES = halfDiff(params, "sig_PESUp", "sig_PESDown", &FitParams::x0);
    const double dsigmaLPES = halfDiff(params, "sig_PESUp", "sig_PESDown", &FitParams::sigmaL);
    const double dsigmaRPES = halfDiff(params, "sig_PESUp", "sig_PESDown", &FitParams::sigmaR);
    const double dalphaLPES = halfDiff(params, "sig_PESUp", "sig_PESDown", &FitParams::alphaL);
    const double dnLPES = halfDiff(params, "sig_PESUp", "sig_PESDown", &FitParams::nL);
    const double dalphaRPES = halfDiff(params, "sig_PESUp", "sig_PESDown", &FitParams::alphaR);
    const double dnRPES = halfDiff(params, "sig_PESUp", "sig_PESDown", &FitParams::nR);

    RooRealVar thetaJES("theta_JES", "theta_JES", 0.0, -5.0, 5.0);
    RooRealVar thetaJER("theta_JER", "theta_JER", 0.0, -5.0, 5.0);
    RooRealVar thetaPES("theta_PES", "theta_PES", 0.0, -5.0, 5.0);

    RooFormulaVar x0Syst = makeSystParam("x0_syst", "x0_syst",
                                         nom.x0, dx0JES, dx0JER, dx0PES,
                                         thetaJES, thetaJER, thetaPES);
    RooFormulaVar sigmaLSyst = makeSystParam("sigmaL_syst", "sigmaL_syst",
                                             nom.sigmaL, dsigmaLJES, dsigmaLJER, dsigmaLPES,
                                             thetaJES, thetaJER, thetaPES);
    RooFormulaVar sigmaRSyst = makeSystParam("sigmaR_syst", "sigmaR_syst",
                                             nom.sigmaR, dsigmaRJES, dsigmaRJER, dsigmaRPES,
                                             thetaJES, thetaJER, thetaPES);
    RooFormulaVar alphaLSyst = makeSystParam("alphaL_syst", "alphaL_syst",
                                             nom.alphaL, dalphaLJES, dalphaLJER, dalphaLPES,
                                             thetaJES, thetaJER, thetaPES);
    RooFormulaVar nLSyst = makeSystParam("nL_syst", "nL_syst",
                                         nom.nL, dnLJES, dnLJER, dnLPES,
                                         thetaJES, thetaJER, thetaPES);
    RooFormulaVar alphaRSyst = makeSystParam("alphaR_syst", "alphaR_syst",
                                             nom.alphaR, dalphaRJES, dalphaRJER, dalphaRPES,
                                             thetaJES, thetaJER, thetaPES);
    RooFormulaVar nRSyst = makeSystParam("nR_syst", "nR_syst",
                                         nom.nR, dnRJES, dnRJER, dnRPES,
                                         thetaJES, thetaJER, thetaPES);

    RooCrystalBall dscb("dscb", "DSCB with shape systematics",
                        x, x0Syst, sigmaLSyst, sigmaRSyst,
                        alphaLSyst, nLSyst, alphaRSyst, nRSyst);

    RooRealVar nsig("nsig", "signal yield", nom.yield, 0.0, 10.0 * nom.yield);

    RooWorkspace ws("ws", "workspace");
    ws.import(dscb);
    ws.import(nsig);
    ws.writeToFile(outputName);

    std::cout << "\nNominal parameters and symmetric deltas:\n";
    std::cout << "parameter nominal delta_JES delta_JER delta_PES\n";
    std::cout << "x0 " << nom.x0 << " " << dx0JES << " " << dx0JER << " " << dx0PES << "\n";
    std::cout << "sigmaL " << nom.sigmaL << " " << dsigmaLJES << " " << dsigmaLJER << " " << dsigmaLPES << "\n";
    std::cout << "sigmaR " << nom.sigmaR << " " << dsigmaRJES << " " << dsigmaRJER << " " << dsigmaRPES << "\n";
    std::cout << "alphaL " << nom.alphaL << " " << dalphaLJES << " " << dalphaLJER << " " << dalphaLPES << "\n";
    std::cout << "nL " << nom.nL << " " << dnLJES << " " << dnLJER << " " << dnLPES << "\n";
    std::cout << "alphaR " << nom.alphaR << " " << dalphaRJES << " " << dalphaRJER << " " << dalphaRPES << "\n";
    std::cout << "nR " << nom.nR << " " << dnRJES << " " << dnRJER << " " << dnRPES << "\n";

    std::cout << "\nWorkspace contents:\n";
    ws.Print();
    std::cout << "\nWrote " << outputName << std::endl;

    if (!allOk) {
        std::cerr << "WARNING: at least one fit returned non-zero status."
                  << std::endl;
    }
}
