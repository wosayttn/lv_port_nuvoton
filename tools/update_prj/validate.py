import os
import sys
import glob
import hashlib
import fnmatch
import re
from tqdm import tqdm
import inspect
import xml.etree.ElementTree as ET
import yaml
from collections import OrderedDict

def validate_mdk(xml_file):
    """
    Update MDK project XML with consistent OutputDirectory, ListingPath, and OutputName per target.

    Example changes:
    - <OutputDirectory> -> .\Data_<TargetName>\
    - <ListingPath> -> .\Data_<TargetName>\
    - <OutputName> -> lvgl_demo_<TargetName>
    """
    print(f"<<<{inspect.currentframe().f_code.co_name}>>> {xml_file}")

    base_name = os.path.basename(xml_file)
    projname = os.path.splitext(base_name)[0]

    # Parse XML
    tree = ET.parse(xml_file)
    root = tree.getroot()
    prj_targets = root.findall('./Targets/Target')

    if prj_targets:
        for target in prj_targets:
            target_name_elem = target.find('TargetName')
            if target_name_elem is None or not target_name_elem.text:
                continue  # Skip targets without a name

            target_name = target_name_elem.text
            # Paths
            output_dir = f'.\\{projname}_{target_name}\\'.lower()
            output_name = f'{projname}_{target_name}'.lower()
            output_adc_includepath = f'..\\'

            # Update OutputDirectory
            output_dir_elem = target.find('./TargetOption/TargetCommonOption/OutputDirectory')
            if output_dir_elem is not None and output_dir_elem.text != output_dir:
                output_dir_elem.text = output_dir

            # Update ListingPath
            listing_path_elem = target.find('./TargetOption/TargetCommonOption/ListingPath')
            if listing_path_elem is not None and listing_path_elem.text != output_dir:
                listing_path_elem.text = output_dir

            # Update OutputName
            output_name_elem = target.find('./TargetOption/TargetCommonOption/OutputName')
            if output_name_elem is not None and output_name_elem.text != output_name:
                output_name_elem.text = output_name

            # Ads IncludePath
            output_ads_incpath_elem = target.find('./TargetOption/TargetArmAds/Aads/VariousControls/IncludePath')
            if output_ads_incpath_elem is not None and output_ads_incpath_elem.text != output_adc_includepath:
                output_ads_incpath_elem.text = output_adc_includepath

            # Ads IncludePath
            output_aftermake_elem = target.find('./TargetOption/TargetCommonOption/AfterMake/IncludePath')
            if output_aftermake_elem is not None:
                '''
                <RunUserProg1>1</RunUserProg1>
                <RunUserProg2>1</RunUserProg2>
                <UserProg1Name>fromelf --bin "$L@L.axf" --output "$L@L.bin"</UserProg1Name>
                <UserProg2Name>fromelf --bin "$L@L.axf" --output "$P../Bin/@L.bin" </UserProg2Name>
                '''
                output_RunUserProg1_elem = output_aftermake_elem.find('RunUserProg1')
                if output_RunUserProg1_elem is not None:
                    output_RunUserProg1_elem.text='1'

                output_RunUserProg2_elem = output_aftermake_elem.find('RunUserProg2')
                if output_RunUserProg2_elem is not None:
                    output_RunUserProg2_elem.text='1'

                output_UserProg1Name_elem = output_aftermake_elem.find('UserProg1Name')
                if output_UserProg1Name_elem is not None:
                    output_UserProg1Name_elem.text=f'fromelf --bin \"$L@L.axf\" --output \"$L@L.bin\"'

                output_UserProg2Name_elem = output_aftermake_elem.find('UserProg2Name')
                if output_UserProg2Name_elem is not None:
                    output_UserProg2Name_elem.text=f'fromelf --bin \"$L@L.axf\" --output \"$P../Bin/@L.bin\"'

        # Pretty print & save
        ET.indent(tree, space="  ", level=0)
        tree.write(xml_file, encoding="utf-8", xml_declaration=True, short_empty_elements=False)
        print(f"??Saved updated MDK project: {xml_file}")

def remove_uvopt(xml_file):
    if os.path.exists(xml_file):
        os.remove(xml_file)

UPDATE_FUNCTIONS = {
    '*.uvproj': validate_mdk,
    '*.uvprojx': validate_mdk,
    '*.uvopt': remove_uvopt,
}

def validate_project_file(file_abs_path, validate_function):
    validate_function(file_abs_path)

def validate_project(board_dir):
    for dirPath, dirNames, fileNames in os.walk(board_dir):
        for pattern, validate_function in UPDATE_FUNCTIONS.items():
            for file in fnmatch.filter(fileNames, pattern):
                prjFileAbsPath = os.path.join(board_dir, dirPath, file)
                validate_project_file(prjFileAbsPath, validate_function)

if __name__ == "__main__":

    board_cwd = os.path.abspath(os.path.join(os.getcwd(), '../../board/'))
    validate_project(board_cwd)
    misc_cwd = os.path.abspath(os.path.join(os.getcwd(), '../../misc/'))
    validate_project(misc_cwd)
