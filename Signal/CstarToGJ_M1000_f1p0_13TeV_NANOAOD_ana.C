#define CstarToGJ_M1000_f1p0_13TeV_NANOAOD_ana_cxx
#include "CstarToGJ_M1000_f1p0_13TeV_NANOAOD_ana.h"

#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TLorentzVector.h>
#include <TLatex.h>
#include <TPaveText.h>
#include <TLegend.h>
#include <TLine.h>
#include <TRandom3.h>
#include <TROOT.h>
#include <TFile.h>

#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iostream>

// ======================================================
// Style
// ======================================================

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
    gStyle->SetStatX(0.92);
    gStyle->SetStatY(0.90);
    gStyle->SetStatW(0.20);
    gStyle->SetStatH(0.15);
}

void CMS_label(double x = 0.08, double y = 0.88,
               const char *text = "Preliminary",
               double lumi_fb = 41.8, double sqrts = 13.0)
{
    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.045);
    latex.SetTextFont(62);
    latex.DrawLatex(x, y, "CMS");

    latex.SetTextFont(52);
    latex.DrawLatex(x + 0.06, y, text);

    latex.SetTextFont(42);
    TString lumiText = Form("%.1f fb^{-1} (%g TeV)", lumi_fb, sqrts);
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.75, 0.95, lumiText);
}

double lumi_pb = 41800.0;
double xsec = 1.307;

// ======================================================
// JER tables
// ======================================================

struct JERSFEntry {
    double etaMin, etaMax;
    double sfNom, sfDown, sfUp;
};

struct JERResEntry {
    double etaMin, etaMax;
    double rhoMin, rhoMax;
    int nPar;
    double ptMin, ptMax;
    double p0, p1, p2, p3;
};

static std::vector<JERSFEntry> gJerSFTable;
static std::vector<JERResEntry> gJerResTable;

// ======================================================
// Helpers
// ======================================================

bool startsWith(const std::string &s, const std::string &prefix) {
    return s.rfind(prefix, 0) == 0;
}

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

bool loadJERSFFile(const std::string &filename)
{
    std::ifstream fin(filename.c_str());
    if (!fin.is_open()) {
        std::cerr << "ERROR: cannot open JER SF file: " << filename << std::endl;
        return false;
    }

    gJerSFTable.clear();

    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        if (startsWith(line, "#")) continue;
        if (startsWith(line, "{")) continue;
        if (startsWith(line, "}")) continue;

        std::stringstream ss(line);

        // expected format from your screenshot:
        // etaMin etaMax 3 sfNom sfDown sfUp
        double etaMin, etaMax;
        int nPar;
        double sfNom, sfDown, sfUp;

        if (!(ss >> etaMin >> etaMax >> nPar >> sfNom >> sfDown >> sfUp)) continue;

        JERSFEntry e;
        e.etaMin = etaMin;
        e.etaMax = etaMax;
        e.sfNom = sfNom;
        e.sfDown = sfDown;
        e.sfUp = sfUp;
        gJerSFTable.push_back(e);
    }

    std::cout << "Loaded " << gJerSFTable.size()
              << " JER SF bins from " << filename << std::endl;

    return !gJerSFTable.empty();
}

