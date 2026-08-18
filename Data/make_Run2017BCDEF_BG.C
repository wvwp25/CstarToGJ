#define Run2017BCDEF_cxx
#include "Invariant_Mass_gJet_M_f/Run2017BCDEF.h"

#include <cmath>
#include <iostream>
#include <vector>

#include "TFile.h"
#include "TH1F.h"
#include "TLorentzVector.h"
#include "TTree.h"

// The generated header declares this virtual method, but its original .C file
// is no longer present. The producer below owns the event loop; this no-op
// definition only completes the generated selector's vtable.
void Run2017BCDEF::Loop() {}

// Build the observed gamma+jet mass histogram from the complete 2017 data
// sample.  Background fitting is intentionally not done here; it belongs in
// Bkg_model.C.
void make_Run2017BCDEF_BG(
    const char *inputFileName =
        "SinglePhoton_Run2017BCDEF-UL2017_MiniAODv2_NanoAODv9-v1_NANOAOD.root",
    const char *outputFileName = "Run2017BCDEF_BG.root") {
  TFile *input = TFile::Open(inputFileName, "READ");
  if (!input || input->IsZombie()) {
    Error("make_Run2017BCDEF_BG", "Cannot open %s", inputFileName);
    delete input;
    return;
  }

  TTree *events = nullptr;
  input->GetObject("Events", events);
  if (!events) {
    Error("make_Run2017BCDEF_BG", "%s does not contain the Events tree",
          inputFileName);
    input->Close();
    delete input;
    return;
  }

  Run2017BCDEF data(events);

  // 1000 bins over 0--4 TeV gives 4 GeV bins and retains the event-by-event
  // distribution needed for the upper sideband of the M3000 hypothesis.
  TH1F massHistogram(
      "hM",
      "Invariant Mass #gamma + Jet;m_{#gamma+jet} [GeV];Events",
      1000, 0.0, 4000.0);
  massHistogram.Sumw2();

  const Long64_t entries = data.fChain->GetEntries();
  Long64_t passedTrigger = 0;
  Long64_t passedPhoton = 0;
  Long64_t passedJet = 0;

  for (Long64_t entry = 0; entry < entries; ++entry) {
    const Long64_t localEntry = data.LoadTree(entry);
    if (localEntry < 0) break;
    if (data.GetEntry(entry) <= 0) continue;

    // The dataset is selected with the single-photon trigger used by the
    // offline photon pT threshold below.
    if (!data.HLT_Photon200) continue;
    ++passedTrigger;

    int photonIndex = -1;
    float leadingPhotonPt = -1.0f;
    for (UInt_t photon = 0; photon < data.nPhoton; ++photon) {
      if (data.Photon_pt[photon] < 240.0f) continue;
      if (std::abs(data.Photon_eta[photon]) >= 1.4442f) continue;
      if (data.Photon_cutBased[photon] < 2) continue;
      if (!data.Photon_electronVeto[photon]) continue;
      if (data.Photon_pt[photon] > leadingPhotonPt) {
        photonIndex = photon;
        leadingPhotonPt = data.Photon_pt[photon];
      }
    }
    if (photonIndex < 0) continue;
    ++passedPhoton;

    TLorentzVector photonP4;
    photonP4.SetPtEtaPhiM(data.Photon_pt[photonIndex],
                          data.Photon_eta[photonIndex],
                          data.Photon_phi[photonIndex], 0.0);

    int jetIndex = -1;
    float leadingJetPt = -1.0f;
    for (UInt_t jet = 0; jet < data.nJet; ++jet) {
      if (data.Jet_pt[jet] < 170.0f) continue;
      if (std::abs(data.Jet_eta[jet]) >= 2.4f) continue;
      if (data.Jet_jetId[jet] < 6) continue;
      if (data.Jet_btagDeepFlavCvB[jet] < 0.340f) continue;
      if (data.Jet_btagDeepFlavCvL[jet] < 0.085f) continue;

      TLorentzVector jetP4;
      jetP4.SetPtEtaPhiM(data.Jet_pt[jet], data.Jet_eta[jet],
                         data.Jet_phi[jet], data.Jet_mass[jet]);
      if (photonP4.DeltaR(jetP4) <= 1.1) continue;

      if (data.Jet_pt[jet] > leadingJetPt) {
        jetIndex = jet;
        leadingJetPt = data.Jet_pt[jet];
      }
    }
    if (jetIndex < 0) continue;
    ++passedJet;

    TLorentzVector jetP4;
    jetP4.SetPtEtaPhiM(data.Jet_pt[jetIndex], data.Jet_eta[jetIndex],
                       data.Jet_phi[jetIndex], data.Jet_mass[jetIndex]);
    massHistogram.Fill((photonP4 + jetP4).M());
  }

  TFile output(outputFileName, "RECREATE");
  if (output.IsZombie()) {
    Error("make_Run2017BCDEF_BG", "Cannot create %s", outputFileName);
    return;
  }
  massHistogram.Write("hM");
  output.Close();

  std::cout << "Processed entries: " << entries << '\n'
            << "Passed HLT_Photon200: " << passedTrigger << '\n'
            << "Passed photon selection: " << passedPhoton << '\n'
            << "Passed jet selection / hM entries: " << passedJet << '\n'
            << "hM overflow above 4 TeV: "
            << massHistogram.GetBinContent(massHistogram.GetNbinsX() + 1)
            << '\n'
            << "Wrote " << outputFileName << std::endl;
}
