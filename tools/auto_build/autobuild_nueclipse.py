import os
import sys
import time
import subprocess
import shutil
import fnmatch
import tempfile
import glob
import re
import xml.etree.ElementTree as ET # phone home :p
import mmap

PROJ_FOLDER_NAME='..\\..\\board'

if __name__ == "__main__":
    err = 0
    f = open('gcc_log.txt', "w+")
    si = subprocess.STARTUPINFO()
    si.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    root = os.getcwd()

    for dirPath, dirNames, fileNames in os.walk(PROJ_FOLDER_NAME):
        for file in fnmatch.filter(fileNames, '*.cproject'):
            #os.chdir(dirPath + "\\..\\..")
            os.mkdir("Temp")
            try:
                ###
                ###print("Building" + " " + file + " " + os.path.basename(os.getcwd()))
                print(dirPath + "\\" + file +  " building ...\n")                
                subprocess.check_call("C:\\Program Files (x86)\\Nuvoton Tools\\NuEclipse\\V1.02.025c\\NuEclipse\\eclipse\\eclipsec.exe " +
                                      "-nosplash --launcher.suppressErrors -application org.eclipse.cdt.managedbuilder.core.headlessbuild " +
                                      "-data Temp -import " + dirPath + " -build all",
                                      startupinfo=si, stdout=f, stderr=f)
            except Exception as e:
                f.write("Build " +  dirPath +  " has error or warning...\n")
                #print("Build" + file +  "has error or warning...\n")
                err += 1
            except OSError:
                f.write("Ooops\n")  ##
                pass                # Silently ignore
            #os.chdir(root)
            shutil.rmtree("Temp")            
    f.close()

    # Find any error/warning
    f = open('gcc_log.txt', "r")
    lines = f.readlines()
    f.close()
    f = open("gcc.txt", "w")
    print("\n")

    for line in lines:
        if line.find(" for project ") >= 0:
            index1 = line.find(" for project ") + len(" for project ")
            index2 = line.find(" ****", index1)
            prjName = line[index1:index2]
        elif line.find(" error: ") >= 0 or line.find(" warning: ") >= 0:
            if line.find("manifest file error") >= 0:  # Skip this strange error report by Eclipse for ARM9 projects
                pass
            else:
                if len(prjName) > 0:
                    f.write("Build " + prjName + " has error or warning...\n")
                    print("Build " + prjName + " has error or warning...\n")
                    err += 1
                    prjName = ""    # Each project has at most one message.
                else:
                    pass            # Skip other messages in the same project
    f.close()

    #clean up
    rmfile_list = {"*.o", "*.d", "*.elf", "*.hex"}
    os.chdir(root)
    for dirPath, dirNames, fileNames in os.walk('.'):
        for rmfile in rmfile_list:
            for file in fnmatch.filter(fileNames, rmfile):
                os.remove(os.path.join(dirPath, file))
    
    if err == 0:
        sys.exit(0)
    else:
        sys.exit(1)