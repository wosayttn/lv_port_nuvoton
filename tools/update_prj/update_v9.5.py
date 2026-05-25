import fnmatch
import os
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

try:
    import yaml
except ImportError as exc:
    raise SystemExit(
        "PyYAML is required. Install it with: pip install PyYAML"
    ) from exc


REPO_ROOT = Path(__file__).resolve().parents[2]
LVGL_ROOT = REPO_ROOT / "lvgl"
LVGL_SRC_ROOT = LVGL_ROOT / "src"
DEMO_ROOT = LVGL_ROOT / "demos"
EXAMPLE_ROOT = LVGL_ROOT / "examples"
SCAN_ROOTS = [REPO_ROOT / "board", REPO_ROOT / "misc"]

GROUP_FILES = {
    "lvgl": [],
    "lv_demo": [],
    "lv_examples": [],
}

LVGL_EXCLUDE_DIRS = {
    "draw/dma2d",
    "draw/espressif",
    "draw/eve",
    "draw/nanovg",
    "draw/nema_gfx",
    "draw/nxp",
    "draw/opengles",
    "draw/renesas",
    "draw/sdl",
    "draw/vg_lite",
    "drivers",
    "libs/barcode",
    "libs/expat",
    "libs/freetype",
    "libs/frogfs",
    "libs/FT800-FT813",
    "libs/gltf",
    "libs/gstreamer",
    "libs/libjpeg_turbo",
    "libs/qrcode",
    "libs/rlottie",
    "libs/rle",
    "libs/svg",
    "libs/thorvg/rapidjson/msinttypes",
    "libs/tiny_ttf",
    "libs/vg_lite_driver",
    "others/fragment",
    "others/xml",
    "stdlib/clib",
    "stdlib/micropython",
    "stdlib/rtthread",
    "stdlib/uefi",
    "debugging/test",
}

DEMO_EXCLUDE_DIRS = {
    "gltf",
    "keypad_encoder",
    "music",
    "render",
    "stress",
    "vector_graphic",
}

PROJECT_UPDATERS = {}


def repo_rel(path: Path) -> str:
    return path.resolve().relative_to(REPO_ROOT).as_posix()


def project_rel(path: Path, project_dir: Path) -> str:
    return os.path.relpath(path, project_dir).replace("\\", "/")


def is_path_excluded(root: Path, base: Path, excludes: set[str]) -> bool:
    rel = root.resolve().relative_to(base.resolve()).as_posix()
    if rel == ".":
        return False
    return any(rel == excluded or rel.startswith(f"{excluded}/") for excluded in excludes)


def collect_group_files(base: Path, excludes: set[str] | None = None) -> list[Path]:
    files = []
    for root, _, filenames in os.walk(base):
        root_path = Path(root)
        if excludes and is_path_excluded(root_path, base, excludes):
            continue

        for filename in filenames:
            suffix = Path(filename).suffix.lower()
            if suffix in {".c", ".s"}:
                files.append((root_path / filename).resolve())

    return sorted(files, key=repo_rel)


def generate_groups() -> None:
    GROUP_FILES["lvgl"] = collect_group_files(LVGL_SRC_ROOT, LVGL_EXCLUDE_DIRS)
    GROUP_FILES["lv_demo"] = collect_group_files(DEMO_ROOT, DEMO_EXCLUDE_DIRS)
    GROUP_FILES["lv_examples"] = collect_group_files(EXAMPLE_ROOT)


def is_m55_project(path: Path) -> bool:
    path_text = path.as_posix().lower()
    return "m55m1" in path_text or "m5531" in path_text


def should_include_file(project_file: Path, file_path: Path) -> bool:
    if file_path.name == "lv_blend_helium.S" and not is_m55_project(project_file):
        return False
    return True


def mdk_add_file_node(project_file: Path, files_node: ET.Element, file_path: Path, project_dir: Path) -> None:
    if not should_include_file(project_file, file_path):
        return

    ext = file_path.suffix.lower()
    if ext == ".c":
        file_type = "1"
    elif ext == ".s" and is_m55_project(project_file):
        file_type = "2"
    else:
        return

    file_node = ET.SubElement(files_node, "File")
    ET.SubElement(file_node, "FileName").text = file_path.name
    ET.SubElement(file_node, "FileType").text = file_type
    ET.SubElement(file_node, "FilePath").text = project_rel(file_path, project_dir).replace("/", "\\")


def update_mdk(project_file: Path) -> None:
    tree = ET.parse(project_file)
    root = tree.getroot()
    project_dir = project_file.parent

    for target in root.findall("./Targets/Target"):
        groups = target.find("Groups")
        if groups is None:
            continue

        for group in list(groups):
            group_name = group.findtext("GroupName", default="").strip()
            if group_name not in GROUP_FILES:
                continue

            files_node = group.find("Files")
            if files_node is None:
                files_node = ET.SubElement(group, "Files")
            else:
                for old_node in list(files_node):
                    files_node.remove(old_node)

            for file_path in GROUP_FILES[group_name]:
                mdk_add_file_node(project_file, files_node, file_path, project_dir)

    ET.indent(tree, space="  ", level=0)
    tree.write(project_file, encoding="utf-8", xml_declaration=True, short_empty_elements=False)


