#define CstarToGJ_M1000_f0p1_13TeV_NANOAOD_ana_cxx
#include "CstarToGJ_M1000_f0p1_13TeV_NANOAOD_ana.h"
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

void SetCMSStyle(){
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
        double sqrts = 13.0)
{
    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.045);
    latex.SetTextFont(62);  // bold for "CMS"
    latex.DrawLatex(x, y, "CMS");

    latex.SetTextFont(52);  // italic for "Preliminary"
    latex.DrawLatex(x + 0.06, y, text);

    latex.SetTextFont(42);  // regular font
    TString lumiText = Form("%g TeV", sqrts);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.75, 0.95, lumiText);
}//void CMS_label

double lumi = 41800.0; 
double xsec = 1.291e-02; 

void CstarToGJ_M1000_f0p1_13TeV_NANOAOD_ana::Loop()
{

    if (fChain == 0) return;

    TH1F *hM_gen   = new TH1F("hM_gen",   "GEN M(#gamma + jet);M^{GEN}_{#gamma j} (GeV);Events",   500, 0., 3000.);
    TH1F *h_M_cstar = new TH1F("h_m_cstar", "Mass of C*; M_{C*} [GeV]; Events", 100, 500, 3000);
    TH1F *hM_reco_selected = new TH1F("hM_reco_selected", "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 300, 0., 3000.);

    TH1F *hPhoton_pt = new TH1F("hPhoton_pT", "Photon p_{T};p_{T}^{photon} (GeV);Events", 500, 0., 1500.);
    TH1F *hJet_pt = new TH1F("hJet_pT", "Jet p_{T};p_{T}^{jet} (GeV);Events", 500, 0., 1500.);

    hM_gen ->Sumw2();
    hPhoton_pt->Sumw2();
    hJet_pt->Sumw2();
    hM_reco_selected->Sumw2();

    double sum_genWeight = 0.0;

    Long64_t nentries = fChain->GetEntriesFast();
    
    for (Long64_t jentry = 0; jentry < nentries; jentry++) {
    LoadTree(jentry);
    fChain->GetEntry(jentry);
    sum_genWeight += genWeight;
}
std::cout << "Total genWeight sum = " << sum_genWeight << std::endl;


    Long64_t nbytes = 0, nb = 0;
    for (Long64_t jentry=0; jentry<nentries;jentry++) {
        Long64_t ientry = LoadTree(jentry);
        if (ientry < 0) break;
        nb = fChain->GetEntry(jentry);   nbytes += nb;
        // if (Cut(ientry) < 0) continue;

        double weight = lumi * xsec * genWeight / sum_genWeight;

        if (nPhoton < 1 || nJet < 1 || nGenPart <= 0 || nGenJet <= 0)   continue;

        int cstarIdx = -1;
        int genPhotonIdx = -1;
        int genCharmIdx = -1;

        std::vector<int> cstar;
        for (int i= 0; i< nGenPart; ++i){
            if (GenPart_pdgId[i] == 4000004){
                cstar.push_back(i);
            }

        }//for (int i =0; i< nGenPart; ++i)
        if (cstar.size() == 0) continue;

        for (int idx : cstar){
            int photon = -1;
            int charm = -1;

            for (int i= 0; i< nGenPart; ++i){
                if (GenPart_genPartIdxMother[i] != idx) continue;
                if (GenPart_pdgId[i] == 22) photon = i;
                if (GenPart_pdgId[i] == 4) charm = i;
            }
            if (photon >=0 && charm >=0){
                cstarIdx = idx;
                genPhotonIdx = photon;
                genCharmIdx = charm;
                break;
            }

        }//for (int idx : cstar)
        if (cstarIdx < 0) continue;

        // Cstar mass
        TLorentzVector cstar_p4;
        cstar_p4.SetPtEtaPhiM(GenPart_pt[cstarIdx], GenPart_eta[cstarIdx], GenPart_phi[cstarIdx], GenPart_mass[cstarIdx]);
        h_M_cstar->Fill(cstar_p4.M(), weight);

        //Match Gen charm → GenJet
        TLorentzVector c_p4, GenJet_p4;

        int genJetIdx = -1;
        float best_deltaR_cJet = 999;
        for (int j= 0; j< nGenJet; ++j){
            GenJet_p4.SetPtEtaPhiM(GenJet_pt[j], GenJet_eta[j], GenJet_phi[j], GenJet_mass[j]);
            c_p4.SetPtEtaPhiM(GenPart_pt[genCharmIdx], GenPart_eta[genCharmIdx], GenPart_phi[genCharmIdx], GenPart_mass[genCharmIdx]);

            float deltaR_cJet = c_p4.DeltaR(GenJet_p4);
            if (deltaR_cJet < best_deltaR_cJet){
                best_deltaR_cJet = deltaR_cJet;
                genJetIdx = j;
            }
        }//for (int j= 0; j< nGenJet; ++j)
        if (best_deltaR_cJet > 0.2) continue;

        //Gen invariant mass
        TLorentzVector gen_photon_p4, gen_jet_p4;
        gen_photon_p4.SetPtEtaPhiM(GenPart_pt[genPhotonIdx], GenPart_eta[genPhotonIdx], GenPart_phi[genPhotonIdx], GenPart_mass[genPhotonIdx]);
        gen_jet_p4.SetPtEtaPhiM(GenJet_pt[genJetIdx], GenJet_eta[genJetIdx], GenJet_phi[genJetIdx], GenJet_mass[genJetIdx]);

        TLorentzVector gen_M_p4 = gen_photon_p4 + gen_jet_p4;
        hM_gen->Fill(gen_M_p4.M(), weight);


        // *******************************  RECO Photon + Jet selection *******************************

        // Photon selection
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
        g_p4.SetPtEtaPhiM(Photon_pt[goodPhotonIdx], Photon_eta[goodPhotonIdx], Photon_phi[goodPhotonIdx], Photon_mass[goodPhotonIdx]);

        // Jet selection
        std::vector<int> goodJets;

        for (UInt_t i= 0; i < nJet; ++i){
            if (Jet_pt[i] < 170) continue;
            if (Jet_eta[i] >= 2.4) continue;
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

        // *********** RECO Invatiant Mass ( selected RECO leadnig photon + RECO leading jet) ***********

        TLorentzVector reco_g_p4, reco_j_p4;
        reco_g_p4.SetPtEtaPhiM(Photon_pt[goodPhotonIdx], Photon_eta[goodPhotonIdx], Photon_phi[goodPhotonIdx], Photon_mass[goodPhotonIdx]);
        reco_j_p4.SetPtEtaPhiM(Jet_pt[goodJetIdx], Jet_eta[goodJetIdx], Jet_phi[goodJetIdx], Jet_mass[goodJetIdx]);

        TLorentzVector reco_m_p4 = reco_g_p4 + reco_j_p4;
        hM_reco_selected -> Fill(reco_m_p4.M(), weight);

        hPhoton_pt->Fill(Photon_pt[goodPhotonIdx], weight);
        hJet_pt->Fill(Jet_pt[goodJetIdx], weight);

    }//jentry

    SetCMSStyle();

    gROOT->SetBatch(kTRUE); // run without opening any windows

    TCanvas *c1 = new TCanvas("c1", "Invariant Mass Gen", 600, 400);
    hM_gen->Draw("HIST");
    hM_gen->GetXaxis()->SetTitleOffset(1.4);
    hM_gen->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c1->SaveAs("Invariant_Mass_gen.png");


    TCanvas *c3 = new TCanvas("c3", "Mass of C*", 600, 400);
    h_M_cstar->Draw("HIST");
    h_M_cstar->GetXaxis()->SetTitleOffset(1.4);    // lower x-title
    h_M_cstar->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c3->SaveAs("Mass_Cstar.png");


    TCanvas *c4 = new TCanvas("c4", "Photon_pT", 600, 400);
    hPhoton_pt->Draw("HIST");
    hPhoton_pt->GetXaxis()->SetTitleOffset(1.4);    // lower x-title
    hPhoton_pt->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c4->SaveAs("Photon_pT.png");


    TCanvas *c5 = new TCanvas("c5", "Jet_pT", 600, 400);
    hJet_pt->Draw("HIST");
    hJet_pt->GetXaxis()->SetTitleOffset(1.4);    // lower x-title
    hJet_pt->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c5->SaveAs("Jet_pT.png");

    TCanvas *c6 = new TCanvas("c6", "Invariant Mass Reco", 600, 400);
    hM_reco_selected->Draw("HIST");
    hM_reco_selected->GetXaxis()->SetTitleOffset(1.4);    // lower x-title
    hM_reco_selected->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c6->SaveAs("Invariant_Mass_reco_selected.png");




    TFile *fOut = new TFile("CstarToGJ.root", "RECREATE");
    h_M_cstar->Write();
    hM_gen->Write();
    hM_reco_selected ->Write();
    hPhoton_pt->Write();
    hJet_pt->Write();
    fOut->Close();
    delete fOut;

}//void
