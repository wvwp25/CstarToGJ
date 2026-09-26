//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Tue Dec  2 05:25:22 2025 by ROOT version 6.36.04
// from TTree Events/Events
// found on file: SinglePhoton_Run2017BCDEF-UL2017_MiniAODv2_NanoAODv9-v1_NANOAOD.root
//////////////////////////////////////////////////////////

#ifndef Run2017BCDEF_h
#define Run2017BCDEF_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

class Run2017BCDEF {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   UInt_t          run;
   UInt_t          luminosityBlock;
   ULong64_t       event;
   UInt_t          nPhoton;
   Float_t         Photon_dEscaleDown[142];   //[nPhoton]
   Float_t         Photon_dEscaleUp[142];   //[nPhoton]
   Float_t         Photon_dEsigmaDown[142];   //[nPhoton]
   Float_t         Photon_dEsigmaUp[142];   //[nPhoton]
   Float_t         Photon_eCorr[142];   //[nPhoton]
   Float_t         Photon_energyErr[142];   //[nPhoton]
   Float_t         Photon_eta[142];   //[nPhoton]
   Float_t         Photon_hoe[142];   //[nPhoton]
   Float_t         Photon_mass[142];   //[nPhoton]
   Float_t         Photon_mvaID[142];   //[nPhoton]
   Float_t         Photon_mvaID_Fall17V1p1[142];   //[nPhoton]
   Float_t         Photon_pfRelIso03_all[142];   //[nPhoton]
   Float_t         Photon_pfRelIso03_chg[142];   //[nPhoton]
   Float_t         Photon_phi[142];   //[nPhoton]
   Float_t         Photon_pt[142];   //[nPhoton]
   Float_t         Photon_r9[142];   //[nPhoton]
   Float_t         Photon_sieie[142];   //[nPhoton]
   Int_t           Photon_charge[142];   //[nPhoton]
   Int_t           Photon_cutBased[142];   //[nPhoton]
   Int_t           Photon_cutBased_Fall17V1Bitmap[142];   //[nPhoton]
   Int_t           Photon_electronIdx[142];   //[nPhoton]
   Int_t           Photon_jetIdx[142];   //[nPhoton]
   Int_t           Photon_pdgId[142];   //[nPhoton]
   Int_t           Photon_vidNestedWPBitmap[142];   //[nPhoton]
   Bool_t          Photon_electronVeto[142];   //[nPhoton]
   Bool_t          Photon_isScEtaEB[142];   //[nPhoton]
   Bool_t          Photon_isScEtaEE[142];   //[nPhoton]
   Bool_t          Photon_mvaID_WP80[142];   //[nPhoton]
   Bool_t          Photon_mvaID_WP90[142];   //[nPhoton]
   Bool_t          Photon_pixelSeed[142];   //[nPhoton]
   UChar_t         Photon_seedGain[142];   //[nPhoton]
   UInt_t          nJet;
   Float_t         Jet_area[59];   //[nJet]
   Float_t         Jet_btagCSVV2[59];   //[nJet]
   Float_t         Jet_btagDeepB[59];   //[nJet]
   Float_t         Jet_btagDeepCvB[59];   //[nJet]
   Float_t         Jet_btagDeepCvL[59];   //[nJet]
   Float_t         Jet_btagDeepFlavB[59];   //[nJet]
   Float_t         Jet_btagDeepFlavCvB[59];   //[nJet]
   Float_t         Jet_btagDeepFlavCvL[59];   //[nJet]
   Float_t         Jet_btagDeepFlavQG[59];   //[nJet]
   Float_t         Jet_chEmEF[59];   //[nJet]
   Float_t         Jet_chFPV0EF[59];   //[nJet]
   Float_t         Jet_chHEF[59];   //[nJet]
   Float_t         Jet_eta[59];   //[nJet]
   Float_t         Jet_hfsigmaEtaEta[59];   //[nJet]
   Float_t         Jet_hfsigmaPhiPhi[59];   //[nJet]
   Float_t         Jet_mass[59];   //[nJet]
   Float_t         Jet_muEF[59];   //[nJet]
   Float_t         Jet_muonSubtrFactor[59];   //[nJet]
   Float_t         Jet_neEmEF[59];   //[nJet]
   Float_t         Jet_neHEF[59];   //[nJet]
   Float_t         Jet_phi[59];   //[nJet]
   Float_t         Jet_pt[59];   //[nJet]
   Float_t         Jet_puIdDisc[59];   //[nJet]
   Float_t         Jet_qgl[59];   //[nJet]
   Float_t         Jet_rawFactor[59];   //[nJet]
   Float_t         Jet_bRegCorr[59];   //[nJet]
   Float_t         Jet_bRegRes[59];   //[nJet]
   Float_t         Jet_cRegCorr[59];   //[nJet]
   Float_t         Jet_cRegRes[59];   //[nJet]
   Int_t           Jet_electronIdx1[59];   //[nJet]
   Int_t           Jet_electronIdx2[59];   //[nJet]
   Int_t           Jet_hfadjacentEtaStripsSize[59];   //[nJet]
   Int_t           Jet_hfcentralEtaStripSize[59];   //[nJet]
   Int_t           Jet_jetId[59];   //[nJet]
   Int_t           Jet_muonIdx1[59];   //[nJet]
   Int_t           Jet_muonIdx2[59];   //[nJet]
   Int_t           Jet_nElectrons[59];   //[nJet]
   Int_t           Jet_nMuons[59];   //[nJet]
   Int_t           Jet_puId[59];   //[nJet]
   UChar_t         Jet_nConstituents[59];   //[nJet]
   UInt_t          nTrigObj;
   Int_t           TrigObj_l1charge[48];   //[nTrigObj]
   Int_t           TrigObj_id[48];   //[nTrigObj]
   Int_t           TrigObj_l1iso[48];   //[nTrigObj]
   Int_t           TrigObj_filterBits[48];   //[nTrigObj]
   Float_t         TrigObj_pt[48];   //[nTrigObj]
   Float_t         TrigObj_eta[48];   //[nTrigObj]
   Float_t         TrigObj_phi[48];   //[nTrigObj]
   Float_t         TrigObj_l1pt[48];   //[nTrigObj]
   Float_t         TrigObj_l1pt_2[48];   //[nTrigObj]
   Float_t         TrigObj_l2pt[48];   //[nTrigObj]
   Int_t           PV_npvs;
   Int_t           PV_npvsGood;
   Float_t         fixedGridRhoFastjetAll;
   Float_t         PuppiMET_phi;
   Float_t         PuppiMET_pt;
   Bool_t          HLT_Photon200;
   Bool_t          HLT_Diphoton30_22_R9Id_OR_IsoCaloId_AND_HE_R9Id_Mass90;

