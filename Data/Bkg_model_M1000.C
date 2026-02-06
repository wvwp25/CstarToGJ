#include <iostream>
#include <cmath>

#include "TFile.h"
#include "TH1.h"
#include "TF1.h"
#include "TMath.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TPaveText.h"
#include "TLegend.h"
#include "TLine.h"
#include "TLatex.h"
#include "TStyle.h"
#include "TParameter.h"

static void SetCMSStyle(){
    gStyle->SetOptStat(0);
    gStyle->SetTitleFontSize(0.05);
    gStyle->SetLineWidth(2);
    gStyle->SetFrameLineWidth(2);
    gStyle->SetLabelSize(0.045,"XY");
    gStyle->SetTitleSize(0.05,"XY");
    gStyle->SetPadTopMargin(0.08);
    gStyle->SetPadBottomMargin(0.12);
    gStyle->SetPadLeftMargin(0.12);
    gStyle->SetPadRightMargin(0.05);
}

static void CMS_label(double x, double y, double lumi_fb, double sqrts_TeV=13.0){
    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.045);
    latex.SetTextFont(62);
    latex.DrawLatex(x, y, "CMS");

    latex.SetTextFont(52);
    latex.DrawLatex(x + 0.07, y, "Preliminary");

    latex.SetTextFont(42);
    latex.SetTextSize(0.03);
    latex.DrawLatex(0.75, 0.93, Form("%.1f fb^{-1} (%g TeV)", lumi_fb, sqrts_TeV));
}

// Background function used in CMS dijet/gamma+jet style fits
static Double_t backgroundFunction(Double_t *xx, Double_t *par) {
    const Double_t m      = xx[0];
    const Double_t sqrt_s = 13000.0;
    const Double_t z      = m / sqrt_s;

    const Double_t P0 = par[0];
    const Double_t P1 = par[1];
    const Double_t P2 = par[2];
    const Double_t P3 = par[3];

    if (z <= 0.0 || z >= 1.0) return 0.0;

    const Double_t num = P0 * TMath::Power(1.0 - z, P1);
    const Double_t den = TMath::Power(z, P2 + P3 * TMath::Log(z));
    if (den == 0.0) return 0.0;
    return num / den;
}

static void Chi2OverSidebands(const TH1* h, TF1* f,
        double fit_low1, double fit_high1,
        double fit_low2, double fit_high2,
        double &chi2, int &nbin_used, int &ndof)
{
    chi2 = 0.0;
    nbin_used = 0;

    for (int i = 1; i <= h->GetNbinsX(); ++i) {
        const double x   = h->GetBinCenter(i);
        const double y   = h->GetBinContent(i);
        const double err = h->GetBinError(i);

        const bool inFit =
            (x >= fit_low1 && x <= fit_high1) ||
            (x >= fit_low2 && x <= fit_high2);

        if (!inFit) continue;
        if (err <= 0) continue;

        const double xl = h->GetXaxis()->GetBinLowEdge(i);
        const double xh = h->GetXaxis()->GetBinUpEdge(i);

        // bin-integrated prediction (consistent with "I" option)
        const double yfit = f->Integral(xl, xh) / (xh - xl);

        const double d = (y - yfit) / err;
        chi2 += d*d;
        nbin_used++;
    }

    ndof = nbin_used - f->GetNpar();
    if (ndof < 1) ndof = 1;
}


