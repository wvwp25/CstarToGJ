void listBranches() {
  TFile *f = TFile::Open("root://cmsxrootd.fnal.gov//store/data/Run2017B/SinglePhoton/NANOAOD/UL2017_MiniAODv2_JMENanoAODv9-v1/2820000/9A35748E-C5E2-B640-9D70-33F765E9A68A.root");
  if (!f || f->IsZombie()) { printf("File open failed\n"); return; }
  TTree *t = (TTree*)f->Get("Events");
  if (!t) { printf("No Events tree found\n"); return; }

  std::ofstream outfile("branches_v9.txt");
  TObjArray *branches = t->GetListOfBranches();
  for (int i = 0; i < branches->GetEntries(); ++i) {
    outfile << branches->At(i)->GetName() << std::endl;
  }
  outfile.close();
  printf(" Wrote %d branches to branches.txt\n", branches->GetEntries());
  f->Close();
}

