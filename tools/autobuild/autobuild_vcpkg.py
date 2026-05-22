"""
Autobuild script for CMSIS csolution projects (VSCode / vcpkg toolchain).

Walks PROJ_FOLDER_ROOTS for *.csolution.yml files, generates a PowerShell
build script, executes it, and reports errors. Supports incremental builds
by skipping projects whose inputs have not changed since the last success.
"""

import fnmatch
import os
import subprocess
import sys

import build_state
from config import (
    PROJ_FOLDER_ROOTS,
    VCPKG_BLACKLIST,
    VCPKG_LOG_FILE,
    VCPKG_PROJECT_PATTERN,
    VCPKG_STATE_FILE,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _collect_source_files(project_path: str) -> list[str]:
    """Collect relevant source files in the csolution project directory."""
    project_dir = os.path.dirname(project_path)
    files = [project_path]
    for src_root, dirs, src_files in os.walk(project_dir):
        # Skip build output directories
        dirs[:] = [d for d in dirs if d != 'out']
        for f in src_files:
            if f.endswith(('.c', '.h', '.s', '.S', '.ld', '.yml', '.yaml')):
                files.append(os.path.join(src_root, f))
    return files


def _blacklist_check(log_path: str) -> int:
    """Return count of blacklisted patterns found in build log."""
    try:
        with open(log_path, 'r', errors='replace') as fp:
            lines = fp.readlines()
    except OSError:
        return 0

    annotated = ''
    hits = 0
    for line in lines:
        annotated += line
        for pattern in VCPKG_BLACKLIST:
            pos = line.find(pattern)
            if pos >= 0:
                annotated += ' ' * pos + '^' * len(pattern) + ' <- Blacklist hit.\n\n'
                hits += 1

    if hits > 0:
        with open(log_path, 'w', errors='replace') as fp:
            fp.write(annotated)

    return hits


def _generate_build_script(output_dir: str, script_name: str, csolution_file: str) -> str:
    """Generate a PowerShell build script and return its path."""
    script_path = os.path.join(output_dir, script_name)
    content = f"""\
. ~/.vcpkg/vcpkg-init.ps1
vcpkg activate
cpackget list

$targets = @(cbuild list contexts "{csolution_file}")
if ($LASTEXITCODE -ne 0) {{
    exit 1
}}
$numberOfElements = $targets.Count

$okay = 0
foreach ($t in $targets) {{
    cbuild "{csolution_file}" --context $t -S
    cbuild "{csolution_file}" -S --rebuild --update-rte -d -v
    if ($LASTEXITCODE -eq 0) {{
        $okay += 1
    }}
}}

Write-Output "BuildMachine: built $okay target in $numberOfElements configuration."
if ($okay -eq $numberOfElements) {{
    exit 0
}} else {{
    exit 1
}}
"""
    with open(script_path, 'w') as fp:
        fp.write(content)
    return script_path


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    root = os.getcwd()
    state_path = os.path.join(root, VCPKG_STATE_FILE)
    state = build_state.load(state_path)

    errors = 0
    count = 0
    attachments: list[str] = []

    with open(VCPKG_LOG_FILE, 'w') as log:
        for folder in PROJ_FOLDER_ROOTS:
            if not os.path.isdir(folder):
                continue

            for dir_path, _, file_names in os.walk(folder):
                for fname in fnmatch.filter(file_names, VCPKG_PROJECT_PATTERN):
                    project_path = os.path.abspath(os.path.join(dir_path, fname))
                    project_dir = os.path.dirname(project_path)
                    build_log = project_path + '.log'

                    inputs = _collect_source_files(project_path)
                    signature = build_state.compute_signature(inputs)
                    key = os.path.normcase(project_path)

                    # Skip if unchanged since last success
                    if build_state.should_skip(state, key, signature):
                        log.write(f'[{count}] {project_path} skipped (previous success).\n')
                        print(f'[{count}] {project_path} skipped (previous success).', flush=True)
                        count += 1
                        log.flush()
                        continue

                    # Generate and run build script
                    prj_name = os.path.splitext(os.path.splitext(fname)[0])[0]
                    script_name = f'build_{prj_name}.ps1'
                    _generate_build_script(project_dir, script_name, fname)

                    cmd = f'cd "{project_dir}" & PowerShell -File ./{script_name} > "{build_log}"'

                    try:
                        ret = os.system(cmd)
                        hits = _blacklist_check(build_log)

                        if ret != 0 or hits > 0:
                            errors += 1
                            log.write(f'[{count}] {dir_path} has error or warning.\n')
                            print(f'[{count}] {project_path} has error or warning.', flush=True)
                            attachments.append(os.path.abspath(build_log))
                            build_state.record(state, project_path, signature, 'failed', build_log)
                        else:
                            print(f'[{count}] {project_path} pass...', flush=True)
                            build_state.record(state, project_path, signature, 'success', build_log)

                    except Exception:
                        errors += 1
                        log.write(f'[{count}] {dir_path} build exception.\n')
                        attachments.append(os.path.abspath(build_log))
                        build_state.record(state, project_path, signature, 'failed', build_log)

                    count += 1
                    log.flush()
                    build_state.save(state_path, state)

        if errors == 0:
            log.write(f'Build {count} projects successfully.\n')

    build_state.save(state_path, state)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
