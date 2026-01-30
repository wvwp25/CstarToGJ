#include <ROOT/RDataFrame.hxx>
#include <TString.h>
#include <iostream>

template <typename T>
auto FindGoodPhotons(T &df) {
    return df.Define("goodPhoton", "Photon_pt > 25 && abs(Photon_eta) < 2.5")
             .Filter("Sum(goodPhoton)>=1");
}

template <typename T>
auto FindGoodJets(T &df) {
    return df.Define("goodJets", "Jet_pt > 25")
             .Filter("Sum(goodJets)>=1")
             .Define("Trig_Decision", "HLT_Photon200==1")
             .Filter("Trig_Decision==1");
}

void ForTrigSF_2017_v2(string inFile, string outFile){
    ROOT::EnableImplicitMT();
    ROOT::RDataFrame dfIn("Events", inFile);

    auto df1 = FindGoodPhotons(dfIn);
    auto df  = FindGoodJets(df1);

    // -----------------------------
    // Snapshot branch list
    // -----------------------------
    std::vector<std::string> branches = {

        "run",
        "luminosityBlock",
        "event",

        // --- Photon ---
        "nPhoton",
        "Photon_dEscaleDown",
        "Photon_dEscaleUp",
        "Photon_dEsigmaDown",
        "Photon_dEsigmaUp",
        "Photon_eCorr",
        "Photon_energyErr",
        "Photon_eta",
        "Photon_hoe",
        "Photon_mass",
        "Photon_mvaID",
        "Photon_mvaID_Fall17V1p1",
        "Photon_pfRelIso03_all",
        "Photon_pfRelIso03_chg",
        "Photon_phi",
        "Photon_pt",
        "Photon_r9",
        "Photon_sieie",
        "Photon_charge",
        "Photon_cutBased",
        "Photon_cutBased_Fall17V1Bitmap",
        "Photon_electronIdx",
        "Photon_jetIdx",
        "Photon_pdgId",
        "Photon_vidNestedWPBitmap",
        "Photon_electronVeto",
        "Photon_isScEtaEB",
        "Photon_isScEtaEE",
        "Photon_mvaID_WP80",
        "Photon_mvaID_WP90",
        "Photon_pixelSeed",
        "Photon_seedGain",

        // --- Jets ---
        "nJet",
        "Jet_area",
        "Jet_btagCSVV2",
        "Jet_btagDeepB",
        "Jet_btagDeepCvB",
        "Jet_btagDeepCvL",
        "Jet_btagDeepFlavB",
        "Jet_btagDeepFlavCvB",
        "Jet_btagDeepFlavCvL",
        "Jet_btagDeepFlavQG",
        "Jet_chEmEF",
        "Jet_chFPV0EF",
        "Jet_chHEF",
        "Jet_eta",
        "Jet_hfsigmaEtaEta",
        "Jet_hfsigmaPhiPhi",
        "Jet_mass",
        "Jet_muEF",
        "Jet_muonSubtrFactor",
        "Jet_neEmEF",
        "Jet_neHEF",
        "Jet_phi",
        "Jet_pt",
        "Jet_puIdDisc",
        "Jet_qgl",
        "Jet_rawFactor",
        "Jet_bRegCorr",
        "Jet_bRegRes",
        "Jet_cRegCorr",
        "Jet_cRegRes",
        "Jet_electronIdx1",
        "Jet_electronIdx2",
        "Jet_hfadjacentEtaStripsSize",
        "Jet_hfcentralEtaStripSize",
        "Jet_jetId",
        "Jet_muonIdx1",
        "Jet_muonIdx2",
        "Jet_nElectrons",
        "Jet_nMuons",
        "Jet_puId",
        "Jet_nConstituents",

        // --- Trigger and PV info ---
        "TrigObj_l1charge",
        "TrigObj_id",
        "TrigObj_l1iso",
        "TrigObj_filterBits",
        "TrigObj_pt",
        "TrigObj_eta",
        "TrigObj_phi",
        "TrigObj_l1pt",
        "TrigObj_l1pt_2",
        "TrigObj_l2pt",

        "PV_npvs",
        "PV_npvsGood",

        "fixedGridRhoFastjetAll",

        "PuppiMET_phi",
        "PuppiMET_pt",

        // --- Triggers ---
        "HLT_Photon200",
        "HLT_Diphoton30_22_R9Id_OR_IsoCaloId_AND_HE_R9Id_Mass90"
    };

    // -----------------------------
    // Write out result
    // -----------------------------
    df.Snapshot("Events", outFile, branches);
}

