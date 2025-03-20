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

PROJ_FOLDER_NAME='..\\..\\board'

def blacklist_check(BUILDLOG):
    Blacklist = ['[Fatal Error]', 'An error has occurred.', ' error: ', ' warning: ', 'Error: ', 'Warning: ', 'warning csolution: ']

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

def export_build_ps1(DirPath, prjBuildScript, csolution_file):

    fps1 = open(os.path.join(DirPath, prjBuildScript), "w")
    script_str = f"""\

. ~/.vcpkg/vcpkg-init.ps1
vcpkg activate
cpackget list

$targets=@(cbuild list contexts \"{csolution_file}\")
if ($LASTEXITCODE -ne 0) {{
    exit 1
}}
$numberOfElements = $targets.Count

$okay = 0
foreach ($t in $targets) {{
    cbuild \"{csolution_file}\" --context $t -S
    cbuild \"{csolution_file}\" -S --rebuild --update-rte -d -v
    if ($LASTEXITCODE -eq 0) {{
        $okay+=1
    }}
}}

Write-Output "BuildMachine: built $okay target in $numberOfElements configuration."
if ($okay -eq $numberOfElements) {{
    exit 0
}}
else {{
    exit 1
}}
"""

    fps1.write(script_str)
    fps1.flush()
    fps1.close()

if __name__ == "__main__":
    LIST_MAIL_ATTACHMENT = []
    si = subprocess.STARTUPINFO()
    si.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    err = 0
    root = os.getcwd()    
    f = open('vscode.txt', "w")
   
    prj_count = 0
    bsp_root = os.getcwd()
    #print(root)

    for dirPath, dirNames, fileNames in os.walk(PROJ_FOLDER_NAME):

        for file in fnmatch.filter(fileNames, '*.csolution.y*ml'):

            prjFileAbsPath = os.path.join(bsp_root, dirPath, file)
            #print("[" + str(prj_count) + "] "+ prjFileAbsPath +  " found.", flush=True)
            #get_memory_info()

            prjDirAbsPath = os.path.dirname(prjFileAbsPath)
            prjName = os.path.splitext(os.path.basename(file))[0]
            prjName = os.path.splitext(prjName)[0]
            prjBuildScript = 'build_' + prjName + '.ps1'

            #print(prjName)
            #print(prjFileAbsPath)
            #print(prjDirAbsPath)
            #os.chdir(prjDirAbsPath)
            #print(os.getcwd())

            BUILDLOG = prjFileAbsPath + '.log'

            buildcommand = "cd \"" + prjDirAbsPath + "\" & PowerShell -File ./" + prjBuildScript + " > \"" + BUILDLOG + "\""

            #print(buildcommand)
            #print(BUILDLOG)

            try:
                #print(dirPath + " building ...")
                export_build_ps1(prjDirAbsPath, prjBuildScript, file)
                found = os.system(buildcommand)

                found += blacklist_check(BUILDLOG)
                if found != 0:
                    err += 1
                    f.write("[" + str(prj_count) + "] "+ dirPath +  " has error or warning.\n")
                    LIST_MAIL_ATTACHMENT.append(str(os.path.abspath(BUILDLOG)))
                    print("[" + str(prj_count) + "] "+ prjFileAbsPath +  " has error or warning.", flush=True)
                else:
                    #f.write("[" + str(prj_count) + "] "+ os.path.abspath(file) +  " pass...\n")
                    print("[" + str(prj_count) + "] "+ prjFileAbsPath +  " pass...", flush=True)
            except Exception as e:
                f.write("[" + str(prj_count) + "] "+ dirPath +  " has Exception.\n")
                #print("[" + str(prj_count) + "] "+ dirPath +  " has Exception.", flush=True)
                LIST_MAIL_ATTACHMENT.append((os.path.abspath(BUILDLOG)))
                err += 1
            except OSError:
                print("Build" + file +  "has Ooops...\n")
                f.write("[" + str(prj_count) + "] "+ dirPath +  " has Ooops.\n")
                err += 1
                pass                # Silently ignore


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