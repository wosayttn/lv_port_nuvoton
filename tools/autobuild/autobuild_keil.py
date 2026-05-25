"""
Autobuild script for Keil MDK (µVision) projects.

Walks PROJ_FOLDER_ROOTS for *.uvproj / *.uvprojx files, builds each one via
the Uv4 command-line interface, and reports errors. Supports incremental builds
by skipping projects whose inputs have not changed since the last success.
"""

import fnmatch
import mmap
import os
import subprocess
import sys
import xml.etree.ElementTree as ET

import build_state
from config import (
    BUILD_TIMEOUT,
    KEIL_BLACKLIST,
    KEIL_LOG_FILE,
    KEIL_PROJECT_EXTENSIONS,
    KEIL_STATE_FILE,
    KEIL_UV4_PATH,
    KEIL_WHITELIST,
    PROJ_FOLDER_ROOTS,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _get_project_input_files(project_path: str) -> list[str]:
    """Extract referenced source files from a Keil .uvproj(x) XML."""
    files = [project_path]
    try:
        tree = ET.parse(project_path)
    except Exception:
        return files

    project_dir = os.path.dirname(project_path)
    seen = {os.path.normcase(os.path.abspath(project_path))}

    for node in tree.getroot().iter('FilePath'):
        rel = (node.text or '').strip()
        if not rel:
            continue
        absolute = os.path.abspath(os.path.join(project_dir, rel.replace('\\', os.sep)))
        key = os.path.normcase(absolute)
        if key not in seen:
            seen.add(key)
            files.append(absolute)

    return files


def _blacklist_check(log_path: str) -> int:
    """Return count of blacklisted patterns found (minus whitelisted)."""
    with open(log_path, 'r', errors='replace') as fp:
        lines = fp.readlines()

    annotated = ''
    hits = 0
    for line in lines:
        annotated += line
        for pattern in KEIL_BLACKLIST:
            pos = line.find(pattern)
            if pos >= 0:
                annotated += ' ' * pos + '^' * len(pattern) + ' <- Blacklist hit.\n\n'
                hits += 1
        for pattern in KEIL_WHITELIST:
            if pattern in line:
                hits -= 1

    if hits > 0:
        with open(log_path, 'w', errors='replace') as fp:
            fp.write(annotated)

    return hits


def _disable_cross_module_opt(project_file: str) -> None:
    """Patch <OptFeed>1</OptFeed> to 0 to speed up builds."""
    try:
        with open(project_file, 'r+b') as fh:
            mm = mmap.mmap(fh.fileno(), 0)
            pos = mm.find(b'<OptFeed>1</OptFeed>')
            if pos != -1:
                mm.seek(pos + 9)
                mm.write(b'0')
            mm.close()
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    si = subprocess.STARTUPINFO()
    si.dwFlags |= subprocess.STARTF_USESHOWWINDOW

    root = os.getcwd()
    state_path = os.path.join(root, KEIL_STATE_FILE)
    state = build_state.load(state_path)

    errors = 0
    count = 0
    attachments: list[str] = []

    print(f'User-> {os.environ.get("USERNAME", "unknown")}', flush=True)

    with open(KEIL_LOG_FILE, 'w+') as log:
        for folder in PROJ_FOLDER_ROOTS:
            if not os.path.isdir(folder):
                continue

            for dir_path, _, file_names in os.walk(folder):
                for ext in KEIL_PROJECT_EXTENSIONS:
                    for fname in fnmatch.filter(file_names, ext):
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

                        _disable_cross_module_opt(fname)

                        try:
                            # Clean
                            p = subprocess.Popen(
                                f'{KEIL_UV4_PATH} -j0 -c {fname}',
                                startupinfo=si, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                            )
                            p.wait(BUILD_TIMEOUT)

                            # Build
                            p = subprocess.Popen(
                                f'{KEIL_UV4_PATH} -b -j0 -z -o {build_log} {fname}',
                                startupinfo=si, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                            )
                            p.wait(BUILD_TIMEOUT)

                            hits = _blacklist_check(build_log)
                            if hits > 0:
                                errors += 1
                                log.write(f'[{count}] {project_path} error/warning({hits}).\n')
                                print(f'[{count}] {project_path} has error or warning.', flush=True)
                                attachments.append(build_log)
                                build_state.record(state, project_path, signature, 'failed', build_log)
                            else:
                                print(f'[{count}] {project_path} pass...', flush=True)
                                build_state.record(state, project_path, signature, 'success', build_log)

                        except subprocess.TimeoutExpired:
                            p.kill()
                            errors += 1
                            log.write(f'[{count}] Build {fname} timed out.\n')
                            build_state.record(state, project_path, signature, 'failed', build_log)
                        except Exception:
                            errors += 1
                            log.write(f'[{count}] Build {fname} raised exception.\n')
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
