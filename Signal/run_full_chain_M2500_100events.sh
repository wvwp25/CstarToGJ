#!/bin/bash
# Description: Executes the full CMSSW Monte Carlo production chain (GEN-SIM -> DIGI-RECO -> MiniAOD -> NanoAOD)
#
# Usage: This script is executed by the Condor scheduler.
# Output: CstarToGJ_M2500_f0p1_13TeV_MINIAOD_1000events.root is written to /eos/user/h/hsiaoche/Signal/CstarToGJ_M2500_f0p1_13TeV_NANOAOD
#-----------------------------------------------------------------------------------------

# --- Job arguments from Condor ---
CLUSTER_ID=$1
PROC_ID=$2

# Events per job
EVENTS_PER_JOB=1000

# Unique random seed per job (to ensure different events)
SEED=$((999 + PROC_ID))

# Output naming per job
#TAG="job${PROC_ID}"
TAG="job${PROC_ID}_$(date +%s)"



# --- Configuration ---
# Your CMSSW path on AFS/EOS.
CMSSW_PROJECT_SRC="/afs/cern.ch/user/h/hsiaoche/CMSSW_10_6_30/src"
# Output directory on EOS
EOS_OUTPUT_DIR="/eos/user/h/hsiaoche/Signal/CstarToGJ_M1000-5000_f0p1_13TeV_NANOAOD"


# Fragment name
FRAGMENT_NAME="Configuration/GenProduction/python/CstarToGJ_M2500_f0p1_13TeV_pythia8_cfi"

# File names 
GENSIM_FILE="CstarToGJ_M2500_f0p1_13TeV_GENSIM_${TAG}.root"
DIGI2RAWHLT_PATH_FILE="CstarToGJ_M2500_f0p1_13TeV_DIGI2RAWHLT_${TAG}.root"
RECO_FILE="CstarToGJ_M2500_f0p1_13TeV_RECO_${TAG}.root"
MINIAOD_FILE="CstarToGJ_M2500_f0p1_13TeV_MINIAOD_${TAG}.root"
NANOAOD_FILE="CstarToGJ_M2500_f0p1_13TeV_NANOAOD_${TAG}.root"

# Configuration File Names
CFG_STEP0="step0_gensim_cfg.py"
CFG_STEP1="step1_digi2rawhlt_cfg.py"
CFG_STEP2="step2_reco_cfg.py"
CFG_STEP3="step3_miniaod_cfg.py"
CFG_STEP4="step4_nanoaod_cfg.py"

# Define the full EOS path for input/output files
# Convert 'file:...' to the plain EOS path for the 'eos rm' command
EOS_GENSIM_PATH="${EOS_OUTPUT_DIR}/${GENSIM_FILE}"
EOS_DIGI2RAWHLT_PATH="${EOS_OUTPUT_DIR}/${DIGI2RAWHLT_PATH_FILE}"
EOS_RECO_PATH="${EOS_OUTPUT_DIR}/${RECO_FILE}"
EOS_MINIAOD_PATH="${EOS_OUTPUT_DIR}/${MINIAOD_FILE}"
EOS_NANOAOD_PATH="${EOS_OUTPUT_DIR}/${NANOAOD_FILE}"

# File-in paths for cmsDriver must use the 'file:' prefix
FULL_GENSIM_PATH="file:${EOS_GENSIM_PATH}"
FULL_DIGI2RAWHLT_PATH="file:${EOS_DIGI2RAWHLT_PATH}"
FULL_RECO_PATH="file:${EOS_RECO_PATH}"
FULL_MINIAOD_PATH="file:${EOS_MINIAOD_PATH}"
FULL_NANOAOD_PATH="file:${EOS_NANOAOD_PATH}"




# --- Setup CMSSW Environment ---
echo "--- Setting up CMSSW environment ---"
cd $CMSSW_PROJECT_SRC
source /cvmfs/cms.cern.ch/cmsset_default.sh
eval `scramv1 runtime -sh`

# Return to the scratch directory to avoid writing temporary files to AFS
# The 'cd -' command takes you back to the directory you were just in (the scratch space).
cd - > /dev/null

# --- STEP 0: GEN-SIM (Pythia + GEANT4) ---
echo "--- Running Step 0: GEN-SIM ---"
cmsDriver.py ${FRAGMENT_NAME} \
    --fileout ${FULL_GENSIM_PATH} \
    --mc \
    --eventcontent RAWSIM \
    --datatier GEN-SIM \
    --conditions 106X_mc2017_realistic_v6 \
    --beamspot Realistic25ns13TeVEarly2017Collision \
    --step GEN,SIM \
    --geometry DB:Extended \
    --era Run2_2017 \
    --customise Configuration/DataProcessing/Utils.addMonitoring \
    --python_filename ${CFG_STEP0} \
    --no_exec -n ${EVENTS_PER_JOB}\
    --customise_commands "process.RandomNumberGeneratorService.generator.initialSeed=${SEED}"

