void loadAna(const char *inputFile,
             const char *outputFile,
             const char *analysisDirectory,
             double crossSectionPb)
{
  gInterpreter->AddIncludePath("/afs/cern.ch/user/h/hsiaoche/.local/lib/python3.9/site-packages/correctionlib/include");
  gInterpreter->AddIncludePath("/eos/user/h/hsiaoche/Signal/uncertainty_sources/jerc-application-tutorial");

  gSystem->Load("/afs/cern.ch/user/h/hsiaoche/.local/lib/python3.9/site-packages/correctionlib/lib/libcorrectionlib.so");

  // Load helpers compiled with ordinary g++, avoiding ROOT dictionary generation.
  gSystem->Load("/eos/user/h/hsiaoche/Signal/uncertainty_sources/jerc-application-tutorial/libSignalJecHelpers.so");
  gSystem->AddLinkedLibs(" /eos/user/h/hsiaoche/Signal/uncertainty_sources/jerc-application-tutorial/libSignalJecHelpers.so");
  const TString analysisMacro = TString::Format(
      "%s/CstarToGJ_analysis.C", analysisDirectory);
  const int loadStatus = gROOT->LoadMacro(analysisMacro);
  if (loadStatus < 0) {
    Error("loadAna", "Failed to load %s", analysisMacro.Data());
    gSystem->Exit(1);
  }

  TString escapedInput(inputFile);
  TString escapedOutput(outputFile);
  escapedInput.ReplaceAll("\\", "\\\\").ReplaceAll("\"", "\\\"");
  escapedOutput.ReplaceAll("\\", "\\\\").ReplaceAll("\"", "\\\"");

  const TString command = TString::Format(
      "runCstarToGJAnalysis(\"%s\", \"%s\", %.17g);",
      escapedInput.Data(), escapedOutput.Data(), crossSectionPb);
  gInterpreter->ProcessLine(command);

}
