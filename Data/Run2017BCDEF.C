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

const double lumiScale = 4.0;
const int  fraction    = 1;   // use 1/fraction of events, i.e. 1/10
double lumi = 41.78 * lumiScale / fraction;
double sqrts = 13.0;

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
        double sqrts = 13.0)
{
    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.045);
    latex.SetTextFont(62);  // bold for "CMS"
    latex.DrawLatex(x, y, "CMS");

    latex.SetTextFont(52);  // italic for "Preliminary"
    latex.DrawLatex(x + 0.07, y, "Preliminary");

    latex.SetTextFont(42);  // regular font
    TString lumiText = Form("%.1f fb^{-1} (%g TeV)", lumi, sqrts);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.75, 0.93, lumiText);
}//void CMS_label



void Run2017BCDEF::Loop()
{
    if (fChain == 0) return;

    TH1F *hM = new TH1F ("hM", "Invariant Mass #gamma + Jet ; m_{#gamma+Jet}[GeV]; Events", 1000, 0, 3000);

    hM->Sumw2();

    Long64_t nentries = fChain->GetEntriesFast();

    Long64_t nbytes = 0, nb = 0;

    const bool useFraction = true;  // set to false when you unblind

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
        hM->Fill(M_p4.M(), lumiScale);

    }//for (Long64_t jentry=0; jentry<nentries;jentry++)


    SetCMSStyle();

    gROOT->SetBatch(kTRUE); // run without opening any windows

    TCanvas *c1 = new TCanvas("c1", "Invariant Mass #gamma + Jet", 600, 700);

    TPad* pad1 = new TPad("pad1", "top", 0, 0.3, 1, 1.0);

    pad1->SetBottomMargin(0.12);
    pad1->SetLeftMargin(0.12);
    pad1->SetRightMargin(0.05);
    pad1->Draw();

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


    c1->SaveAs("Invariant_Mass_gJet.png");

    TFile *fOut = new TFile("Run2017BCDEF_BG_ana.root", "RECREATE");
    hM->Write();
    lumi->Write();
    fOut->Close();
    delete fOut;

}

