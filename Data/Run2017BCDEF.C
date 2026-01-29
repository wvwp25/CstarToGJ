#define Run2017BCDEF_cxx
#include "Run2017BCDEF.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TF1.h>
#include <vector>
#include <cmath>
#include <TLorentzVector.h>
#include "TLatex.h"
#include "TStyle.h"
#include "TPaveText.h"
#include <TLegend.h>
#include <TLine.h>
#include <iostream>

void SetCMSStyle(){
    //test
    //gStyle->SetOptStat(0);   // no stat box
    gStyle->SetTitleFontSize(0.05);
    gStyle->SetLineWidth(2);
    gStyle->SetFrameLineWidth(2);
    gStyle->SetLabelSize(0.045,"XY");
    gStyle->SetTitleSize(0.05,"XY");
    gStyle->SetPadTopMargin(0.08);
    gStyle->SetPadBottomMargin(0.12);
    gStyle->SetPadLeftMargin(0.12);
    gStyle->SetPadRightMargin(0.05);
    gStyle->SetStatX(0.92);   // x=0.92 (right)
    gStyle->SetStatY(0.90);   // y=0.60 (lower than before)
    gStyle->SetStatW(0.20);   // width
    gStyle->SetStatH(0.15);   // height

}//void SetCMSStyle()

void CMS_label(double x = 0.08, double y = 0.88,
        const char *text = "Preliminary",
        double lumi = 41.78, double sqrts = 13.0)
{
    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.045);
    latex.SetTextFont(62);  // bold for "CMS"
    latex.DrawLatex(x, y, "CMS");

    latex.SetTextFont(52);  // italic for "Preliminary"
    latex.DrawLatex(x + 0.06, y, text);

    latex.SetTextFont(42);  // regular font
    TString lumiText = Form("%.1f fb^{-1} (%g TeV)", lumi, sqrts);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.75, 0.93, lumiText);
}//void CMS_label


float sigma = 31.3;
float signal_mass = 1000.0;
float signal_low = signal_mass - 3* sigma;
float signal_high = signal_mass + 3* sigma;



Double_t backgroundFunction(Double_t *x, Double_t *par) {
    Double_t m = x[0]; // Invariant mass
    Double_t sqrt_s = 13000.0;
    Double_t m_over_sqrt_s = m / sqrt_s;

    Double_t P0 = par[0]; // Normalization
    Double_t P1 = par[1]; // Exponent for (1 - m/sqrt_s)
    Double_t P2 = par[2]; // Constant term in denominator exponent
    Double_t P3 = par[3]; // Logarithmic term coefficient

    Double_t numerator = P0 * TMath::Power(1.0 - m_over_sqrt_s, P1);
    Double_t denominator = TMath::Power(m_over_sqrt_s, P2 + P3 * TMath::Log(m_over_sqrt_s));

    if (denominator == 0) return 0; // Avoid division by zero
    return numerator / denominator;
}