# Execute the GEN-SIM configuration (writes directly to EOS)
cmsRun ${CFG_STEP0}


# --- STEP 1: DIGI2RAW-HLT (DR) ---
echo "--- Running Step 1: DIGI2RAW-HLT ---"
cmsDriver.py step1 \
    --filein ${FULL_GENSIM_PATH} \
    --fileout ${FULL_DIGI2RAWHLT_PATH} \
    --mc \
    --eventcontent FEVTDEBUGHLT \
    --datatier GEN-SIM-DIGI-RAW \
    --conditions 106X_mc2017_realistic_v6 \
    --beamspot Realistic25ns13TeVEarly2017Collision \
    --step DIGI,L1,DIGI2RAW,HLT:@relval2017 \
    --nThreads 8 \
    --geometry DB:Extended \
    --era Run2_2017 \
    --customise Configuration/DataProcessing/Utils.addMonitoring \
    --python_filename ${CFG_STEP1} \
    --no_exec -n ${EVENTS_PER_JOB}

# Execute the DIGI2RAW-HLT configuration (writes directly to EOS)
cmsRun ${CFG_STEP1}

# *** DELETION 1: Delete GEN-SIM file after successful use in Step 1 ***
echo "--- Cleaning up GEN-SIM file ---"
rm -f ${EOS_OUTPUT_DIR}/${GENSIM_FILE}




# --- STEP 2: RECO ---
echo "--- Running Step 2: RECO ---"
# Input is the DIGI2RAW-HLT file on EOS; Output is the RECO file on EOS
cmsDriver.py step2 \
    --filein ${FULL_DIGI2RAWHLT_PATH} \
    --fileout ${FULL_RECO_PATH} \
    --mc \
    --eventcontent AODSIM \
    --runUnscheduled \
    --datatier AODSIM \
    --conditions 106X_mc2017_realistic_v6 \
    --beamspot Realistic25ns13TeVEarly2017Collision \
    --step RAW2DIGI,L1Reco,RECO,RECOSIM,EI \
    --nThreads 8 \
    --geometry DB:Extended \
    --era Run2_2017 \
    --customise Configuration/DataProcessing/Utils.addMonitoring \
    --python_filename ${CFG_STEP2} \
    --no_exec -n ${EVENTS_PER_JOB}

# Execute the RECO configuration (reads from EOS, writes to EOS)
cmsRun ${CFG_STEP2}


# *** DELETION 2: Delete DIGI2RAW-HLT file after successful use in Step 2 ***
echo "--- Cleaning up DIGI2RAW-HLT file ---"
rm -f ${EOS_OUTPUT_DIR}/${DIGI2RAWHLT_PATH_FILE}


# --- STEP 3: MINIAOD PRODUCTION (Final Analysis Format) ---
echo "--- Running Step 3: MiniAOD ---"
# Input is the RECO file on EOS; Output is the MiniAOD file on EOS
cmsDriver.py step3 \
    --filein ${FULL_RECO_PATH} \
    --fileout ${FULL_MINIAOD_PATH} \
    --mc \
    --eventcontent MINIAODSIM \
    --runUnscheduled \
    --datatier MINIAODSIM \
    --conditions 106X_mc2017_realistic_v6 \
    --step PAT \
    --nThreads 8 \
    --geometry DB:Extended\
    --era Run2_2017 \
    --customise Configuration/DataProcessing/Utils.addMonitoring \
    --python_filename ${CFG_STEP3} \
    --no_exec -n ${EVENTS_PER_JOB}

# Execute the MiniAOD configuration (reads from EOS, writes to EOS)
cmsRun ${CFG_STEP3}

# *** DELETION 3: Delete RECO file after successful use in Step 3 ***
echo "--- Cleaning up RECO file ---"
rm -f ${EOS_OUTPUT_DIR}/${RECO_FILE}


# --- STEP 4: NANOAOD PRODUCTION (Final Analysis Nano format) ---
echo "--- Running Step 4: NanoAOD ---"

# Create NanoAOD configuration
cmsDriver.py step4 \
    --filein ${FULL_MINIAOD_PATH} \
    --fileout ${FULL_NANOAOD_PATH} \
    --mc \
    --eventcontent NANOAODSIM \
    --datatier NANOAODSIM \
    --conditions 106X_mc2017_realistic_v6 \
    --step NANO \
    --nThreads 8 \
    --era Run2_2017,run2_nanoAOD_106Xv1 \
    --python_filename ${CFG_STEP4} \
    --no_exec -n ${EVENTS_PER_JOB}

# Execute NanoAOD config
cmsRun ${CFG_STEP4}

# *** DELETION 4: Delete MiniAOD file after successful use in Step 4 ***
echo "--- Cleaning up MiniAOD file ---"
rm -f ${EOS_OUTPUT_DIR}/${MINIAOD_FILE}

echo "--- Congratulations! NanoAOD production finished successfully! Final file: ${FULL_NANOAOD_PATH} ---"



