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
FN_PATTERNS = ['*.uvproj', '*.uvprojx']

if __name__ == "__main__":
    si = subprocess.STARTUPINFO()
    si.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    err = 0
    root = os.getcwd()    
    f = open('keil.txt', "w+")

    
    #clean up first
    for dirPath, dirNames, fileNames in os.walk(PROJ_FOLDER_NAME):
        for file in fnmatch.filter(fileNames, '*.o'):
            os.remove(os.path.join(dirPath, file))
    os.chdir(root)

    for dirPath, dirNames, fileNames in os.walk(PROJ_FOLDER_NAME):
        for extension in ('*.uvproj', '*.uvprojx'):
            for file in fnmatch.filter(fileNames, extension):
                os.chdir(dirPath)
                try:
                    f1 = open(file, 'r+')
                    mm = mmap.mmap(f1.fileno(), 0)
                except:
                    #printf("Parse " + file + " failed\n")
                    f1.close()
                    pass
                else:
                    # disable cross module optimization to reduce build time
                    pos = mm.find(b'<OptFeed>1</OptFeed>')
                    if pos != -1:
                        mm.seek(pos + 9)
                        mm.write(b"0")
                    mm.close()
                    f1.close()
                try:
                    #f.write("Build " +  os.path.abspath(file) +  "...\n")
                    print(os.getcwd() + "\\" + file +  " building ...\n")                
                    subprocess.call("C:\\Keil_v5\\UV4\\Uv4.exe -b -j0 -z -o k.log " + file, startupinfo=si, stdout=f, stderr=f)
                except Exception as e:
                    f.write("Build " + file +  " has error or warning...\n")
                    #print("Build" + file +  "has error or warning...\n")
                    err += 1                
                except OSError:
                    f.write("Ooops\n") ##
                    pass #Silently ignore

                # It's a bit strange keil report error code as 0 even build failed. so parse k.log
                tmp = open("k.log", "r")
                lines = tmp.readlines()
                tmp.close()
                #os.remove("k.log")
                found = 0
                for line in lines:
                    if line.find("0 Error(s), 0 Warning(s)") >= 0:
                        found = 1
                if found == 0:
                    err += 1
                    f.write("Build " + file +  " has error or warning...\n")
                    print("\t" + os.getcwd() + "\\" + file +  " has error or warning...\n")
                else:
                    print("\t" + os.getcwd() + "\\" + file +  " pass...\n")

                os.chdir(root)
    f.close()
    
    #clean up
    rmfile_list = {"*.o", "*.lst", "*.crf", "*.axf",  "*.bin",  "*.map", "*.i"}
    os.chdir(root)
    for dirPath, dirNames, fileNames in os.walk('.'):
        for rmfile in rmfile_list:
            for file in fnmatch.filter(fileNames, rmfile):
                os.remove(os.path.join(dirPath, file))
    
    if err == 0:
        sys.exit(0)
    else:
        sys.exit(1)