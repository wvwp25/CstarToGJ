#include <iostream>

void check_workspace_ranges(
    const char* filename =
        "/eos/user/h/hsiaoche/workspace/workspace_M1000_f1p0.root") {
  gSystem->Load("libHiggsAnalysisCombinedLimit");

  TFile f(filename);
  RooWorkspace* w = static_cast<RooWorkspace*>(f.Get("w"));
  if (!w) {
    w = static_cast<RooWorkspace*>(f.Get("ws"));
  }
  if (!w) {
    std::cout << "workspace not found in " << filename << std::endl;
    f.ls();
    return;
  }

  const char* names[] = {"P1", "P2", "P3", "bkg_norm"};
  for (const char* name : names) {
    RooRealVar* v = w->var(name);
    if (!v) {
      std::cout << name << " not found" << std::endl;
      continue;
    }

    std::cout
      << name
      << " val=" << v->getVal()
      << " min=" << v->getMin()
      << " max=" << v->getMax()
      << " hasMin=" << v->hasMin()
      << " hasMax=" << v->hasMax()
      << " constant=" << v->isConstant()
      << std::endl;
  }
}
