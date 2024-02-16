import os
import sys
import fnmatch
import xml.etree.ElementTree as ET
import fileinput
from shutil import copyfile

LVGL_FOLDER_NAME = '..\\..\\..\\lvgl\\'

PATH_SCRIPT = os.getcwd()

if __name__ == "__main__":
    root = os.getcwd()
    os.chdir(root)

    print('          <Files>')
    for dirPath, dirNames, fileNames in os.walk(LVGL_FOLDER_NAME):

        if (dirPath.find('examples') > 0):
            continue
        if (dirPath.find('env_support') > 0):
            continue
        if (dirPath.find('libs') > 0):
            continue
        if (dirPath.find('drivers') > 0):
            continue
        if (dirPath.find('test') > 0):
            continue
        if (dirPath.find('others') > 0):
            continue
        if (dirPath.find('renesas') > 0):
            continue
        if (dirPath.find('nxp') > 0):
            continue

        for file in fnmatch.filter(fileNames, '*.c'):
            print('              <File>')
            print('                <FileName>'+ file +'</FileName>')
            print('                <FileType>1</FileType>')
            print('                <FilePath>'+ dirPath + '\\' + file +'</FilePath>')
            print('              </File>')
    print('          </Files>')

    print('')
    print('')

    INCLUDE=[]

    for dirPath, dirNames, fileNames in os.walk(LVGL_FOLDER_NAME):

        if (dirPath.find('examples') > 0):
            continue
        if (dirPath.find('env_support') > 0):
            continue
        if (dirPath.find('libs') > 0):
            continue
        if (dirPath.find('drivers') > 0):
            continue
        if (dirPath.find('test') > 0):
            continue
        if (dirPath.find('others') > 0):
            continue
        if (dirPath.find('renesas') > 0):
            continue
        if (dirPath.find('nxp') > 0):
            continue

        for file in fnmatch.filter(fileNames, '*.h'):

            INCLUDE.append(dirPath)

    inc_str=''
    for inc in set(INCLUDE):
        inc_str += inc +';'

    print(inc_str)