bool loadJERResolutionFile(const std::string &filename)
{
    std::ifstream fin(filename.c_str());
    if (!fin.is_open()) {
        std::cerr << "ERROR: cannot open JER resolution file: " << filename << std::endl;
        return false;
    }

    gJerResTable.clear();

    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        if (startsWith(line, "#")) continue;
        if (startsWith(line, "{")) continue;
        if (startsWith(line, "}")) continue;

        std::stringstream ss(line);

        // expected format from your screenshot:
        // etaMin etaMax rhoMin rhoMax nPar ptMin ptMax p0 p1 p2 p3
        double etaMin, etaMax, rhoMin, rhoMax;
        int nPar;
        double ptMin, ptMax, p0, p1, p2, p3;

        if (!(ss >> etaMin >> etaMax >> rhoMin >> rhoMax
                 >> nPar >> ptMin >> ptMax >> p0 >> p1 >> p2 >> p3)) continue;

        JERResEntry e;
        e.etaMin = etaMin;
        e.etaMax = etaMax;
        e.rhoMin = rhoMin;
        e.rhoMax = rhoMax;
        e.nPar = nPar;
        e.ptMin = ptMin;
        e.ptMax = ptMax;
        e.p0 = p0;
        e.p1 = p1;
        e.p2 = p2;
        e.p3 = p3;

        gJerResTable.push_back(e);
    }

    std::cout << "Loaded " << gJerResTable.size()
              << " JER resolution bins from " << filename << std::endl;

    return !gJerResTable.empty();
}

double getJERSF(double eta, int variation)
{
    for (const auto &e : gJerSFTable) {
        if (eta >= e.etaMin && eta < e.etaMax) {
            if (variation == 0) return e.sfNom;
            if (variation > 0)  return e.sfUp;
            return e.sfDown;
        }
    }

    // inclusive upper edge protection
    for (const auto &e : gJerSFTable) {
        if (std::abs(eta - e.etaMax) < 1e-6) {
            if (variation == 0) return e.sfNom;
            if (variation > 0)  return e.sfUp;
            return e.sfDown;
        }
    }

    std::cerr << "WARNING: no JER SF found for eta = " << eta << std::endl;
    return 1.0;
}

double getJERResolution(double pt, double eta, double rho)
{
    // clamp to safe ranges
    double ptEval  = std::max(15.0, std::min(pt, 2999.0));
    double rhoEval = std::max(0.0, rho);

    for (const auto &e : gJerResTable) {
        bool etaOK = (eta >= e.etaMin && eta < e.etaMax) ||
                     (std::abs(eta - e.etaMax) < 1e-6);
        bool rhoOK = (rhoEval >= e.rhoMin && rhoEval < e.rhoMax) ||
                     (std::abs(rhoEval - e.rhoMax) < 1e-6);
        bool ptOK  = (ptEval >= e.ptMin && ptEval < e.ptMax) ||
                     (std::abs(ptEval - e.ptMax) < 1e-6);

        if (!etaOK || !rhoOK || !ptOK) continue;

        // from your screenshot:
        // sqrt([0]*abs([0])/(x*x)+[1]*[1]*pow(x,[3])+[2]*[2])
        // x = pt
        double term1 = e.p0 * std::abs(e.p0) / (ptEval * ptEval);
        double term2 = e.p1 * e.p1 * std::pow(ptEval, e.p3);
        double term3 = e.p2 * e.p2;

        double sigma = std::sqrt(std::max(term1 + term2 + term3, 0.0));
        return sigma; // relative resolution sigma_pt / pt
    }

    std::cerr << "WARNING: no JER resolution found for pt=" << pt
              << ", eta=" << eta << ", rho=" << rho << std::endl;
    return 0.1;
}

int findMatchedGenJet(int recoJetIdx,
                      int nGenJet,
                      const Float_t *GenJet_pt,
                      const Float_t *GenJet_eta,
                      const Float_t *GenJet_phi,
                      const Float_t *GenJet_mass,
                      const Float_t *Jet_pt,
                      const Float_t *Jet_eta,
                      const Float_t *Jet_phi,
                      const Float_t *Jet_mass,
                      double sigmaJER)
{
    TLorentzVector recoJet;
    recoJet.SetPtEtaPhiM(Jet_pt[recoJetIdx], Jet_eta[recoJetIdx],
                         Jet_phi[recoJetIdx], Jet_mass[recoJetIdx]);

    int bestIdx = -1;
    double bestDR = 999.0;

    for (int j = 0; j < nGenJet; ++j) {
        TLorentzVector genJet;
        genJet.SetPtEtaPhiM(GenJet_pt[j], GenJet_eta[j],
                            GenJet_phi[j], GenJet_mass[j]);

        double dR = recoJet.DeltaR(genJet);
        if (dR >= 0.2) continue; // AK4: R/2 = 0.2

        double dPt = std::fabs(recoJet.Pt() - genJet.Pt());
        if (dPt >= 3.0 * sigmaJER * recoJet.Pt()) continue;

        if (dR < bestDR) {
            bestDR = dR;
            bestIdx = j;
        }
    }

    return bestIdx;
}

