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

lvgl = []
lv_demo = []
lv_examples = []

GROUP_FILES = {
    'lvgl': lvgl,
    'lv_demo': lv_demo,
    'lv_examples': lv_examples
}

inc = []
group = []
cwd = os.getcwd()  # get current dir path

def Glob(pattern):
    return glob.glob(pattern, recursive=False)

port_src = Glob('*.c')
port_inc = [cwd]

lvgl_cwd = cwd + '/../../lvgl'
lvgl_src_cwd = lvgl_cwd + '/src/'
inc.append(lvgl_src_cwd)

def dump_set(array):
    for i in array:
        print(os.path.abspath(i))

def gen_lv_examples():
    global lv_examples
    examples_root = lvgl_cwd + '/examples/'
    add_inc(examples_root)

    for root, dirs, files in os.walk(examples_root):
        lv_examples += Glob(os.path.join(root, '*.c'))
        if check_h_hpp_exists(root):
            add_inc(root)

# check if .h or .hpp files exists
def check_h_hpp_exists(path):
    for f in os.listdir(path):
        if os.path.splitext(f)[1] in ['.h', '.hpp']:
            return True
    return False


def add_inc(path):
    """Add include path without duplication"""
    global inc
    if path not in inc:
        inc.append(os.path.abspath(path))


def gen_lvgl():
    global lvgl, inc

    # Normalize all exclude paths so Windows/Linux behave the same
    EXCLUDE_DIRS = [
        'draw/dma2d',
        'draw/espressif',
        'draw/eve',
        'draw/nema_gfx',
        'draw/opengles',
        'draw/nxp',
        'draw/renesas',
        'draw/sdl',
        'draw/vg_lite',
        'libs/barcode',
        'libs/expat',
        'drivers',
        'libs/freetype',
        'libs/frogfs',
        'libs/FT800-FT813',
        'libs/gstreamer',
        'libs/gltf',
        'libs/libjpeg_turbo',
        'libs/thorvg/rapidjson/msinttypes',
        'libs/qrcode',
        'libs/svg',
        'libs/tiny_ttf',
        'libs/rlottie',
        'libs/rle',
        'libs/vg_lite_driver',
        'libs/lv_linux.c',
        'libs/lv_mqx.c',
        'libs/lv_pthread.c',
        'libs/lv_rtthread.c',
        'libs/lv_sdl2.c',
        'libs/lv_windows.c',
        'others/xml',
        'others/xml/parsers',
        'others/fragment',
        'others/test',
        'stdlib/rtthread',
        'stdlib/micropython',
        'stdlib/clib',
        'stdlib/uefi',
    ]

    # normalize patterns for cross-platform
    EXCLUDE_DIRS = [os.path.normpath(x) for x in EXCLUDE_DIRS]

    for root, dirs, files in os.walk(lvgl_src_cwd):
        root_norm = os.path.normpath(root)

        # ??PARTIAL MATCH: exclude if any keyword appears in the path
        if any(ex in root_norm for ex in EXCLUDE_DIRS):
            continue

        lvgl += Glob(os.path.join(root, '*.c'))
        lvgl += Glob(os.path.join(root, '*.S'))

        if check_h_hpp_exists(root):
            add_inc(root)

def gen_lv_demos():
    global lv_demo
    demos_root = lvgl_cwd + '/demos/'
    add_inc(demos_root)

    # Normalize all exclude paths so Windows/Linux behave the same
    EXCLUDE_DIRS = [
        'gltf',
        'music',
        'render',
        'stress',
        'vector_graphic',
        'keypad_encoder',
    ]

    # normalize patterns for cross-platform
    EXCLUDE_DIRS = [os.path.normpath(x) for x in EXCLUDE_DIRS]

    for root, dirs, files in os.walk(demos_root):
        root_norm = os.path.normpath(root)

        # ??PARTIAL MATCH: exclude if any keyword appears in the path
        if any(ex in root_norm for ex in EXCLUDE_DIRS):
            continue

        lv_demo += Glob(os.path.join(root, '*.c'))
        if check_h_hpp_exists(root):
            add_inc(root)

def gen_lv_examples():
    global lv_examples
    examples_root = lvgl_cwd + '/examples/'
    add_inc(examples_root)

    for root, dirs, files in os.walk(examples_root):
        lv_examples += Glob(os.path.join(root, '*.c'))
        if check_h_hpp_exists(root):
            add_inc(root)

def is_m55_path(path: str) -> bool:
    """Detect whether file path belongs to Cortex-M55-related code."""
    p = path.replace("\\", "/").lower()

    M55_KEYWORDS = [
        "m55m1",
        "m5531",
    ]

    return any(key in p for key in M55_KEYWORDS)

def mdk_add_file_node(xml_file, files_node, path):
    """
    Insert 1 file entry into <Files> and auto-detect file type,
    assign different .text or attributes for C and ASM files.

    Args:
        files_node: <Files> XML node (MDK)
        path: absolute or relative file path
    """
    ext = os.path.splitext(path)[1].lower()

    # Determine file type
    if ext == ".c":
        file_type = "1"   # C file
    elif ext == ".s" and is_m55_path(xml_file):
        print(path)
        file_type = "2"
    else:
        return

    # Create file node
    file_node = ET.SubElement(files_node, "File")
    ET.SubElement(file_node, "FileName").text = os.path.basename(path)
    ET.SubElement(file_node, "FileType").text = file_type
    ET.SubElement(file_node, "FilePath").text = path

