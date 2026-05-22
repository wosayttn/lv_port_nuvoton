from __future__ import annotations

import argparse
import configparser
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Update git submodules declared under bsp/* in this repository."
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="Repository root path. Defaults to the parent of this script's directory.",
    )
    parser.add_argument(
        "--no-recursive",
        action="store_true",
        help="Do not pass --recursive to git submodule update.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the git commands without executing them.",
    )
    return parser.parse_args()


def load_bsp_submodules(repo_root: Path) -> list[str]:
    gitmodules_path = repo_root / ".gitmodules"
    if not gitmodules_path.is_file():
        raise FileNotFoundError(f"Cannot find .gitmodules: {gitmodules_path}")

    parser = configparser.ConfigParser(interpolation=None)
    parser.read(gitmodules_path, encoding="utf-8")

    paths: list[str] = []
    for section in parser.sections():
        if not parser.has_option(section, "path"):
            continue

        submodule_path = parser.get(section, "path").strip().replace("\\", "/")
        if not submodule_path.startswith("bsp/"):
            continue

        relative_parts = Path(submodule_path).parts
        if len(relative_parts) != 2:
            continue

        paths.append(submodule_path)

    return sorted(paths)


def run_update(repo_root: Path, submodule_path: str, recursive: bool, dry_run: bool) -> int:
    command = ["git", "submodule", "update", "--init"]
    if recursive:
        command.append("--recursive")
    command.append(submodule_path)

    print(f"[INFO] {' '.join(command)}")
    if dry_run:
        return 0

    completed = subprocess.run(command, cwd=repo_root, check=False)
    return completed.returncode


def main() -> int:
    args = parse_args()
    repo_root = args.repo_root.resolve()

    try:
        submodules = load_bsp_submodules(repo_root)
    except Exception as exc:
        print(f"[ERROR] {exc}", file=sys.stderr)
        return 1

    if not submodules:
        print("[WARN] No bsp/* submodules found in .gitmodules")
        return 0

    recursive = not args.no_recursive
    failed_paths: list[str] = []

    for submodule_path in submodules:
        return_code = run_update(
            repo_root=repo_root,
            submodule_path=submodule_path,
            recursive=recursive,
            dry_run=args.dry_run,
        )
        if return_code != 0:
            failed_paths.append(submodule_path)

    if failed_paths:
        print("[ERROR] Failed submodules:")
        for failed_path in failed_paths:
            print(f"  - {failed_path}")
        return 1

    print(f"[OK] Updated {len(submodules)} bsp submodule(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())