#include "RooWorkspace.h"
#include "RooRealVar.h"
#include "RooDataHist.h"
#include "RooAbsPdf.h"
#include "RooArgList.h"
#include "RooFit.h"

using namespace RooFit;

void workspace_M1000() {

    // ----------------------------
    // Input files
    // ----------------------------
    TFile f_sig("/eos/user/h/hsiaoche/Signal/CstarToGJ_M1000_f0p1_13TeV_NANOAOD_v2/signal_DSCB_workspace.root");
    RooWorkspace* w_sig = (RooWorkspace*)f_sig.Get("ws");

    TFile f_bkg("/eos/user/h/hsiaoche/Data/bkg_workspace_M1000.root");
    RooWorkspace* w_bkg = (RooWorkspace*)f_bkg.Get("ws");

    TFile f_data("/eos/user/h/hsiaoche/Data/Run2017BCDEF_BG_ana.root");
    TH1F* hM = (TH1F*)f_data.Get("hM");

    // ----------------------------
    // Observable
    // ----------------------------
    RooRealVar* x = w_sig->var("x");
    //x->setRange(700, 3000);

    // ----------------------------
    // PDFs
    // ----------------------------
    RooAbsPdf* sig = w_sig->pdf("dscb");
    RooAbsPdf* bkg = w_bkg->pdf("bkg");

    // ----------------------------
    // Data
    // ----------------------------
    RooDataHist data_obs(
            "data_obs",
            "data_obs",
            RooArgList(*x),
            RooFit::Import(*hM)
            );

    // ----------------------------
    // Final workspace
    // ----------------------------
    RooWorkspace w("w", "combined workspace");
    w.import(*sig, RecycleConflictNodes());
    w.import(*bkg, RecycleConflictNodes());
    w.import(*x);
    w.import(data_obs);

    // ----------------------------
    // Write output
    // ----------------------------
    TFile fout("Cstar_GammaJet_M1000.root", "RECREATE");
    w.Write("w");
    fout.Close();

    std::cout << "Combined workspace written: Cstar_GammaJet_M1000.root\n";
}

