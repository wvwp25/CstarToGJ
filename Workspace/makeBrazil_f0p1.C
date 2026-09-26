#include <vector>
#include <string>
#include <iostream>
#include <cmath>

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TGraphAsymmErrors.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TStyle.h"

struct Lim {
    double obs, m2, m1, exp, p1, p2; // obs, -2, -1, median, +1, +2
};

Lim readLim(const std::string& fn){
    TFile* f = TFile::Open(fn.c_str(),"READ");
    TTree* t = (TTree*)f->Get("limit");

    Double_t limit=0;
    Float_t  q = 0; 
    t->SetBranchAddress("limit",&limit);
    t->SetBranchAddress("quantileExpected",&q);

    Lim L; L.obs=L.m2=L.m1=L.exp=L.p1=L.p2 = NAN;

    for(Long64_t i=0;i<t->GetEntries();i++){
        t->GetEntry(i);
        if(q<0) L.obs = limit;
        else if(std::abs(q-0.025)<1e-6) L.m2 = limit;
        else if(std::abs(q-0.16 )<1e-6) L.m1 = limit;
        else if(std::abs(q-0.5  )<1e-6) L.exp = limit;
        else if(std::abs(q-0.84 )<1e-6) L.p1 = limit;
        else if(std::abs(q-0.975)<1e-6) L.p2 = limit;
    }

    f->Close();
    delete f;

    return L;
}

void makeBrazil_f0p1(){
    gStyle->SetOptStat(0);
    const std::string workDir = "/eos/user/h/hsiaoche/workspace";

    std::vector<double> mGeV = {1000,1200,1400,1600,1800,2000,2200,2400,2600,2800,3000};

    // Production cross section used to normalize the forced c* -> c gamma samples.
    std::vector<double> sigma_pb = {
        1.291e-02,   // 1000
        4.822e-03,
        2.075e-03,
        9.501e-04,
        5.415e-04,
        2.360e-04,    // 2000
        1.459e-04,
        7.103e-05,
        4.109e-05,
        2.135e-05,
        1.255e-05
    };

    // Use B(c* -> c gamma) = 100% for the theory curve.
    constexpr double br_cgamma = 1.0;

    const int N = (int)mGeV.size();
    std::vector<double> x(N), xerr(N,0.0);
    std::vector<double> yObs(N), yExp(N), yTh(N);
    std::vector<double> y1(N), eyl1(N), eyh1(N);
    std::vector<double> y2(N), eyl2(N), eyh2(N);

    for(int i=0;i<N;i++){
        int m = (int)mGeV[i];
        std::string fn = Form("%s/higgsCombine_f0p1.AsymptoticLimits.mH%d.root", workDir.c_str(), m);
        Lim L = readLim(fn);

        double norm = sigma_pb[i];
        double th = sigma_pb[i] * br_cgamma;

        x[i]    = mGeV[i]/1000.0;
        yObs[i] = L.obs * norm;
        yExp[i] = L.exp * norm;

        y1[i]   = L.exp * norm;
        eyl1[i] = (L.exp - L.m1) * norm;
        eyh1[i] = (L.p1  - L.exp) * norm;

        y2[i]   = L.exp * norm;
        eyl2[i] = (L.exp - L.m2) * norm;
        eyh2[i] = (L.p2  - L.exp) * norm;

        yTh[i]  = th;
    }

    auto gr2 = new TGraphAsymmErrors(N, x.data(), y2.data(), xerr.data(), xerr.data(), eyl2.data(), eyh2.data());
    auto gr1 = new TGraphAsymmErrors(N, x.data(), y1.data(), xerr.data(), xerr.data(), eyl1.data(), eyh1.data());
    auto grE = new TGraph(N, x.data(), yExp.data());
    auto grO = new TGraph(N, x.data(), yObs.data());
    auto grT = new TGraph(N, x.data(), yTh.data());

    gr2->SetFillColor(kOrange);
    gr1->SetFillColor(kGreen+1);

    grE->SetLineStyle(2);
    grE->SetLineWidth(3);
    grE->SetLineColor(kBlue);

    grO->SetLineWidth(3);
    grO->SetLineColor(kBlack);
    grO->SetMarkerStyle(20);
    grO->SetMarkerSize(0.8);

    grT->SetLineWidth(3);
    grT->SetLineColor(kRed+1);

    auto c = new TCanvas("c","c",900,700);
    c->SetLogy();
    c->SetLeftMargin(0.12);
    c->SetRightMargin(0.05);
    c->SetTopMargin(0.08);
    c->SetBottomMargin(0.12);

    gPad->SetTickx(1);
    gPad->SetTicky(1);

    gr2->SetTitle("");
    gr2->GetXaxis()->SetTitle("Resonance mass [TeV]");
    gr2->GetYaxis()->SetTitle("#sigma #times B [pb]");
    gr2->GetXaxis()->SetLimits(0.8, 3.2);
    gr2->SetMinimum(1e-5);
    gr2->SetMaximum(100);

    gr2->Draw("A3");
    gr1->Draw("3 SAME");
    grE->Draw("L SAME");
    //grO->Draw("LP SAME");
    grT->Draw("L SAME");

    auto leg = new TLegend(0.55,0.62,0.88,0.86);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    //leg->AddEntry(grO,"Observed","lp");
    leg->AddEntry(grE,"Expected","l");
    leg->AddEntry(gr1,"Expected #pm 1 std. deviation","f");
    leg->AddEntry(gr2,"Expected #pm 2 std. deviation","f");
    leg->AddEntry(grT,"Excited quark (f=0.1)","l");
    leg->Draw();

    TLatex lat; lat.SetNDC(true); lat.SetTextFont(52); lat.SetTextSize(0.035);
    lat.DrawLatex(0.12,0.93,"Private work (CMS simulation)");
    lat.SetTextFont(42);
    lat.SetTextAlign(31);
    lat.DrawLatex(0.95,0.93,"41.8 fb^{-1} (13 TeV)");
    lat.SetTextAlign(11);
    lat.DrawLatex(0.80,0.81,"c* #rightarrow c#gamma");

    c->SaveAs((workDir + "/brazil_sigmaB_f0p1.png").c_str());
}
