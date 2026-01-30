template <typename T>
auto FindGoodPhotons(T &df) {
   return df.Define("goodPhoton","Photon_pt > 25 && abs(Photon_eta) < 2.5").Filter("Sum(goodPhoton)>=2 && HLT_Diphoton30_22_R9Id_OR_IsoCaloId_AND_HE_R9Id_Mass90==1");
}

template <typename T>
auto FindGoodJets(T &df){
   return df.Define("goodJets","Jet_pt > 25 && Jet_jetId==6").Filter("Sum(goodJets)>=2");
}

void ForTrigSF_Data(string inFile, string outFile){
   //enable multi-threding
   ROOT::EnableImplicitMT();
   ROOT::RDataFrame dfIn("Events", inFile);

   auto df1 = FindGoodPhotons(dfIn);
   auto df = FindGoodJets(df1);
   
   df.Redefine("run",  				"run")
     .Redefine("luminosityBlock", 		"luminosityBlock")
     .Redefine("event", 			"event")
     .Redefine("Photon_dEscaleDown",            "Photon_dEscaleDown")
     .Redefine("Photon_dEscaleUp",              "Photon_dEscaleUp")
     .Redefine("Photon_dEsigmaDown",		"Photon_dEsigmaDown")
     .Redefine("Photon_dEsigmaUp",      	"Photon_dEsigmaUp")
     .Redefine("Photon_eCorr",			"Photon_eCorr")
     .Redefine("Photon_electronVeto",           "Photon_electronVeto")
     .Redefine("Photon_isScEtaEB",              "Photon_isScEtaEB")
     .Redefine("Photon_isScEtaEE",              "Photon_isScEtaEE")
     .Redefine("Photon_mvaID_WP80",             "Photon_mvaID_WP80")
     .Redefine("Photon_mvaID_WP90",             "Photon_mvaID_WP90")
     .Redefine("Photon_pixelSeed",              "Photon_pixelSeed")
     .Redefine("Photon_seedGain",               "Photon_seedGain")
     .Redefine("Photon_electronIdx",            "Photon_electronIdx")
     .Redefine("Photon_vidNestedWPBitmap",      "Photon_vidNestedWPBitmap")
     .Redefine("Photon_energyErr",              "Photon_energyErr")
     .Redefine("Photon_eta",                	"Photon_eta")
     .Redefine("Photon_hoe",                	"Photon_hoe")
     .Redefine("Photon_mvaID",                	"Photon_mvaID")
     .Redefine("Photon_pfRelIso03_all",     	"Photon_pfRelIso03_all")
     .Redefine("Photon_pfRelIso03_chg",     	"Photon_pfRelIso03_chg")
     .Redefine("Photon_phi",                	"Photon_phi")
     .Redefine("Photon_pt"      ,         	"Photon_pt")
     .Redefine("Photon_r9"      ,        	"Photon_r9")
     .Redefine("Photon_sieie"   ,            	"Photon_sieie")
     .Redefine("fixedGridRhoFastjetAll",    	"fixedGridRhoFastjetAll")
     .Redefine("Jet_jetId",			"Jet_jetId")
     .Redefine("Jet_puId",			"Jet_puId")
     .Redefine("Jet_muonIdx1",                  "Jet_muonIdx1")
     .Redefine("Jet_muonIdx2",                  "Jet_muonIdx2")
     .Redefine("Jet_eta",			"Jet_eta")
     .Redefine("Jet_mass",			"Jet_mass")
     .Redefine("Jet_muEF",			"Jet_muEF")
     .Redefine("Jet_muonSubtrFactor",		"Jet_muonSubtrFactor")
     .Redefine("Jet_chHEF",                     "Jet_chHEF")
     .Redefine("Jet_chEmEF",			"Jet_chEmEF")
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
     .Redefine("PV_npvs",                       "PV_npvs")
     .Redefine("PV_npvsGood",                   "PV_npvsGood")
     .Redefine("PuppiMET_pt", 			"PuppiMET_pt")
     .Redefine("PuppiMET_phi", 			"PuppiMET_phi") 
     .Redefine("HLT_Diphoton30_22_R9Id_OR_IsoCaloId_AND_HE_R9Id_Mass90", "HLT_Diphoton30_22_R9Id_OR_IsoCaloId_AND_HE_R9Id_Mass90");
;

     char filename2[100];
         sprintf(filename2,"%s", outFile.c_str());
   df.Snapshot("Events", filename2, {
	"run",
	"luminosityBlock",
	"event",
	"Photon_dEscaleDown",
	"Photon_dEscaleUp",
	"Photon_dEsigmaDown",
	"Photon_dEsigmaUp",
	"Photon_eCorr",
	"Photon_isScEtaEB",
	"Photon_isScEtaEE",
	"Photon_mvaID_WP80",
	"Photon_mvaID_WP90",
	"Photon_pixelSeed",
	"Photon_seedGain",
	"Photon_electronIdx",
	"Photon_vidNestedWPBitmap",
	"Photon_energyErr",
	"Photon_eta",
	"Photon_hoe",
	"Photon_mvaID",
	"Photon_pfRelIso03_all",
	"Photon_pfRelIso03_chg",
	"Photon_phi",
	"Photon_pt",
	"Photon_r9",
	"Photon_sieie",
	"fixedGridRhoFastjetAll",
	"Jet_jetId",
	"Jet_puId",
	"Jet_muonIdx1",
	"Jet_muonIdx2",
	"Jet_eta",
	"Jet_mass",
	"Jet_muEF",
	"Jet_muonSubtrFactor",
	"Jet_chEmEF",
	"Jet_chHEF",
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
	"HLT_Diphoton30_22_R9Id_OR_IsoCaloId_AND_HE_R9Id_Mass90",
		   });
} 
