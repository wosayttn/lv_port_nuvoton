"""
Shared build-state utilities for autobuild scripts.

Provides incremental build support: if a project was previously built
successfully and none of its input files have changed, the build is skipped.
"""

import datetime
import hashlib
import json
import os


# ---------------------------------------------------------------------------
# State persistence
# ---------------------------------------------------------------------------

def load(state_path: str) -> dict:
    """Load build state from a JSON file. Returns empty dict on failure."""
    try:
        with open(state_path, 'r', encoding='utf-8') as fp:
            data = json.load(fp)
            return data if isinstance(data, dict) else {}
    except (FileNotFoundError, json.JSONDecodeError, OSError):
        return {}


def save(state_path: str, state: dict) -> None:
    """Atomically save build state to a JSON file."""
    temp_path = state_path + '.tmp'
    with open(temp_path, 'w', encoding='utf-8') as fp:
        json.dump(state, fp, indent=2, sort_keys=True)
    os.replace(temp_path, state_path)


# ---------------------------------------------------------------------------
# Signature computation
# ---------------------------------------------------------------------------

def _file_metadata(path: str) -> str:
    """Return size:mtime_ns string, or 'missing' if file not found."""
    try:
        st = os.stat(path)
        return f'{st.st_size}:{st.st_mtime_ns}'
    except (FileNotFoundError, OSError):
        return 'missing'


def compute_signature(file_list: list[str]) -> str:
    """
    Compute a SHA-256 signature over a sorted list of file paths and their
    metadata (size + mtime). Any change in the file set invalidates the hash.
    """
    sha = hashlib.sha256()
    for path in sorted(set(file_list), key=lambda p: os.path.normcase(p)):
        norm = os.path.normcase(os.path.abspath(path))
        sha.update(norm.encode('utf-8', errors='ignore'))
        sha.update(b'\0')
        sha.update(_file_metadata(path).encode('utf-8', errors='ignore'))
        sha.update(b'\0')
    return sha.hexdigest()


# ---------------------------------------------------------------------------
# State query / update helpers
# ---------------------------------------------------------------------------

def should_skip(state: dict, project_key: str, signature: str) -> bool:
    """Return True if the project was previously successful with same signature."""
    entry = state.get(project_key, {})
    return entry.get('status') == 'success' and entry.get('signature') == signature


def record(state: dict, project_path: str, signature: str, status: str,
           build_log: str = '') -> None:
    """Record a build result into the state dict."""
    state[os.path.normcase(os.path.abspath(project_path))] = {
        'status': status,
        'signature': signature,
        'build_log': os.path.abspath(build_log) if build_log else '',
        'updated_at': datetime.datetime.now().isoformat(timespec='seconds'),
    }
