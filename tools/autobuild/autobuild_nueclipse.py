"""
Autobuild script for NuEclipse (GCC) projects.

Walks PROJ_FOLDER_ROOTS for .cproject files, builds via the Eclipse headless
builder, and reports errors. Supports incremental builds by skipping projects
whose inputs have not changed since the last success.
"""

import fnmatch
import os
import shutil
import subprocess
import sys
import tempfile

import build_state
from config import (
    NUECLIPSE_BLACKLIST,
    NUECLIPSE_EXE_PATH,
    NUECLIPSE_LOG_FILE,
    NUECLIPSE_LOG_SKIP_LINES,
    NUECLIPSE_PROJECT_FILE,
    NUECLIPSE_STATE_FILE,
    PROJ_FOLDER_ROOTS,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _collect_source_files(project_dir: str) -> list[str]:
    """Collect source/header/linker files under the project directory."""
    files: list[str] = []
    project_file = os.path.join(project_dir, '.project')
    if os.path.isfile(project_file):
        files.append(project_file)
    cproject_file = os.path.join(project_dir, '.cproject')
    if os.path.isfile(cproject_file):
        files.append(cproject_file)

    for root_dir, _, filenames in os.walk(project_dir):
        for f in filenames:
            if f.endswith(('.c', '.h', '.s', '.S', '.ld')):
                files.append(os.path.join(root_dir, f))
    return files


def _blacklist_check(log_path: str) -> int:
    """Return count of blacklisted patterns found in build log."""
    try:
        with open(log_path, 'r', errors='replace') as fp:
            lines = fp.readlines()
    except OSError:
        return 0

    # Skip initial JVM deprecation warnings
    lines = lines[NUECLIPSE_LOG_SKIP_LINES:]

    annotated = ''
    hits = 0
    for line in lines:
        annotated += line
        for pattern in NUECLIPSE_BLACKLIST:
            pos = line.find(pattern)
            if pos >= 0:
                annotated += ' ' * pos + '^' * len(pattern) + ' <- Blacklist hit.\n\n'
                hits += 1

    if hits > 0:
        with open(log_path, 'w', errors='replace') as fp:
            fp.write(annotated)

    return hits


def _create_temp_workspace(root_dir: str) -> str:
    """Create a unique temporary Eclipse workspace under the autobuild folder."""
    return tempfile.mkdtemp(prefix='Temp_', dir=root_dir)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    si = subprocess.STARTUPINFO()
    si.dwFlags |= subprocess.STARTF_USESHOWWINDOW

    root = os.getcwd()
    state_path = os.path.join(root, NUECLIPSE_STATE_FILE)
    state = build_state.load(state_path)

    errors = 0
    count = 0

    with open(NUECLIPSE_LOG_FILE, 'w') as log:
        for folder in PROJ_FOLDER_ROOTS:
            if not os.path.isdir(folder):
                continue

            for dir_path, _, file_names in os.walk(folder):
                for fname in fnmatch.filter(file_names, NUECLIPSE_PROJECT_FILE):
                    project_path = os.path.abspath(os.path.join(dir_path, fname))
                    project_dir = os.path.dirname(project_path)
                    basename = os.path.basename(os.path.dirname(dir_path))
                    build_log = os.path.abspath(os.path.join(dir_path, basename) + '.log')

                    inputs = _collect_source_files(project_dir)
                    signature = build_state.compute_signature(inputs)
                    key = os.path.normcase(project_path)

                    # Skip if unchanged since last success
                    if build_state.should_skip(state, key, signature):
                        log.write(f'[{count}] {project_path} skipped (previous success).\n')
                        print(f'[{count}] {project_path} skipped (previous success).', flush=True)
                        count += 1
                        log.flush()
                        continue

                    # Prepare temp workspace
                    temp_dir = _create_temp_workspace(root)

                    cmd = (
                        f'{NUECLIPSE_EXE_PATH}'
                        f' -nosplash --launcher.suppressErrors'
                        f' -application org.eclipse.cdt.managedbuilder.core.headlessbuild'
                        f' -data {temp_dir} -import {dir_path} -build all'
                    )

                    try:
                        with open(build_log, 'w') as fp:
                            subprocess.check_call(cmd, startupinfo=si, stdout=fp, stderr=fp)

                        hits = _blacklist_check(build_log)
                        if hits > 0:
                            errors += 1
                            log.write(f'[{count}] {dir_path} has error or warning.\n')
                            print(f'[{count}] {dir_path} has error or warning.', flush=True)
                            build_state.record(state, project_path, signature, 'failed', build_log)
                        else:
                            print(f'[{count}] {dir_path} pass...', flush=True)
                            build_state.record(state, project_path, signature, 'success', build_log)

                    except Exception:
                        errors += 1
                        log.write(f'[{count}] {dir_path} build failed.\n')
                        build_state.record(state, project_path, signature, 'failed', build_log)
                    finally:
                        shutil.rmtree(temp_dir, ignore_errors=True)

                    count += 1
                    log.flush()
                    build_state.save(state_path, state)

        if errors == 0:
            log.write(f'Build {count} projects successfully.\n')

    build_state.save(state_path, state)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
