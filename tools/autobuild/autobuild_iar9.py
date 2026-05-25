"""
Autobuild script for IAR Embedded Workbench projects.

Walks PROJ_FOLDER_ROOTS for *.ewp files, builds via iarbuild, and reports
errors. Supports incremental builds by skipping projects whose inputs have
not changed since the last success.
"""

import fnmatch
import os
import subprocess
import sys
import xml.etree.ElementTree as ET

import build_state
from config import (
    BUILD_TIMEOUT,
    IAR_EXE_PATH,
    IAR_LOG_FILE,
    IAR_PROJECT_EXTENSION,
    IAR_STATE_FILE,
    PROJ_FOLDER_ROOTS,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _get_project_input_files(project_path: str) -> list[str]:
    """Extract referenced source files from an IAR .ewp XML."""
    files = [project_path]
    try:
        tree = ET.parse(project_path)
    except Exception:
        return files

    project_dir = os.path.dirname(project_path)
    seen = {os.path.normcase(os.path.abspath(project_path))}

    for node in tree.getroot().iter('name'):
        text = (node.text or '').strip()
        if not text.startswith('$PROJ_DIR$'):
            continue
        rel = text.replace('$PROJ_DIR$', '').lstrip('\\/').replace('\\', os.sep)
        absolute = os.path.abspath(os.path.join(project_dir, rel))
        key = os.path.normcase(absolute)
        if key not in seen:
            seen.add(key)
            files.append(absolute)

    return files


def _check_build_log(log_path: str, project_name: str) -> bool:
    """
    Parse IAR build log. Returns True if all configurations built with
    zero errors and zero warnings.
    """
    try:
        with open(log_path, 'r', errors='replace') as fp:
            lines = fp.readlines()
    except OSError:
        return False

    total_conf = 0
    clean_count = 0
    pattern = f'{project_name} - '

    for line in lines:
        if line.startswith(' ERROR, '):
            break
        if pattern in line:
            total_conf += 1
        elif 'Total number of errors: 0' in line:
            clean_count += 1
        elif 'Total number of warnings: 0' in line:
            clean_count += 1

    return total_conf > 0 and clean_count == 2 * total_conf


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    si = subprocess.STARTUPINFO()
    si.dwFlags |= subprocess.STARTF_USESHOWWINDOW

    root = os.getcwd()
    state_path = os.path.join(root, IAR_STATE_FILE)
    state = build_state.load(state_path)

    errors = 0
    count = 0
    attachments: list[str] = []

    with open(IAR_LOG_FILE, 'w') as log:
        for folder in PROJ_FOLDER_ROOTS:
            if not os.path.isdir(folder):
                continue

            for dir_path, _, file_names in os.walk(folder):
                for fname in fnmatch.filter(file_names, IAR_PROJECT_EXTENSION):
                    os.chdir(dir_path)
                    project_path = os.path.abspath(fname)
                    build_log = os.path.abspath(fname + '.log')
                    inputs = _get_project_input_files(project_path)
                    signature = build_state.compute_signature(inputs)
                    key = os.path.normcase(project_path)

                    # Skip if unchanged since last success
                    if build_state.should_skip(state, key, signature):
                        log.write(f'[{count}] {project_path} skipped (previous success).\n')
                        print(f'[{count}] {project_path} skipped (previous success).', flush=True)
                        count += 1
                        log.flush()
                        os.chdir(root)
                        continue

                    prj_name = os.path.splitext(fname)[0]
                    cmd = f'"{IAR_EXE_PATH}" {fname} -build * -log warnings'

                    process = None
                    try:
                        with open(build_log, 'w') as fp:
                            process = subprocess.Popen(cmd, startupinfo=si, stdout=fp, stderr=fp)
                            process.wait(BUILD_TIMEOUT)

                        if _check_build_log(build_log, prj_name):
                            print(f'[{count}] {project_path} pass...', flush=True)
                            build_state.record(state, project_path, signature, 'success', build_log)
                        else:
                            errors += 1
                            log.write(f'[{count}] {dir_path} has error or warning.\n')
                            print(f'[{count}] {project_path} has error or warning.', flush=True)
                            attachments.append(build_log)
                            build_state.record(state, project_path, signature, 'failed', build_log)

                    except subprocess.TimeoutExpired:
                        if process is not None:
                            process.kill()
                        errors += 1
                        log.write(f'[{count}] Build {fname} timed out.\n')
                        print(f'[{count}] {project_path} build timed out.', flush=True)
                        attachments.append(build_log)
                        build_state.record(state, project_path, signature, 'failed', build_log)
                    except Exception:
                        errors += 1
                        log.write(f'[{count}] Build {fname} raised exception.\n')
                        print(f'[{count}] {project_path} build exception.', flush=True)
                        attachments.append(build_log)
                        build_state.record(state, project_path, signature, 'failed', build_log)

                    count += 1
                    log.flush()
                    build_state.save(state_path, state)
                    os.chdir(root)

        if errors == 0:
            log.write(f'Build {count} projects successfully.\n')

    build_state.save(state_path, state)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
