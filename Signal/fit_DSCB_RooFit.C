#include "RooRealVar.h"
#include "RooDataHist.h"
#include "RooArgList.h"
#include "RooPlot.h"
#include "RooCrystalBall.h"
#include "RooWorkspace.h"
#include "RooFitResult.h"
#include "RooFit.h"
#include "TSystem.h"
#include <TROOT.h>

#include <TFile.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TLegend.h>
#include <TStyle.h>
#include <iostream>

using namespace RooFit;

void fit_DSCB_RooFit()
{
    gStyle->SetOptStat(1); // no stats box
    gROOT->SetBatch(kTRUE);

    // ----------------------------------------------------------
    // 1. Load your signal histogram
    // ----------------------------------------------------------
    TFile *f = TFile::Open("CstarToGJ.root");
    if (!f) { std::cerr << "File not found!" << std::endl; return; }

    TH1F *h = (TH1F*)f->Get("hM_reco_selected");
    if (!h) { std::cerr << "Histogram not found!" << std::endl; return; }

    // rms and mean for initial guesses
    double xmin = h->GetXaxis()->GetXmin();
    double xmax = h->GetXaxis()->GetXmax();
    double peak = h->GetBinCenter(h->GetMaximumBin());


    // ----------------------------------------------------------
    // 2. Define RooFit observable (mass)
    // ----------------------------------------------------------
    RooRealVar x("x", "M_{#gamma+jet} [GeV]", xmin, xmax);

    // Convert TH1F to RooDataHist
    RooDataHist data("data", "dataset", x, h);

    // ----------------------------------------------------------
    // 3. Define DSCB parameters
    // ----------------------------------------------------------
    RooRealVar x0("x0", "mean", peak, peak-100, peak+100);

    RooRealVar sigmaL("sigmaL", "sigmaL", 50, 1, 200);
    RooRealVar sigmaR("sigmaR", "sigmaR", 50, 1, 200);

    RooRealVar alphaL("alphaL", "alphaL", 1.5, 0.1, 5.0);
    RooRealVar nL("nL", "nL", 5, 0.1, 20);

    RooRealVar alphaR("alphaR", "alphaR", 1.5, 0.1, 5.0);
    RooRealVar nR("nR", "nR", 5, 0.1, 20);

    // ----------------------------------------------------------
    // 4. Build the Double-Sided Crystal Ball PDF
    // ----------------------------------------------------------
    RooCrystalBall dscb("dscb", "Double-Sided Crystal Ball",
            x, x0, sigmaL, sigmaR,
            alphaL, nL, alphaR, nR);

    // ----------------------------------------------------------
    // 5. Fit the model to the data
    // ----------------------------------------------------------
    x.setRange("fitRange", 400, 2000);
    dscb.fitTo(data, SumW2Error(kTRUE), PrintLevel(-1), Range("fitRange"));

    //{{{    // ----------------------------------------------------------
    // Plot residual
    // ----------------------------------------------------------
    TH1F* hResidual = (TH1F*)h->Clone("hResidual");
    hResidual->Reset();
    hResidual->SetTitle("");
    hResidual->SetStats(0);

    RooArgSet obs(x);
    double Ntot = h->Integral();

    for (int i = 1; i <= h->GetNbinsX(); ++i){
        double binLow  = h->GetXaxis()->GetBinLowEdge(i);
        double binUp   = h->GetXaxis()->GetBinUpEdge(i);
        double dataVal = h->GetBinContent(i);
        double dataErr = h->GetBinError(i);
        double binCenter = h->GetBinCenter(i);
        double binWidth  = h->GetBinWidth(i);

        // Set integration range
        x.setRange("binRange", binLow, binUp);

        // Integrate PDF over bin
        RooAbsReal* integral = dscb.createIntegral(
                RooArgSet(x),
                NormSet(x),
                Range("binRange")
                );

        double fitVal = integral->getVal() * Ntot;



        double residual = dataVal - fitVal;

        hResidual->SetBinContent(i, residual);
        hResidual->SetBinError(i, dataErr);//??

        delete integral;

    }
    //}}}


    // ----------------------------------------------------------
    // 6. Plot the fit result
    // ----------------------------------------------------------
    TCanvas *c = new TCanvas("c", "DSCB Fit", 800, 600);

    TPad* pad1 = new TPad("pad1", "top", 0, 0.2, 1, 1.0);
    TPad* pad2 = new TPad("pad2", "bottom", 0, 0.0, 1, 0.2);

    pad1->SetBottomMargin(0.02);
    pad2->SetTopMargin(0.05);
    pad2->SetBottomMargin(0.3);
    pad1->SetLeftMargin(0.12);
    pad2->SetLeftMargin(0.12);

    pad1->Draw();
    pad2->Draw();

    pad1->cd();

    RooPlot *frame = x.frame(Bins(60), Title("DSCB fit to C* signal"));
    data.plotOn(frame, Name("data"),
            MarkerStyle(20),
            MarkerSize(0.8),
            MarkerColor(kBlack),
            LineColor(kBlack));

    dscb.plotOn(frame, Name("dscb"),
            LineColor(kRed),
            LineWidth(2));
    frame->GetXaxis()->SetTitle("");
    frame->GetYaxis()->SetTitle("Events");
    frame->GetXaxis()->SetLabelSize(0);
    frame->Draw();

    double chi2 = frame->chiSquare("dscb", "data");
    std::cout << "\nChi2/Ndof = " << chi2 << std::endl;


    TLegend *leg = new TLegend(0.60, 0.75, 0.88, 0.88);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->AddEntry(frame->getObject(0), "Signal (MC)", "lep"); // data points
    leg->AddEntry(frame->getObject(1), "DSCB fit",    "l");   // fit curve
    leg->Draw();

    TPaveText *pt = new TPaveText(0.60, 0.45, 0.88, 0.73, "NDC");
    pt->SetBorderSize(0);
    pt->SetFillColor(0);
    pt->SetTextAlign(12);
    pt->SetTextSize(0.03);

    pt->AddText(Form("Mean = %.1f #pm %.1f",  x0.getVal(),      x0.getError()));
    pt->AddText(Form("#sigma_{L} = %.1f #pm %.1f", sigmaL.getVal(), sigmaL.getError()));
    pt->AddText(Form("#sigma_{R} = %.1f #pm %.1f", sigmaR.getVal(), sigmaR.getError()));
    pt->AddText(Form("#alpha_{L} = %.2f #pm %.2f", alphaL.getVal(), alphaL.getError()));
    pt->AddText(Form("n_{L} = %.2f #pm %.2f",      nL.getVal(),     nL.getError()));
    pt->AddText(Form("#alpha_{R} = %.2f #pm %.2f", alphaR.getVal(), alphaR.getError()));
    pt->AddText(Form("n_{R} = %.2f #pm %.2f",      nR.getVal(),     nR.getError()));
    pt->AddText(Form("#chi^{2}/N_{dof} = %.6f", chi2));
    pt->Draw();

    pad2->cd();

    hResidual->GetYaxis()->SetTitle("Data-Fit");
    hResidual->GetYaxis()->SetTitleSize(0.12);
    hResidual->GetYaxis()->SetLabelSize(0.10);
    hResidual->GetYaxis()->SetTitleOffset(0.2);
    hResidual->GetXaxis()->SetTitle("m_{#gamma j} [GeV]");
    hResidual->GetXaxis()->SetTitleSize(0.12);
    hResidual->GetXaxis()->SetLabelSize(0.10);
    hResidual->GetXaxis()->SetTitleOffset(1.0);

    hResidual->SetMarkerStyle(10);
    hResidual->SetLineColor(kBlack);

    hResidual->Draw("PE");

    TLine* line = new TLine(
            x.getMin(),0,
            x.getMax(),0
            );
    line->SetLineStyle(2);
    line->Draw("same");


    c->SaveAs("DSCB_fit_RooFit.png");

    // ----------------------------------------------------------
    // 7. Print parameters
    // ----------------------------------------------------------
    std::cout << "\n===== DSCB Fit Parameters =====\n";
    x0.Print();
    sigmaL.Print();
    sigmaR.Print();
    alphaL.Print();
    nL.Print();
    alphaR.Print();
    nR.Print();



    // ----------------------------------------------------------
    // 8. Save everything into a RooWorkspace for Combine
    // ----------------------------------------------------------
    // Find the bin numbers corresponding to the range [700, 2000]
    int bin_start = h->GetXaxis()->FindBin(700.0);
    int bin_end   = h->GetXaxis()->FindBin(2000.0);

    // Integrate only within those bins
    double Nsig = h->Integral(bin_start, bin_end);


    RooRealVar nsig("nsig", "signal yield", Nsig, 0.0, 10.0*Nsig);
    RooExtendPdf sig_ext("sig_ext", "extended signal pdf", dscb, nsig);

    RooWorkspace ws("ws", "workspace");
    ws.import(x);
    //ws.import(dscb);
    //ws.import(data);
    ws.import(sig_ext); 

    ws.writeToFile("signal_DSCB_workspace.root");

    std::cout << "Signal Yield in [700, 2000]: " << Nsig << std::endl;
    std::cout << "\nWorkspace saved to: signal_DSCB_workspace.root\n";
    f->Close();
}