   // List of branches
   TBranch        *b_run;   //!
   TBranch        *b_luminosityBlock;   //!
   TBranch        *b_event;   //!
   TBranch        *b_nPhoton;   //!
   TBranch        *b_Photon_dEscaleDown;   //!
   TBranch        *b_Photon_dEscaleUp;   //!
   TBranch        *b_Photon_dEsigmaDown;   //!
   TBranch        *b_Photon_dEsigmaUp;   //!
   TBranch        *b_Photon_eCorr;   //!
   TBranch        *b_Photon_energyErr;   //!
   TBranch        *b_Photon_eta;   //!
   TBranch        *b_Photon_hoe;   //!
   TBranch        *b_Photon_mass;   //!
   TBranch        *b_Photon_mvaID;   //!
   TBranch        *b_Photon_mvaID_Fall17V1p1;   //!
   TBranch        *b_Photon_pfRelIso03_all;   //!
   TBranch        *b_Photon_pfRelIso03_chg;   //!
   TBranch        *b_Photon_phi;   //!
   TBranch        *b_Photon_pt;   //!
   TBranch        *b_Photon_r9;   //!
   TBranch        *b_Photon_sieie;   //!
   TBranch        *b_Photon_charge;   //!
   TBranch        *b_Photon_cutBased;   //!
   TBranch        *b_Photon_cutBased_Fall17V1Bitmap;   //!
   TBranch        *b_Photon_electronIdx;   //!
   TBranch        *b_Photon_jetIdx;   //!
   TBranch        *b_Photon_pdgId;   //!
   TBranch        *b_Photon_vidNestedWPBitmap;   //!
   TBranch        *b_Photon_electronVeto;   //!
   TBranch        *b_Photon_isScEtaEB;   //!
   TBranch        *b_Photon_isScEtaEE;   //!
   TBranch        *b_Photon_mvaID_WP80;   //!
   TBranch        *b_Photon_mvaID_WP90;   //!
   TBranch        *b_Photon_pixelSeed;   //!
   TBranch        *b_Photon_seedGain;   //!
   TBranch        *b_nJet;   //!
   TBranch        *b_Jet_area;   //!
   TBranch        *b_Jet_btagCSVV2;   //!
   TBranch        *b_Jet_btagDeepB;   //!
   TBranch        *b_Jet_btagDeepCvB;   //!
   TBranch        *b_Jet_btagDeepCvL;   //!
   TBranch        *b_Jet_btagDeepFlavB;   //!
   TBranch        *b_Jet_btagDeepFlavCvB;   //!
   TBranch        *b_Jet_btagDeepFlavCvL;   //!
   TBranch        *b_Jet_btagDeepFlavQG;   //!
   TBranch        *b_Jet_chEmEF;   //!
   TBranch        *b_Jet_chFPV0EF;   //!
   TBranch        *b_Jet_chHEF;   //!
   TBranch        *b_Jet_eta;   //!
   TBranch        *b_Jet_hfsigmaEtaEta;   //!
   TBranch        *b_Jet_hfsigmaPhiPhi;   //!
   TBranch        *b_Jet_mass;   //!
   TBranch        *b_Jet_muEF;   //!
   TBranch        *b_Jet_muonSubtrFactor;   //!
   TBranch        *b_Jet_neEmEF;   //!
   TBranch        *b_Jet_neHEF;   //!
   TBranch        *b_Jet_phi;   //!
   TBranch        *b_Jet_pt;   //!
   TBranch        *b_Jet_puIdDisc;   //!
   TBranch        *b_Jet_qgl;   //!
   TBranch        *b_Jet_rawFactor;   //!
   TBranch        *b_Jet_bRegCorr;   //!
   TBranch        *b_Jet_bRegRes;   //!
   TBranch        *b_Jet_cRegCorr;   //!
   TBranch        *b_Jet_cRegRes;   //!
   TBranch        *b_Jet_electronIdx1;   //!
   TBranch        *b_Jet_electronIdx2;   //!
   TBranch        *b_Jet_hfadjacentEtaStripsSize;   //!
   TBranch        *b_Jet_hfcentralEtaStripSize;   //!
   TBranch        *b_Jet_jetId;   //!
   TBranch        *b_Jet_muonIdx1;   //!
   TBranch        *b_Jet_muonIdx2;   //!
   TBranch        *b_Jet_nElectrons;   //!
   TBranch        *b_Jet_nMuons;   //!
   TBranch        *b_Jet_puId;   //!
   TBranch        *b_Jet_nConstituents;   //!
   TBranch        *b_nTrigObj;   //!
   TBranch        *b_TrigObj_l1charge;   //!
   TBranch        *b_TrigObj_id;   //!
   TBranch        *b_TrigObj_l1iso;   //!
   TBranch        *b_TrigObj_filterBits;   //!
   TBranch        *b_TrigObj_pt;   //!
   TBranch        *b_TrigObj_eta;   //!
   TBranch        *b_TrigObj_phi;   //!
   TBranch        *b_TrigObj_l1pt;   //!
   TBranch        *b_TrigObj_l1pt_2;   //!
   TBranch        *b_TrigObj_l2pt;   //!
   TBranch        *b_PV_npvs;   //!
   TBranch        *b_PV_npvsGood;   //!
   TBranch        *b_fixedGridRhoFastjetAll;   //!
   TBranch        *b_PuppiMET_phi;   //!
   TBranch        *b_PuppiMET_pt;   //!
   TBranch        *b_HLT_Photon200;   //!
   TBranch        *b_HLT_Diphoton30_22_R9Id_OR_IsoCaloId_AND_HE_R9Id_Mass90;   //!

