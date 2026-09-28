// File: plot_xsec_cstar_bstar.C
// Usage:
//   root -l -q 'plot_xsec_cstar_bstar.C(1.0,true)'
//   root -l -q 'plot_xsec_cstar_bstar.C(1.0,false)'
//
// BR is applied as a common factor (default BR=1).
// If y_in_fb=true, convert pb -> fb by *1000 and label axis [fb].
//
// Draws 3 curves:
//   1) c*  f=0.1  (your original numbers)
//   2) c*  f=1.0
//   3) b*  f=1.0

#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TAxis.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TStyle.h"

void plot_xsec_cstar_bstar(double BR = 1.0, bool y_in_fb = true)
{
  // -----------------------------
  // Style (CMS-ish)
  // -----------------------------
  gStyle->SetOptStat(0);
  gStyle->SetTitle(0);
  gStyle->SetLineWidth(2);
  gStyle->SetFrameLineWidth(2);
  gStyle->SetPadLeftMargin(0.12);
  gStyle->SetPadBottomMargin(0.12);
  gStyle->SetPadRightMargin(0.05);
  gStyle->SetPadTopMargin(0.08);

  const int N = 4;
  double mTeV[N] = {1.0, 1.5, 2.0, 2.5};
  double xerr[N] = {0,0,0,0};

  // -----------------------------
  // Inputs (sigma in pb)
  // -----------------------------
  // c* f=0.1  (your original numbers)
  double c01_pb[N]    = {1.291e-02, 1.479e-03, 2.360e-04, 5.052e-05};
  double c01_errpb[N] = {2.863e-04, 3.216e-05, 5.162e-06, 1.083e-06};

  // c* f=1.0
  double c10_pb[N]    = {1.272e+00, 1.368e-01, 2.596e-02, 5.793e-03};
  double c10_errpb[N] = {2.730e-02, 2.883e-03, 1.686e-03, 3.898e-04};

  // b* f=1.0
  double b10_pb[N]    = {2.154e-01, 2.168e-02, 3.553e-03, 7.210e-04};
  double b10_errpb[N] = {4.629e-03, 4.548e-04, 7.347e-05, 1.489e-05};

  auto scale = [&](double v_pb){ return (y_in_fb ? v_pb*1000.0 : v_pb); };

  // Convert to sigma*BR and optional fb
  double c01_y[N], c01_yerr[N], c10_y[N], c10_yerr[N], b10_y[N], b10_yerr[N];
  for (int i=0;i<N;++i){
    c01_y[i]    = scale(c01_pb[i]    * BR);
    c01_yerr[i] = scale(c01_errpb[i] * BR);

    c10_y[i]    = scale(c10_pb[i]    * BR);
    c10_yerr[i] = scale(c10_errpb[i] * BR);

    b10_y[i]    = scale(b10_pb[i]    * BR);
    b10_yerr[i] = scale(b10_errpb[i] * BR);
  }

  // -----------------------------
  // Canvas
  // -----------------------------
  TCanvas *c = new TCanvas("c","c",900,700);
  c->SetLogy();

  // Build graphs
  TGraphErrors *gr_c01 = new TGraphErrors(N, mTeV, c01_y, xerr, c01_yerr);
  TGraphErrors *gr_c10 = new TGraphErrors(N, mTeV, c10_y, xerr, c10_yerr);
  TGraphErrors *gr_b10 = new TGraphErrors(N, mTeV, b10_y, xerr, b10_yerr);

  // Style each curve (match CMS-like multi-curve look)
  gr_c01->SetMarkerStyle(20);
  gr_c01->SetMarkerSize(1.2);
  gr_c01->SetLineWidth(2);
  gr_c01->SetMarkerColor(kGreen-2);
  gr_c01->SetLineColor(kGreen-2);

  gr_c10->SetMarkerStyle(21);
  gr_c10->SetMarkerSize(1.2);
  gr_c10->SetLineWidth(2);
  gr_c10->SetMarkerColor(kAzure+2);
  gr_c10->SetLineColor(kAzure+2);

  gr_b10->SetMarkerStyle(22);
  gr_b10->SetMarkerSize(1.2);
  gr_b10->SetLineWidth(2);
  gr_b10->SetMarkerColor(kPink+8);
  gr_b10->SetLineColor(kPink+8);

  // Axes: use first graph as the frame
  gr_c01->SetTitle("");
  gr_c01->GetXaxis()->SetTitle("Mass [TeV]");
  gr_c01->GetYaxis()->SetTitle(y_in_fb ? "#sigma B [fb]" : "#sigma B [pb]");
  gr_c01->GetXaxis()->SetTitleSize(0.05);
  gr_c01->GetYaxis()->SetTitleSize(0.05);
  gr_c01->GetXaxis()->SetLabelSize(0.045);
  gr_c01->GetYaxis()->SetLabelSize(0.045);
  gr_c01->GetYaxis()->SetTitleOffset(1.0);
  // x-range
  gr_c01->GetXaxis()->SetLimits(0.9, 2.6);

  // y-range: auto-ish for these three curves
  // (in fb: b* ~0.7 fb at 2.5 TeV, c* f1 ~1270 fb at 1 TeV)
  if (y_in_fb) { gr_c01->SetMinimum(3e-2); gr_c01->SetMaximum(3e3); }
  else        { gr_c01->SetMinimum(3e-5); gr_c01->SetMaximum(3.0); }

  gr_c01->Draw("AP");          // axes + points
  gr_c01->Draw("LP SAME");
  gr_c10->Draw("LP SAME");
  gr_b10->Draw("LP SAME");

  // Legend
  TLegend *leg = new TLegend(0.55, 0.68, 0.88, 0.86);
  leg->SetBorderSize(0);
  leg->SetFillStyle(0);
  leg->SetTextSize(0.035);
  leg->AddEntry(gr_c10, Form("c*  f=1.0  "), "lp");
  leg->AddEntry(gr_c01, Form("c*  f=0.1  "), "lp");
  leg->AddEntry(gr_b10, Form("b*  f=1.0  "), "lp");
  leg->Draw();

  // CMS-like text
  TLatex lat;
  lat.SetNDC(true);
  lat.SetTextFont(42);
  lat.SetTextSize(0.055);
  lat.DrawLatex(0.14, 0.93, "");
  lat.SetTextSize(0.04);
  lat.DrawLatex(0.85, 0.93, "13 TeV");

        {
            gPad->Update();
            if (auto *statsBox = gPad->GetPrimitive("stats")) statsBox->Delete();
            TLatex privateWorkLabel;
            privateWorkLabel.SetNDC();
            privateWorkLabel.SetTextFont(52);
            privateWorkLabel.SetTextSize(0.035);
            privateWorkLabel.DrawLatex(0.14, 0.93, "Private work (CMS simulation)");
        }
  c->SaveAs("sigmaB_cstar_bstar.png");
}
