import os
#Reading the file names from the list
file_in = open("/afs/cern.ch/user/h/hsiaoche/work/Data/Run2018A-17Sep2018-v2_MINIAOD.txt","r") #Path of the file list
Lines = file_in.read().splitlines();
file_in.close()

#Creating the directories for logFile, outFiles and errorFiles
os.system("mkdir logFiles")
os.system("mkdir outFiles")
os.system("mkdir errorFiles")

submit_file = open("finalSubmit.sh","w")
submit_file.writelines("#!/bin/sh \n")

#Making the exe and condor submit files
count = 0
for line in Lines:
    #line.replace('\n','')
    line.strip()
    print(line)
    filename1 = "job_"+str(count)+".sh"
    file_out1 = open(filename1,"w")
    out_rootFileName = "/eos/user/h/hsiaoche/Data_2018/outFile_EGamma2018A-17Sep2018-v2_"+str(count)+".root" #Output file name
    l0 = "#!/bin/sh"
    l1 = "cd /afs/cern.ch/user/h/hsiaoche/work/Data" #Give the path of the executable code
    l2 = "export SCRAM_ARCH=slc7_amd64_gcc700"
    l3 = "export X509_USER_PROXY=/afs/cern.ch/user/h/hsiaoche/proxy"
    l4 = "source /cvmfs/cms.cern.ch/cmsset_default.sh"
    l5 = "eval cmsenv"
    l6 = "root -l -b -q 'ForTrigSF.C(\"{}\",\"{}\")'".format("root://cmsxrootd.fnal.gov/"+line,out_rootFileName)
    
    file_out1.writelines("{0}\n" "{1}\n" "{2}\n" "{3}\n" "{4}\n" "{5}\n" "{6}\n".format(l0,l1,l2,l3,l4,l5,l6))
    file_out1.close()

    
    filename2 = "submit_"+str(count)+".sh"
    file_out2 = open(filename2,"w")
    s0 = "+MaxRuntime = 259200"
    s1 = "universe = vanilla"
    s2 = "executable ={}".format(filename1)
    s3 = "getenv = TRUE"
    s4 = "log =logFiles/job_"+str(count)+".log"
    s5 = "output =outFiles/job_"+str(count)+".out"
    s6 = "error =errorFiles/job_"+str(count)+".error"
    s7 = "notification = never"
    s8 = "should_transfer_files = YES"
    s9 = "when_to_transfer_output = ON_EXIT"
    s10 = "max_retries   = 2"
    s11 = "queue"
    
    file_out2.writelines("{0}\n" "{1}\n" "{2}\n" "{3}\n" "{4}\n" "{5}\n" "{6}\n" "{7}\n" "{8}\n" "{10}\n" "{11}\n".format(s0,s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11))
    file_out2.close();

    count = count+1
    os.system("chmod 755 {}".format(filename1))

    submit_file.writelines("condor_submit {}\n".format(filename2))

submit_file.close()
#os.system("chmod 755 {}".format(submit_file))