def update_mdk(xml_file):
    global lvgl, lv_examples, lv_demo

    print(f"<<<" + inspect.currentframe().f_code.co_name + ">>> " + xml_file)
    xml_dir = os.path.dirname(xml_file)

    # Read XML
    tree = ET.parse(xml_file)
    prj_targets = tree.findall('./Targets/Target')
    if prj_targets is not None:
        for target in prj_targets:
            TargetName = target.find('TargetName')
            if TargetName is None:
                continue
            groups = target.find('Groups')
            for gp, file_list in GROUP_FILES.items():
                # Find gp group
                for group in list(groups):
                    gname = group.find('GroupName')
                    if gname is not None and gname.text == gp:
                        # locate <Files>
                        files_node = group.find('Files')
                        if files_node is None:
                            files_node = ET.SubElement(group, 'Files')
                        # Clear existing file nodes
                        for old in list(files_node):
                            files_node.remove(old)
                        # Add new lvgl src
                        for f in file_list:
                            rel = os.path.relpath(f, xml_dir)
                            mdk_add_file_node(xml_file, files_node, rel)
                        #print(f"[{TargetName.text}] ??Updated {gp} group")
        # Save
        ET.indent(tree, space="  ", level=0)
        tree.write(xml_file, encoding="utf-8", xml_declaration=True, short_empty_elements=False)
        print(f"??Saved updated MDK project: {xml_file}")

def nueclipse_add_linked_file(linked_resources, gp, name, relpath):
    # Normalize path
    relpath = relpath.replace("\\", "/")

    # ---- Detect file type ----
    ext = os.path.splitext(name)[1].lower()
    if ext == ".s":
        return

    # ---- Calculate depth (number of "..") ----
    parts = relpath.split("/")
    depth = sum(1 for p in parts if p == "..")
    if depth < 1:
        depth = 1
    parent_str = f"PARENT-{depth}-PROJECT_LOC"

    # ---- Clean relative path (remove ..) ----
    clean_parts = [p for p in parts if p != ".."]
    clean_path = "/".join(clean_parts)

    # ---- Create <link> entry ----
    link = ET.SubElement(linked_resources, "link")
    ET.SubElement(link, "name").text = f"{gp}/{name}"
    ET.SubElement(link, "type").text = "1"
    ET.SubElement(link, "locationURI").text = f"{parent_str}/{clean_path}"

def update_nueclipse(xml_file):
    print(f"<<<{inspect.currentframe().f_code.co_name}>>> {xml_file}")

    xml_dir = os.path.dirname(xml_file)

    # Parse .project XML
    tree = ET.parse(xml_file)
    root = tree.getroot()

    # Locate <linkedResources>
    linked_resources = root.find(".//linkedResources")
    if linked_resources is None:
        print("??No <linkedResources> found in the project. Creating one.")
        linked_resources = ET.SubElement(root, "linkedResources")

    # ---- Step 1: Remove existing old lvgl-related links ----
    REMOVE_PREFIX = ("lvgl", "lv_examples", "lv_demo")

    CurPrjGroup = []
    for link in list(linked_resources):
        type_node = link.find("type")
        name_node = link.find("name")

        if name_node is None or type_node is None:
            continue

        # remove old linked C files
        if name_node.text.startswith(REMOVE_PREFIX) and type_node.text == "1":
            linked_resources.remove(link)
            #print(f"  - Removed old link: {name_node.text}")

        # collect existing group entries (type 2)
        if type_node.text == "2":
            CurPrjGroup.append(name_node.text)

    # ---- Step 2: Insert new links from GROUP_FILES ----
    for gp, file_list in GROUP_FILES.items():
        if gp in CurPrjGroup:
            for f in file_list:
                rel = os.path.relpath(f, xml_dir).replace("\\", "/")
                name = os.path.basename(f)
                nueclipse_add_linked_file(linked_resources, gp, name, rel)
            #print(f"  ??Updated {gp} ({len(file_list)} files))")

    # ---- Step 3: Save back (indent with spaces first) ----
    ET.indent(tree, space="    ", level=0)
    tree.write(xml_file, encoding="UTF-8", xml_declaration=True, short_empty_elements=False)

    # ---- Step 4: Replace 4 spaces ??tab ----
    with open(xml_file, "r", encoding="UTF-8") as f:
        xml_text = f.read()

    xml_text = xml_text.replace("    ", "\t")

    with open(xml_file, "w", encoding="UTF-8") as f:
        f.write(xml_text)

    print(f"??Saved updated NuEclipse project (TAB indented): {xml_file}")

