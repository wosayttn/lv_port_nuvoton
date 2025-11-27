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
PATH_UV4="C:\\Keil_v5\\UV4\\Uv4.exe"

def blacklist_check(BUILDLOG):
    Blacklist = ['warning:', 'Warning: ', 'error: ', 'Error: ']
    Whitelist = ['[-Wlicense-management]', 'Q9931W:', 'Your license ', ' does not support the selected User Based Licensing technology']

    # Find any error/warning
    fp = open(BUILDLOG, "r")
    lines = fp.readlines()
    fp.close()

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
        for br in Whitelist:
            pos = l.find(br)
            if pos >= 0:
                found -= 1

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

    print('User-> ' + os.environ['USERNAME'], flush=True)

    root = os.getcwd()    
    f = open('keil.txt', "w+")

    os.chdir(root)
    
    prj_count = 0

    for dirPath, dirNames, fileNames in os.walk(PROJ_FOLDER_NAME):
        for extension in ('*.uvproj', '*.uvprojx'):
            for file in fnmatch.filter(fileNames, extension):
                os.chdir(dirPath)

                #print("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " found.", flush=True)

                #get_memory_info()

                try:
                    f1 = open(file, 'r+')
                    mm = mmap.mmap(f1.fileno(), 0)
                except:
                    printf("Parse " + file + " failed\n")
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
                    BUILDLOG = file + ".log"
                    buildcommnd = PATH_UV4 + " -b -j0 -z -o " + BUILDLOG + " " + file
                    cleancommnd = PATH_UV4 + " -j0 -c " + file

                    # https://www.keil.com/support/man/docs/uv4cl/uv4cl_commandline.htm
                    # -j0   Hides the µVision GUI. Messages are suppressed. Use this option for batch testing.
                    # -z    Re-builds all targets of a project or multiple-project. 
                    #       Ensure that each target has another object output folder.
                    #       Use the menu Projects - Options for Target - Output - Select Folder for Objects.
                    # -b    Builds the last current target of a project and exits after the build process finished.
                    #       Refer to option -t to change the target. 
                    #       For multi-projects, the command builds the targets as defined in the dialog Project - Batch Build.
                    # -o outputfile
                    #f.write("[" + str(prj_count) + "] Build " + os.path.abspath(file) +  "\n")

                    #print("[" + str(prj_count) + "] "+ os.getcwd() + "\\" + file +  " cleaning.\n", flush=True)                
                    #subprocess.call(cleancommnd, startupinfo=si, stdout=f, stderr=f)
                    p = subprocess.Popen(cleancommnd, startupinfo=si, stdout=f, stderr=f)
                    p.wait(300) #Anti-zombie

                    #print("[" + str(prj_count) + "] "+ os.getcwd() + "\\" + file +  " building.\n", flush=True)                
                    #subprocess.call(buildcommnd, startupinfo=si, stdout=f, stderr=f)
                    p = subprocess.Popen(buildcommnd, startupinfo=si, stdout=f, stderr=f)
                    p.wait(300)  #Anti-zombie

                    found = blacklist_check(BUILDLOG)
                    if found > 0:
                        err += 1
                        f.write("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " has error or warning(" + str(found) + ").\n")
                        print("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " has error or warning.", flush=True)
                        LIST_MAIL_ATTACHMENT.append(os.path.abspath(BUILDLOG))
                    else:
                        print("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " pass...", flush=True)
                        #f.write("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " pass...\n")

                except subprocess.TimeoutExpired:
                    p.kill()
                    f.write("[" + str(prj_count) + "] "+ "Build " + file +  " has exception.\n")
                    #print("[" + str(prj_count) + "] "+ "Build" + file +  "has exception.\n")
                    err += 1
                except Exception as e:
                    f.write("[" + str(prj_count) + "] "+ "Build " + file +  " has exception.\n")
                    #print("[" + str(prj_count) + "] "+ "Build" + file +  "has exception.", flush=True)
                    err += 1                
                except OSError:
                    f.write("[" + str(prj_count) + "] " + os.path.abspath(file) + "Ooops\n")
                    #print("[" + str(prj_count) + "] " + os.path.abspath(file) + "Ooops", flush=True)
                    pass #Silently ignore

                prj_count += 1

                f.flush()
                os.chdir(root)

    if err == 0:
        f.write("Build " + str(prj_count-1) + " projects successfully.\n")
        #print("Build " + str(prj_count-1) + " projects successfully.\n", flush=True)

    BLOG_SUMMARY = f.name
    f.close()

    os.chdir(root)

    if err == 0:
        sys.exit(0)
    else:
        sys.exit(1)