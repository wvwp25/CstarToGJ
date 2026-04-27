#define CstarToGJ_M1000_f1p0_13TeV_NANOAOD_ana_cxx
#include "CstarToGJ_M1000_f1p0_13TeV_NANOAOD_ana.h"
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
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <random>
#include <TVector2.h>
#include <TLegend.h>
#include <TLine.h>
#include <TSystem.h>
#include "JecApplication.h"

// ***** CMS style/label *****
//{{{
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
        double lumi_fb = 41.8, double sqrts = 13.0)
{
    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.045);
    latex.SetTextFont(62);  // bold for "CMS"
    latex.DrawLatex(x, y, "CMS");

    latex.SetTextFont(52);  // italic for "Preliminary"
    latex.DrawLatex(x + 0.06, y, text);

    latex.SetTextFont(42);  // regular font
    TString lumiText = Form("%.1f fb^{-1} (%g TeV)", lumi_fb, sqrts);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.75, 0.95, lumiText);
}//void CMS_label

//}}}

double lumi_pb = 41800.0;
double xsec = 1.307;

// --------------------------------------------------
// Helper: pick leading jet in a vector of p4
// --------------------------------------------------
int GetLeadingJetIndex(const std::vector<TLorentzVector> &jets)
{
    int bestIdx = -1;
    double bestPt = -1.0;

    for (int i = 0; i < (int)jets.size(); ++i) {
        if (jets[i].Pt() > bestPt) {
            bestPt = jets[i].Pt();
            bestIdx = i;
        }
    }
    return bestIdx;
}

double deltaR(double eta1, double phi1, double eta2, double phi2)
{
    double dphi = TVector2::Phi_mpi_pi(phi1 - phi2);
    double deta = eta1 - eta2;
    return std::sqrt(deta*deta + dphi*dphi);
}

