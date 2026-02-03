void build_bkg_workspace() {

    TFile f("Run2017BCDEF_BG_ana.root");
    TF1* f_bkg = (TF1*)f.Get("bkgFit");

    double M = 1000.0;
    double xmin = M - 300.0;
    double xmax = M + 300.0;
    RooRealVar x("x", "m_{#gamma j}", xmin, xmax);

    RooRealVar P0("P0","P0", f_bkg->GetParameter(0));
    RooRealVar P1("P1","P1", f_bkg->GetParameter(1));
    RooRealVar P2("P2","P2", f_bkg->GetParameter(2));
    RooRealVar P3("P3","P3", f_bkg->GetParameter(3));

    P0.setConstant(false);
    P1.setConstant(false);
    P2.setConstant(false);
    P3.setConstant(false);

    RooGenericPdf bkg(
            "bkg",
            "pow(1 - x/13000, P1) / pow(x/13000, P2 + P3*log(x/13000))",
            RooArgSet(x, P1, P2, P3)
            );
    TFile fdata("Run2017BCDEF_BG_ana.root");

    TH1F* hData = (TH1F*)fdata.Get("hM"); 
    RooDataHist data_obs("data_obs", "data_obs", x, hData);

    // Find the bin numbers corresponding to the range [700, 3000]
    int bin_start = hData->GetXaxis()->FindBin(xmin);
    int bin_end   = hData->GetXaxis()->FindBin(xmax);

    // Integrate only within those bins
    double Nbkg = hData->Integral(bin_start, bin_end);
    std::cout << "Background Yield in [" << xmin << "," << xmax << "]: " << Nbkg << std::endl;

    RooRealVar nbkg("nbkg", "background yield", Nbkg, 0, 1e6);
    RooExtendPdf bkg_ext("bkg_ext", "extended background", bkg, nbkg);


    RooWorkspace ws("ws","background workspace");
    ws.import(x);
    ws.import(bkg_ext);
    ws.import(data_obs);

    TFile fout("bkg_workspace_M1000.root", "RECREATE");
    ws.Write("ws");
    fout.Close();
}

