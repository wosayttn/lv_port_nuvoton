from __future__ import annotations

import argparse
import configparser
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class SubmoduleInfo:
    path: str
    branch: str | None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Pull the latest commits for git submodules declared under bsp/* in this repository."
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="Repository root path. Defaults to the parent of this script's directory.",
    )
    parser.add_argument(
        "--remote",
        default="origin",
        help="Remote name used for git pull. Defaults to origin.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the git commands without executing them.",
    )
    return parser.parse_args()


def load_bsp_submodules(repo_root: Path) -> list[SubmoduleInfo]:
    gitmodules_path = repo_root / ".gitmodules"
    if not gitmodules_path.is_file():
        raise FileNotFoundError(f"Cannot find .gitmodules: {gitmodules_path}")

    parser = configparser.ConfigParser(interpolation=None)
    parser.read(gitmodules_path, encoding="utf-8")

    submodules: list[SubmoduleInfo] = []
    for section in parser.sections():
        if not parser.has_option(section, "path"):
            continue

        submodule_path = parser.get(section, "path").strip().replace("\\", "/")
        if not submodule_path.startswith("bsp/"):
            continue

        relative_parts = Path(submodule_path).parts
        if len(relative_parts) != 2:
            continue

        branch = None
        if parser.has_option(section, "branch"):
            branch = parser.get(section, "branch").strip() or None

        submodules.append(SubmoduleInfo(path=submodule_path, branch=branch))

    return sorted(submodules, key=lambda item: item.path)


def run_command(
    command: list[str],
    cwd: Path,
    dry_run: bool,
    *,
    allow_in_dry_run: bool = False,
) -> subprocess.CompletedProcess[str] | None:
    print(f"[INFO] ({cwd.as_posix()}) {' '.join(command)}")
    if dry_run and not allow_in_dry_run:
        return None

    return subprocess.run(command, cwd=cwd, text=True, capture_output=True, check=False)


def print_command_output(completed: subprocess.CompletedProcess[str] | None) -> None:
    if completed is None:
        return
    if completed.stdout:
        print(completed.stdout, end="")
    if completed.stderr:
        print(completed.stderr, end="", file=sys.stderr)


def get_current_branch(submodule_dir: Path, dry_run: bool) -> str | None:
    completed = run_command(
        ["git", "rev-parse", "--abbrev-ref", "HEAD"],
        cwd=submodule_dir,
        dry_run=dry_run,
        allow_in_dry_run=True,
    )
    if completed is None:
        return None
    if completed.returncode != 0:
        print_command_output(completed)
        return None

    branch_name = completed.stdout.strip()
    if branch_name == "HEAD":
        return None
    return branch_name or None


def get_remote_default_branch(submodule_dir: Path, remote: str, dry_run: bool) -> str | None:
    completed = run_command(
        ["git", "symbolic-ref", f"refs/remotes/{remote}/HEAD", "--short"],
        cwd=submodule_dir,
        dry_run=dry_run,
        allow_in_dry_run=True,
    )
    if completed is None:
        return None
    if completed.returncode != 0:
        print_command_output(completed)
        return None

    ref_name = completed.stdout.strip()
    if "/" not in ref_name:
        return None

    return ref_name.rsplit("/", 1)[-1] or None


def checkout_branch(submodule_dir: Path, branch: str, dry_run: bool) -> bool:
    completed = run_command(["git", "checkout", branch], cwd=submodule_dir, dry_run=dry_run)
    if completed is None:
        return True

    print_command_output(completed)
    return completed.returncode == 0


def pull_submodule(submodule_dir: Path, remote: str, branch: str, dry_run: bool) -> bool:
    completed = run_command(
        ["git", "pull", "--ff-only", remote, branch],
        cwd=submodule_dir,
        dry_run=dry_run,
    )
    if completed is None:
        return True

    print_command_output(completed)
    return completed.returncode == 0


def update_one_submodule(repo_root: Path, submodule: SubmoduleInfo, remote: str, dry_run: bool) -> bool:
    submodule_dir = repo_root / submodule.path
    if not submodule_dir.is_dir():
        print(f"[ERROR] Missing submodule directory: {submodule.path}", file=sys.stderr)
        return False

    current_branch = get_current_branch(submodule_dir, dry_run=dry_run)
    target_branch = current_branch or submodule.branch or get_remote_default_branch(
        submodule_dir,
        remote=remote,
        dry_run=dry_run,
    )

    if target_branch is None:
        print(
            f"[ERROR] Cannot determine branch for {submodule.path}; configure it in .gitmodules or check out a branch first.",
            file=sys.stderr,
        )
        return False

    if current_branch is None and not checkout_branch(submodule_dir, target_branch, dry_run=dry_run):
        return False

    return pull_submodule(submodule_dir, remote=remote, branch=target_branch, dry_run=dry_run)


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

    failed_paths: list[str] = []
    for submodule in submodules:
        if not update_one_submodule(repo_root, submodule, remote=args.remote, dry_run=args.dry_run):
            failed_paths.append(submodule.path)

    if failed_paths:
        print("[ERROR] Failed submodules:")
        for failed_path in failed_paths:
            print(f"  - {failed_path}")
        return 1

    print(f"[OK] Pulled {len(submodules)} bsp submodule(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())