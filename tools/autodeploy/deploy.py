#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
deploy.py - Nuvoton MCU Auto-Deploy & Flashing CLI Tool via probe-rs
Supports Device ID probing, APROM firmware programming, target reset, and target YAML generation.
Supported MCU Platforms: M3331, M5531, M3351, M55M1, M460, M480, M2354, M2L31, M031, etc.
"""

import argparse
import json
import os
import sys
from typing import Optional

# Support both standalone script execution and module import
try:
    from .nuvoton_db import (
        decode_pdid,
        get_target_yaml_path,
        FAMILY_CONFIGS,
        DEFAULT_RESET_PDIDS,
    )
    from .probe_rs import ProbeRsWrapper, ProbeRsError, ProbeRsNotFound
    from .flm_parser import generate_target_yaml
except (ImportError, ValueError):
    current_dir = os.path.dirname(os.path.abspath(__file__))
    if current_dir not in sys.path:
        sys.path.insert(0, current_dir)
    from nuvoton_db import (
        decode_pdid,
        get_target_yaml_path,
        FAMILY_CONFIGS,
        DEFAULT_RESET_PDIDS,
    )
    from probe_rs import ProbeRsWrapper, ProbeRsError, ProbeRsNotFound
    from flm_parser import generate_target_yaml


class Colors:
    """ANSI color codes for terminal formatting."""
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BOLD = '\033[1m'
    DIM = '\033[2m'
    RESET = '\033[0m'

    @classmethod
    def disable(cls):
        cls.HEADER = ''
        cls.BLUE = ''
        cls.CYAN = ''
        cls.GREEN = ''
        cls.YELLOW = ''
        cls.RED = ''
        cls.BOLD = ''
        cls.DIM = ''
        cls.RESET = ''


# Disable colors on Windows if ANSI is not supported
if os.name == 'nt' and not os.environ.get('WT_SESSION') and not os.environ.get('ANSICON'):
    try:
        import ctypes
        kernel32 = ctypes.windll.kernel32
        # Enable ENABLE_VIRTUAL_TERMINAL_PROCESSING
        kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
    except Exception:
        Colors.disable()


def print_banner():
    banner = f"""{Colors.CYAN}{Colors.BOLD}
  _   _                 _               __  __  ____ _   _ 
 | \\ | |_   ___   _____| |_ ___  _ __  |  \\/  |/ ___| | | |
 |  \\| | | | \\ \\ / / _ \\ __/ _ \\| '_ \\ | |\\/| | |   | | | |
 | |\\  | |_| |\\ V / (_) | || (_) | | | || |  | | |___| |_| |
 |_| \\_|\\__,_| \\_/ \\___/ \\__\\___/|_| |_||_|  |_|\\____|\\___/ 
            Auto-Deploy Tool (powered by probe-rs)
{Colors.RESET}"""
    print(banner)


def cmd_list_probes(wrapper: ProbeRsWrapper, args: argparse.Namespace):
    """List connected debug probes."""
    probes = wrapper.list_probes()
    if not probes:
        print(f"{Colors.YELLOW}No debug probes were found connected.{Colors.RESET}")
        print("Tip: Check Nu-Link2 / CMSIS-DAP USB cable connection.")
        return 1

    print(f"{Colors.GREEN}{Colors.BOLD}Connected Debug Probes ({len(probes)} found):{Colors.RESET}")
    for p in probes:
        print(f"  [{p['index']}] {Colors.BOLD}{p['name']}{Colors.RESET}")
        if p.get('selector'):
            print(f"      Selector: {p['selector']}")
        if p.get('serial'):
            print(f"      Serial  : {p['serial']}")
        if p.get('details'):
            print(f"      Details : {p['details']}")
    print(f"\n{Colors.DIM}Hint: You can pass --probe 0 or --probe 1 to select by index.{Colors.RESET}")
    return 0


def cmd_list_targets(args: argparse.Namespace):
    """List supported Nuvoton MCU targets and families."""
    print(f"{Colors.GREEN}{Colors.BOLD}Supported Nuvoton MCU Platforms:{Colors.RESET}\n")
    print(f"{'Family':<10} {'Core':<12} {'Arch':<10} {'Flash Base':<12} {'Flash Size':<12} {'Target YAML'}")
    print("-" * 75)
    for fam, cfg in FAMILY_CONFIGS.items():
        fb = f"0x{cfg['flash_base']:08X}"
        fs = f"{cfg['flash_size'] // 1024} KB"
        yaml_name = cfg.get('target_yaml') or "Built-in"
        print(f"{fam:<10} {cfg['core']:<12} {cfg['arch']:<10} {fb:<12} {fs:<12} {yaml_name}")
    print()
    return 0


def print_device_card(dev_info: dict, index_str: str = ""):
    """Print formatted device specifications card."""
    if "error" in dev_info:
        header = f"[{index_str}] Probe [{dev_info.get('probe_index', 0)}]: {dev_info.get('probe_name', '')} -- Failed"
        print(f"{Colors.RED}{Colors.BOLD}{header}{Colors.RESET}")
        print(f"  {Colors.RED}Error: {dev_info['error']}{Colors.RESET}\n")
        return

    header = f"[{index_str}] {dev_info['part_name']} ({dev_info['family_name']})" if index_str else "Target Probe Successful!"
    print(f"{Colors.GREEN}{Colors.BOLD}{header}{Colors.RESET}")
    if dev_info.get("probe_selector"):
        print(f"  {Colors.CYAN}Debug Probe     :{Colors.RESET} [{dev_info.get('probe_index', 0)}] {dev_info.get('probe_name', '')} ({dev_info['probe_selector']})")
    print(f"  {Colors.CYAN}Part Number     :{Colors.RESET} {Colors.BOLD}{dev_info['part_name']}{Colors.RESET}")
    print(f"  {Colors.CYAN}MCU Family      :{Colors.RESET} {dev_info['family_name']} ({dev_info['family']})")
    print(f"  {Colors.CYAN}Device ID (PDID):{Colors.RESET} {dev_info['pdid']} (SYS_PDID @ 0x40000000)")
    print(f"  {Colors.CYAN}CPU Core        :{Colors.RESET} {dev_info['core']} ({dev_info['arch']})")
    print(f"  {Colors.CYAN}APROM Base Addr :{Colors.RESET} 0x{dev_info['flash_base']:08X}")
    print(f"  {Colors.CYAN}APROM Flash Size:{Colors.RESET} {dev_info['flash_size'] // 1024} KB")
    print(f"  {Colors.CYAN}Flash Page Size :{Colors.RESET} {dev_info['page_size']} Bytes")
    print(f"  {Colors.CYAN}SRAM Base Addr  :{Colors.RESET} 0x{dev_info['ram_base']:08X}")
    print(f"  {Colors.CYAN}SRAM Size       :{Colors.RESET} {dev_info['ram_size'] // 1024} KB")

    target_yaml = get_target_yaml_path(dev_info['family'])
    if target_yaml:
        print(f"  {Colors.CYAN}Target Profile  :{Colors.RESET} {target_yaml}")
    else:
        print(f"  {Colors.YELLOW}Target Profile  :{Colors.RESET} Generic {dev_info['core']}")
    print()


def cmd_probe(wrapper: ProbeRsWrapper, args: argparse.Namespace):
    """Probe connected Nuvoton MCU target Device ID."""
    try:
        if args.probe is not None:
            # Probe specific probe
            dev_info = wrapper.probe_target(
                probe=args.probe,
                speed=args.speed,
                preferred_family=args.chip
            )
            devices = [dev_info]
        else:
            # Probe all connected probes
            devices = wrapper.probe_all_targets(
                speed=args.speed,
                preferred_family=args.chip
            )
    except ProbeRsError as e:
        print(f"{Colors.RED}Target Probe Error: {e}{Colors.RESET}")
        return 1

    if getattr(args, 'json', False):
        if len(devices) == 1 and args.probe is not None:
            print(json.dumps(devices[0], indent=2))
        else:
            print(json.dumps(devices, indent=2))
        return 0

    if len(devices) == 1:
        print_device_card(devices[0])
    else:
        print(f"{Colors.GREEN}{Colors.BOLD}Target Probe Successful! Found {len(devices)} connected target devices:{Colors.RESET}\n")
        for i, dev in enumerate(devices):
            print_device_card(dev, index_str=f"Device {i + 1}/{len(devices)}")

    return 0


def cmd_program(wrapper: ProbeRsWrapper, args: argparse.Namespace):
    """Program firmware image to target APROM."""
    firmware_path = os.path.abspath(args.firmware)
    if not os.path.exists(firmware_path):
        print(f"{Colors.RED}Error: Firmware file '{firmware_path}' not found!{Colors.RESET}")
        return 1

    file_size = os.path.getsize(firmware_path)
    print(f"{Colors.BLUE}Firmware image:{Colors.RESET} {firmware_path} ({file_size} bytes)")

    # Target chip resolution
    chip_family = args.chip
    target_yaml = args.target_yaml
    base_address = args.address

    # Auto-probe if chip or target YAML not explicitly given
    if not chip_family and not target_yaml:
        print(f"{Colors.CYAN}Auto-probing connected target device...{Colors.RESET}")
        try:
            dev_info = wrapper.probe_target(probe=args.probe, speed=args.speed)
            chip_family = dev_info['family']
            print(f"{Colors.GREEN}Detected target:{Colors.RESET} {dev_info['part_name']} ({dev_info['family_name']})")
            if base_address is None:
                base_address = dev_info['flash_base']
        except ProbeRsError as e:
            print(f"{Colors.YELLOW}Warning: Target auto-probe failed ({e}).{Colors.RESET}")
            print("Please specify --chip (e.g. --chip M3331, M5531, M3351) or --target-yaml.")
            return 1

    # Resolve target YAML
    if not target_yaml and chip_family:
        target_yaml = get_target_yaml_path(chip_family)
        if not target_yaml:
            print(f"{Colors.RED}Error: No bundled target profile found for family '{chip_family}'.{Colors.RESET}")
            print(f"Use 'deploy.py list-targets' to view supported platforms or specify --target-yaml.")
            return 1

    # Default base address if still None
    if base_address is None:
        fam_cfg = FAMILY_CONFIGS.get(chip_family, {})
        base_address = fam_cfg.get('flash_base', 0x00000000)

    print(f"{Colors.BLUE}Target Family  :{Colors.RESET} {chip_family}")
    if target_yaml:
        print(f"{Colors.BLUE}Target YAML    :{Colors.RESET} {target_yaml}")
    print(f"{Colors.BLUE}APROM Base     :{Colors.RESET} 0x{base_address:08X}")
    print(f"{Colors.BLUE}Verify         :{Colors.RESET} {'Yes' if not args.no_verify else 'No'}")
    print(f"{Colors.BLUE}Chip Erase     :{Colors.RESET} {'Yes' if args.chip_erase else 'No'}")
    print(f"{Colors.BLUE}Reset on finish:{Colors.RESET} {'Yes' if not args.no_reset else 'No'}")
    print()

    try:
        print(f"{Colors.CYAN}Starting firmware download via probe-rs...{Colors.RESET}")
        wrapper.download_firmware(
            firmware_path=firmware_path,
            chip=chip_family,
            target_yaml=target_yaml,
            base_address=base_address,
            binary_format=args.format,
            verify=not args.no_verify,
            chip_erase=args.chip_erase,
            reset=not args.no_reset,
            probe=args.probe,
            speed=args.speed,
            dry_run=args.dry_run,
            stream_output=True
        )
        print(f"\n{Colors.GREEN}{Colors.BOLD}Firmware successfully programmed and verified!{Colors.RESET}")

        if getattr(args, "observe", 0) > 0 and not args.no_reset and not args.dry_run:
            observe_sec = float(args.observe)
            print(f"{Colors.CYAN}[Observe] Waiting {observe_sec:.1f}s to observe MCU runtime execution...{Colors.RESET}")
            obs = wrapper.observe_target_health(
                duration_sec=observe_sec,
                chip=chip_family,
                target_yaml=target_yaml,
                probe=args.probe,
                speed=args.speed
            )
            if obs.get("serial_logs"):
                print(f"{Colors.BLUE}[Serial Logs (115200)]:\n{obs['serial_logs']}{Colors.RESET}")
            if not obs.get("healthy", True):
                print(f"{Colors.RED}[Observe] Runtime Fault: {obs.get('detail')}{Colors.RESET}")
                return 1
            else:
                print(f"{Colors.GREEN}[Observe] MCU is running healthy: {obs.get('detail')}{Colors.RESET}")

        return 0
    except ProbeRsError as e:
        print(f"\n{Colors.RED}Download Error: {e}{Colors.RESET}")
        return 1


def find_candidate_binaries(search_dir: str) -> list:
    """Find all potential firmware files (.bin, .hex, .elf) under directory."""
    exts = ('.bin', '.hex', '.elf')
    candidates = []
    if os.path.isfile(search_dir):
        if search_dir.lower().endswith(exts):
            return [search_dir]
        return []
    for root, _, files in os.walk(search_dir):
        for f in files:
            if f.lower().endswith(exts):
                candidates.append(os.path.join(root, f))
    return candidates


def match_binary_for_device(dev: dict, candidate_files: list, filter_str: Optional[str] = None) -> Optional[str]:
    """
    Match the best firmware binary for a given target device from candidate files.
    Scores candidates based on part number, family, directory ('Bin/'), and recency.
    """
    part = dev.get('part_name', '').lower()
    fam = dev.get('family', '').lower()
    prefix = part[:5] if len(part) >= 5 else part

    scored = []
    for fp in candidate_files:
        f_norm = fp.replace('\\', '/').lower()
        fn = os.path.basename(f_norm)

        if filter_str and filter_str.lower() not in f_norm:
            continue

        score = 0
        device_matched = False
        if prefix and prefix in fn:
            score += 50
            device_matched = True
        elif prefix and prefix in f_norm:
            score += 30
            device_matched = True

        if fam and fam in fn:
            score += 20
            device_matched = True
        elif fam and fam in f_norm:
            score += 10
            device_matched = True

        # If this file doesn't match the target device at all, skip it
        if not device_matched:
            continue

        # Prefer Bin/ or bin/ folder (standard release build directory)
        if '/bin/' in f_norm:
            score += 25

        # Prefer .bin over other extensions
        if fn.endswith('.bin'):
            score += 5

        if score > 0:
            try:
                mtime = os.path.getmtime(fp)
            except OSError:
                mtime = 0
            scored.append((score, mtime, fp))

    scored.sort(key=lambda x: (x[0], x[1]), reverse=True)
    return scored[0][2] if scored else None


def cmd_auto_deploy(wrapper: ProbeRsWrapper, args: argparse.Namespace):
    """
    Auto-detect all connected MCU boards and automatically deploy matching firmware binaries.
    Ideal for CI/CD deploy stages with multiple connected hardware targets.
    """
    search_dir = os.path.abspath(args.firmware_dir)
    if not os.path.exists(search_dir):
        print(f"{Colors.RED}Error: Firmware search directory '{search_dir}' not found!{Colors.RESET}")
        return 1

    print(f"{Colors.CYAN}Scanning for firmware binaries in: {search_dir}...{Colors.RESET}")
    candidates = find_candidate_binaries(search_dir)
    print(f"{Colors.BLUE}Found {len(candidates)} firmware files in search path.{Colors.RESET}")

    print(f"{Colors.CYAN}Probing all connected Nuvoton MCU target boards...{Colors.RESET}")
    try:
        devices = wrapper.probe_all_targets(speed=args.speed)
    except ProbeRsError as e:
        print(f"{Colors.RED}Target Probe Error: {e}{Colors.RESET}")
        return 1

    if not devices:
        print(f"{Colors.YELLOW}No target MCU devices detected!{Colors.RESET}")
        return 1

    print(f"{Colors.GREEN}Detected {len(devices)} connected target boards.{Colors.RESET}\n")

    deploy_plan = []
    for i, dev in enumerate(devices):
        if "error" in dev:
            deploy_plan.append({
                "index": i,
                "device": dev,
                "binary": None,
                "status": "PROBE_FAILED",
                "message": dev["error"]
            })
            continue

        matched_bin = match_binary_for_device(dev, candidates, filter_str=args.filter)
        deploy_plan.append({
            "index": i,
            "device": dev,
            "binary": matched_bin,
            "status": "PENDING" if matched_bin else "NO_BINARY_MATCHED",
            "message": ""
        })

    if getattr(args, 'json', False) and args.dry_run:
        output_json = []
        for item in deploy_plan:
            dev = item["device"]
            output_json.append({
                "probe_index": dev.get("probe_index", item["index"]),
                "probe_selector": dev.get("probe_selector"),
                "part_name": dev.get("part_name"),
                "family": dev.get("family"),
                "binary": item["binary"],
                "status": item["status"],
                "message": item.get("message", "")
            })
        print(json.dumps(output_json, indent=2))
        return 0

    # Print matching summary table
    print(f"{Colors.BOLD}{'#':<3} {'Probe':<8} {'Part Number':<16} {'Family':<8} {'Status':<18} {'Firmware Binary'}{Colors.RESET}")
    print("-" * 80)
    for item in deploy_plan:
        dev = item["device"]
        probe_id = f"[{dev.get('probe_index', item['index'])}]"
        part = dev.get("part_name", "Unknown")
        fam = dev.get("family", "Unknown")
        bin_rel = os.path.relpath(item["binary"], search_dir) if item["binary"] else "(No match)"
        status_color = Colors.GREEN if item["status"] == "PENDING" else Colors.YELLOW
        print(f"{item['index']:<3} {probe_id:<8} {part:<16} {fam:<8} {status_color}{item['status']:<18}{Colors.RESET} {bin_rel}")
    print()

    if args.dry_run:
        print(f"{Colors.YELLOW}{Colors.BOLD}[Dry Run Mode] Matching complete. No firmware was flashed.{Colors.RESET}")
        return 0

    # Execute Flashing
    overall_success = True
    for item in deploy_plan:
        if item["status"] != "PENDING":
            overall_success = False
            continue

        dev = item["device"]
        bin_path = item["binary"]
        probe_sel = dev.get("probe_selector")
        chip_fam = dev.get("family")
        target_yaml = get_target_yaml_path(chip_fam)
        base_addr = args.address if args.address is not None else dev.get("flash_base", 0)

        print(f"{Colors.CYAN}{Colors.BOLD}>>> Flashing Board [{dev.get('probe_index')}]: {dev.get('part_name')} ({chip_fam}) <<<{Colors.RESET}")
        print(f"    Binary : {bin_path}")
        print(f"    Probe  : {probe_sel}")
        print(f"    Address: 0x{base_addr:08X}")

        try:
            wrapper.download_firmware(
                firmware_path=bin_path,
                chip=chip_fam,
                target_yaml=target_yaml,
                base_address=base_addr,
                binary_format=args.format,
                verify=not args.no_verify,
                chip_erase=args.chip_erase,
                reset=not args.no_reset,
                probe=probe_sel,
                speed=args.speed,
                stream_output=True
            )
            item["status"] = "SUCCESS"
            print(f"{Colors.GREEN}Successfully programmed {dev.get('part_name')}!{Colors.RESET}")

            # Observe MCU runtime health if requested
            if getattr(args, "observe", 0) > 0 and not args.no_reset:
                observe_sec = float(args.observe)
                print(f"{Colors.CYAN}[Observe] Waiting {observe_sec:.1f}s to observe MCU runtime execution...{Colors.RESET}")
                obs = wrapper.observe_target_health(
                    duration_sec=observe_sec,
                    chip=chip_fam,
                    target_yaml=target_yaml,
                    probe=probe_sel,
                    speed=args.speed
                )
                item["observe"] = obs
                if obs.get("serial_logs"):
                    print(f"{Colors.BLUE}[Serial Logs (115200)]:\n{obs['serial_logs']}{Colors.RESET}")
                if not obs.get("healthy", True):
                    item["status"] = "RUNTIME_FAULT"
                    item["message"] = obs.get("detail", "Runtime fault detected")
                    overall_success = False
                    print(f"{Colors.RED}[Observe] Runtime Fault Detected: {obs.get('detail')}{Colors.RESET}\n")
                else:
                    print(f"{Colors.GREEN}[Observe] MCU is running healthy! ({obs.get('detail')}){Colors.RESET}\n")
            else:
                print()
        except Exception as e:
            item["status"] = "FLASH_FAILED"
            item["message"] = str(e)
            overall_success = False
            print(f"{Colors.RED}Failed programming {dev.get('part_name')}: {e}{Colors.RESET}\n")

    # GitHub Actions Step Summary Support
    github_summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    summary_target = github_summary_path or args.summary
    if summary_target:
        try:
            with open(summary_target, "a", encoding="utf-8") as sf:
                sf.write("### Nuvoton MCU Auto-Deploy Summary\n\n")
                has_observe = any("observe" in item for item in deploy_plan)
                if has_observe:
                    sf.write("| Probe | Target MCU | Family | Firmware Binary | Flash | Runtime Health (Observe) |\n")
                    sf.write("| :--- | :--- | :--- | :--- | :--- | :--- |\n")
                else:
                    sf.write("| Probe | Target MCU | Family | Firmware Binary | Status |\n")
                    sf.write("| :--- | :--- | :--- | :--- | :--- |\n")

                for item in deploy_plan:
                    dev = item["device"]
                    p_idx = dev.get("probe_index", item["index"])
                    part = dev.get("part_name", "Unknown")
                    fam = dev.get("family", "Unknown")
                    bf = os.path.basename(item["binary"]) if item["binary"] else "-"
                    if item["status"] == "SUCCESS":
                        flash_st = "PASS"
                    elif item["status"] == "RUNTIME_FAULT":
                        flash_st = "PASS"
                    else:
                        flash_st = f"FAIL ({item['status']})"

                    if has_observe:
                        obs = item.get("observe")
                        if obs:
                            if obs.get("healthy", True):
                                hlth_st = f"🟢 PASS ({obs.get('state', 'Healthy')})"
                            else:
                                hlth_st = f"🔴 FAIL ({obs.get('state', 'Fault')})"
                        else:
                            hlth_st = "-"
                        sf.write(f"| [{p_idx}] | {part} | {fam} | `{bf}` | {flash_st} | {hlth_st} |\n")
                    else:
                        st = "PASS" if item["status"] == "SUCCESS" else f"FAIL ({item['status']})"
                        sf.write(f"| [{p_idx}] | {part} | {fam} | `{bf}` | {st} |\n")
                sf.write("\n")

                # If serial logs were captured, append details
                for item in deploy_plan:
                    obs = item.get("observe")
                    if obs and obs.get("serial_logs"):
                        dev = item["device"]
                        sf.write(f"<details><summary><b>UART Serial Logs: {dev.get('part_name')}</b></summary>\n\n")
                        sf.write("```text\n")
                        sf.write(obs["serial_logs"])
                        sf.write("\n```\n</details>\n\n")
        except Exception as e:
            print(f"Warning: could not write to summary file: {e}")

    return 0 if overall_success else 1


def cmd_reset(wrapper: ProbeRsWrapper, args: argparse.Namespace):
    """Reset target MCU."""
    chip = args.chip
    target_yaml = args.target_yaml or (get_target_yaml_path(chip) if chip else None)

    print(f"{Colors.CYAN}Resetting target MCU...{Colors.RESET}")
    try:
        wrapper.reset_target(
            chip=chip,
            target_yaml=target_yaml,
            probe=args.probe,
            speed=args.speed
        )
        print(f"{Colors.GREEN}Target MCU reset successful!{Colors.RESET}")
        return 0
    except ProbeRsError as e:
        print(f"{Colors.RED}Reset Error: {e}{Colors.RESET}")
        return 1


def cmd_gen_target(args: argparse.Namespace):
    """Generate target YAML from .FLM file."""
    try:
        out = generate_target_yaml(
            flm_path=args.flm,
            output_path=args.output,
            ram_start=args.ram_start,
            ram_size=args.ram_size,
            core_type=args.core,
            chip_family=args.family
        )
        print(f"{Colors.GREEN}Generated target YAML successfully: {out}{Colors.RESET}")
        return 0
    except Exception as e:
        print(f"{Colors.RED}Failed to generate target YAML: {e}{Colors.RESET}")
        return 1


def main():
    parser = argparse.ArgumentParser(
        description="Nuvoton MCU Auto-Deploy & Flash Programmer using probe-rs",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    subparsers = parser.add_subparsers(dest="command", help="Subcommand to execute")

    # Common options
    parent_parser = argparse.ArgumentParser(add_help=False)
    parent_parser.add_argument("--probe", help="Debug probe selector (VID:PID or Serial)")
    parent_parser.add_argument("--speed", type=int, help="Protocol speed in kHz (e.g. 4000)")
    parent_parser.add_argument("--chip", "--family", dest="chip", help="Chip family (M3331, M5531, M3351, M55M1, M460, M480, M2354, etc.)")
    parent_parser.add_argument("--target-yaml", help="Path to custom probe-rs target description YAML")
    parent_parser.add_argument("--probe-rs-path", help="Path to probe-rs executable")
    parent_parser.add_argument("--observe", type=float, default=0.0, help="Wait N seconds post-flash to observe and verify MCU runtime health (e.g. --observe 3)")

    # probe subcommand
    p_probe = subparsers.add_parser("probe", parents=[parent_parser], help="Probe connected MCU target and read Device ID (PDID)")
    p_probe.add_argument("--json", action="store_true", help="Output probe results in JSON format")

    # program subcommand
    p_prog = subparsers.add_parser("program", parents=[parent_parser], aliases=["flash"], help="Program firmware image to target APROM")
    p_prog.add_argument("firmware", help="Path to firmware image (.bin, .hex, .elf)")
    p_prog.add_argument("--address", type=lambda x: int(x, 0), help="APROM start address for BIN files (default: auto from chip)")
    p_prog.add_argument("--format", choices=["bin", "hex", "elf"], help="Firmware image format (default: auto from extension)")
    p_prog.add_argument("--no-verify", action="store_true", help="Skip readback verification")
    p_prog.add_argument("--chip-erase", action="store_true", help="Erase entire flash before programming")
    p_prog.add_argument("--no-reset", action="store_true", help="Do not reset target after programming")
    p_prog.add_argument("--dry-run", action="store_true", help="Simulate download without flashing")

    # reset subcommand
    p_reset = subparsers.add_parser("reset", parents=[parent_parser], help="Reset target MCU")

    # list-probes subcommand
    subparsers.add_parser("list-probes", help="List all connected debug probes")

    # list-targets subcommand
    subparsers.add_parser("list-targets", help="List supported Nuvoton MCU target platforms")

    # gen-target subcommand
    p_gen = subparsers.add_parser("gen-target", help="Convert Keil CMSIS .FLM to probe-rs target YAML")
    p_gen.add_argument("flm", help="Path to .FLM file")
    p_gen.add_argument("-o", "--output", required=True, help="Output .yaml path")
    p_gen.add_argument("--family", default="Nuvoton", help="Chip family name")
    p_gen.add_argument("--core", default="armv8m", help="Core architecture (armv8m, armv7em, armv6m)")
    p_gen.add_argument("--ram-start", type=lambda x: int(x, 0), default=0x20000000, help="SRAM start address")
    p_gen.add_argument("--ram-size", type=lambda x: int(x, 0), default=0x20000, help="SRAM size")

    # auto-deploy subcommand
    p_auto = subparsers.add_parser("auto-deploy", parents=[parent_parser], help="Auto-probe all connected boards and deploy matching binaries")
    p_auto.add_argument("--firmware-dir", "--dir", default="board", help="Directory to search for binaries (default: board)")
    p_auto.add_argument("--filter", help="Filter binary filename/path keyword (e.g. nutft, nufun, 320x240)")
    p_auto.add_argument("--address", type=lambda x: int(x, 0), help="Override flash base address for all binaries")
    p_auto.add_argument("--format", choices=["bin", "hex", "elf"], help="Firmware image format")
    p_auto.add_argument("--no-verify", action="store_true", help="Skip readback verification")
    p_auto.add_argument("--chip-erase", action="store_true", help="Erase entire flash before programming")
    p_auto.add_argument("--no-reset", action="store_true", help="Do not reset target after programming")
    p_auto.add_argument("--dry-run", action="store_true", help="Preview matched binaries and boards without flashing")
    p_auto.add_argument("--summary", help="Path to markdown summary output file (defaults to GITHUB_STEP_SUMMARY env var)")
    p_auto.add_argument("--json", action="store_true", help="Output deployment plan in JSON format")

    args = parser.parse_args()

    if not args.command:
        print_banner()
        parser.print_help()
        return 0

    if args.command == "list-targets":
        return cmd_list_targets(args)
    elif args.command == "gen-target":
        return cmd_gen_target(args)

    # All probe-rs interacting commands require wrapper
    try:
        wrapper = ProbeRsWrapper(probe_rs_path=getattr(args, "probe_rs_path", None))
    except ProbeRsNotFound as e:
        print(f"{Colors.RED}{e}{Colors.RESET}")
        return 1

    if args.command == "list-probes":
        return cmd_list_probes(wrapper, args)
    elif args.command == "probe":
        return cmd_probe(wrapper, args)
    elif args.command in ("program", "flash"):
        return cmd_program(wrapper, args)
    elif args.command == "auto-deploy":
        return cmd_auto_deploy(wrapper, args)
    elif args.command == "reset":
        return cmd_reset(wrapper, args)

    return 0


if __name__ == "__main__":
    sys.exit(main())
