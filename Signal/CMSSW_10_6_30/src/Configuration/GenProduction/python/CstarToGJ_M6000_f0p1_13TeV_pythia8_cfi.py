import FWCore.ParameterSet.Config as cms

from Configuration.Generator.Pythia8CommonSettings_cfi import *
from Configuration.Generator.Pythia8CUEP8M1Settings_cfi import *

generator = cms.EDFilter("Pythia8GeneratorFilter",
    comEnergy = cms.double(13000.0),
    crossSection = cms.untracked.double(1.0),
    filterEfficiency = cms.untracked.double(1.0),
    maxEventsToPrint = cms.untracked.int32(0),
    pythiaHepMCVerbosity = cms.untracked.bool(False),
    pythiaPylistVerbosity = cms.untracked.int32(0),
    PythiaParameters = cms.PSet(
        pythia8CommonSettingsBlock,
        pythia8CUEP8M1SettingsBlock,
        processParameters = cms.vstring(
            'ExcitedFermion:cg2cStar = on',
            '4000004:m0 = 6000.',
            '4000004:onMode = off',
            '4000004:onIfMatch = 22 4',
            'ExcitedFermion:Lambda = 6000.',
            'ExcitedFermion:coupFprime = 0.1', 
            'ExcitedFermion:coupF = 0.1', 
            'ExcitedFermion:coupFcol = 0.1' 
        ),
        parameterSets = cms.vstring(
            'pythia8CommonSettings',
            'pythia8CUEP8M1Settings',
            'processParameters',
        )
    )
)
ProductionFilterSequence = cms.Sequence(generator)
