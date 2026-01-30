import os
#Reading the file names from the list
file_in = open("SinglePhoton_Run2017D-UL2017_MiniAODv2_NanoAODv9-v1_NANOAOD.txt","r") #Path of the file list
Lines = file_in.read().splitlines();
file_in.close()

# 1. The absolute path where you run cmsenv successfully (your CMSSW src area)
CMSSW_SRC_PATH = "/afs/cern.ch/user/h/hsiaoche/CMSSW_10_6_30/src"
# 2. The absolute path where your data/job scripts/text files are located
DATA_DIR_PATH = "/afs/cern.ch/user/h/hsiaoche/work/Data"

# Define the target subdirectory path
SUBDIR = "SinglePhoton_Run2017D-UL2017_MiniAODv2_NanoAODv9-v1_NANOAOD"

# Ensure the subdirectory itself exists (optional, but safer)
os.system(f"mkdir -p {SUBDIR}")

#Creating the directories for logFile, outFiles and errorFiles
os.system(f"mkdir -p {SUBDIR}/logFiles")
os.system(f"mkdir -p {SUBDIR}/outFiles")
os.system(f"mkdir -p {SUBDIR}/errorFiles")

submit_file = open("finalSubmit_.sh","w")
submit_file.writelines("#!/bin/sh \n")

#Making the exe and condor submit files
count = 0
for line in Lines:
    #line.replace('\n','')
    line.strip()
    print(line)
    filename1 =f"{SUBDIR}/job_"+str(count)+".sh"
    file_out1 = open(filename1,"w")
    out_rootFileName = "/eos/user/h/hsiaoche/Data/SinglePhoton_Run2017D-UL2017_MiniAODv2_NanoAODv9-v1_NANOAOD/"+str(count)+".root" #Output file name
   # out_rootFileName = "SinglePhoton_Run2017B-UL2017_MiniAODv2_NanoAODv9-v1_"+str(count)+".root" #Output file name
    l0 = "#!/bin/sh"
    l1 =f"cd {CMSSW_SRC_PATH}" #Give the path of the executable code
   # l1 = "export SCRAM_ARCH=el9_amd64_gcc12"
    l2 = "export X509_USER_PROXY=/afs/cern.ch/user/h/hsiaoche/proxy/x509up_u183606"
    l3 = "source /cvmfs/cms.cern.ch/cmsset_default.sh"
   # l5 = "eval cmsenv"
    l4 = "root -l -b -q '/afs/cern.ch/user/h/hsiaoche/work/Data/ForTrigSF_2017_v2.C(\"{}\",\"{}\")'".format("root://cmsxrootd.fnal.gov/"+line,out_rootFileName)

    file_out1.writelines("{0}\n" "{1}\n" "{2}\n" "{3}\n" "{4}\n".format(l0,l1,l2,l3,l4))
    file_out1.close()



    filename2 = f"{SUBDIR}/submit_"+str(count)+".sh"
    file_out2 = open(filename2,"w")
    s0 = "+MaxRuntime = 259200"
    s1 = "universe = vanilla"
    s2 = f"executable ={filename1}"
    s3 = "getenv = TRUE"
    s4 = f"log ={SUBDIR}/logFiles/job_{count}_$(ClusterId).$(ProcId).log"
    s5 = f"output ={SUBDIR}/outFiles/job_{count}_$(ClusterId).$(ProcId).out"
    s6 = f"error ={SUBDIR}/errorFiles/job_{count}_$(ClusterId).$(ProcId).error"
    s7 = "notification = never"
    s8 = "should_transfer_files = YES"
    s9 = "when_to_transfer_output = ON_EXIT"
    s10 = "max_retries   = 2"
    s11 = "queue"
    
    file_out2.writelines("{0}\n" "{1}\n" "{2}\n" "{3}\n" "{4}\n" "{5}\n" "{6}\n" "{7}\n" "{8}\n" "{10}\n" "{11}\n".format(s0,s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11))
    file_out2.close();

    count = count+1
    os.system(f"chmod 755 {filename1}")

    submit_file.writelines(f"condor_submit {filename2}\n")

submit_file.close()
#os.system("chmod 755 {}".format(submit_file))
