template <typename T>
auto FindGoodPhotons(T &df) {
    return df.Define("goodPhoton","Photon_pt > 220 && abs(Photon_eta) < 1.4442").Filter("Sum(goodPhoton)>=1");
}

template <typename T>
auto FindGoodJets(T &df) {
    //return df.Define("goodJet","Jet_pt > 50").Filter("Sum(goodJet)>=1");
    //return df.Define("Trig_Decision","HLT_PFJet40==1 || HLT_PFJet60==1 || HLT_PFJet80==1 || HLT_PFJet110==1 || HLT_PFJet140==1 || HLT_PFJet200==1 || HLT_PFJet260==1 || HLT_PFJet320==1 || HLT_PFJet400==1 || HLT_PFJet450==1 || HLT_PFJet500==1 || HLT_PFJet550==1").Filter("Trig_Decision==1");
    return df.Define("Trig_Decision","HLT_Photon200==1").Filter("Trig_Decision==1");
}

void ForTrigSF(string inFile, string outFile){
    //enable multi-threding
    ROOT::EnableImplicitMT();
    ROOT::RDataFrame dfIn("Events", inFile);
    auto df1 = FindGoodPhotons(dfIn);
    auto df = FindGoodJets(df1);

    df.Redefine("run",  "run")
        .Redefine("luminosityBlock", "luminosityBlock")
        .Redefine("event", "event")
        .Redefine("bunchCrossing",  "bunchCrossing")
        .Redefine("Photon_seediEtaOriX", 		"Photon_seediEtaOriX")
        .Redefine("Photon_cutBased",                	"Photon_cutBased")
        .Redefine("Photon_electronVeto",                 "Photon_electronVeto")
        .Redefine("Photon_isScEtaEB",                    "Photon_isScEtaEB")
        .Redefine("Photon_isScEtaEE",                    "Photon_isScEtaEE")
        .Redefine("Photon_mvaID_WP80",                   "Photon_mvaID_WP80")
        .Redefine("Photon_mvaID_WP90",                   "Photon_mvaID_WP90")
        .Redefine("Photon_pixelSeed",                    "Photon_pixelSeed")
        .Redefine("Photon_seedGain",                	"Photon_seedGain")
        .Redefine("Photon_electronIdx",                  "Photon_electronIdx")
        .Redefine("Photon_seediPhiOriY",                 "Photon_seediPhiOriY")
        .Redefine("Photon_vidNestedWPBitmap",            "Photon_vidNestedWPBitmap")
        .Redefine("Photon_energyErr",                    "Photon_energyErr")
        .Redefine("Photon_energyRaw",                    "Photon_energyRaw")
        .Redefine("Photon_esEffSigmaRR",                 "Photon_esEffSigmaRR")
        .Redefine("Photon_esEnergyOverRawE",             "Photon_esEnergyOverRawE")
        .Redefine("Photon_eta",                		"Photon_eta")
        .Redefine("Photon_etaWidth",                	"Photon_etaWidth")
        .Redefine("Photon_haloTaggerMVAVal",             "Photon_haloTaggerMVAVal") 
        .Redefine("Photon_hoe",                		"Photon_hoe")
        .Redefine("Photon_hoe_PUcorr",                   "Photon_hoe_PUcorr")
        .Redefine("Photon_mvaID",                	"Photon_mvaID")
        .Redefine("Photon_pfChargedIsoPFPV",             "Photon_pfChargedIsoPFPV")
        .Redefine("Photon_pfChargedIsoWorstVtx",         "Photon_pfChargedIsoWorstVtx")
        .Redefine("Photon_pfPhoIso03",                   "Photon_pfPhoIso03")
        .Redefine("Photon_pfRelIso03_all_quadratic",     "Photon_pfRelIso03_all_quadratic")
        .Redefine("Photon_pfRelIso03_chg_quadratic",     "Photon_pfRelIso03_chg_quadratic")
        .Redefine("Photon_phi",                		"Photon_phi")
        .Redefine("Photon_phiWidth",                	"Photon_phiWidth")
        .Redefine("Photon_pt"      ,         		"Photon_pt")
        .Redefine("Photon_r9"      ,        		"Photon_r9")
        .Redefine("Photon_s4"      ,          		"Photon_s4")
        .Redefine("Photon_sieie"   ,            		"Photon_sieie")
        .Redefine("Photon_sieip",                	"Photon_sieip")
        .Redefine("Photon_sipip",                	"Photon_sipip")
        .Redefine("Photon_trkSumPtHollowConeDR03",       "Photon_trkSumPtHollowConeDR03")
        .Redefine("Photon_trkSumPtSolidConeDR04",        "Photon_trkSumPtSolidConeDR04")
        .Redefine("Photon_x_calo",                	"Photon_x_calo")
        .Redefine("Photon_y_calo",			"Photon_y_calo")
        .Redefine("Photon_z_calo",			"Photon_z_calo")
        .Redefine("Rho_fixedGridRhoFastjetAll",    "Rho_fixedGridRhoFastjetAll")
        .Redefine("Jet_jetId",			"Jet_jetId")
        .Redefine("Jet_nConstituents",		"Jet_nConstituents")
        .Redefine("Jet_nElectrons",		        "Jet_nElectrons")
        .Redefine("Jet_nMuons",			"Jet_nMuons")
        .Redefine("Jet_nSVs",			        "Jet_nSVs")
        .Redefine("Jet_electronIdx1",		        "Jet_electronIdx1")
        .Redefine("Jet_electronIdx2",		        "Jet_electronIdx2")
        .Redefine("Jet_muonIdx1",			"Jet_muonIdx1")
        .Redefine("Jet_muonIdx2",			"Jet_muonIdx2")
        .Redefine("Jet_svIdx1",			"Jet_svIdx1")
        .Redefine("Jet_svIdx2",			"Jet_svIdx2")
        .Redefine("Jet_hfadjacentEtaStripsSize",	"Jet_hfadjacentEtaStripsSize")
        .Redefine("Jet_hfcentralEtaStripSize",	"Jet_hfcentralEtaStripSize")
        .Redefine("Jet_PNetRegPtRawCorr",		"Jet_PNetRegPtRawCorr")
        .Redefine("Jet_PNetRegPtRawCorrNeutrino",	"Jet_PNetRegPtRawCorrNeutrino")
        .Redefine("Jet_PNetRegPtRawRes",		"Jet_PNetRegPtRawRes")
        .Redefine("Jet_area",			"Jet_area")
        .Redefine("Jet_chEmEF",			"Jet_chEmEF")
        .Redefine("Jet_chHEF",			"Jet_chHEF")
        .Redefine("Jet_eta",			"Jet_eta")
        .Redefine("Jet_hfsigmaEtaEta",		"Jet_hfsigmaEtaEta")
        .Redefine("Jet_hfsigmaPhiPhi",		"Jet_hfsigmaPhiPhi")
        .Redefine("Jet_mass",			"Jet_mass")
        .Redefine("Jet_muEF",			"Jet_muEF")
        .Redefine("Jet_muonSubtrFactor",		"Jet_muonSubtrFactor")
        .Redefine("Jet_neEmEF",			"Jet_neEmEF")
        .Redefine("Jet_neHEF",			"Jet_neHEF")
        .Redefine("Jet_phi",		        "Jet_phi")
        .Redefine("Jet_pt",			"Jet_pt")
        .Redefine("Jet_rawFactor",			"Jet_rawFactor")
        .Redefine("TrigObj_l1charge",		"TrigObj_l1charge")
        .Redefine("TrigObj_id",			"TrigObj_id")
        .Redefine("TrigObj_l1iso",			"TrigObj_l1iso")
        .Redefine("TrigObj_filterBits",		"TrigObj_filterBits")
        .Redefine("TrigObj_pt",			"TrigObj_pt")
        .Redefine("TrigObj_eta",			"TrigObj_eta")
        .Redefine("TrigObj_phi",			"TrigObj_phi")
        .Redefine("TrigObj_l1pt",			"TrigObj_l1pt")
        .Redefine("TrigObj_l1pt_2",		"TrigObj_l1pt_2")
        .Redefine("TrigObj_l2pt",			"TrigObj_l2pt")
        .Redefine("HLT_Photon200", "HLT_Photon200")
        .Redefine("HLT_Photon175", "HLT_Photon175")
        .Redefine("HLT_Photon150", "HLT_Photon150")
        .Redefine("HLT_Photon120", "HLT_Photon120")
        /*.Redefine("HLT_PFJet40", "HLT_PFJet40")
          .Redefine("HLT_PFJet60", "HLT_PFJet60")
          .Redefine("HLT_PFJet80", "HLT_PFJet80")
          .Redefine("HLT_PFJet110", "HLT_PFJet110")
          .Redefine("HLT_PFJet140", "HLT_PFJet140")
          .Redefine("HLT_PFJet200", "HLT_PFJet200")
          .Redefine("HLT_PFJet260", "HLT_PFJet260")
          .Redefine("HLT_PFJet320", "HLT_PFJet320")
          .Redefine("HLT_PFJet400", "HLT_PFJet400")
          .Redefine("HLT_PFJet450", "HLT_PFJet450")
          .Redefine("HLT_PFJet500", "HLT_PFJet500")
          .Redefine("HLT_PFJet550", "HLT_PFJet550")*/
        .Redefine("PV_npvs",                           "PV_npvs")
        .Redefine("PV_npvsGood",                       "PV_npvsGood")
        /*.Redefine("Pileup_nPU",                        "Pileup_nPU")
          .Redefine("GenPart_genPartIdxMother", "GenPart_genPartIdxMother")
          .Redefine("GenPart_statusFlags", "GenPart_statusFlags")
          .Redefine("GenPart_pdgId", "GenPart_pdgId")
          .Redefine("GenPart_status", "GenPart_status")
          .Redefine("GenPart_eta", "GenPart_eta")
          .Redefine("GenPart_mass", "GenPart_mass")
          .Redefine("GenPart_phi", "GenPart_phi")
          .Redefine("GenPart_pt", "GenPart_pt")*/
        .Redefine("PuppiMET_pt", "PuppiMET_pt")
        .Redefine("PuppiMET_phi", "PuppiMET_phi"); 
    ;

    char filename2[100];
    sprintf(filename2,"%s", outFile.c_str());
    df.Snapshot("Events", filename2, {
            "run",
            "luminosityBlock",
            "event",
            "bunchCrossing", 				   
            "Photon_seediEtaOriX",
            "Photon_cutBased",
            "Photon_isScEtaEB",
            "Photon_isScEtaEE",
            "Photon_mvaID_WP80",
            "Photon_mvaID_WP90",
            "Photon_pixelSeed",
            "Photon_seedGain",
            "Photon_electronIdx",
            "Photon_seediPhiOriY",
            "Photon_vidNestedWPBitmap",
            "Photon_energyErr",
            "Photon_energyRaw",
            "Photon_esEffSigmaRR",
            "Photon_esEnergyOverRawE",
            "Photon_eta",
            "Photon_etaWidth",
            "Photon_haloTaggerMVAVal",
            "Photon_hoe",
            "Photon_hoe_PUcorr",
            "Photon_mvaID",
            "Photon_pfChargedIsoPFPV",
            "Photon_pfChargedIsoWorstVtx",
            "Photon_pfPhoIso03",
            "Photon_pfRelIso03_all_quadratic",
            "Photon_pfRelIso03_chg_quadratic",
            "Photon_phi",
            "Photon_phiWidth",
            "Photon_pt",
            "Photon_r9",
            "Photon_s4",
            "Photon_sieie",
            "Photon_sieip",
            "Photon_sipip",
            "Photon_trkSumPtHollowConeDR03",
            "Photon_trkSumPtSolidConeDR04",
            "Photon_x_calo",
            "Photon_y_calo",
            "Photon_z_calo",
            "Rho_fixedGridRhoFastjetAll",
            "Jet_jetId",
            "Jet_nConstituents",
            "Jet_nElectrons",
            "Jet_nMuons",
            "Jet_nSVs",
            "Jet_electronIdx1",
            "Jet_electronIdx2",
            "Jet_muonIdx1",
            "Jet_muonIdx2",
            "Jet_svIdx1",
            "Jet_svIdx2",
            "Jet_hfadjacentEtaStripsSize",
            "Jet_hfcentralEtaStripSize",
            "Jet_PNetRegPtRawCorr",
            "Jet_PNetRegPtRawCorrNeutrino",
            "Jet_PNetRegPtRawRes",
            "Jet_area",
            "Jet_chEmEF",
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
            "Jet_rawFactor",
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
            "PuppiMET_phi",
            "PuppiMET_pt",
            "HLT_Photon200",
            "HLT_Photon175",
            "HLT_Photon150",
            "HLT_Photon120",
            /* "HLT_PFJet40",
               "HLT_PFJet60",
               "HLT_PFJet80",
               "HLT_PFJet110",
               "HLT_PFJet140",
               "HLT_PFJet200",
               "HLT_PFJet260",
               "HLT_PFJet320",
               "HLT_PFJet400",
               "HLT_PFJet450",
               "HLT_PFJet500",
               "HLT_PFJet550"*/
    });
} 
