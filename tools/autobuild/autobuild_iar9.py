import os
import sys
import time
import subprocess
import shutil
import fnmatch
import tempfile
import glob
import re
import sys
import datetime
import xml.etree.ElementTree as ET # phone home :p
import mmap

PROJ_FOLDER_NAME='..\\..\\board'
PATH_IAREXE="C:\\Program Files\\IAR Systems\\Embedded Workbench 9.4\\common\\bin\\iarbuild.exe"

def get_memory_info():
    cmd = 'cmd /c \"systeminfo | find /i \"Available Physical Memory\"\"'
    si = subprocess.STARTUPINFO()
    si.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    subprocess.check_call(cmd, startupinfo=si)
    sys.stdout.flush()

if __name__ == "__main__":
    LIST_MAIL_ATTACHMENT = []
    si = subprocess.STARTUPINFO()
    si.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    err = 0
    root = os.getcwd()    
    os.chdir(root)

    f = open('iar.txt', "w")
    prj_count = 0

    for dirPath, dirNames, fileNames in os.walk(PROJ_FOLDER_NAME):
        for file in fnmatch.filter(fileNames, '*.ewp'):
            #print('Building', root, dirPath, file)
            #skip dsp_fft.ewp, this project requires IAR 7.3 or above  --ya 2017/3/31
            if file == "dsp_fft.ewp" or file == "DSP_FFT.ewp":
                continue

            #print("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " found.", flush=True)
            #get_memory_info()

            prjName = os.path.splitext(file)[0]
            buildcommnd = PATH_IAREXE + " " + file + " -build * -log warnings"
            
            os.chdir(dirPath)
            try:
                BUILDLOG = file + ".log"
                fp = open(BUILDLOG, "w")

                subprocess.call(buildcommnd, startupinfo=si, stdout=fp, stderr=fp)
                fp.flush()
                fp.close()

                # Find any error/warning
                fp = open(BUILDLOG, "r")
                lines = fp.readlines()
                fp.close()

                total_conf = 0
                found = 0
                prjNamePat = prjName + " - "
                for line in lines:
                    if line.find(" ERROR, ") == 0:
                        break
                    elif line.find(prjNamePat) >=0:
                        total_conf += 1
                    elif line.find("Total number of errors: 0") >=0:
                        found += 1
                    elif line.find("Total number of warnings: 0") >=0:
                        found += 1

                if total_conf == 0 or (total_conf > 0 and found != 2*total_conf):
                    err += 1
                    f.write("[" + str(prj_count) + "] "+ dirPath +  " has error or warning.\n")
                    LIST_MAIL_ATTACHMENT.append(str(os.path.abspath(BUILDLOG)))
                    print("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " has error or warning.", flush=True)
                else:
                    #f.write("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " pass...\n")
                    print("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " pass...", flush=True)
            except OSError:
                err += 1
                #print("[" + str(prj_count) + "] " + os.path.abspath(file) + "Ooops", flush=True)
                pass    # Silently ignore

            prj_count += 1
            f.flush()
            os.chdir(root)

    if err == 0:
        f.write("Build " + str(prj_count-1) + " projects successfully.\n")
        #print("Build " + str(prj_count-1) + " projects successfully.\n", flush=True)

    BLOG_SUMMARY = f.name
    f.close()

    if err == 0:
        sys.exit(0)
    else:
        sys.exit(1)