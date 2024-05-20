import os
import sys
import time
import subprocess
import shutil
import fnmatch
import tempfile
import glob

PROJ_FOLDER_NAME='..\\..\\board'

if __name__ == "__main__":
    err = 0
    f = open('iar2.txt', "w+")
    si = subprocess.STARTUPINFO()
    si.dwFlags |= subprocess.STARTF_USESHOWWINDOW

    #os.chdir(os.environ["WORKSPACE"])
    root = os.getcwd()
    #clean up first
    for dirPath, dirNames, fileNames in os.walk(PROJ_FOLDER_NAME):
        for file in fnmatch.filter(fileNames, '*.o'):
            os.remove(os.path.join(dirPath, file))
    os.chdir(root)

    for dirPath, dirNames, fileNames in os.walk(PROJ_FOLDER_NAME):
        for file in fnmatch.filter(fileNames, '*.ewp'):
            os.chdir(dirPath)
            try:
                print(os.getcwd() + "\\" + file +  " building ...\n")
                subprocess.call("C:\\Program Files\\IAR Systems\\Embedded Workbench 9.4\\common\\bin\\iarbuild.exe " + file + " -build Release -log warnings", startupinfo=si, stdout=f, stderr=f)
            except OSError:
                pass 	#Silently ignore
            os.chdir(root)
    f.close()

    # IAR output is a little messy, now clean it up
    f = open('iar2.txt', "r")
    lines = f.readlines()
    f.close()
    f = open("iar.txt","w")
    found = 0
    print("\n")

    for line in lines:
        if line.find("Building configuration") >= 0:
            if found != 0:
                index1 = line1.find(":") + 2 # ignore ": "
                index2 = line1.find(" ", index1)
                f.write("Build " + line1[index1:index2] + " has error or warning...\n")
                print("Build " + line1[index1:index2] + " has error or warning...\n")
                err += 1
            line1 = line
            found = 0
            continue
        if line.find("Total number of ") >= 0:
            if line.find("Total number of errors: 0") >= 0:
                continue
            if line.find("Total number of warnings: 0") >=0:
                continue
            print("\t" + os.getcwd() + "\\" + file +  " has error or warning...\n")
            found = 1
    f.close()

    #clean up
    rmfile_list = {"*.o", "*.lst", "*.out"}
    os.chdir(root)
    for dirPath, dirNames, fileNames in os.walk('.'):
        for rmfile in rmfile_list:
            for file in fnmatch.filter(fileNames, rmfile):
                os.remove(os.path.join(dirPath, file))

    if err == 0:
        sys.exit(0)
    else:
        sys.exit(1)
