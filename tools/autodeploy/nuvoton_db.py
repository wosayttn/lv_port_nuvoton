#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
nuvoton_db.py - Nuvoton MCU Device ID (PDID) Database & Decoder
Maps 32-bit Part Device Identification Number (SYS_PDID at 0x40000000)
to part numbers, families, core architectures, and memory layouts.
"""

import os
from typing import Optional, Dict, Any

# Try importing the extracted devices database
try:
    from .devices_db import MCU_DEVICES
except ImportError:
    try:
        from devices_db import MCU_DEVICES
    except ImportError:
        MCU_DEVICES = {}

# Well-known PDID aliases and TRM default reset values
DEFAULT_RESET_PDIDS = {
    0x01E33340: "M3331/M3334_DEFAULT",
    0x03302140: "M3351LICAE",
    0x03300040: "M3334KGGAE",
    0xA2000060: "M5531K2LJAE",
    0xA2000040: "M5531R2LJAE",
    0xA2000120: "M5531H2LJAE",
    0xA2000160: "M55M1K2LJAE",
    0xA2000220: "M55M1H2LJAE",
    0x01B46751: "M467HJHAE",
    0x0D48640:  "M487JIDAE",
    0xA1735440: "M2354SJFBE",
    0x01F31040: "M2L31KGDAE",
}

# Family specifications and default target YAML definitions
FAMILY_CONFIGS = {
    "M3331": {
        "name": "Nuvoton NuMicro M3331/M3334 Series",
        "family": "M3331",
        "core": "Cortex-M33",
        "probe_rs_core": "Cortex-M33",
        "arch": "armv8m",
        "flash_base": 0x00000000,
        "flash_size": 512 * 1024,
        "page_size": 1024,
        "ram_base": 0x20000000,
        "ram_size": 128 * 1024,
        "target_yaml": "M3331.yaml",
    },
    "M3351": {
        "name": "Nuvoton NuMicro M3351 Series",
        "family": "M3351",
        "core": "Cortex-M33",
        "probe_rs_core": "Cortex-M33",
        "arch": "armv8m",
        "flash_base": 0x00000000,
        "flash_size": 512 * 1024,
        "page_size": 8192,
        "ram_base": 0x20000000,
        "ram_size": 128 * 1024,
        "target_yaml": "M3351.yaml",
    },
    "M5531": {
        "name": "Nuvoton NuMicro M5531 Series",
        "family": "M5531",
        "core": "Cortex-M55",
        "probe_rs_core": "Cortex-M55",
        "arch": "armv8m",
        "flash_base": 0x00100000,
        "flash_size": 2048 * 1024,
        "page_size": 512,
        "ram_base": 0x20100000,
        "ram_size": 1024 * 1024,
        "target_yaml": "M5531.yaml",
    },
    "M55M1": {
        "name": "Nuvoton NuMicro M55M1 Series",
        "family": "M55M1",
        "core": "Cortex-M55",
        "probe_rs_core": "Cortex-M55",
        "arch": "armv8m",
        "flash_base": 0x00100000,
        "flash_size": 2048 * 1024,
        "page_size": 512,
        "ram_base": 0x20100000,
        "ram_size": 1024 * 1024,
        "target_yaml": "M55M1.yaml",
    },
    "M460": {
        "name": "Nuvoton NuMicro M460 Series",
        "family": "M460",
        "core": "Cortex-M4",
        "probe_rs_core": "Cortex-M4",
        "arch": "armv7em",
        "flash_base": 0x00000000,
        "flash_size": 1024 * 1024,
        "page_size": 4096,
        "ram_base": 0x20000000,
        "ram_size": 256 * 1024,
        "target_yaml": "M460.yaml",
    },
    "M480": {
        "name": "Nuvoton NuMicro M480 Series",
        "family": "M480",
        "core": "Cortex-M4",
        "probe_rs_core": "Cortex-M4",
        "arch": "armv7em",
        "flash_base": 0x00000000,
        "flash_size": 512 * 1024,
        "page_size": 4096,
        "ram_base": 0x20000000,
        "ram_size": 160 * 1024,
        "target_yaml": "M480.yaml",
    },
    "M2354": {
        "name": "Nuvoton NuMicro M2354 Series",
        "family": "M2354",
        "core": "Cortex-M23",
        "probe_rs_core": "Cortex-M23",
        "arch": "armv8m",
        "flash_base": 0x00000000,
        "flash_size": 1024 * 1024,
        "page_size": 2048,
        "ram_base": 0x20000000,
        "ram_size": 256 * 1024,
        "target_yaml": "M2354.yaml",
    },
    "M2L31": {
        "name": "Nuvoton NuMicro M2L31 Series",
        "family": "M2L31",
        "core": "Cortex-M23",
        "probe_rs_core": "Cortex-M23",
        "arch": "armv8m",
        "flash_base": 0x00000000,
        "flash_size": 512 * 1024,
        "page_size": 128,
        "ram_base": 0x20000000,
        "ram_size": 64 * 1024,
        "target_yaml": "M2L31.yaml",
    },
    "M031": {
        "name": "Nuvoton NuMicro M031 Series",
        "family": "M031",
        "core": "Cortex-M0+",
        "probe_rs_core": "Cortex-M0+",
        "arch": "armv6m",
        "flash_base": 0x00000000,
        "flash_size": 512 * 1024,
        "page_size": 512,
        "ram_base": 0x20000000,
        "ram_size": 64 * 1024,
        "target_yaml": "M031.yaml",
    },
}


def extract_family_from_part(part_name: str) -> str:
    """
    Deduce MCU family key from part number name (e.g. 'M3351LICAE' -> 'M3351').
    """
    p = part_name.upper()
    if p.startswith("M333") or p.startswith("M3331") or p.startswith("M3334"):
        return "M3331"
    elif p.startswith("M335") or p.startswith("M3351"):
        return "M3351"
    elif p.startswith("M553") or p.startswith("M5531"):
        return "M5531"
    elif p.startswith("M55M") or p.startswith("M55S") or p.startswith("M55M1"):
        return "M55M1"
    elif p.startswith("M46") or p.startswith("M460") or p.startswith("M463") or p.startswith("M467"):
        return "M460"
    elif p.startswith("M48") or p.startswith("M480") or p.startswith("M481") or p.startswith("M482") or p.startswith("M487"):
        return "M480"
    elif p.startswith("M2354"):
        return "M2354"
    elif p.startswith("M235"):
        return "M2354"
    elif p.startswith("M2L") or p.startswith("M2L31"):
        return "M2L31"
    elif p.startswith("M03") or p.startswith("M031"):
        return "M031"
    return "UNKNOWN"


def guess_family_from_pdid(pdid: int) -> str:
    """
    Bitmask heuristic fallback when PDID is not in the known database.
    """
    # M33 series
    if (pdid & 0xFFFF0000) == 0x03300000:
        sub = pdid & 0x0000FF00
        if sub in (0x2100, 0x0100):
            return "M3351"
        return "M3331"
    if pdid == 0x01E33340:
        return "M3331"

    # M55 series
    if (pdid & 0xFFFF0000) == 0xA2000000:
        sub = pdid & 0x00000F00
        if sub in (0x0100, 0x0200):
            return "M55M1"
        return "M5531"

    # M460 / M480 series
    if (pdid & 0x00FF0000) == 0x00460000 or (pdid & 0x0F000000) == 0x01000000 and (pdid & 0x000F0000) == 0x00060000:
        return "M460"
    if (pdid & 0x00FF0000) == 0x00480000 or (pdid & 0x000FF000) == 0x00048000:
        return "M480"

    # M2354 / M2L31 series
    if (pdid & 0x00FF0000) == 0x00730000:
        return "M2354"
    if (pdid & 0x00FF0000) == 0x00310000:
        return "M2L31"

    return "UNKNOWN"


def decode_pdid(pdid: int) -> Dict[str, Any]:
    """
    Decode a 32-bit Part Device Identification Number (PDID).

    Returns a dictionary containing:
        pdid: Hex string of PDID
        part_name: Part number name
        family: Family key (M3331, M5531, M3351, etc.)
        family_name: Human readable family title
        core: CPU Core (Cortex-M33, Cortex-M55, etc.)
        probe_rs_core: Generic core target for probe-rs
        arch: Architecture (armv8m, armv7em, etc.)
        flash_base: APROM flash starting address
        flash_size: APROM flash size in bytes
        page_size: Flash sector/page size in bytes
        ram_base: SRAM starting address
        ram_size: SRAM size in bytes
        target_yaml: Name of target YAML file in targets/
    """
    # 1. Exact lookup
    part_name = MCU_DEVICES.get(pdid) or DEFAULT_RESET_PDIDS.get(pdid)

    if part_name:
        family = extract_family_from_part(part_name)
    else:
        family = guess_family_from_pdid(pdid)
        part_name = f"Nuvoton_{family}_0x{pdid:08X}"

    fam_cfg = FAMILY_CONFIGS.get(family, {
        "name": f"Nuvoton {family} Series",
        "family": family,
        "core": "Cortex-M",
        "probe_rs_core": "Cortex-M33",
        "arch": "armv8m",
        "flash_base": 0x00000000,
        "flash_size": 512 * 1024,
        "page_size": 1024,
        "ram_base": 0x20000000,
        "ram_size": 64 * 1024,
        "target_yaml": f"{family}.yaml" if family != "UNKNOWN" else None,
    })

    return {
        "pdid": f"0x{pdid:08X}",
        "pdid_int": pdid,
        "part_name": part_name,
        "family": family,
        "family_name": fam_cfg.get("name", f"Nuvoton {family}"),
        "core": fam_cfg.get("core", "Cortex-M"),
        "probe_rs_core": fam_cfg.get("probe_rs_core", "Cortex-M33"),
        "arch": fam_cfg.get("arch", "armv8m"),
        "flash_base": fam_cfg.get("flash_base", 0x00000000),
        "flash_size": fam_cfg.get("flash_size", 512 * 1024),
        "page_size": fam_cfg.get("page_size", 1024),
        "ram_base": fam_cfg.get("ram_base", 0x20000000),
        "ram_size": fam_cfg.get("ram_size", 64 * 1024),
        "target_yaml": fam_cfg.get("target_yaml"),
    }


def get_target_yaml_path(family: str) -> Optional[str]:
    """
    Get the absolute path to a bundled target YAML file in tools/autodeploy/targets/.
    """
    targets_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "targets")
    cand = os.path.join(targets_dir, f"{family}.yaml")
    if os.path.exists(cand):
        return cand
    return None