void CstarToGJ_M1000_f1p0_13TeV_NANOAOD_ana::Loop()
{

    if (fChain == 0) return;

    //Histograms
    //{{{
    TH1F *hM_gen   = new TH1F("hM_gen",   "GEN M(#gamma + jet);M^{GEN}_{#gamma j} (GeV);Events",   500, 0., 3000.);
    TH1F *hM_reco  = new TH1F("hM_reco",  "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000.);
    TH1F *h_M_cstar = new TH1F("h_m_cstar", "Mass of C*; M_{C*} [GeV]; Events", 100, 500, 3000);
    TH1F *hM_reco_selected = new TH1F("hM_reco_selected", "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000.);
    TH1F *hM_reco_selected_PU_nom = new TH1F("hM_reco_selected_PU_nom", "PU_nom M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000.);
    TH1F *hM_reco_selected_PU_up = new TH1F("hM_reco_selected_PU_up", "PU_nom M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000.);
    TH1F *hM_reco_selected_PU_down = new TH1F("hM_reco_selected_PU_down", "PU_nom M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000.);

    TH1F *hPhoton_pt = new TH1F("hPhoton_pT", "Photon p_{T};p_{T}^{photon} (GeV);Events", 500, 0., 1500.);
    TH1F *hJet_pt = new TH1F("hJet_pT", "Jet p_{T};p_{T}^{jet} (GeV);Events", 500, 0., 1500.);

    // -----------------------------
    // Central + nuisance histograms
    // -----------------------------
    TH1D *h_sig        = new TH1D("h_sig",        "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_PUUp   = new TH1D("h_sig_PUUp",   "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_PUDown = new TH1D("h_sig_PUDown", "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_JERUp  = new TH1D("h_sig_JERUp",  "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_JERDown= new TH1D("h_sig_JERDown","RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);

    TH1D *hPU_MC = new TH1D("hPU_MC", "MC PU;True interactions;Events", 100, 0, 100);
    //}}}


    TFile *fPU = TFile::Open("DataPileupHistograms.root");
    TH1D *hPU_nom_data  = (TH1D*)fPU->Get("pu_nominal");
    TH1D *hPU_up_data   = (TH1D*)fPU->Get("pu_up");
    TH1D *hPU_down_data = (TH1D*)fPU->Get("pu_down");



    hM_gen ->Sumw2();
    hM_reco->Sumw2();
    hPhoton_pt->Sumw2();
    hJet_pt->Sumw2();
    hM_reco_selected->Sumw2();

    h_sig->Sumw2();
    h_sig_PUUp->Sumw2();
    h_sig_PUDown->Sumw2();
    h_sig_JERUp->Sumw2();
    h_sig_JERDown->Sumw2();
    hPU_MC->Sumw2();

    double sum_genWeight = 0.0;

    Long64_t nentries = fChain->GetEntriesFast();

    // ----------------------------------
    // First loop: sum weights + MC PU
    // ----------------------------------
    for (Long64_t jentry = 0; jentry < nentries; jentry++) {
        LoadTree(jentry);
        fChain->GetEntry(jentry);
        sum_genWeight += genWeight;
        hPU_MC->Fill(Pileup_nTrueInt, genWeight);
    }
    std::cout << "Total genWeight sum = " << sum_genWeight << std::endl;

    //Normalize
    hPU_MC->Scale(1.0 / hPU_MC->Integral());
    hPU_nom_data->Scale(1.0 / hPU_nom_data->Integral());
    hPU_up_data->Scale(1.0 / hPU_up_data->Integral());
    hPU_down_data->Scale(1.0 / hPU_down_data->Integral());

    //Divide to get weight histograms
    TH1D *hPU_nom = (TH1D*)hPU_nom_data->Clone("hPU_weight_nom");
    TH1D *hPU_up  = (TH1D*)hPU_up_data->Clone("hPU_weight_up");
    TH1D *hPU_down= (TH1D*)hPU_down_data->Clone("hPU_weight_down");

    hPU_nom->Divide(hPU_MC);
    hPU_up->Divide(hPU_MC);
    hPU_down->Divide(hPU_MC);

    // -----------------------------
    // Main event loop
    // -----------------------------

    JecConfigReader::ConfigPaths paths;
paths.jecConfig = "/eos/user/h/hsiaoche/Signal/uncertainty_sources/jerc-application-tutorial/JecConfigAK4.json";
paths.jerSmear  = "/eos/user/h/hsiaoche/Signal/uncertainty_sources/jerc-application-tutorial/jer_smear.json.gz";


    // MC sample:
    auto jec = JecApplication::Applier::McAK4(cfg, "2017", false);

    JecApplication::SystematicOptions systNom;
    systNom.jerVar = "nom";

    Long64_t nbytes = 0, nb = 0;
    for (Long64_t jentry=0; jentry<nentries;jentry++) {
        Long64_t ientry = LoadTree(jentry);
        if (ientry < 0) break;
        nb = fChain->GetEntry(jentry);   nbytes += nb;
        // if (Cut(ientry) < 0) continue;

        int bin = hPU_nom->FindBin(Pileup_nTrueInt);
        bin = std::max(1, std::min(bin, hPU_nom->GetNbinsX())); // cleaner clamp

        // pileup reweighting factors
        double w_PU_nom  = hPU_nom->GetBinContent(bin);
        double w_PU_up   = hPU_up->GetBinContent(bin);
        double w_PU_down = hPU_down->GetBinContent(bin);

        double weight_raw = lumi_pb * xsec * genWeight / sum_genWeight;

        // Central weight should already include nominal PU
        double weight_central = weight_raw * w_PU_nom;
        double weight_PUUp    = weight_raw * w_PU_up;
        double weight_PUDown  = weight_raw * w_PU_down;


        if (nPhoton < 1 || nJet < 1 || nGenPart <= 0 || nGenJet <= 0)   continue;

        // ***** Gen *****

        int cstarIdx = -1;
        int genPhotonIdx = -1;
        int genCharmIdx = -1;

        std::vector<int> cstar; //contains indices of all generated c* particles
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
        h_M_cstar->Fill(cstar_p4.M(), weight_central);

        //Match Gen charm GenJet
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
        hM_gen->Fill(gen_M_p4.M(), weight_central);


        // *******************************  RECO Photon + Jet selection *******************************

        // Photon selection
        int goodPhotonIdx = -1;
        float leadingPhotonPt = -1.0;

        for (int i= 0; i< nPhoton; ++i){
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

        // -----------------------------
        // Jet selection:
        // central, JER up, JER down
        // -----------------------------

        std::vector<int> goodJets;
        std::vector<TLorentzVector> goodJetP4s;


        for (int i= 0; i < nJet; ++i){

            // ***** JES nominal *****
            JecApplication::JesInputs jin;
            jin.pt        = Jet_pt[i];
            jin.eta       = Jet_eta[i];
            jin.phi       = Jet_phi[i];
            jin.area      = Jet_area[i];
            jin.rho       = fixedGridRhoFastjetAll;
            jin.rawFactor = Jet_rawFactor[i];

            double jes = jec.jesFactorNominal(jin);

            // Rebuild raw jet
            double rawPt   = Jet_pt[i]   * (1.0 - Jet_rawFactor[i]);
            double rawMass = Jet_mass[i] * (1.0 - Jet_rawFactor[i]);

            double ptAfterJes   = rawPt   * jes;
            double massAfterJes = rawMass * jes;

            // ***** Hybrid JER matching *****
            JecApplication::JerInputs jrin;
            jrin.hasGen = false;      
            jrin.event  = jentry;
            jrin.maxDr  = 0.2;

            double bestDr = 999.0;
            int matchedGenJetIdx = -1;

            for (int ig = 0; ig < nGenJet; ++ig) {
        double dR = deltaR(Jet_eta[i], Jet_phi[i],
                           GenJet_eta[ig], GenJet_phi[ig]);

        if (dR < bestDr) {
            bestDr = dR;
            matchedGenJetIdx = ig;
        }
    }
            if (matchedGenJetIdx >= 0) {
        jrin.hasGen = true;
        jrin.genPt  = GenJet_pt[matchedGenJetIdx];
        jrin.genEta = GenJet_eta[matchedGenJetIdx];
        jrin.genPhi = GenJet_phi[matchedGenJetIdx];
    }
            // jerFactor() itself checks:
    //   DeltaR < maxDr
    //   |ptReco - ptGen| < 3 * resolution * ptReco
    // If failed, it automatically uses stochastic smearing.

            double jer = jec.jerFactor(
                    {ptAfterJes, Jet_eta[i], Jet_phi[i],
                    Jet_area[i], fixedGridRhoFastjetAll, 0.0},
                    jrin,
                    systNom
                    );

            // final corrected jet
            double ptCorr   = ptAfterJes * jer;
            double massCorr = massAfterJes * jer;

            if (ptCorr < 170) continue;
            if (fabs(Jet_eta[i]) >= 2.4) continue;
            if (Jet_jetId[i] < 6) continue;
            if (Jet_btagDeepFlavCvB[i] < 0.340) continue;
            if (Jet_btagDeepFlavCvL[i] < 0.085) continue;

            TLorentzVector j_raw;
            j_raw.SetPtEtaPhiM(Jet_pt[i], Jet_eta[i], Jet_phi[i], Jet_mass[i]);

            TLorentzVector j_corr;
            j_corr.SetPtEtaPhiM(ptCorr, Jet_eta[i], Jet_phi[i], massCorr);


            if (g_p4.DeltaR(j_raw) <= 0.4) continue;
            if (g_p4.DeltaR(j_corr) <= 0.4) continue;
            goodJets.push_back(i);
            goodJetP4s.push_back(j_corr);
        }
        if (goodJets.size() == 0) continue;



        // Select Leading Jet
        int goodJetIdx = -1;
        int goodJetVecIdx = -1;
        float leadingJetPt = -1.0;

        for (int k = 0; k < (int)goodJets.size(); ++k) {
            if (goodJetP4s[k].Pt() > leadingJetPt) {
                goodJetIdx = goodJets[k];
                goodJetVecIdx = k;
                leadingJetPt = goodJetP4s[k].Pt();
            }
        }

        if (goodJetIdx < 0) continue;

        // *********** RECO Invatiant Mass ( selected RECO leadnig photon + RECO leading jet) ***********

        TLorentzVector reco_g_p4;
        reco_g_p4.SetPtEtaPhiM(Photon_pt[goodPhotonIdx], Photon_eta[goodPhotonIdx], Photon_phi[goodPhotonIdx], Photon_mass[goodPhotonIdx]);
        TLorentzVector reco_j_p4 = goodJetP4s[goodJetVecIdx];

        TLorentzVector reco_m_p4 = reco_g_p4 + reco_j_p4;
        hM_reco_selected -> Fill(reco_m_p4.M(), weight_raw);

        hM_reco_selected_PU_nom -> Fill(reco_m_p4.M(), weight_central);
        hM_reco_selected_PU_up -> Fill(reco_m_p4.M(), weight_PUUp);
        hM_reco_selected_PU_down -> Fill(reco_m_p4.M(), weight_PUDown);

        hPhoton_pt->Fill(Photon_pt[goodPhotonIdx], weight_raw);
        hJet_pt->Fill(Jet_pt[goodJetIdx], weight_raw);




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

    TCanvas *c7 = new TCanvas("c7", "Invariant Mass Reco PU Nom", 600, 400);
    hM_reco_selected_PU_nom->Draw("HIST");
    hM_reco_selected_PU_nom->GetXaxis()->SetTitleOffset(1.4);
    hM_reco_selected_PU_nom->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c7->SaveAs("Invariant_Mass_reco_selected_PU_nom.png");

    TCanvas *c8 = new TCanvas("c8", "Invariant Mass Reco PU Up", 600, 400);
    hM_reco_selected_PU_up->Draw("HIST");
    hM_reco_selected_PU_up->GetXaxis()->SetTitleOffset(1.4);
    hM_reco_selected_PU_up->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c8->SaveAs("Invariant_Mass_reco_selected_PU_up.png");

    TCanvas *c9 = new TCanvas("c9", "Invariant Mass Reco PU Down", 600, 400);
    hM_reco_selected_PU_down->Draw("HIST");
    hM_reco_selected_PU_down->GetXaxis()->SetTitleOffset(1.4);
    hM_reco_selected_PU_down->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c9->SaveAs("Invariant_Mass_reco_selected_PU_down.png");

    TCanvas *c10 = new TCanvas("c10", "PU comparison", 600, 400);

    hM_reco_selected_PU_down->SetLineColor(kBlue);
    hM_reco_selected_PU_down->SetLineWidth(1);

    hM_reco_selected_PU_nom->SetLineColor(kBlack);
    hM_reco_selected_PU_nom->SetLineWidth(1);

    hM_reco_selected_PU_up->SetLineColor(kRed);
    hM_reco_selected_PU_up->SetLineWidth(1);

    hM_reco_selected_PU_down->Draw("HIST");
    hM_reco_selected_PU_nom->Draw("HIST SAME");
    hM_reco_selected_PU_up->Draw("HIST SAME");


    hM_reco_selected_PU_nom->GetXaxis()->SetTitleOffset(1.4);
    hM_reco_selected_PU_nom->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);

    TLegend *leg = new TLegend(0.62, 0.52, 0.88, 0.68);
    leg->AddEntry(hM_reco_selected_PU_nom, "PU nominal", "l");
    leg->AddEntry(hM_reco_selected_PU_up,  "PU up", "l");
    leg->AddEntry(hM_reco_selected_PU_down,"PU down", "l");
    leg->SetBorderSize(0);
    leg->Draw();

    c10->SaveAs("Invariant_Mass_reco_selected_PU_compare.png");

    //===============
    //PU comparison
    //===============

    TCanvas *cPUcomp = new TCanvas("cPUcomp", "PU comparison", 700, 700);

    // Pads
    TPad *pad1 = new TPad("pad1", "top pad", 0.0, 0.30, 1.0, 1.0);
    TPad *pad2 = new TPad("pad2", "bottom pad", 0.0, 0.00, 1.0, 0.30);

    pad1->SetBottomMargin(0.02);
    pad1->SetLeftMargin(0.12);
    pad1->SetRightMargin(0.05);

    pad2->SetTopMargin(0.03);
    pad2->SetBottomMargin(0.30);
    pad2->SetLeftMargin(0.12);
    pad2->SetRightMargin(0.05);

    pad1->Draw();
    pad2->Draw();


    // Top pad
    pad1->cd();
    gStyle->SetOptStat(0);
    TH1D *hData_plot = (TH1D*)hPU_nom_data->Clone("hData_plot");
    TH1D *hMC_plot   = (TH1D*)hPU_MC->Clone("hMC_plot");

    hData_plot->SetTitle("");
    hData_plot->SetLineColor(kBlack);
    hData_plot->SetMarkerColor(kBlack);
    hData_plot->SetMarkerStyle(20);
    hData_plot->SetMarkerSize(0.9);
    hData_plot->SetLineWidth(2);
    hData_plot->SetMinimum(0.0);

    hMC_plot->SetLineColor(kCyan+2);
    hMC_plot->SetMarkerColor(kCyan+2);
    hMC_plot->SetFillColor(kCyan+2);
    hMC_plot->SetFillStyle(3001);
    hMC_plot->SetLineWidth(2);

    hData_plot->GetYaxis()->SetTitle("entries / 1.0");
    hData_plot->GetYaxis()->SetTitleSize(0.05);
    hData_plot->GetYaxis()->SetLabelSize(0.035);
    hData_plot->GetYaxis()->SetTitleOffset(1.1);

    hData_plot->GetXaxis()->SetLabelSize(0); // hide x labels on top pad

    double ymax = std::max(hData_plot->GetMaximum(), hMC_plot->GetMaximum());
    hData_plot->SetMaximum(1.15 * ymax);

    hData_plot->Draw("E1");
    hMC_plot->Draw("HIST SAME");
    hData_plot->Draw("E1 SAME");

    TLegend *legPU = new TLegend(0.62, 0.58, 0.88, 0.82);
    legPU->AddEntry(hData_plot, "data pileup", "lep");
    legPU->AddEntry(hMC_plot, "MC Pileup_nTrueInt", "f");
    legPU->SetBorderSize(1);
    legPU->SetFillStyle(0);
    legPU->Draw();

    CMS_label(0.15, 0.87);


    // Bottom pad
    pad2->cd();
    gStyle->SetOptStat(0);
    TH1D *hRatio = (TH1D*)hData_plot->Clone("hRatio");
    hRatio->Divide(hMC_plot);
    hRatio->SetTitle("");
    hRatio->SetLineColor(kBlack);
    hRatio->SetMarkerColor(kBlack);
    hRatio->SetMarkerStyle(20);
    hRatio->SetMarkerSize(0.8);

    hRatio->GetYaxis()->SetTitle("Data / MC");
    hRatio->GetXaxis()->SetTitle("pileup");

    hRatio->GetYaxis()->SetNdivisions(505);
    hRatio->GetYaxis()->SetTitleSize(0.10);
    hRatio->GetYaxis()->SetLabelSize(0.08);
    hRatio->GetYaxis()->SetTitleOffset(0.5);

    hRatio->GetXaxis()->SetTitleSize(0.12);
    hRatio->GetXaxis()->SetLabelSize(0.10);
    hRatio->GetXaxis()->SetTitleOffset(1.0);

    hRatio->SetMinimum(0.0);
    hRatio->SetMaximum(2.0);

    hRatio->Draw("E1");

    TLine *line1 = new TLine(hRatio->GetXaxis()->GetXmin(), 1.0,
            hRatio->GetXaxis()->GetXmax(), 1.0);
    line1->SetLineStyle(2);
    line1->Draw();

    cPUcomp->SaveAs("Pileup_Data_vs_MC_ratio.png");


    TFile *fOut = new TFile("CstarToGJ.root", "RECREATE");
    h_M_cstar->Write();
    hM_gen->Write();
    hM_reco_selected ->Write();
    hM_reco_selected_PU_nom->Write("sig_puNominal");   
    hM_reco_selected_PU_up->Write("sig_puUp");
    hM_reco_selected_PU_down->Write("sig_puDown");
    hPhoton_pt->Write();
    hJet_pt->Write();
    fOut->Close();
    delete fOut;

}//void
