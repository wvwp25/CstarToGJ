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
#include "TParameter.h"
#include "CMSStyle.h"

SetCMSStyle();
CMS_label(0.18, 0.87, lumi, 13.0);


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
        hM->Fill(M_p4.M());
        hM->Scale(lumiScale);
    }//for (Long64_t jentry=0; jentry<nentries;jentry++)


    SetCMSStyle();

    gROOT->SetBatch(kTRUE); // run without opening any windows

    TCanvas *c1 = new TCanvas("c1", "Invariant Mass #gamma + Jet", 600, 700);


    hM->Draw("E");
    hM->GetXaxis()->SetTitleOffset(1.4);
    hM->GetXaxis()->SetLabelOffset(0.02);
    hM->GetXaxis()->SetTitle("m_{#gamma+Jet} [GeV]");
    hM->GetXaxis()->SetTitleSize(0.12);
    
    c1->SaveAs("Invariant_Mass_gJet.png");

    TFile *fOut = new TFile("Run2017BCDEF_BG_ana.root", "RECREATE");
    hM->Write();
    TParameter<double> pLumi("lumi_fb", lumi);
    pLumi.Write();
    TParameter<double> pLumiScale("lumiScale", lumiScale);
    pLumiScale.Write();
    TParameter<int>    pFraction("fraction", fraction);
    pFraction.Write();
    fOut->Close();
    delete fOut;

}