   Run2017BCDEF(TTree *tree=0);
   virtual ~Run2017BCDEF();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual bool     Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef Run2017BCDEF_cxx
Run2017BCDEF::Run2017BCDEF(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("SinglePhoton_Run2017BCDEF-UL2017_MiniAODv2_NanoAODv9-v1_NANOAOD.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("SinglePhoton_Run2017BCDEF-UL2017_MiniAODv2_NanoAODv9-v1_NANOAOD.root");
      }
      f->GetObject("Events",tree);

   }
   Init(tree);
}

Run2017BCDEF::~Run2017BCDEF()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t Run2017BCDEF::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t Run2017BCDEF::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->GetTreeNumber() != fCurrent) {
      fCurrent = fChain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void Run2017BCDEF::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("run", &run, &b_run);
   fChain->SetBranchAddress("luminosityBlock", &luminosityBlock, &b_luminosityBlock);
   fChain->SetBranchAddress("event", &event, &b_event);
   fChain->SetBranchAddress("nPhoton", &nPhoton, &b_nPhoton);
   fChain->SetBranchAddress("Photon_dEscaleDown", Photon_dEscaleDown, &b_Photon_dEscaleDown);
   fChain->SetBranchAddress("Photon_dEscaleUp", Photon_dEscaleUp, &b_Photon_dEscaleUp);
   fChain->SetBranchAddress("Photon_dEsigmaDown", Photon_dEsigmaDown, &b_Photon_dEsigmaDown);
   fChain->SetBranchAddress("Photon_dEsigmaUp", Photon_dEsigmaUp, &b_Photon_dEsigmaUp);
   fChain->SetBranchAddress("Photon_eCorr", Photon_eCorr, &b_Photon_eCorr);
   fChain->SetBranchAddress("Photon_energyErr", Photon_energyErr, &b_Photon_energyErr);
   fChain->SetBranchAddress("Photon_eta", Photon_eta, &b_Photon_eta);
   fChain->SetBranchAddress("Photon_hoe", Photon_hoe, &b_Photon_hoe);
   fChain->SetBranchAddress("Photon_mass", Photon_mass, &b_Photon_mass);
   fChain->SetBranchAddress("Photon_mvaID", Photon_mvaID, &b_Photon_mvaID);
   fChain->SetBranchAddress("Photon_mvaID_Fall17V1p1", Photon_mvaID_Fall17V1p1, &b_Photon_mvaID_Fall17V1p1);
   fChain->SetBranchAddress("Photon_pfRelIso03_all", Photon_pfRelIso03_all, &b_Photon_pfRelIso03_all);
   fChain->SetBranchAddress("Photon_pfRelIso03_chg", Photon_pfRelIso03_chg, &b_Photon_pfRelIso03_chg);
   fChain->SetBranchAddress("Photon_phi", Photon_phi, &b_Photon_phi);
   fChain->SetBranchAddress("Photon_pt", Photon_pt, &b_Photon_pt);
   fChain->SetBranchAddress("Photon_r9", Photon_r9, &b_Photon_r9);
   fChain->SetBranchAddress("Photon_sieie", Photon_sieie, &b_Photon_sieie);
   fChain->SetBranchAddress("Photon_charge", Photon_charge, &b_Photon_charge);
   fChain->SetBranchAddress("Photon_cutBased", Photon_cutBased, &b_Photon_cutBased);
   fChain->SetBranchAddress("Photon_cutBased_Fall17V1Bitmap", Photon_cutBased_Fall17V1Bitmap, &b_Photon_cutBased_Fall17V1Bitmap);
   fChain->SetBranchAddress("Photon_electronIdx", Photon_electronIdx, &b_Photon_electronIdx);
   fChain->SetBranchAddress("Photon_jetIdx", Photon_jetIdx, &b_Photon_jetIdx);
   fChain->SetBranchAddress("Photon_pdgId", Photon_pdgId, &b_Photon_pdgId);
   fChain->SetBranchAddress("Photon_vidNestedWPBitmap", Photon_vidNestedWPBitmap, &b_Photon_vidNestedWPBitmap);
   fChain->SetBranchAddress("Photon_electronVeto", Photon_electronVeto, &b_Photon_electronVeto);
   fChain->SetBranchAddress("Photon_isScEtaEB", Photon_isScEtaEB, &b_Photon_isScEtaEB);
   fChain->SetBranchAddress("Photon_isScEtaEE", Photon_isScEtaEE, &b_Photon_isScEtaEE);
   fChain->SetBranchAddress("Photon_mvaID_WP80", Photon_mvaID_WP80, &b_Photon_mvaID_WP80);
   fChain->SetBranchAddress("Photon_mvaID_WP90", Photon_mvaID_WP90, &b_Photon_mvaID_WP90);
   fChain->SetBranchAddress("Photon_pixelSeed", Photon_pixelSeed, &b_Photon_pixelSeed);
   fChain->SetBranchAddress("Photon_seedGain", Photon_seedGain, &b_Photon_seedGain);
   fChain->SetBranchAddress("nJet", &nJet, &b_nJet);
   fChain->SetBranchAddress("Jet_area", Jet_area, &b_Jet_area);
   fChain->SetBranchAddress("Jet_btagCSVV2", Jet_btagCSVV2, &b_Jet_btagCSVV2);
   fChain->SetBranchAddress("Jet_btagDeepB", Jet_btagDeepB, &b_Jet_btagDeepB);
   fChain->SetBranchAddress("Jet_btagDeepCvB", Jet_btagDeepCvB, &b_Jet_btagDeepCvB);
   fChain->SetBranchAddress("Jet_btagDeepCvL", Jet_btagDeepCvL, &b_Jet_btagDeepCvL);
   fChain->SetBranchAddress("Jet_btagDeepFlavB", Jet_btagDeepFlavB, &b_Jet_btagDeepFlavB);
   fChain->SetBranchAddress("Jet_btagDeepFlavCvB", Jet_btagDeepFlavCvB, &b_Jet_btagDeepFlavCvB);
   fChain->SetBranchAddress("Jet_btagDeepFlavCvL", Jet_btagDeepFlavCvL, &b_Jet_btagDeepFlavCvL);
   fChain->SetBranchAddress("Jet_btagDeepFlavQG", Jet_btagDeepFlavQG, &b_Jet_btagDeepFlavQG);
   fChain->SetBranchAddress("Jet_chEmEF", Jet_chEmEF, &b_Jet_chEmEF);
   fChain->SetBranchAddress("Jet_chFPV0EF", Jet_chFPV0EF, &b_Jet_chFPV0EF);
   fChain->SetBranchAddress("Jet_chHEF", Jet_chHEF, &b_Jet_chHEF);
   fChain->SetBranchAddress("Jet_eta", Jet_eta, &b_Jet_eta);
   fChain->SetBranchAddress("Jet_hfsigmaEtaEta", Jet_hfsigmaEtaEta, &b_Jet_hfsigmaEtaEta);
   fChain->SetBranchAddress("Jet_hfsigmaPhiPhi", Jet_hfsigmaPhiPhi, &b_Jet_hfsigmaPhiPhi);
   fChain->SetBranchAddress("Jet_mass", Jet_mass, &b_Jet_mass);
   fChain->SetBranchAddress("Jet_muEF", Jet_muEF, &b_Jet_muEF);
   fChain->SetBranchAddress("Jet_muonSubtrFactor", Jet_muonSubtrFactor, &b_Jet_muonSubtrFactor);
   fChain->SetBranchAddress("Jet_neEmEF", Jet_neEmEF, &b_Jet_neEmEF);
   fChain->SetBranchAddress("Jet_neHEF", Jet_neHEF, &b_Jet_neHEF);
   fChain->SetBranchAddress("Jet_phi", Jet_phi, &b_Jet_phi);
   fChain->SetBranchAddress("Jet_pt", Jet_pt, &b_Jet_pt);
   fChain->SetBranchAddress("Jet_puIdDisc", Jet_puIdDisc, &b_Jet_puIdDisc);
   fChain->SetBranchAddress("Jet_qgl", Jet_qgl, &b_Jet_qgl);
   fChain->SetBranchAddress("Jet_rawFactor", Jet_rawFactor, &b_Jet_rawFactor);
   fChain->SetBranchAddress("Jet_bRegCorr", Jet_bRegCorr, &b_Jet_bRegCorr);
   fChain->SetBranchAddress("Jet_bRegRes", Jet_bRegRes, &b_Jet_bRegRes);
   fChain->SetBranchAddress("Jet_cRegCorr", Jet_cRegCorr, &b_Jet_cRegCorr);
   fChain->SetBranchAddress("Jet_cRegRes", Jet_cRegRes, &b_Jet_cRegRes);
   fChain->SetBranchAddress("Jet_electronIdx1", Jet_electronIdx1, &b_Jet_electronIdx1);
   fChain->SetBranchAddress("Jet_electronIdx2", Jet_electronIdx2, &b_Jet_electronIdx2);
   fChain->SetBranchAddress("Jet_hfadjacentEtaStripsSize", Jet_hfadjacentEtaStripsSize, &b_Jet_hfadjacentEtaStripsSize);
   fChain->SetBranchAddress("Jet_hfcentralEtaStripSize", Jet_hfcentralEtaStripSize, &b_Jet_hfcentralEtaStripSize);
   fChain->SetBranchAddress("Jet_jetId", Jet_jetId, &b_Jet_jetId);
   fChain->SetBranchAddress("Jet_muonIdx1", Jet_muonIdx1, &b_Jet_muonIdx1);
   fChain->SetBranchAddress("Jet_muonIdx2", Jet_muonIdx2, &b_Jet_muonIdx2);
   fChain->SetBranchAddress("Jet_nElectrons", Jet_nElectrons, &b_Jet_nElectrons);
   fChain->SetBranchAddress("Jet_nMuons", Jet_nMuons, &b_Jet_nMuons);
   fChain->SetBranchAddress("Jet_puId", Jet_puId, &b_Jet_puId);
   fChain->SetBranchAddress("Jet_nConstituents", Jet_nConstituents, &b_Jet_nConstituents);
   fChain->SetBranchAddress("nTrigObj", &nTrigObj, &b_nTrigObj);
   fChain->SetBranchAddress("TrigObj_l1charge", TrigObj_l1charge, &b_TrigObj_l1charge);
   fChain->SetBranchAddress("TrigObj_id", TrigObj_id, &b_TrigObj_id);
   fChain->SetBranchAddress("TrigObj_l1iso", TrigObj_l1iso, &b_TrigObj_l1iso);
   fChain->SetBranchAddress("TrigObj_filterBits", TrigObj_filterBits, &b_TrigObj_filterBits);
   fChain->SetBranchAddress("TrigObj_pt", TrigObj_pt, &b_TrigObj_pt);
   fChain->SetBranchAddress("TrigObj_eta", TrigObj_eta, &b_TrigObj_eta);
   fChain->SetBranchAddress("TrigObj_phi", TrigObj_phi, &b_TrigObj_phi);
   fChain->SetBranchAddress("TrigObj_l1pt", TrigObj_l1pt, &b_TrigObj_l1pt);
   fChain->SetBranchAddress("TrigObj_l1pt_2", TrigObj_l1pt_2, &b_TrigObj_l1pt_2);
   fChain->SetBranchAddress("TrigObj_l2pt", TrigObj_l2pt, &b_TrigObj_l2pt);
   fChain->SetBranchAddress("PV_npvs", &PV_npvs, &b_PV_npvs);
   fChain->SetBranchAddress("PV_npvsGood", &PV_npvsGood, &b_PV_npvsGood);
   fChain->SetBranchAddress("fixedGridRhoFastjetAll", &fixedGridRhoFastjetAll, &b_fixedGridRhoFastjetAll);
   fChain->SetBranchAddress("PuppiMET_phi", &PuppiMET_phi, &b_PuppiMET_phi);
   fChain->SetBranchAddress("PuppiMET_pt", &PuppiMET_pt, &b_PuppiMET_pt);
   fChain->SetBranchAddress("HLT_Photon200", &HLT_Photon200, &b_HLT_Photon200);
   fChain->SetBranchAddress("HLT_Diphoton30_22_R9Id_OR_IsoCaloId_AND_HE_R9Id_Mass90", &HLT_Diphoton30_22_R9Id_OR_IsoCaloId_AND_HE_R9Id_Mass90, &b_HLT_Diphoton30_22_R9Id_OR_IsoCaloId_AND_HE_R9Id_Mass90);
   Notify();
}

bool Run2017BCDEF::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return true;
}

void Run2017BCDEF::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t Run2017BCDEF::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef Run2017BCDEF_cxx
