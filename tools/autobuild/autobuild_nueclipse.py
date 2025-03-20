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
PATH_ECLIPSE="C:\\Program Files (x86)\\Nuvoton Tools\\NuEclipse\\V1.02.025c\\NuEclipse\\eclipse\\eclipsec.exe"

def blacklist_check(BUILDLOG):
    Blacklist = ['[Fatal Error]', 'An error has occurred.', ' error: ', 'Error: ']

    # Find any error/warning
    fp = open(BUILDLOG, "r")
    lines = fp.readlines()
    fp.close()

    # Skip 3 lines at first
    # For ignore warning. OpenJDK 64-Bit Server VM warning: Options -Xverify:none and -noverify were deprecated in JDK 13 and will likely be removed in a future release.
    lines.pop(0)
    lines.pop(1)
    lines.pop(2)
    lines.pop(3)

    new_lines = ''

    found = 0
    for l in lines:
        new_lines += l
        for br in Blacklist:
            pos = l.find(br)
            if pos >= 0:
                space_string = ' ' * pos
                arrow_string = '^' * len(br)
                new_lines += space_string + arrow_string + ' <- Received a blacklist rule.\n\n'
                found += 1

    if found > 0:
        # Update BUILDLOG
        fp = open(BUILDLOG, "w")
        fp.writelines(new_lines)
        fp.close()

    return found

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
    f = open('gcc.txt', "w")
   
    prj_count = 0

    for dirPath, dirNames, fileNames in os.walk(PROJ_FOLDER_NAME):

        for file in fnmatch.filter(fileNames, '*.cproject'):

            if os.path.isdir('Temp'):
                shutil.rmtree('Temp')

            #print("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " found.", flush=True)
            #get_memory_info()

            os.mkdir('Temp')
            basename = os.path.basename(os.path.dirname(dirPath))
            buildcommnd = PATH_ECLIPSE + " -nosplash --launcher.suppressErrors -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data Temp -import " + dirPath + " -build all"

            BUILDLOG = os.path.join(dirPath, basename) + '.log'

            try:
                #print(dirPath + " building ...")
                fp = open(BUILDLOG, "w")
                found = subprocess.check_call(buildcommnd, startupinfo=si, stdout=fp, stderr=fp)
                fp.flush()
                fp.close()

                found += blacklist_check(BUILDLOG)
                if found > 0:
                    err += 1
                    f.write("[" + str(prj_count) + "] "+ dirPath +  " has error or warning.\n")
                    LIST_MAIL_ATTACHMENT.append(str(os.path.abspath(BUILDLOG)))
                    print("[" + str(prj_count) + "] "+ dirPath +  " has error or warning.", flush=True)
                else:
                    #f.write("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " pass...\n")
                    print("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " pass...", flush=True)
            except Exception as e:
                f.write("[" + str(prj_count) + "] "+ dirPath +  " has error or warning.\n")
                #print("[" + str(prj_count) + "] "+ dirPath +  " has error or warning.", flush=True)
                LIST_MAIL_ATTACHMENT.append((os.path.abspath(BUILDLOG)))
                err += 1
            except OSError:
                #print("Build" + file +  "has Ooops...\n")
                f.write("[" + str(prj_count) + "] "+ dirPath +  " has Ooops.\n")
                err += 1
                pass                # Silently ignore

            prj_count += 1
            f.flush()

    shutil.rmtree('Temp')

    if err == 0:
        f.write("Build " + str(prj_count-1) + " projects successfully.\n")
        #print("Build " + str(prj_count-1) + " projects successfully.\n", flush=True)

    BLOG_SUMMARY = f.name
    f.close()

    if err == 0:
        sys.exit(0)
    else:
        sys.exit(1)