def nueclipse_add_linked_file(project_file: Path, linked_resources: ET.Element, group_name: str, file_path: Path, project_dir: Path) -> None:
    if not should_include_file(project_file, file_path):
        return

    rel_path = project_rel(file_path, project_dir)
    parts = rel_path.split("/")
    depth = max(1, sum(1 for part in parts if part == ".."))
    parent_str = f"PARENT-{depth}-PROJECT_LOC"
    clean_path = "/".join(part for part in parts if part != "..")

    link = ET.SubElement(linked_resources, "link")
    ET.SubElement(link, "name").text = f"{group_name}/{file_path.name}"
    ET.SubElement(link, "type").text = "1"
    ET.SubElement(link, "locationURI").text = f"{parent_str}/{clean_path}"


def update_nueclipse(project_file: Path) -> None:
    tree = ET.parse(project_file)
    root = tree.getroot()
    project_dir = project_file.parent

    linked_resources = root.find(".//linkedResources")
    if linked_resources is None:
        linked_resources = ET.SubElement(root, "linkedResources")

    existing_groups = set()
    for link in list(linked_resources):
        type_text = link.findtext("type", default="")
        name_text = link.findtext("name", default="")

        if type_text == "2":
            existing_groups.add(name_text)

        if type_text == "1" and name_text.startswith(("lvgl/", "lv_demo/", "lv_examples/")):
            linked_resources.remove(link)

    for group_name, file_list in GROUP_FILES.items():
        if group_name not in existing_groups:
            continue
        for file_path in file_list:
            nueclipse_add_linked_file(project_file, linked_resources, group_name, file_path, project_dir)

    ET.indent(tree, space="    ", level=0)
    tree.write(project_file, encoding="UTF-8", xml_declaration=True, short_empty_elements=False)

    project_text = project_file.read_text(encoding="UTF-8")
    project_file.write_text(project_text.replace("    ", "\t"), encoding="UTF-8")


def iar_add_file_node(project_file: Path, group_node: ET.Element, file_path: Path, project_dir: Path) -> None:
    if not should_include_file(project_file, file_path):
        return

    if file_path.suffix.lower() not in {".c", ".s"}:
        return

    file_node = ET.SubElement(group_node, "file")
    rel_path = project_rel(file_path, project_dir).replace("/", "\\")
    ET.SubElement(file_node, "name").text = f"$PROJ_DIR$\\{rel_path}"


def update_iar(project_file: Path) -> None:
    tree = ET.parse(project_file)
    root = tree.getroot()
    project_dir = project_file.parent

    for group in root.findall(".//group"):
        group_name = group.findtext("name", default="").strip()
        if group_name not in GROUP_FILES:
            continue

        for file_node in list(group.findall("file")):
            group.remove(file_node)

        for file_path in GROUP_FILES[group_name]:
            iar_add_file_node(project_file, group, file_path, project_dir)

    ET.indent(tree, space="    ", level=0)
    tree.write(project_file, encoding="UTF-8", xml_declaration=True)


def update_csolution(project_file: Path) -> None:
    with project_file.open("r", encoding="utf-8") as file_handle:
        data = yaml.safe_load(file_handle)

    project = data.get("project", {})
    groups = project.get("groups", [])
    for group in groups:
        group_name = group.get("group")
        if group_name not in GROUP_FILES:
            continue

        group["files"] = [
            {"file": project_rel(file_path, project_file.parent)}
            for file_path in GROUP_FILES[group_name]
            if should_include_file(project_file, file_path)
        ]

    with project_file.open("w", encoding="utf-8") as file_handle:
        yaml.safe_dump(
            data,
            file_handle,
            allow_unicode=True,
            default_flow_style=False,
            sort_keys=False,
            indent=2,
        )


def register_updaters() -> None:
    PROJECT_UPDATERS.update(
        {
            "*.uvproj": update_mdk,
            "*.uvprojx": update_mdk,
            "*.ewp": update_iar,
            "*.project": update_nueclipse,
            "*.cproject.yml": update_csolution,
        }
    )


def update_project_tree(root_dir: Path) -> int:
    updated_count = 0
    for current_root, _, filenames in os.walk(root_dir):
        current_root_path = Path(current_root)
        for pattern, updater in PROJECT_UPDATERS.items():
            for filename in fnmatch.filter(filenames, pattern):
                project_file = current_root_path / filename
                updater(project_file)
                updated_count += 1
    return updated_count


def validate_layout() -> None:
    if not LVGL_ROOT.exists():
        raise SystemExit(f"LVGL directory not found: {LVGL_ROOT}")

    if not (LVGL_ROOT / "lv_version.h").exists():
        raise SystemExit("LVGL tree is incomplete; expected lv_version.h")


def main() -> int:
    validate_layout()
    register_updaters()
    generate_groups()

    updated_total = 0
    for scan_root in SCAN_ROOTS:
        if scan_root.exists():
            updated_total += update_project_tree(scan_root)

    version_text = (LVGL_ROOT / "lv_version.h").read_text(encoding="utf-8")
    if "#define LVGL_VERSION_MINOR 5" not in version_text:
        print("warning: lvgl tree is not at v9.5.x", file=sys.stderr)

    print(
        f"Updated {updated_total} project files with "
        f"{len(GROUP_FILES['lvgl'])} lvgl, {len(GROUP_FILES['lv_demo'])} demo, "
        f"{len(GROUP_FILES['lv_examples'])} example sources."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