def iar_add_file_node(group_node, xml_dir, file_path):
    """
    Add a <file> node to the given <group> node in an IAR .ewp project.
    Supports detecting whether the file is C (*.c) or ASM (*.S).

    - C file  : normal <file>
    - ASM file: adds <fileType>asm</fileType>

    Args:
        group_node: <group> XML element
        xml_dir: directory containing the .ewp file
        file_path: absolute or relative path to the source file
    """

    # Compute relative path from project dir
    rel_path = os.path.relpath(file_path, xml_dir)

    # --- Detect extension ---
    ext = os.path.splitext(file_path)[1].lower()

    if ext == ".c":
        # C file (IAR default: no extra tag)
        pass

    else:
        # Unknown ??treat as normal file
        return

    # Convert / to \ (Windows style)
    rel_path = rel_path.replace("/", "\\")

    # Full path with $PROJ_DIR$
    proj_path = f"$PROJ_DIR$\\{rel_path}"

    # Create <file> node
    file_node = ET.SubElement(group_node, "file")

    # <name>xxx</name>
    name_node = ET.SubElement(file_node, "name")
    name_node.text = proj_path


def update_iar9(xml_file):
    print(f"<<<{inspect.currentframe().f_code.co_name}>>> {xml_file}")

    xml_dir = os.path.dirname(xml_file)

    # Load XML
    tree = ET.parse(xml_file)
    root = tree.getroot()

    # group -> <group><name>xxx</name> ... <file><name>xxx</name>
    # Remove old files
    REMOVE_PREFIX = ("lvgl", "lv_examples", "lv_demo")

    for group in root.findall('.//group'):
        name_node = group.find('name')
        if name_node is None:
            continue

        gp_name = name_node.text.strip()

        # Skip groups not in GROUP_FILES
        if gp_name not in GROUP_FILES:
            continue

        # Clean old <file> entries
        for fnode in list(group.findall('file')):
            fname_node = fnode.find('name')
            if fname_node is None:
                continue
            ftext = fname_node.text.replace("\\", "/")
            group.remove(fnode)
            #print(f"Removed: {ftext}")

        # Add new files to this group
        for f in GROUP_FILES[gp_name]:
            iar_add_file_node(group, xml_dir, f)

        #print(f"??Updated IAR group {gp_name}")

    # Save with XML indent first (spaces)
    ET.indent(tree, space="    ", level=0)
    tree.write(xml_file, encoding="UTF-8", xml_declaration=True)

    print(f"??Saved updated IAR9 project (TAB indented): {xml_file}")

def update_csolution(yml_file):
    """
    Update a C solution YAML project file:
    - Remove old lvgl/lv_demo/lv_examples files
    - Insert new files from GROUP_FILES
    """

    print(f"<<<{inspect.currentframe().f_code.co_name}>>> {yml_file}")

    with open(yml_file, 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)

    if 'project' not in data or 'groups' not in data['project']:
        print("??No 'project' or 'groups' found in YAML.")
        return

    groups = data['project']['groups']
    REMOVE_PREFIX = ("lvgl", "lv_examples", "lv_demo")

    for group in groups:
        gp_name = group.get('group')
        print(gp_name)
        if not gp_name or gp_name not in GROUP_FILES:
            continue

        # ---- Step 1: Remove old files ----
        new_files = []
        group['files'] = new_files

        # ---- Step 2: Add new files ----
        group_files = []
        xml_dir = os.path.dirname(yml_file)
        for f in GROUP_FILES[gp_name]:
            rel_path = os.path.relpath(f, xml_dir).replace("\\", "/")
            group_files.append({'file': rel_path})

        group['files'].extend(group_files)

        print(f"??Updated group {gp_name} ({len(group['files'])} files)")

    # ---- Step 3: Write back YAML ----
    with open(yml_file, 'w', encoding='utf-8') as f:
        yaml.dump(
            data,
            f,
            default_flow_style=False,
            allow_unicode=True,
            sort_keys=False,
            indent=2
        )

    print(f"??Saved updated C solution YAML: {yml_file}")

UPDATE_FUNCTIONS = {
    #'*.project': update_nueclipse,
    '*.uvproj': update_mdk,
    '*.uvprojx': update_mdk,
    #'*.ewp': update_iar9,
    #'*.cproject.yml': update_csolution,
}

def update_project_file(file_abs_path, update_function):
    update_function(file_abs_path)

def update_lv_demo_group(board_dir):
    for dirPath, dirNames, fileNames in os.walk(board_dir):
        for pattern, update_function in UPDATE_FUNCTIONS.items():
            for file in fnmatch.filter(fileNames, pattern):
                prjFileAbsPath = os.path.join(board_dir, dirPath, file)
                update_project_file(prjFileAbsPath, update_function)

if __name__ == "__main__":

    gen_lvgl()
    gen_lv_demos()
    gen_lv_examples()

    dump_set(lvgl)
    dump_set(lv_demo)
    dump_set(lv_examples)

    board_cwd = os.path.abspath(os.path.join(os.getcwd(), '../../board/'))
    update_lv_demo_group(board_cwd)

    misc_cwd = os.path.abspath(os.path.join(os.getcwd(), '../../misc/'))
    update_lv_demo_group(misc_cwd)