// ======================================================
// Main loop
// ======================================================

void CstarToGJ_M1000_f1p0_13TeV_NANOAOD_ana::Loop()
{
    if (fChain == 0) return;

    // -----------------------------
    // Load official JER text files
    // -----------------------------
    const std::string jerSFFile  = "/eos/user/h/hsiaoche/Signal/uncertainty_sources/JRDatabase/textFiles/Summer19UL17_JRV2_MC/Summer19UL17_JRV2_MC_SF_AK4PFchs.txt";
    const std::string jerResFile = "/eos/user/h/hsiaoche/Signal/uncertainty_sources/JRDatabase/textFiles/Summer19UL17_JRV2_MC/Summer19UL17_JRV2_MC_PtResolution_AK4PFchs.txt";

    if (!loadJERSFFile(jerSFFile)) return;
    if (!loadJERResolutionFile(jerResFile)) return;

    TH1F *hM_gen   = new TH1F("hM_gen",   "GEN M(#gamma + jet);M^{GEN}_{#gamma j} (GeV);Events",   500, 0., 3000.);
    TH1F *h_M_cstar = new TH1F("h_m_cstar", "Mass of C*; M_{C*} [GeV]; Events", 100, 500, 3000);
    TH1F *hPhoton_pt = new TH1F("hPhoton_pT", "Photon p_{T};p_{T}^{photon} (GeV);Events", 500, 0., 1500.);
    TH1F *hJet_pt = new TH1F("hJet_pT", "Jet p_{T};p_{T}^{jet} (GeV);Events", 500, 0., 1500.);

    TH1D *h_sig        = new TH1D("h_sig",        "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_PUUp   = new TH1D("h_sig_PUUp",   "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_PUDown = new TH1D("h_sig_PUDown", "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_JERUp  = new TH1D("h_sig_JERUp",  "RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);
    TH1D *h_sig_JERDown= new TH1D("h_sig_JERDown","RECO M(#gamma + jet);M^{RECO}_{#gamma j} (GeV);Events", 500, 0., 3000);

    TH1D *hPU_MC = new TH1D("hPU_MC", "MC PU;True interactions;Events", 100, 0, 100);

    TFile *fPU = TFile::Open("DataPileupHistograms.root");
    if (!fPU || fPU->IsZombie()) {
        std::cerr << "ERROR: cannot open DataPileupHistograms.root" << std::endl;
        return;
    }

    TH1D *hPU_nom_data  = (TH1D*)fPU->Get("pu_nominal");
    TH1D *hPU_up_data   = (TH1D*)fPU->Get("pu_up");
    TH1D *hPU_down_data = (TH1D*)fPU->Get("pu_down");

    if (!hPU_nom_data || !hPU_up_data || !hPU_down_data) {
        std::cerr << "ERROR: missing pileup histograms in DataPileupHistograms.root" << std::endl;
        return;
    }

    hM_gen->Sumw2();
    h_M_cstar->Sumw2();
    hPhoton_pt->Sumw2();
    hJet_pt->Sumw2();

    h_sig->Sumw2();
    h_sig_PUUp->Sumw2();
    h_sig_PUDown->Sumw2();
    h_sig_JERUp->Sumw2();
    h_sig_JERDown->Sumw2();
    hPU_MC->Sumw2();

    double sum_genWeight = 0.0;
    Long64_t nentries = fChain->GetEntriesFast();

    // ======================================================
    // First loop: sum genWeight and build MC pileup dist
    // ======================================================
    for (Long64_t jentry = 0; jentry < nentries; jentry++) {
        LoadTree(jentry);
        fChain->GetEntry(jentry);
        sum_genWeight += genWeight;
        hPU_MC->Fill(Pileup_nTrueInt, genWeight);
    }

    std::cout << "Total genWeight sum = " << sum_genWeight << std::endl;

    if (hPU_MC->Integral() > 0) hPU_MC->Scale(1.0 / hPU_MC->Integral());
    if (hPU_nom_data->Integral() > 0) hPU_nom_data->Scale(1.0 / hPU_nom_data->Integral());
    if (hPU_up_data->Integral() > 0) hPU_up_data->Scale(1.0 / hPU_up_data->Integral());
    if (hPU_down_data->Integral() > 0) hPU_down_data->Scale(1.0 / hPU_down_data->Integral());

    TH1D *hPU_nom  = (TH1D*)hPU_nom_data->Clone("hPU_weight_nom");
    TH1D *hPU_up   = (TH1D*)hPU_up_data->Clone("hPU_weight_up");
    TH1D *hPU_down = (TH1D*)hPU_down_data->Clone("hPU_weight_down");

    hPU_nom->Divide(hPU_MC);
    hPU_up->Divide(hPU_MC);
    hPU_down->Divide(hPU_MC);

    // ======================================================
    // Main event loop
    // ======================================================
    Long64_t nbytes = 0, nb = 0;
    for (Long64_t jentry = 0; jentry < nentries; jentry++) {
        Long64_t ientry = LoadTree(jentry);
        if (ientry < 0) break;
        nb = fChain->GetEntry(jentry);
        nbytes += nb;

        int bin = hPU_nom->FindBin(Pileup_nTrueInt);
        bin = std::max(1, std::min(bin, hPU_nom->GetNbinsX()));

        double w_PU_nom  = hPU_nom->GetBinContent(bin);
        double w_PU_up   = hPU_up->GetBinContent(bin);
        double w_PU_down = hPU_down->GetBinContent(bin);

        double weight_raw = lumi_pb * xsec * genWeight / sum_genWeight;
        double weight_central = weight_raw * w_PU_nom;
        double weight_PUUp    = weight_raw * w_PU_up;
        double weight_PUDown  = weight_raw * w_PU_down;

        if (nPhoton < 1 || nJet < 1 || nGenPart <= 0 || nGenJet <= 0) continue;

        int cstarIdx = -1;
        int genPhotonIdx = -1;
        int genCharmIdx = -1;

        std::vector<int> cstar;
        for (int i = 0; i < nGenPart; ++i) {
            if (GenPart_pdgId[i] == 4000004) cstar.push_back(i);
        }
        if (cstar.empty()) continue;

        for (int idx : cstar) {
            int photon = -1;
            int charm = -1;

            for (int i = 0; i < nGenPart; ++i) {
                if (GenPart_genPartIdxMother[i] != idx) continue;
                if (GenPart_pdgId[i] == 22) photon = i;
                if (GenPart_pdgId[i] == 4)  charm = i;
            }

            if (photon >= 0 && charm >= 0) {
                cstarIdx = idx;
                genPhotonIdx = photon;
                genCharmIdx = charm;
                break;
            }
        }
        if (cstarIdx < 0) continue;

        TLorentzVector cstar_p4;
        cstar_p4.SetPtEtaPhiM(GenPart_pt[cstarIdx], GenPart_eta[cstarIdx],
                              GenPart_phi[cstarIdx], GenPart_mass[cstarIdx]);
        h_M_cstar->Fill(cstar_p4.M(), weight_central);

        // ---------- GEN mass ----------
        TLorentzVector c_p4, GenJet_p4;
        int genJetIdx = -1;
        float best_deltaR_cJet = 999.0;

        for (int j = 0; j < nGenJet; ++j) {
            GenJet_p4.SetPtEtaPhiM(GenJet_pt[j], GenJet_eta[j], GenJet_phi[j], GenJet_mass[j]);
            c_p4.SetPtEtaPhiM(GenPart_pt[genCharmIdx], GenPart_eta[genCharmIdx],
                              GenPart_phi[genCharmIdx], GenPart_mass[genCharmIdx]);

            float deltaR_cJet = c_p4.DeltaR(GenJet_p4);
            if (deltaR_cJet < best_deltaR_cJet) {
                best_deltaR_cJet = deltaR_cJet;
                genJetIdx = j;
            }
        }
        if (best_deltaR_cJet > 0.2) continue;

        TLorentzVector gen_photon_p4, gen_jet_p4;
        gen_photon_p4.SetPtEtaPhiM(GenPart_pt[genPhotonIdx], GenPart_eta[genPhotonIdx],
                                   GenPart_phi[genPhotonIdx], GenPart_mass[genPhotonIdx]);
        gen_jet_p4.SetPtEtaPhiM(GenJet_pt[genJetIdx], GenJet_eta[genJetIdx],
                                GenJet_phi[genJetIdx], GenJet_mass[genJetIdx]);

        TLorentzVector gen_M_p4 = gen_photon_p4 + gen_jet_p4;
        hM_gen->Fill(gen_M_p4.M(), weight_central);

        // ======================================================
        // Photon selection
        // ======================================================
        int goodPhotonIdx = -1;
        float leadingPhotonPt = -1.0;

        for (int i = 0; i < nPhoton; ++i) {
            if (Photon_pt[i] < 240) continue;
            if (fabs(Photon_eta[i]) >= 1.4442) continue;
            if (Photon_cutBased[i] < 2) continue;
            if (Photon_electronVeto[i] != 1) continue;

            if (Photon_pt[i] > leadingPhotonPt) {
                goodPhotonIdx = i;
                leadingPhotonPt = Photon_pt[i];
            }
        }
        if (goodPhotonIdx < 0) continue;

        TLorentzVector reco_g_p4;
        reco_g_p4.SetPtEtaPhiM(Photon_pt[goodPhotonIdx], Photon_eta[goodPhotonIdx],
                               Photon_phi[goodPhotonIdx], Photon_mass[goodPhotonIdx]);

        // ======================================================
        // Jet selection with central / JER up / JER down
        // ======================================================
        std::vector<TLorentzVector> goodJets_central;
        std::vector<TLorentzVector> goodJets_JERUp;
        std::vector<TLorentzVector> goodJets_JERDown;

        // If your branch name is different, change this line only:
        double rho = fixedGridRhoFastjetAll;

        for (int i = 0; i < nJet; ++i) {
            if (fabs(Jet_eta[i]) >= 2.4) continue;
            if (Jet_jetId[i] < 6) continue;
            if (Jet_btagDeepFlavCvB[i] < 0.340) continue;
            if (Jet_btagDeepFlavCvL[i] < 0.085) continue;

            TLorentzVector j_raw;
            j_raw.SetPtEtaPhiM(Jet_pt[i], Jet_eta[i], Jet_phi[i], Jet_mass[i]);

            double pt  = Jet_pt[i];
            double eta = Jet_eta[i];

            if (pt <= 0.0) continue;

            // Official JER numbers from text files
            double sf_nom  = getJERSF(eta, 0);
            double sf_up   = getJERSF(eta, +1);
            double sf_down = getJERSF(eta, -1);

            double sigmaJER = getJERResolution(pt, eta, rho);

            int matchedGenJetIdx = findMatchedGenJet(i,
                                                     nGenJet,
                                                     GenJet_pt, GenJet_eta, GenJet_phi, GenJet_mass,
                                                     Jet_pt, Jet_eta, Jet_phi, Jet_mass,
                                                     sigmaJER);

            double cJER_nom  = 1.0;
            double cJER_up   = 1.0;
            double cJER_down = 1.0;

            if (matchedGenJetIdx >= 0) {
                // ---------- scaling method ----------
                double genpt = GenJet_pt[matchedGenJetIdx];

                cJER_nom  = 1.0 + (sf_nom  - 1.0) * (pt - genpt) / pt;
                cJER_up   = 1.0 + (sf_up   - 1.0) * (pt - genpt) / pt;
                cJER_down = 1.0 + (sf_down - 1.0) * (pt - genpt) / pt;
            } else {
                // ---------- stochastic smearing ----------
                // same random number for nominal/up/down
                unsigned int seed = (unsigned int)(jentry * 1000 + i + 12345);
                TRandom3 rng(seed);
                double gaus = rng.Gaus(0.0, sigmaJER);

                cJER_nom  = 1.0 + gaus * std::sqrt(std::max(sf_nom  * sf_nom  - 1.0, 0.0));
                cJER_up   = 1.0 + gaus * std::sqrt(std::max(sf_up   * sf_up   - 1.0, 0.0));
                cJER_down = 1.0 + gaus * std::sqrt(std::max(sf_down * sf_down - 1.0, 0.0));
            }

            // truncate at zero
            cJER_nom  = std::max(0.0, cJER_nom);
            cJER_up   = std::max(0.0, cJER_up);
            cJER_down = std::max(0.0, cJER_down);

            TLorentzVector j_central = j_raw;
            TLorentzVector j_JERUp   = j_raw;
            TLorentzVector j_JERDown = j_raw;

            j_central *= cJER_nom;
            j_JERUp   *= cJER_up;
            j_JERDown *= cJER_down;

            if (j_central.Pt() > 170.0 && reco_g_p4.DeltaR(j_central) > 0.4)
                goodJets_central.push_back(j_central);

            if (j_JERUp.Pt() > 170.0 && reco_g_p4.DeltaR(j_JERUp) > 0.4)
                goodJets_JERUp.push_back(j_JERUp);

            if (j_JERDown.Pt() > 170.0 && reco_g_p4.DeltaR(j_JERDown) > 0.4)
                goodJets_JERDown.push_back(j_JERDown);
        }

        int idx_central = GetLeadingJetIndex(goodJets_central);
        int idx_JERUp   = GetLeadingJetIndex(goodJets_JERUp);
        int idx_JERDown = GetLeadingJetIndex(goodJets_JERDown);

        if (idx_central < 0) continue;

        // ---------- central + PU ----------
        TLorentzVector reco_m_central_p4 = reco_g_p4 + goodJets_central[idx_central];
        double m_central = reco_m_central_p4.M();

        h_sig->Fill(m_central, weight_central);
        h_sig_PUUp->Fill(m_central, weight_PUUp);
        h_sig_PUDown->Fill(m_central, weight_PUDown);

        hPhoton_pt->Fill(Photon_pt[goodPhotonIdx], weight_central);
        hJet_pt->Fill(goodJets_central[idx_central].Pt(), weight_central);

        // ---------- JER ----------
        if (idx_JERUp >= 0) {
            TLorentzVector reco_m_JERUp_p4 = reco_g_p4 + goodJets_JERUp[idx_JERUp];
            h_sig_JERUp->Fill(reco_m_JERUp_p4.M(), weight_central);
        }

        if (idx_JERDown >= 0) {
            TLorentzVector reco_m_JERDown_p4 = reco_g_p4 + goodJets_JERDown[idx_JERDown];
            h_sig_JERDown->Fill(reco_m_JERDown_p4.M(), weight_central);
        }
    }

    // ======================================================
    // Plots
    // ======================================================
    SetCMSStyle();
    gROOT->SetBatch(kTRUE);

    TCanvas *c1 = new TCanvas("c1", "Invariant Mass Gen", 600, 400);
    hM_gen->Draw("HIST");
    hM_gen->GetXaxis()->SetTitleOffset(1.4);
    hM_gen->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c1->SaveAs("Invariant_Mass_gen.png");

    TCanvas *c3 = new TCanvas("c3", "Mass of C*", 600, 400);
    h_M_cstar->Draw("HIST");
    h_M_cstar->GetXaxis()->SetTitleOffset(1.4);
    h_M_cstar->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c3->SaveAs("Mass_Cstar.png");

    TCanvas *c4 = new TCanvas("c4", "Photon_pT", 600, 400);
    hPhoton_pt->Draw("HIST");
    hPhoton_pt->GetXaxis()->SetTitleOffset(1.4);
    hPhoton_pt->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c4->SaveAs("Photon_pT.png");

    TCanvas *c5 = new TCanvas("c5", "Jet_pT", 600, 400);
    hJet_pt->Draw("HIST");
    hJet_pt->GetXaxis()->SetTitleOffset(1.4);
    hJet_pt->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c5->SaveAs("Jet_pT.png");

    TCanvas *c6 = new TCanvas("c6", "Central signal", 600, 400);
    h_sig->Draw("HIST");
    h_sig->GetXaxis()->SetTitleOffset(1.4);
    h_sig->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);
    c6->SaveAs("sig.png");

    TCanvas *c7 = new TCanvas("c7", "Signal PU compare", 600, 400);
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
    leg->AddEntry(h_sig, "central", "l");
    leg->AddEntry(h_sig_PUUp, "PU Up", "l");
    leg->AddEntry(h_sig_PUDown, "PU Down", "l");
    leg->SetBorderSize(0);
    leg->Draw();

    c7->SaveAs("sig_PU_compare.png");

    TCanvas *c8 = new TCanvas("c8", "Signal JER compare", 600, 400);
    h_sig_JERDown->SetLineColor(kBlue);
    h_sig_JERDown->SetLineWidth(1);

    h_sig->SetLineColor(kBlack);
    h_sig->SetLineWidth(1);

    h_sig_JERUp->SetLineColor(kRed);
    h_sig_JERUp->SetLineWidth(1);

    h_sig_JERDown->Draw("HIST");
    h_sig->Draw("HIST SAME");
    h_sig_JERUp->Draw("HIST SAME");

    h_sig->GetXaxis()->SetTitleOffset(1.4);
    h_sig->GetXaxis()->SetLabelOffset(0.02);
    CMS_label(0.18, 0.87);

    TLegend *legJER = new TLegend(0.62, 0.52, 0.88, 0.68);
    legJER->AddEntry(h_sig, "central", "l");
    legJER->AddEntry(h_sig_JERUp, "JER Up", "l");
    legJER->AddEntry(h_sig_JERDown, "JER Down", "l");
    legJER->SetBorderSize(0);
    legJER->Draw();

    c8->SaveAs("sig_JER_compare.png");

    // PU comparison plot
    TCanvas *cPUcomp = new TCanvas("cPUcomp", "PU comparison", 700, 700);

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
    hData_plot->GetXaxis()->SetLabelSize(0);

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

    // ======================================================
    // Output ROOT file for Combine
    // ======================================================
    TFile *fOut = new TFile("CstarToGJ.root", "RECREATE");
    h_M_cstar->Write();
    hM_gen->Write();
    hPhoton_pt->Write();
    hJet_pt->Write();

    h_sig->Write("sig");
    h_sig_PUUp->Write("sig_PUUp");
    h_sig_PUDown->Write("sig_PUDown");
    h_sig_JERUp->Write("sig_JERUp");
    h_sig_JERDown->Write("sig_JERDown");

    fOut->Close();
    delete fOut;
}