void Bkg_model_M1000() {

    // -------------------------
    // User config
    // -------------------------
    const double lumi_fb   = 41.78 * 4.0; // if you want this shown on plot
    const double sqrts_TeV = 13.0;

    const double fit_min = 700.0;
    const double fit_max = 3000.0;

    const double signal_mass = 1000.0;
    const double sigmaL = 37.5;
    const double sigmaR = 31.6;

    // exclude +/- 3 sigma around signal
    const double signal_low  = signal_mass - 3.0*sigmaL;
    const double signal_high = signal_mass + 3.0*sigmaR;

    const double fit_low1  = fit_min;
    const double fit_high1 = signal_low;
    const double fit_low2  = signal_high;
    const double fit_high2 = fit_max;

    // -------------------------
    // Read histogram
    // -------------------------
    TFile *fin = TFile::Open("Run2017BCDEF_BG_ana.root", "READ");
    TH1F *hM_in = dynamic_cast<TH1F*>(fin->Get("hM"));

    // Clone so output file owns it (and safe after closing fin)
    TH1F *hM = dynamic_cast<TH1F*>(hM_in->Clone("hM"));
    hM->SetDirectory(nullptr);
    fin->Close();

    hM->Sumw2();

    // Mask signal window by inflating errors
for (int i = 1; i <= hM->GetNbinsX(); ++i) {
    double x = hM->GetBinCenter(i);
    if (x > signal_low && x < signal_high) {
        hM->SetBinError(i, 1e9);  // effectively remove from chi2
    }
}

    // -------------------------
    // Define TF1 and fit sidebands
    // -------------------------
    TF1 *bkgFit = new TF1("bkgFit", backgroundFunction, fit_min, fit_max, 4);
    bkgFit->SetParNames("P0","P1","P2","P3");

    // Initial values + limits (tune if fit unstable)
    bkgFit->SetParameters(1.0, 18.0, 4.0, 0.2);
    bkgFit->SetParLimits(0, 1e-6, 1e12);
    bkgFit->SetParLimits(1, 0.0,  50.0);
    bkgFit->SetParLimits(2, 0.0,  50.0);
    bkgFit->SetParLimits(3, -50.0, 50.0);

    // Fit first sideband then second, keep params
    // "I" -> use integral of function in bin when fitting histogram
    // "R" -> fit in range
    // "S" -> return fit result
    // "0" -> quiet drawing during fit

    double chi2 = 0.0;
    int nbin_used = 0;
    int ndof = 0;

TFitResultPtr r = hM->Fit(bkgFit, "SR0", "", fit_min, fit_max);

    Chi2OverSidebands(hM, bkgFit, fit_low1, fit_high1, fit_low2, fit_high2,
            chi2, nbin_used, ndof);

    std::cout << "=== Background Fit (sidebands) ===\n";
    std::cout << "Excluded window: [" << signal_low << ", " << signal_high << "] GeV\n";
    std::cout << "Chi2 = " << chi2 << "\n";
    std::cout << "NDOF = " << ndof << "\n";
    std::cout << "Chi2/NDOF = " << (chi2 / ndof) << "\n";
    std::cout << "Params:\n";
    for (int ip=0; ip<bkgFit->GetNpar(); ++ip) {
        std::cout << "  " << bkgFit->GetParName(ip) << " = " << bkgFit->GetParameter(ip)
            << " +/- " << bkgFit->GetParError(ip) << "\n";
    }

    // -------------------------
    // Residual histogram: (Data - Fit)
    // only fill in fitted regions
    // -------------------------
    TH1F *hResidual = dynamic_cast<TH1F*>(hM->Clone("hResidual"));
    hResidual->Reset("ICES");
    hResidual->SetDirectory(nullptr);
    hResidual->SetStats(0);
    hResidual->SetTitle("");

    const int nb = hM->GetNbinsX();
    for (int i = 1; i <= nb; ++i) {
        const double x   = hM->GetBinCenter(i);
        const bool inFit =
            (x >= fit_low1 && x <= fit_high1) ||
            (x >= fit_low2 && x <= fit_high2);

        if (!inFit) continue;

        const double data = hM->GetBinContent(i);
        const double err  = hM->GetBinError(i);
        const double fitv = bkgFit->Eval(x);

        const double resid = (data - fitv);
        hResidual->SetBinContent(i, resid);

        // residual error: keep data stat error (useful visually)
        hResidual->SetBinError(i, err);
    }

    // -------------------------
    // Plot
    // -------------------------
    SetCMSStyle();
    gROOT->SetBatch(kTRUE);

    TCanvas *c = new TCanvas("c", "Bkg fit", 600, 700);

    TPad *pad1 = new TPad("pad1","top",0,0.30,1,1.00);
    TPad *pad2 = new TPad("pad2","bottom",0,0.00,1,0.30);

    pad1->SetBottomMargin(0.02);
    pad1->SetLeftMargin(0.12);
    pad1->SetRightMargin(0.05);

    pad2->SetTopMargin(0.05);
    pad2->SetBottomMargin(0.30);
    pad2->SetLeftMargin(0.12);
    pad2->SetRightMargin(0.05);

    pad1->Draw();
    pad2->Draw();

    // top pad
    pad1->cd();
    pad1->SetLogy();

    hM->SetMarkerStyle(0);
    hM->SetLineWidth(2);
    hM->GetXaxis()->SetLabelSize(0.0); // hide x labels on top pad
    hM->GetYaxis()->SetTitle("Events");
    hM->Draw("HIST");

    bkgFit->SetLineColor(kRed);
    bkgFit->SetLineWidth(2);
    bkgFit->Draw("same");

    // show excluded window with vertical lines
    TLine *l1 = new TLine(signal_low,  pad1->GetUymin(), signal_low,  pad1->GetUymax());
    TLine *l2 = new TLine(signal_high, pad1->GetUymin(), signal_high, pad1->GetUymax());
    l1->SetLineStyle(2); l2->SetLineStyle(2);
    l1->Draw("same");
    l2->Draw("same");

    CMS_label(0.18, 0.87, lumi_fb, sqrts_TeV);

    TPaveText *pt = new TPaveText(0.58, 0.60, 0.88, 0.82, "NDC");
    pt->SetBorderSize(0);
    pt->SetFillStyle(0);
    pt->SetTextAlign(12);
    pt->AddText(Form("#chi^{2}/ndf = %.3f", chi2/ndof));
    pt->Draw();

    // bottom pad
    pad2->cd();

    hResidual->SetMarkerStyle(20);
    hResidual->SetMarkerSize(0.8);

    hResidual->GetYaxis()->SetTitle("Data - Fit");
    hResidual->GetYaxis()->SetTitleSize(0.08);
    hResidual->GetYaxis()->SetLabelSize(0.06);
    hResidual->GetYaxis()->SetTitleOffset(0.45);
    hResidual->GetYaxis()->SetNdivisions(505);

    hResidual->GetXaxis()->SetTitle("m_{#gamma+Jet} [GeV]");
    hResidual->GetXaxis()->SetTitleSize(0.08);
    hResidual->GetXaxis()->SetLabelSize(0.08);
    hResidual->GetXaxis()->SetTitleOffset(1.0);

    // set a reasonable residual range based on max abs residual in fitted region
    double maxAbs = 0.0;
    for (int i = 1; i <= nb; ++i) {
        const double x = hResidual->GetBinCenter(i);
        const bool inFit =
            (x >= fit_low1 && x <= fit_high1) ||
            (x >= fit_low2 && x <= fit_high2);
        if (!inFit) continue;
        maxAbs = std::max(maxAbs, std::fabs(hResidual->GetBinContent(i)));
    }
    if (maxAbs <= 0.0) maxAbs = 1.0;
    hResidual->GetYaxis()->SetRangeUser(-1.2*maxAbs, 1.2*maxAbs);

    hResidual->Draw("E");

    TLine *zero = new TLine(fit_min, 0.0, fit_max, 0.0);
    zero->SetLineStyle(2);
    zero->Draw("same");

    c->SaveAs("Invariant_Mass_gJet_M1000.png");

    // -------------------------
    // Save output ROOT
    // -------------------------
    TFile *fout = new TFile("Run2017BCDEF_BG_ana_M1000.root", "RECREATE");
    hM->Write("hM");
    bkgFit->Write("bkgFit");
    hResidual->Write("hResidual");

    TParameter<double>("lumi_fb", lumi_fb).Write();
    TParameter<double>("sqrts_TeV", sqrts_TeV).Write();
    TParameter<double>("fit_min", fit_min).Write();
    TParameter<double>("fit_max", fit_max).Write();
    TParameter<double>("signal_mass", signal_mass).Write();
    TParameter<double>("sigmaL", sigmaL).Write();
    TParameter<double>("sigmaR", sigmaR).Write();
    TParameter<double>("excluded_low", signal_low).Write();
    TParameter<double>("excluded_high", signal_high).Write();
    TParameter<double>("chi2_sb", chi2).Write();
    TParameter<int>("ndof_sb", ndof).Write();

    fout->Close();
    delete fout;

    // cleanup
    delete c;
    delete l1;
    delete l2;
    delete zero;
    delete pt;

    delete hM;
    delete hResidual;
    delete bkgFit;
}
