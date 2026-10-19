#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
flm_parser.py - Keil / CMSIS Flash Algorithm (.FLM) Parser & Probe-rs Target Generator
Extracts flash algorithm instructions and FlashDevice metadata from .FLM ELF binaries
to produce probe-rs compatible target description YAML files.
"""

import os
import struct
import base64
import argparse


def parse_flm(flm_path: str,
              ram_start: int = 0x20000000,
              ram_size: int = 0x20000,
              core_type: str = 'armv8m',
              chip_family: str = 'Nuvoton') -> str:
    """
    Parse a CMSIS .FLM file and generate a probe-rs target description YAML string.

    Args:
        flm_path: Path to the .FLM file.
        ram_start: Target SRAM start address.
        ram_size: Target SRAM size in bytes.
        core_type: Core type recognized by probe-rs ('armv8m', 'armv7em', 'armv6m', etc.)
        chip_family: Family name for probe-rs.

    Returns:
        YAML string formatted for probe-rs.
    """
    if not os.path.exists(flm_path):
        raise FileNotFoundError(f"FLM file not found: {flm_path}")

    with open(flm_path, 'rb') as f:
        elf = f.read()

    if len(elf) < 52 or elf[:4] != b'\x7fELF':
        raise ValueError(f"Invalid ELF file: {flm_path}")

    fields = struct.unpack('<16sHHIIIIIHHHHHH', elf[:52])
    shoff, shentsize, shnum, shstrndx = fields[6], fields[11], fields[12], fields[13]

    sections = [
        struct.unpack('<IIIIIIIIII', elf[shoff + i * shentsize : shoff + (i + 1) * shentsize])
        for i in range(shnum)
    ]

    shstr_sh = sections[shstrndx]
    shstrtab = elf[shstr_sh[4] : shstr_sh[4] + shstr_sh[5]]

    def get_str(tab: bytes, off: int) -> str:
        end = tab.find(b'\0', off)
        if end == -1:
            end = len(tab)
        return tab[off:end].decode('latin1', errors='ignore')

    sec_map = {}
    symtab_sh = None
    strtab_sh = None

    for sh in sections:
        name = get_str(shstrtab, sh[0])
        sec_map[name] = sh
        if sh[1] == 2:  # SHT_SYMTAB
            symtab_sh = sh
        elif sh[1] == 3 and name == '.strtab':
            strtab_sh = sh

    if not symtab_sh or not strtab_sh:
        raise ValueError("ELF file missing symbol table or string table")

    strtab = elf[strtab_sh[4] : strtab_sh[4] + strtab_sh[5]]
    symbols = {}
    for i in range(symtab_sh[5] // 16):
        st_name, st_val, st_sz, st_info, st_other, st_shndx = struct.unpack(
            '<IIIBBH', elf[symtab_sh[4] + i * 16 : symtab_sh[4] + (i + 1) * 16]
        )
        sname = get_str(strtab, st_name)
        if sname:
            symbols[sname] = st_val

    # Parse FlashDevice structure
    dev_sh = sec_map.get('DevDscr')
    if not dev_sh:
        # Fallback to finding FlashDevice symbol
        if 'FlashDevice' not in symbols:
            raise ValueError("No DevDscr section or FlashDevice symbol found")
        dev_file_off = symbols['FlashDevice']
    else:
        dev_addr = symbols.get('FlashDevice', dev_sh[3])
        dev_file_off = dev_sh[4] + (dev_addr - dev_sh[3])

    dev_raw = elf[dev_file_off : dev_file_off + 160]
    vers, dev_name, dev_type, dev_adr, sz_dev, sz_page, res, val_empty, to_prog, to_erase = struct.unpack(
        '<H128sHIIIIB3xII', dev_raw[:160]
    )
    dev_name_str = dev_name.split(b'\0')[0].decode('latin1', errors='ignore').strip()

    # Parse Sectors array (terminated by 0xFFFFFFFF, 0xFFFFFFFF)
    sectors = []
    sec_off = dev_file_off + 160
    while sec_off + 8 <= len(elf):
        s_sz, s_addr = struct.unpack('<II', elf[sec_off : sec_off + 8])
        if s_sz == 0xFFFFFFFF and s_addr == 0xFFFFFFFF:
            break
        sectors.append({'size': s_sz, 'address': s_addr})
        sec_off += 8

    if not sectors:
        # Default single sector fallback if table is empty
        sectors.append({'size': sz_page, 'address': dev_adr})

    # Loadable code and data sections
    code_sh = sec_map.get('PrgCode')
    if not code_sh:
        for sname, sh in sec_map.items():
            if sh[2] & 0x4:  # SHF_EXECINSTR
                code_sh = sh
                break
    if not code_sh:
        raise ValueError("Could not locate executable code section in FLM")

    code_bytes = elf[code_sh[4] : code_sh[4] + code_sh[5]]
    code_start = code_sh[3]

    data_sh = sec_map.get('PrgData')
    data_bytes = elf[data_sh[4] : data_sh[4] + data_sh[5]] if data_sh else b''
    blob = code_bytes + data_bytes
    instr_b64 = base64.b64encode(blob).decode('ascii')
    data_sec_offset = len(code_bytes)

    def sym_pc(name: str):
        v = symbols.get(name)
        if v is None:
            return None
        return (v & ~1) - code_start

    pc_init = sym_pc('Init')
    if pc_init is None:
        raise ValueError("Required symbol 'Init' not found in FLM")

    pc_uninit = sym_pc('UnInit')
    pc_erase_sector = sym_pc('EraseSector')
    pc_erase_all = sym_pc('EraseChip') if 'EraseChip' in symbols else sym_pc('Erase')
    pc_program_page = sym_pc('ProgramPage')
    pc_verify = sym_pc('Verify')

    algo_name = os.path.splitext(os.path.basename(flm_path))[0]

    # Determine AP address format: Cortex-M55/DPv3 uses APv2 base 0x0
    ap_str = '!v2 0x0' if 'M55' in chip_family.upper() else '!v1 0'

    yaml_lines = [
        '---',
        f'name: {chip_family}',
        'variants:',
        f'  - name: {chip_family}',
        '    cores:',
        '      - name: main',
        f'        type: {core_type}',
        '        core_access_options: !Arm',
        f'          ap: {ap_str}',
        '    memory_map:',
        '      - !Ram',
        '        range:',
        f'          start: 0x{ram_start:x}',
        f'          end: 0x{ram_start + ram_size:x}',
        '        cores:',
        '          - main',
        '      - !Nvm',
        '        range:',
        f'          start: 0x{dev_adr:x}',
        f'          end: 0x{dev_adr + sz_dev:x}',
        '        cores:',
        '          - main',
        '        access:',
        '          boot: true',
        '    flash_algorithms:',
        f'      - {algo_name}',
        'flash_algorithms:',
        f'  - name: {algo_name}',
        f'    description: "{dev_name_str}"',
        '    default: true',
        f'    instructions: {instr_b64}',
        f'    pc_init: 0x{pc_init:x}',
    ]

    if pc_uninit is not None:
        yaml_lines.append(f'    pc_uninit: 0x{pc_uninit:x}')
    if pc_program_page is not None:
        yaml_lines.append(f'    pc_program_page: 0x{pc_program_page:x}')
    if pc_erase_sector is not None:
        yaml_lines.append(f'    pc_erase_sector: 0x{pc_erase_sector:x}')
    if pc_erase_all is not None:
        yaml_lines.append(f'    pc_erase_all: 0x{pc_erase_all:x}')
    if pc_verify is not None:
        yaml_lines.append(f'    pc_verify: 0x{pc_verify:x}')

    yaml_lines.extend([
        f'    data_section_offset: 0x{data_sec_offset:x}',
        '    flash_properties:',
        '      address_range:',
        f'        start: 0x{dev_adr:x}',
        f'        end: 0x{dev_adr + sz_dev:x}',
        f'      page_size: 0x{sz_page:x}',
        f'      erased_byte_value: 0x{val_empty:x}',
        f'      program_page_timeout: {to_prog}',
        f'      erase_sector_timeout: {to_erase}',
        '      sectors:',
    ])

    for s in sectors:
        sz = s['size']
        addr = s['address']
        yaml_lines.append(f'        - size: 0x{sz:x}')
        yaml_lines.append(f'          address: 0x{addr:x}')

    yaml_lines.append('      cores:')
    yaml_lines.append('        - main')

    return '\n'.join(yaml_lines) + '\n'


def generate_target_yaml(flm_path: str,
                         output_path: str,
                         ram_start: int = 0x20000000,
                         ram_size: int = 0x20000,
                         core_type: str = 'armv8m',
                         chip_family: str = 'Nuvoton') -> str:
    """
    Generate and save a probe-rs target YAML file from a .FLM file.
    """
    yaml_text = parse_flm(
        flm_path=flm_path,
        ram_start=ram_start,
        ram_size=ram_size,
        core_type=core_type,
        chip_family=chip_family
    )
    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(yaml_text)
    return output_path


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Convert Keil CMSIS .FLM file to probe-rs target description YAML")
    parser.add_argument('flm', help="Path to input .FLM file")
    parser.add_argument('-o', '--output', required=True, help="Path to output .yaml file")
    parser.add_argument('--family', default='Nuvoton', help="Chip family name")
    parser.add_argument('--core', default='armv8m', help="Core architecture (armv8m, armv7em, armv6m)")
    parser.add_argument('--ram-start', type=lambda x: int(x, 0), default=0x20000000, help="SRAM start address")
    parser.add_argument('--ram-size', type=lambda x: int(x, 0), default=0x20000, help="SRAM size")
    args = parser.parse_args()

    out = generate_target_yaml(
        args.flm,
        args.output,
        ram_start=args.ram_start,
        ram_size=args.ram_size,
        core_type=args.core,
        chip_family=args.family
    )
    print(f"Successfully generated target description: {out}")
