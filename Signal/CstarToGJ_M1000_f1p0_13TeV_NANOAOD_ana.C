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
#include "/eos/user/h/hsiaoche/Signal/uncertainty_sources/jerc-application-tutorial/JecApplication.h"
#include <algorithm>
#include "correction.h"
#include <TFormula.h>

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

    for (UInt_t i = 0; i < (UInt_t)jets.size(); ++i) {
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
    TH1F *h_M_cstar = new TH1F("h_m_cstar", "Mass of C*; M_{C*} [GeV]; Events", 100, 500, 3000);
    TH1F *hM_reco_selected = new TH1F("hM_reco_selected", "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000.);

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
    TH1D *h_sig_JESUp  = new TH1D("h_sig_JESUp",  "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_JESDown= new TH1D("h_sig_JESDown","RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *hPU_MC = new TH1D("hPU_MC", "MC PU;True interactions;Events", 100, 0, 100);

    TH1D *h_sig_PERUp  = new TH1D("h_sig_PERUp",  "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_PERDown= new TH1D("h_sig_PERDown","RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_PESUp  = new TH1D("h_sig_PESUp",  "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_PESDown= new TH1D("h_sig_PESDown","RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);

    TH1D *h_sig_CTagUp   = new TH1D("h_sig_CTagUp","RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);

    TH1D *h_sig_CTagDown = new TH1D("h_sig_CTagDown","RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);

    //}}}


    TFile *fPU = TFile::Open("DataPileupHistograms.root");
    TH1D *hPU_nom_data  = (TH1D*)fPU->Get("pu_nominal");
    TH1D *hPU_up_data   = (TH1D*)fPU->Get("pu_up");
    TH1D *hPU_down_data = (TH1D*)fPU->Get("pu_down");



    hM_gen ->Sumw2();
    hPhoton_pt->Sumw2();
    hJet_pt->Sumw2();
    hM_reco_selected->Sumw2();

    h_sig->Sumw2();
    h_sig_PUUp->Sumw2();
    h_sig_PUDown->Sumw2();
    h_sig_JERUp->Sumw2();
    h_sig_JERDown->Sumw2();
    h_sig_JESUp->Sumw2();
    h_sig_JESDown->Sumw2();
    h_sig_CTagUp->Sumw2();
    h_sig_CTagDown->Sumw2();
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

    paths.ak4 ="/eos/user/h/hsiaoche/Signal/uncertainty_sources/jerc-application-tutorial/JecConfigAK4.json";
    paths.ak8 ="/eos/user/h/hsiaoche/Signal/uncertainty_sources/jerc-application-tutorial/JecConfigAK8.json";

    JecConfigReader::JecConfig cfg(paths);

    std::ofstream fout("jes_sources.txt");

    auto jesUncRefs = cfg.getJesUncSetsMcAK4Ref("2017");
    auto jesTotalRef = jesUncRefs.total.begin()->second;

    auto cs = correction::CorrectionSet::from_file(
            "/eos/user/h/hsiaoche/Signal/uncertainty_sources/EGM_ScaleUnc.json.gz"
            );

    auto egmScale = cs->at("UL-EGM_ScaleUnc");

    // MC sample:
    auto jec = JecApplication::Applier::McAK4(cfg, "2017", false);

    JecApplication::SystematicOptions systNom;
    systNom.jerVar = "nom";

    JecApplication::SystematicOptions systJERUp;
    systJERUp.jerVar = "up";

    JecApplication::SystematicOptions systJERDown;
    systJERDown.jerVar = "down";

    auto cs_ctag = correction::CorrectionSet::from_file(
            "/cvmfs/cms-griddata.cern.ch/cat/metadata/BTV/Run2-2017-UL-NanoAODv9/latest/ctagging.json.gz"
            );
    auto ctagSF_corr = cs_ctag->at("deepJet_wp");

    Long64_t nbytes = 0, nb = 0;
    for (Long64_t jentry=0; jentry<nentries;jentry++) {
        Long64_t ientry = LoadTree(jentry);
        if (ientry < 0) break;
        nb = fChain->GetEntry(jentry);   nbytes += nb;
        // if (Cut(ientry) < 0) continue;

        UInt_t bin = hPU_nom->FindBin(Pileup_nTrueInt);
        UInt_t nbins = (UInt_t)hPU_nom->GetNbinsX();
        bin = std::clamp((UInt_t)bin, (UInt_t)1, nbins);

        // pileup reweighting factors
        double w_PU_nom  = hPU_nom->GetBinContent(bin);
        double w_PU_up   = hPU_up->GetBinContent(bin);
        double w_PU_down = hPU_down->GetBinContent(bin);

        double weight_raw = lumi_pb * xsec * genWeight / sum_genWeight;

        // Central weight should already include nominal PU
        double weight_central = weight_raw * w_PU_nom;
        double weight_PUUp    = weight_raw * w_PU_up;
        double weight_PUDown  = weight_raw * w_PU_down;

        if (jentry < 20){
            std::cout << "Event " << jentry << " | weight up = " << weight_PUUp << std::endl;
            std::cout << "Event " << jentry << " | weight down = " << weight_PUDown << std::endl;
        }

        if (nPhoton < 1 || nJet < 1 || nGenPart <= 0 || nGenJet <= 0)   continue;

        // ***** Gen *****

        int cstarIdx = -1;
        int genPhotonIdx = -1;
        int genCharmIdx = -1;

        std::vector<UInt_t> cstar; //contains indices of all generated c* particles
        for (UInt_t i= 0; i< nGenPart; ++i){
            if (GenPart_pdgId[i] == 4000004){
                cstar.push_back(i);
            }

        }//for (UInt_t i =0; i< nGenPart; ++i)
        if (cstar.size() == 0) continue;

        for (UInt_t idx : cstar){
            int photon = -1;
            int charm = -1;

            for (UInt_t i= 0; i< nGenPart; ++i){
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

        }//for (UInt_t idx : cstar)
        if (cstarIdx < 0) continue;

        // Cstar mass
        TLorentzVector cstar_p4;
        cstar_p4.SetPtEtaPhiM(GenPart_pt[cstarIdx], GenPart_eta[cstarIdx], GenPart_phi[cstarIdx], GenPart_mass[cstarIdx]);
        h_M_cstar->Fill(cstar_p4.M(), weight_central);

        //Match Gen charm GenJet
        TLorentzVector c_p4, GenJet_p4;

        int genJetIdx = -1;
        float best_deltaR_cJet = 999;
        for (UInt_t j= 0; j< nGenJet; ++j){


            GenJet_p4.SetPtEtaPhiM(GenJet_pt[j], GenJet_eta[j], GenJet_phi[j], GenJet_mass[j]);
            c_p4.SetPtEtaPhiM(GenPart_pt[genCharmIdx], GenPart_eta[genCharmIdx], GenPart_phi[genCharmIdx], GenPart_mass[genCharmIdx]);

            float deltaR_cJet = c_p4.DeltaR(GenJet_p4);
            if (deltaR_cJet < best_deltaR_cJet){
                best_deltaR_cJet = deltaR_cJet;
                genJetIdx = j;
            }
        }//for (UInt_t j= 0; j< nGenJet; ++j)
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


        double pho_pt  = Photon_pt[goodPhotonIdx];
        double pho_eta = Photon_eta[goodPhotonIdx];
        double pho_phi = Photon_phi[goodPhotonIdx];
        double pho_m   = Photon_mass[goodPhotonIdx];

        // --- Get scale factors from JSON ---
        double scaleUp = egmScale->evaluate({
                "2017",
                "scaleup",
                pho_eta,
                12   // gain (use 12 if not available)
                });

        double scaleDown = egmScale->evaluate({
                "2017",
                "scaledown",
                pho_eta,
                12
                });

        // --- Build photons ---
        TLorentzVector g_nom, g_PESUp, g_PESDown;

        g_nom.SetPtEtaPhiM(pho_pt, pho_eta, pho_phi, pho_m);
        g_PESUp.SetPtEtaPhiM(pho_pt * scaleUp, pho_eta, pho_phi, pho_m);
        g_PESDown.SetPtEtaPhiM(pho_pt * scaleDown, pho_eta, pho_phi, pho_m);


        // -----------------------------
        // Jet selection:
        // central, JER up, JER down
        // -----------------------------

        std::vector<TLorentzVector> goodJetP4s_nom;
        std::vector<TLorentzVector> goodJetP4s_JERUp;
        std::vector<TLorentzVector> goodJetP4s_JERDown;
        std::vector<TLorentzVector> goodJetP4s_JESUp;
        std::vector<TLorentzVector> goodJetP4s_JESDown;

        std::vector<int> goodJetOriginalIdx;

        for (UInt_t i= 0; i < nJet; ++i){

            // ***** JES nominal *****
            JecApplication::JesInputs jin;
            jin.pt        = Jet_pt[i];
            jin.eta       = Jet_eta[i];
            jin.phi       = Jet_phi[i];
            jin.area      = Jet_area[i];
            jin.rho       = fixedGridRhoFastjetAll;
            jin.rawFactor = Jet_rawFactor[i];

            double jesNom = jec.jesFactorNominal(jin);

            // Rebuild raw jet
            double rawPt   = Jet_pt[i]   * (1.0 - Jet_rawFactor[i]);
            double rawMass = Jet_mass[i] * (1.0 - Jet_rawFactor[i]);

            double ptAfterJes   = rawPt   * jesNom;
            double massAfterJes = rawMass * jesNom;

            double jesUncUp = JecApplication::Applier::jesComponentSyst(
                    jesTotalRef,
                    "Up",
                    Jet_eta[i],
                    ptAfterJes,
                    false
                    );

            double jesUncDown = JecApplication::Applier::jesComponentSyst(
                    jesTotalRef,
                    "Down",
                    Jet_eta[i],
                    ptAfterJes,
                    false
                    );

            double ptAfterJesUp     = ptAfterJes   * jesUncUp;
            double massAfterJesUp   = massAfterJes * jesUncUp;

            double ptAfterJesDown   = ptAfterJes   * jesUncDown;
            double massAfterJesDown = massAfterJes * jesUncDown;

            // ***** Hybrid JER matching *****
            JecApplication::JerInputs jrin;
            jrin.hasGen = false;      
            jrin.event  = jentry;
            jrin.maxDr  = 0.2;

            double bestDr = 999.0;
            int matchedGenJetIdx = -1;

            for (UInt_t ig = 0; ig < nGenJet; ++ig) {
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

            JecApplication::JesInputs jinAfterJes;
            jinAfterJes.pt        = ptAfterJes;
            jinAfterJes.eta       = Jet_eta[i];
            jinAfterJes.phi       = Jet_phi[i];
            jinAfterJes.area      = Jet_area[i];
            jinAfterJes.rho       = fixedGridRhoFastjetAll;
            jinAfterJes.rawFactor = 0.0;

            JecApplication::JesInputs jinAfterJesUp;
            jinAfterJesUp.pt        = ptAfterJesUp;
            jinAfterJesUp.eta       = Jet_eta[i];
            jinAfterJesUp.phi       = Jet_phi[i];
            jinAfterJesUp.area      = Jet_area[i];
            jinAfterJesUp.rho       = fixedGridRhoFastjetAll;
            jinAfterJesUp.rawFactor = 0.0;

            JecApplication::JesInputs jinAfterJesDown;
            jinAfterJesDown.pt        = ptAfterJesDown;
            jinAfterJesDown.eta       = Jet_eta[i];
            jinAfterJesDown.phi       = Jet_phi[i];
            jinAfterJesDown.area      = Jet_area[i];
            jinAfterJesDown.rho       = fixedGridRhoFastjetAll;
            jinAfterJesDown.rawFactor = 0.0;

            double jerForJESUp   = jec.jerFactor(jinAfterJesUp,   jrin, systNom);
            double jerForJESDown = jec.jerFactor(jinAfterJesDown, jrin, systNom);

            double ptJESUp   = ptAfterJesUp   * jerForJESUp;
            double massJESUp = massAfterJesUp * jerForJESUp;

            double ptJESDown   = ptAfterJesDown   * jerForJESDown;
            double massJESDown = massAfterJesDown * jerForJESDown;

            double jerNom = jec.jerFactor(jinAfterJes, jrin, systNom);
            double jerUp  = jec.jerFactor(jinAfterJes, jrin, systJERUp);
            double jerDown= jec.jerFactor(jinAfterJes, jrin, systJERDown);

            double ptNom   = ptAfterJes * jerNom;
            double massNom = massAfterJes * jerNom;

            double ptJERUp   = ptAfterJes * jerUp;
            double massJERUp = massAfterJes * jerUp;

            double ptJERDown   = ptAfterJes * jerDown;
            double massJERDown = massAfterJes * jerDown;


            if (ptNom < 170) continue;
            if (fabs(Jet_eta[i]) >= 2.4) continue;
            if (Jet_jetId[i] < 6) continue;
            if (Jet_btagDeepFlavCvB[i] < 0.340) continue;
            if (Jet_btagDeepFlavCvL[i] < 0.085) continue;

            TLorentzVector j_raw;
            j_raw.SetPtEtaPhiM(Jet_pt[i], Jet_eta[i], Jet_phi[i], Jet_mass[i]);

            TLorentzVector j_nom, j_JERUp, j_JERDown,j_JESUp, j_JESDown;

            j_nom.SetPtEtaPhiM(ptNom, Jet_eta[i], Jet_phi[i], massNom);
            j_JERUp.SetPtEtaPhiM(ptJERUp, Jet_eta[i], Jet_phi[i], massJERUp);
            j_JERDown.SetPtEtaPhiM(ptJERDown, Jet_eta[i], Jet_phi[i], massJERDown);
            j_JESUp.SetPtEtaPhiM(ptJESUp, Jet_eta[i], Jet_phi[i], massJESUp);
            j_JESDown.SetPtEtaPhiM(ptJESDown, Jet_eta[i], Jet_phi[i], massJESDown);

            if (g_nom.DeltaR(j_raw) <= 0.4) continue;
            if (g_nom.DeltaR(j_nom) <= 0.4) continue;

            goodJetP4s_nom.push_back(j_nom);
            goodJetP4s_JERUp.push_back(j_JERUp);
            goodJetP4s_JERDown.push_back(j_JERDown);
            goodJetP4s_JESUp.push_back(j_JESUp);
            goodJetP4s_JESDown.push_back(j_JESDown);

            goodJetOriginalIdx.push_back(i);
        }

        if (goodJetP4s_nom.size() == 0) continue;

        int goodJetVecIdx = GetLeadingJetIndex(goodJetP4s_nom);
        int selectedJetIdx = goodJetOriginalIdx[goodJetVecIdx];
        if (goodJetVecIdx < 0) continue;


        TLorentzVector reco_j_nom     = goodJetP4s_nom[goodJetVecIdx];
        TLorentzVector reco_j_JERUp   = goodJetP4s_JERUp[goodJetVecIdx];
        TLorentzVector reco_j_JERDown = goodJetP4s_JERDown[goodJetVecIdx];
        TLorentzVector reco_j_JESUp   = goodJetP4s_JESUp[goodJetVecIdx];
        TLorentzVector reco_j_JESDown = goodJetP4s_JESDown[goodJetVecIdx];


        double ctagSF_nom = 1.0, ctagSF_up = 1.0, ctagSF_down = 1.0;
        int hadFlav = (int)Jet_hadronFlavour[selectedJetIdx];
        if (hadFlav == 4) {
            ctagSF_nom = ctagSF_corr->evaluate({
                    std::string("central"), std::string("wcharm"), std::string("M"),
                    4, std::abs(Jet_eta[selectedJetIdx]), reco_j_nom.Pt()
                    });
            ctagSF_up = ctagSF_corr->evaluate({
                    std::string("up"), std::string("wcharm"), std::string("M"),
                    4, std::abs(Jet_eta[selectedJetIdx]), reco_j_nom.Pt()
                    });
            ctagSF_down = ctagSF_corr->evaluate({
                    std::string("down"), std::string("wcharm"), std::string("M"),
                    4, std::abs(Jet_eta[selectedJetIdx]), reco_j_nom.Pt()
                    });
        }

        // temporary debug — remove after confirming
        if (jentry < 50 && hadFlav != 4) {
            std::cout << "Event " << jentry
                << " | non-c-jet slipped through! hadFlav=" << hadFlav
                << " | SF set to 1.0" << std::endl;
        }

        // ---- ADD THIS ----
        if (jentry < 50) {
            std::cout << "Event " << jentry
                << " | hadronFlavour=" << Jet_hadronFlavour[selectedJetIdx]
                << " | pt="            << reco_j_nom.Pt()
                << " | eta="           << Jet_eta[selectedJetIdx]
                << " | SF_nom="        << ctagSF_nom
                << " | SF_up="         << ctagSF_up
                << " | SF_down="       << ctagSF_down
                << std::endl;
        }
        // ------------------

        double weight_CTag_nom  = weight_central * ctagSF_nom;
        double weight_CTagUp    = weight_central * ctagSF_up;
        double weight_CTagDown  = weight_central * ctagSF_down;

        double m_JERUp   = (g_nom + reco_j_JERUp).M();
        double m_JERDown = (g_nom + reco_j_JERDown).M();
        double m_JESUp   = (g_nom + reco_j_JESUp).M();
        double m_JESDown = (g_nom + reco_j_JESDown).M();
        double m_nom     = (g_nom     + reco_j_nom).M();
        double m_PESUp   = (g_PESUp   + reco_j_nom).M();
        double m_PESDown = (g_PESDown + reco_j_nom).M();

        hM_reco_selected->Fill(m_nom, weight_raw);

        hPhoton_pt->Fill(Photon_pt[goodPhotonIdx], weight_central);
        hJet_pt->Fill(reco_j_nom.Pt(), weight_central);

        h_sig->Fill(m_nom, weight_CTag_nom);

        h_sig_PUUp->Fill(m_nom, weight_PUUp * ctagSF_nom);
        h_sig_PUDown->Fill(m_nom, weight_PUDown * ctagSF_nom);

        h_sig_JERUp->Fill(m_JERUp, weight_CTag_nom);
        h_sig_JERDown->Fill(m_JERDown, weight_CTag_nom);

        h_sig_JESUp->Fill(m_JESUp, weight_CTag_nom);
        h_sig_JESDown->Fill(m_JESDown, weight_CTag_nom);

        h_sig_PESUp->Fill(m_PESUp, weight_CTag_nom);
        h_sig_PESDown->Fill(m_PESDown, weight_CTag_nom);

        h_sig_CTagUp->Fill(m_nom, weight_CTagUp);
        h_sig_CTagDown->Fill(m_nom, weight_CTagDown);

    }//jentry

    SetCMSStyle();

    //weighted_yield_summary
    //{{{

    // =============================
    // Weighted yield summary output
    // =============================
    std::ofstream yieldOut("weighted_yield_summary.txt");

    if (!yieldOut.is_open()) {
        std::cerr << "ERROR: cannot open weighted_yield_summary.txt" << std::endl;
    } else {

        double y_nom    = h_sig->Integral();
        double y_PUUp   = h_sig_PUUp->Integral();
        double y_PUDown = h_sig_PUDown->Integral();

        double y_CTagUp   = h_sig_CTagUp->Integral();
        double y_CTagDown = h_sig_CTagDown->Integral();

        yieldOut << "===== PU weighted yield check =====\n\n";

        yieldOut << "Nominal integral = " << y_nom << "\n";
        yieldOut << "PUUp integral    = " << y_PUUp << "\n";
        yieldOut << "PUDown integral  = " << y_PUDown << "\n\n";

        yieldOut << "PUUp / Nominal   = " << y_PUUp / y_nom << "\n";
        yieldOut << "PUDown / Nominal = " << y_PUDown / y_nom << "\n\n";

        yieldOut << "PUUp change (%)   = "
            << 100.0 * (y_PUUp / y_nom - 1.0) << " %\n";

        yieldOut << "PUDown change (%) = "
            << 100.0 * (y_PUDown / y_nom - 1.0) << " %\n\n";


        yieldOut << "===== CTag weighted yield check =====\n\n";

        yieldOut << "Nominal integral  = " << y_nom << "\n";
        yieldOut << "CTagUp integral   = " << y_CTagUp << "\n";
        yieldOut << "CTagDown integral = " << y_CTagDown << "\n\n";

        yieldOut << "CTagUp / Nominal   = " << y_CTagUp / y_nom << "\n";
        yieldOut << "CTagDown / Nominal = " << y_CTagDown / y_nom << "\n\n";

        yieldOut << "CTagUp change (%)   = "
            << 100.0 * (y_CTagUp / y_nom - 1.0) << " %\n";

        yieldOut << "CTagDown change (%) = "
            << 100.0 * (y_CTagDown / y_nom - 1.0) << " %\n";

        yieldOut.close();

        std::cout << "Saved weighted yield summary to weighted_yield_summary.txt" << std::endl;
    }

    //}}}

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
    h_sig->Draw("HIST");
    h_sig->GetXaxis()->SetTitleOffset(1.4);
    h_sig->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c7->SaveAs("Invariant_Mass_PU_nom.png");

    TCanvas *c8 = new TCanvas("c8", "Invariant Mass Reco PU Up", 600, 400);
    h_sig_PUUp->Draw("HIST");
    h_sig_PUUp->GetXaxis()->SetTitleOffset(1.4);
    h_sig_PUUp->GetXaxis()->SetLabelOffset(0.02);
    h_sig_PUUp->SetStats(0);
    CMS_label(0.18, 0.87);
    c8->SaveAs("Invariant_Mass_PU_up.png");

    TCanvas *c9 = new TCanvas("c9", "Invariant Mass Reco PU Down", 600, 400);
    h_sig_PUDown->Draw("HIST");
    h_sig_PUDown->GetXaxis()->SetTitleOffset(1.4);
    h_sig_PUDown->GetXaxis()->SetLabelOffset(0.02);
    h_sig_PUDown->SetStats(0);
    CMS_label(0.18, 0.87);
    c9->SaveAs("Invariant_Mass_PU_down.png");

    TCanvas *c10 = new TCanvas("c8", "Invariant Mass Reco JES Up", 600, 400);
    h_sig_JESUp->Draw("HIST");
    h_sig_JESUp->GetXaxis()->SetTitleOffset(1.4);
    h_sig_JESUp->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c10->SaveAs("Invariant_Mass_JES_up.png");

    TCanvas *c11 = new TCanvas("c9", "Invariant Mass Reco JES Down", 600, 400);
    h_sig_JESDown->Draw("HIST");
    h_sig_JESDown->GetXaxis()->SetTitleOffset(1.4);
    h_sig_JESDown->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c11->SaveAs("Invariant_Mass_JES_down.png");

    TCanvas *c12 = new TCanvas("c8", "Invariant Mass Reco JER Up", 600, 400);
    h_sig_JERUp->Draw("HIST");
    h_sig_JERUp->GetXaxis()->SetTitleOffset(1.4);
    h_sig_JERUp->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c12->SaveAs("Invariant_Mass_JER_up.png");

    TCanvas *c13 = new TCanvas("c9", "Invariant Mass Reco JER Down", 600, 400);
    h_sig_JERDown->Draw("HIST");
    h_sig_JERDown->GetXaxis()->SetTitleOffset(1.4);
    h_sig_JERDown->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c13->SaveAs("Invariant_Mass_JER_down.png");

    TCanvas *c14 = new TCanvas("c8", "Invariant Mass Reco PES Up", 600, 400);
    h_sig_PESUp->Draw("HIST");
    h_sig_PESUp->GetXaxis()->SetTitleOffset(1.4);
    h_sig_PESUp->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c14->SaveAs("Invariant_Mass_PES_up.png");

    TCanvas *c15 = new TCanvas("c9", "Invariant Mass Reco PES Down", 600, 400);
    h_sig_PESDown->Draw("HIST");
    h_sig_PESDown->GetXaxis()->SetTitleOffset(1.4);
    h_sig_PESDown->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c15->SaveAs("Invariant_Mass_PES_down.png");

    TCanvas *c16 = new TCanvas("c16", "Invariant Mass Reco CTag Up", 600, 400);
    h_sig_CTagUp->Draw("HIST");
    h_sig_CTagUp->GetXaxis()->SetTitleOffset(1.4);
    h_sig_CTagUp->GetXaxis()->SetLabelOffset(0.02);
    h_sig_CTagUp->SetStats(0);
    CMS_label(0.18, 0.87);
    c16->SaveAs("Invariant_Mass_CTag_up.png");

    TCanvas *c17 = new TCanvas("c17", "Invariant Mass Reco CTag Down", 600, 400);
    h_sig_CTagDown->Draw("HIST");
    h_sig_CTagDown->GetXaxis()->SetTitleOffset(1.4);
    h_sig_CTagDown->GetXaxis()->SetLabelOffset(0.02);
    h_sig_CTagDown->SetStats(0);
    CMS_label(0.18, 0.87);
    c17->SaveAs("Invariant_Mass_CTag_down.png");

    //PU JER JES C-tag comparison plots
    //{{{

    TCanvas *c20 = new TCanvas("c10", "PU comparison", 600, 400);

    h_sig_PUDown->SetLineColor(kBlue);
    h_sig_PUDown->SetLineWidth(1);

    h_sig->SetLineColor(kBlack);
    h_sig->SetLineWidth(1);

    h_sig_PUUp->SetLineColor(kRed);
    h_sig_PUUp->SetLineWidth(1);

    h_sig_PUDown->Draw("HIST");
    h_sig->Draw("HIST SAME");
    h_sig_PUUp->Draw("HIST SAME");


    h_sig->GetXaxis()->SetTitleOffset(1.4);
    h_sig->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);

    TLegend *leg = new TLegend(0.62, 0.52, 0.88, 0.68);
    leg->AddEntry(h_sig, "PU nominal", "l");
    leg->AddEntry(h_sig_PUUp,  "PU up", "l");
    leg->AddEntry(h_sig_PUDown,"PU down", "l");
    leg->SetBorderSize(0);
    leg->Draw();

    c20->SaveAs("Invariant_Mass_PU_compare.png");

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

    // =====================
    // JER comparison
    // =====================
    TCanvas *cJER = new TCanvas("cJER", "JER comparison", 600, 400);

    h_sig_JERDown->SetLineColor(kBlue);
    h_sig_JERDown->SetLineWidth(1);

    h_sig->SetLineColor(kBlack);
    h_sig->SetLineWidth(1);

    h_sig_JERUp->SetLineColor(kRed);
    h_sig_JERUp->SetLineWidth(1);

    h_sig_JERDown->Draw("HIST");
    h_sig->Draw("HIST SAME");
    h_sig_JERUp->Draw("HIST SAME");

    TLegend *legJER = new TLegend(0.62,0.52,0.88,0.68);
    legJER->AddEntry(h_sig,"Nominal","l");
    legJER->AddEntry(h_sig_JERUp,"JER Up","l");
    legJER->AddEntry(h_sig_JERDown,"JER Down","l");
    legJER->SetBorderSize(0);
    legJER->Draw();

    CMS_label(0.18,0.87);
    cJER->SaveAs("Invariant_Mass_JER_compare.png");


    // =====================
    // JES comparison
    // =====================
    TCanvas *cJES = new TCanvas("cJES", "JES comparison", 600, 400);

    h_sig_JESDown->SetLineColor(kBlue);
    h_sig_JESDown->SetLineWidth(1);

    h_sig->SetLineColor(kBlack);
    h_sig->SetLineWidth(1);

    h_sig_JESUp->SetLineColor(kRed);
    h_sig_JESUp->SetLineWidth(1);

    h_sig_JESDown->Draw("HIST");
    h_sig->Draw("HIST SAME");
    h_sig_JESUp->Draw("HIST SAME");

    TLegend *legJES = new TLegend(0.62,0.52,0.88,0.68);
    legJES->AddEntry(h_sig,"Nominal","l");
    legJES->AddEntry(h_sig_JESUp,"JES Up","l");
    legJES->AddEntry(h_sig_JESDown,"JES Down","l");
    legJES->SetBorderSize(0);
    legJES->Draw();

    CMS_label(0.18,0.87);
    cJES->SaveAs("Invariant_Mass_JES_compare.png");

    // =====================
    // JC-tag comparison
    // =====================
    TCanvas *cCTag = new TCanvas("cCTag", "CTag comparison", 600, 400);

    h_sig_CTagDown->SetLineColor(kBlue);
    h_sig->SetLineColor(kBlack);
    h_sig_CTagUp->SetLineColor(kRed);

    h_sig_CTagDown->Draw("HIST");
    h_sig->Draw("HIST SAME");
    h_sig_CTagUp->Draw("HIST SAME");

    TLegend *legCTag = new TLegend(0.62,0.52,0.88,0.68);
    legCTag->AddEntry(h_sig,"Nominal","l");
    legCTag->AddEntry(h_sig_CTagUp,"CTag Up","l");
    legCTag->AddEntry(h_sig_CTagDown,"CTag Down","l");
    legCTag->SetBorderSize(0);
    legCTag->Draw();

    CMS_label(0.18,0.87);
    cCTag->SaveAs("Invariant_Mass_CTag_compare.png");

    //}}}

    TFile *fOut = new TFile("CstarToGJ.root", "RECREATE");
    h_M_cstar->Write();
    hM_gen->Write();
    hM_reco_selected ->Write();
    hPhoton_pt->Write();
    hJet_pt->Write();
    h_sig->Write("sig");
    h_sig_PUUp->Write("sig_PUUp");
    h_sig_PUDown->Write("sig_PUDown");
    h_sig_JERUp->Write("sig_JERUp");
    h_sig_JERDown->Write("sig_JERDown");
    h_sig_JESUp->Write("sig_JESUp");
    h_sig_JESDown->Write("sig_JESDown");
    h_sig_PESUp->Write("sig_PESUp");
    h_sig_PESDown->Write("sig_PESDown");
    h_sig_CTagUp->Write("sig_CTagUp");
    h_sig_CTagDown->Write("sig_CTagDown");
    fOut->Close();
    delete fOut;

}//void