void Run2017BCDEF::Loop()
{
    if (fChain == 0) return;

    TH1F *hM = new TH1F ("hM", "Invariant Mass #gamma + Jet ; m_{#gamma+Jet}[GeV]; Events", 1000, 0, 3000);
   // TH1F *hM_700 = new TH1F ("hM_700", "Invariant Mass #gamma + Jet ; m_{#gamma+Jet}[GeV]; Events", 1000, 0, 3000);

    hM->Sumw2();
   // hM_700->Sumw2();


    Long64_t nentries = fChain->GetEntriesFast();

    Long64_t nbytes = 0, nb = 0;

    const bool useFraction = true;  // set to false when you unblind
    const int  fraction    = 2;   // use 1/fraction of events, i.e. 1/10

    for (Long64_t jentry=0; jentry<nentries;jentry++) {
        if (useFraction && (jentry % fraction != 0)) continue; //take only 1/10 of events
        Long64_t ientry = LoadTree(jentry);
        if (ientry < 0) break;
        nb = fChain->GetEntry(jentry);   nbytes += nb;
        // if (Cut(ientry) < 0) continue;

        //photon selection
        int goodPhotonIdx = -1;
        float leadingPhotonPt = -1.0;
        for (UInt_t i= 0; i< nPhoton; ++i){
            if (Photon_pt[i] < 240) continue;
            if (fabs(Photon_eta[i]) >= 1.4442) continue;
            if (Photon_cutBased[i] < 2) continue;
            if (Photon_electronVeto[i] !=1) continue;
            if (Photon_pt[i] > leadingPhotonPt){
                goodPhotonIdx = i;
                leadingPhotonPt = Photon_pt[i];
            }
        }
        if (goodPhotonIdx < 0) continue;

        TLorentzVector g_p4;
        g_p4.SetPtEtaPhiM(Photon_pt[goodPhotonIdx], Photon_eta[goodPhotonIdx], Photon_phi[goodPhotonIdx],0);

        // Jet selection
        std::vector<int> goodJets;

        for (UInt_t i= 0; i < nJet; ++i){
            if (Jet_pt[i] < 170) continue;
            if (fabs(Jet_eta[i]) >= 2.4) continue;
            if (Jet_jetId[i] < 6) continue;
            if (Jet_btagDeepFlavCvB[i] < 0.340) continue;
            if (Jet_btagDeepFlavCvL[i] < 0.085) continue;
            TLorentzVector j_p4;
            j_p4.SetPtEtaPhiM(Jet_pt[i], Jet_eta[i], Jet_phi[i], Jet_mass[i]);

            if (g_p4.DeltaR(j_p4) <= 1.1) continue;
            goodJets.push_back(i);
        }

        if (goodJets.size() == 0) continue;

        // Select Leading Jet
        int goodJetIdx = -1;
        float leadingJetPt = -1.0;

        for (int idx : goodJets){
            if (Jet_pt[idx] > leadingJetPt){
                goodJetIdx = idx;
                leadingJetPt = Jet_pt[idx];
            }
        }
        if (goodJetIdx < 0) continue;

        // ********** Invatiant Mass ( selected leadnig photon + leading jet) ***********


        TLorentzVector gamma_p4, jet_p4;
        gamma_p4.SetPtEtaPhiM(Photon_pt[goodPhotonIdx], Photon_eta[goodPhotonIdx], Photon_phi[goodPhotonIdx], 0);
        jet_p4.SetPtEtaPhiM(Jet_pt[goodJetIdx], Jet_eta[goodJetIdx], Jet_phi[goodJetIdx], Jet_mass[goodJetIdx]);

        TLorentzVector M_p4 = gamma_p4 + jet_p4;
        hM->Fill(M_p4.M());
      
    }//for (Long64_t jentry=0; jentry<nentries;jentry++)

    //    int bin_low  = hM->GetXaxis()->FindBin(signal_low);
    //    int bin_high = hM->GetXaxis()->FindBin(signal_high);

    //    for (int bin = bin_low; bin <= bin_high; ++bin) {
    //        hM->SetBinContent(bin, 0);
    //        hM->SetBinError(bin, 0);
    //    }



    ///for (Long64_t jentry=0; jentry<nentries;jentry++)

    // ********** Fitting **********

    TF1 *bkgFit = new TF1("bkgFit", backgroundFunction, 700, 3000, 4);

    bkgFit->SetParameters(0.4, 18, 4, 0.2); // Initial guesses for P0, P1, P2, P3
    bkgFit->SetParNames("P0", "P1", "P2", "P3");

    // Set reasonable parameter ranges to stabilize the fit
    bkgFit->SetParLimits(0, 0.01, 1e6);  // P0: Normalization, positive
    bkgFit->SetParLimits(1, 1.0, 30.0);   // P1: Exponent, reasonable range
    bkgFit->SetParLimits(2, 0.1, 10.0);   // P2: Denominator exponent
    bkgFit->SetParLimits(3, -5.0, 5.0);   // P3: Logarithmic coefficient


    //    hM->Fit(bkgFit, "IR", "", 700, signal_low); // "R" for range-based fit (0 to 300 GeV)
    //    hM->Fit(bkgFit, "IR+", "", signal_high, 3000); // "R" for range-based fit (0 to 300 GeV)
    TFitResultPtr r1 = hM->Fit(bkgFit, "IRS", "", 700, signal_low); 
    TFitResultPtr r2 = hM->Fit(bkgFit, "IRS+", "", signal_high, 3000); 

    //TFitResultPtr r3 = hM_700->Fit(bkgFit, "IRS", "", 700, signal_low); 
    //TFitResultPtr r4 = hM_700->Fit(bkgFit, "IRS+", "", signal_high, 3000); 

    double chi2 = r2->Chi2();
    int ndof = r2->Ndf();

    std::cout << "Background Fit Results:" << std::endl;
    std::cout << "Chi2: " << chi2 << std::endl;
    std::cout << "NDF: " << ndof << std::endl;
    std::cout << "Chi2/NDF: " << chi2/ndof << std::endl;


    SetCMSStyle();

    gROOT->SetBatch(kTRUE); // run without opening any windows


    const double fit_low1  = 700.0;
    const double fit_high1 = signal_low;

    const double fit_low2  = signal_high;
    const double fit_high2 = 3000.0;

    TH1F* hPull = (TH1F*)hM->Clone("hPull");
    hPull->Reset();
    hPull->SetStats(0);
    hPull->SetTitle("");

    for (int i = 1; i <= hM->GetNbinsX(); ++i) {

        double data    = hM->GetBinContent(i);
        double error   = hM->GetBinError(i);
        double center  = hM->GetBinCenter(i);
        double width   = hM->GetBinWidth(i);

        // ---- only show pull in fitted regions ----
        bool inFitRange =
            (center >= fit_low1  && center <= fit_high1) ||
            (center >= fit_low2  && center <= fit_high2);

        if (!inFitRange) continue;

        double fitVal = bkgFit->Eval(center) ;

        double pull = (data - fitVal);
        hPull->SetBinContent(i, pull);
        hPull->SetBinError(i, 1.0);  // pull has unit variance
    }

    TCanvas *c1 = new TCanvas("c1", "Invariant Mass #gamma + Jet", 600, 700);

    TPad* pad1 = new TPad("pad1", "top", 0, 0.3, 1, 1.0);
    TPad* pad2 = new TPad("pad2", "bottom", 0, 0.0, 1, 0.3);

    pad1->SetBottomMargin(0.12);
    pad1->SetLeftMargin(0.12);
    pad1->SetRightMargin(0.05);

    pad2->SetTopMargin(0.05);
    pad2->SetBottomMargin(0.30);
    pad2->SetLeftMargin(0.12);
    pad2->SetRightMargin(0.05);

    pad1->Draw();
    pad2->Draw();

    pad1->cd();
    hM->Draw("E");
    hM->GetXaxis()->SetTitleOffset(1.4);
    hM->GetXaxis()->SetLabelOffset(0.02);
    hM->GetXaxis()->SetTitle("m_{#gamma+Jet} [GeV]");
    hM->GetXaxis()->SetTitleSize(0.12);
    CMS_label(0.18, 0.87);
    pad1->SetLogy();
    TPaveText *pt_chi = new TPaveText(0.65, 0.45, 0.88, 0.73, "NDC");
    pt_chi->SetBorderSize(0);
    pt_chi->SetFillStyle(0);
    pt_chi->SetTextAlign(12);
    pt_chi->AddText(Form("#chi^{2} / ndf = %.6f", chi2/ndof));
    pt_chi->Draw();


    pad2->cd();

    hPull->GetYaxis()->SetTitle("Data - Fit");
    hPull->GetYaxis()->SetTitleSize(0.12);
    hPull->GetYaxis()->SetLabelSize(0.10);
    hPull->GetYaxis()->SetTitleOffset(0.45);
    hPull->GetYaxis()->SetRangeUser(-5, 5);
    hPull->GetYaxis()->SetNdivisions(505);

    hPull->GetXaxis()->SetTitle("m_{#gamma+Jet} [GeV]");
    hPull->GetXaxis()->SetTitleSize(0.12);
    hPull->GetXaxis()->SetLabelSize(0.10);
    hPull->GetXaxis()->SetTitleOffset(1.0);

    hPull->SetMarkerStyle(20);
    hPull->SetMarkerSize(0.9);


    //hPull->Draw("PE");

    // zero line
    TLine* line = new TLine(
            hPull->GetXaxis()->GetXmin(), 0,
            hPull->GetXaxis()->GetXmax(), 0
            );
    line->SetLineStyle(2);
    line->Draw("same");

    c1->SaveAs("Invariant_Mass_gJet.png");


    TFile *fOut = new TFile("Run2017BCDEF_BG_ana.root", "RECREATE");
    hM->Write();
    bkgFit->Write("bkgFit");
    fOut->Close();
    delete fOut;

}

