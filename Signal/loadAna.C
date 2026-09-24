{
  gInterpreter->AddIncludePath("/afs/cern.ch/user/h/hsiaoche/.local/lib/python3.9/site-packages/correctionlib/include");
  gInterpreter->AddIncludePath("/eos/user/h/hsiaoche/Signal/uncertainty_sources/jerc-application-tutorial");

  gSystem->Load("/afs/cern.ch/user/h/hsiaoche/.local/lib/python3.9/site-packages/correctionlib/lib/libcorrectionlib.so");

  // Load helpers compiled with ordinary g++, avoiding ROOT dictionary generation.
  gSystem->Load("/eos/user/h/hsiaoche/Signal/uncertainty_sources/jerc-application-tutorial/libSignalJecHelpers.so");
  gSystem->AddLinkedLibs(" /eos/user/h/hsiaoche/Signal/uncertainty_sources/jerc-application-tutorial/libSignalJecHelpers.so");
gROOT->LoadMacro("CstarToGJ_M1000_f0p1_13TeV_NANOAOD_ana.C");

  gInterpreter->ProcessLine(R"cpp(
  {
      CstarToGJ_M1000_f0p1_13TeV_NANOAOD_ana t;
      t.Loop();
  }
  )cpp");

}
