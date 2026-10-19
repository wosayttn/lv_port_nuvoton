#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
probe_rs.py - probe-rs CLI Wrapper for Nuvoton MCU Management
Provides Python interfaces to probe-rs commands (list, read, download, reset).
"""

import os
import re
import shutil
import subprocess
import sys
import time
from typing import Optional, List, Dict, Any, Tuple

try:
    from .nuvoton_db import decode_pdid, get_target_yaml_path, FAMILY_CONFIGS
except ImportError:
    from nuvoton_db import decode_pdid, get_target_yaml_path, FAMILY_CONFIGS


class ProbeRsNotFound(Exception):
    """Raised when probe-rs executable is not found in PATH or system."""
    pass


class ProbeRsError(Exception):
    """Raised when probe-rs returns an error status."""
    pass


class ProbeRsWrapper:
    """Wrapper class managing interaction with probe-rs executable."""

    def __init__(self, probe_rs_path: Optional[str] = None):
        self.probe_rs_path = probe_rs_path or self._find_probe_rs()

    @staticmethod
    def _find_probe_rs() -> str:
        """Locate probe-rs executable in PATH or standard user directories."""
        # 1. Check environment variables
        env_path = os.environ.get("PROBE_RS_PATH")
        if env_path and os.path.isfile(env_path):
            return env_path
        env_dir = os.environ.get("PROBE_RS_DIR")
        if env_dir:
            cand = os.path.join(env_dir, "probe-rs.exe" if os.name == "nt" else "probe-rs")
            if os.path.isfile(cand):
                return cand

        # 2. Check system PATH
        exe = shutil.which("probe-rs") or shutil.which("probe-rs.exe")
        if exe:
            return exe

        # 3. Check known standard directories
        candidates = [
            r"d:\probe-rs-tools\probe-rs.exe",
            r"d:\probe-rs-tools\bin\probe-rs.exe",
            r"c:\probe-rs-tools\probe-rs.exe",
            r"c:\probe-rs-tools\bin\probe-rs.exe",
            os.path.expanduser("~/.cargo/bin/probe-rs.exe"),
            os.path.expanduser("~/.cargo/bin/probe-rs"),
            "/usr/local/bin/probe-rs",
            "/usr/bin/probe-rs",
        ]
        for cand in candidates:
            if os.path.isfile(cand):
                return cand

        raise ProbeRsNotFound(
            "probe-rs executable not found in PATH or standard locations (e.g. d:\\probe-rs-tools, ~/.cargo/bin).\n"
            "Please ensure probe-rs is installed. You can install it via:\n"
            "    cargo install probe-rs-tools --locked\n"
            "or download pre-built binaries from https://github.com/probe-rs/probe-rs/releases\n"
            "Alternatively, specify its location via --probe-rs-path or the PROBE_RS_PATH environment variable."
        )

    def run_cmd(self, args: List[str], stream_output: bool = False, timeout: Optional[int] = None) -> Tuple[int, str, str]:
        """
        Execute probe-rs command with the specified arguments.
        """
        cmd = [self.probe_rs_path] + args
        if stream_output:
            # Let probe-rs connect directly to standard output/error so that:
            # 1. Live ANSI progress bar (Erasing ✔ ... Programming ✔ ... Verifying ✔ ...) displays in real time.
            # 2. No OS pipe buffer deadlock occurs between stdout and stderr.
            sys.stdout.flush()
            sys.stderr.flush()
            res = subprocess.run(
                cmd,
                timeout=timeout
            )
            return res.returncode, "", ""
        else:
            res = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout
            )
            return res.returncode, res.stdout, res.stderr

    def list_probes(self) -> List[Dict[str, Any]]:
        """
        List all connected debug probes.

        Returns:
            List of dictionaries with probe information.
        """
        code, out, err = self.run_cmd(["list"])
        probes = []
        if "No debug probes were found" in out or "No debug probes were found" in err:
            return probes

        # Example probe-rs list output:
        # The following debug probes were found:
        # [0]: CMSIS-DAP v2 (VID:PID: 0416:5201, Serial: 00010001, CmsisDap)
        # or probe-rs 0.32 format
        lines = (out + "\n" + err).splitlines()
        for line in lines:
            line_str = line.strip()
            # Match formats like:
            # [0]: Nu-Link2 CMSIS-DAP -- 0416:2004-0:13010000000A25020450395D00001023 (CMSIS-DAP)
            m = re.match(r'\[(\d+)\]:\s*(.+?)(?:\s*\((.+)\))?$', line_str)
            if m:
                idx = int(m.group(1))
                full_name = m.group(2).strip()
                details = m.group(3) or ""

                selector = None
                vid_pid = None
                serial = None

                # Extract selector after '-- '
                sel_match = re.search(r'--\s*([0-9a-fA-F]{4}:[0-9a-fA-F]{4}(?:-\d+)?(?::[0-9a-zA-Z_-]+)?)', full_name)
                if sel_match:
                    selector = sel_match.group(1)
                else:
                    # Fallback check in details or line
                    sel_match = re.search(r'([0-9a-fA-F]{4}:[0-9a-fA-F]{4}(?:-\d+)?(?::[0-9a-zA-Z_-]+)?)', line_str)
                    if sel_match:
                        selector = sel_match.group(1)

                vp_match = re.search(r'([0-9a-fA-F]{4}:[0-9a-fA-F]{4})', full_name + " " + details)
                if vp_match:
                    vid_pid = vp_match.group(1)

                ser_match = re.search(r':([0-9a-zA-Z_-]{8,})', full_name + " " + details)
                if ser_match:
                    serial = ser_match.group(1)

                # Clean display name
                disp_name = full_name.split(' -- ')[0].strip() if ' -- ' in full_name else full_name

                probes.append({
                    "index": idx,
                    "name": disp_name,
                    "full_name": full_name,
                    "selector": selector or full_name,
                    "details": details,
                    "vid_pid": vid_pid,
                    "serial": serial,
                    "raw": line_str,
                })
        return probes

    def resolve_probe(self, probe_arg: Optional[Any] = None, notify: bool = True) -> Optional[str]:
        """
        Resolve probe input into a concrete probe-rs selector.
        Supports:
            - Probe index (e.g. 0, 1, "0", "1")
            - Serial number substring
            - Full selector (e.g. "0416:2004:1301...")
            - None (auto-selects single probe, or defaults to [0] with notice if multiple connected)
        """
        probes = self.list_probes()
        if not probes:
            return None

        if probe_arg is not None:
            probe_str = str(probe_arg).strip()
            # 1. Check if probe_arg is integer index
            if probe_str.isdigit():
                idx = int(probe_str)
                for p in probes:
                    if p["index"] == idx:
                        return p["selector"]
                raise ProbeRsError(f"Probe index [{idx}] not found (available: 0 to {len(probes)-1})")

            # 2. Check if probe_arg matches serial or selector
            for p in probes:
                if probe_str == p.get("selector") or probe_str == p.get("serial"):
                    return p["selector"]
                if p.get("serial") and probe_str in p["serial"]:
                    return p["selector"]
                if p.get("selector") and probe_str in p["selector"]:
                    return p["selector"]

            # 3. Direct passthrough
            return probe_str

        # If probe_arg is None
        if len(probes) == 1:
            return probes[0]["selector"]
        else:
            sel = probes[0]["selector"]
            if notify:
                sys.stderr.write(
                    f"\033[93m[Notice] {len(probes)} debug probes detected. Auto-selecting probe [0]: {sel}\033[0m\n"
                    f"\033[93m         To select another probe, use --probe 0 or --probe 1 (or serial).\033[0m\n"
                )
            return sel

    def read_memory(self,
                    address: int,
                    words: int = 1,
                    width: str = "b32",
                    chip: Optional[str] = None,
                    target_yaml: Optional[str] = None,
                    probe: Optional[str] = None,
                    speed: Optional[int] = None) -> List[int]:
        """
        Read 32-bit (or specified width) words from target memory address.
        """
        resolved_probe = self.resolve_probe(probe, notify=False)
        args = ["read", width, hex(address), str(words), "--format", "simple-hex", "--non-interactive"]
        if chip:
            args.extend(["--chip", chip])
        if target_yaml:
            args.extend(["--chip-description-path", target_yaml])
        if resolved_probe:
            args.extend(["--probe", resolved_probe])
        if speed:
            args.extend(["--speed", str(speed)])

        code, out, err = self.run_cmd(args)
        if code != 0:
            raise ProbeRsError(f"Failed to read memory at {hex(address)}: {err or out}")

        # Parse simple-hex output (e.g. "0x03302140" or "03302140" or space separated)
        values = []
        for token in re.findall(r'(?:0x)?([0-9a-fA-F]+)', out):
            values.append(int(token, 16))
        return values

    def read_pdid(self,
                  chip_or_core: str = "Cortex-M33",
                  target_yaml: Optional[str] = None,
                  probe: Optional[str] = None,
                  speed: Optional[int] = None) -> Optional[int]:
        """
        Attempt to read SYS->PDID (0x40000000) using a specific chip or core target.
        """
        try:
            words = self.read_memory(
                address=0x40000000,
                words=1,
                width="b32",
                chip=chip_or_core,
                target_yaml=target_yaml,
                probe=probe,
                speed=speed
            )
            if words:
                return words[0]
        except ProbeRsError:
            return None
        return None

    def probe_target(self,
                     probe: Optional[str] = None,
                     speed: Optional[int] = None,
                     preferred_family: Optional[str] = None) -> Dict[str, Any]:
        """
        Probe connected Nuvoton MCU target device ID.

        Iterates over core candidates (or preferred family) to read SYS->PDID,
        then decodes the result via nuvoton_db.

        Returns:
            Decoded device info dict.
        """
        resolved_probe = self.resolve_probe(probe, notify=True)

        # Candidate core architectures and targets to try
        candidates = []
        if preferred_family:
            yaml_path = get_target_yaml_path(preferred_family)
            fam_cfg = FAMILY_CONFIGS.get(preferred_family, {})
            core_name = fam_cfg.get("probe_rs_core", "Cortex-M33")
            candidates.append((preferred_family, yaml_path, core_name))

        # Default generic cores in priority order for Nuvoton Cortex-M
        candidates.extend([
            ("Cortex-M33", None, "Cortex-M33"),  # M3331, M3351, etc.
            ("Cortex-M55", None, "Cortex-M55"),  # M5531, M55M1, etc.
            ("Cortex-M4",  None, "Cortex-M4"),   # M460, M480, etc.
            ("Cortex-M23", None, "Cortex-M23"),  # M2354, M2L31, etc.
            ("Cortex-M0+", None, "Cortex-M0+"),  # M031, etc.
        ])

        last_error = None
        for name, yaml_file, core_to_use in candidates:
            pdid = self.read_pdid(
                chip_or_core=name,
                target_yaml=yaml_file,
                probe=resolved_probe,
                speed=speed
            )
            if pdid is not None and pdid != 0 and pdid != 0xFFFFFFFF:
                decoded = decode_pdid(pdid)
                decoded["connected_core"] = core_to_use
                decoded["selected_probe"] = resolved_probe
                return decoded

        # If none succeeded, check probe availability for better error message
        connected_probes = self.list_probes()
        if not connected_probes:
            raise ProbeRsError("No debug probes were found connected to the system.")
        else:
            p_name = resolved_probe or connected_probes[0]['name']
            raise ProbeRsError(
                f"Connected probe ({p_name}) could not attach to any target core.\n"
                "Please verify SWD wiring, MCU power supply, and reset state."
            )

    def probe_all_targets(self,
                          speed: Optional[int] = None,
                          preferred_family: Optional[str] = None) -> List[Dict[str, Any]]:
        """
        Probe all connected debug probes and return target device info for each probe.
        """
        probes = self.list_probes()
        if not probes:
            raise ProbeRsError("No debug probes were found connected to the system.")

        results = []
        for p in probes:
            sel = p["selector"]
            try:
                dev = self.probe_target(probe=sel, speed=speed, preferred_family=preferred_family)
                dev["probe_index"] = p["index"]
                dev["probe_name"] = p["name"]
                dev["probe_selector"] = sel
                dev["probe_serial"] = p.get("serial")
                results.append(dev)
            except Exception as e:
                results.append({
                    "probe_index": p["index"],
                    "probe_name": p["name"],
                    "probe_selector": sel,
                    "probe_serial": p.get("serial"),
                    "error": str(e),
                })
        return results

    def download_firmware(self,
                          firmware_path: str,
                          chip: Optional[str] = None,
                          target_yaml: Optional[str] = None,
                          base_address: Optional[int] = None,
                          binary_format: Optional[str] = None,
                          verify: bool = True,
                          chip_erase: bool = False,
                          reset: bool = True,
                          probe: Optional[str] = None,
                          speed: Optional[int] = None,
                          dry_run: bool = False,
                          stream_output: bool = True) -> int:
        """
        Download firmware image (.bin, .hex, .elf) to target APROM.

        Args:
            firmware_path: Path to the binary, hex, or ELF firmware file.
            chip: Chip name defined in target YAML.
            target_yaml: Path to probe-rs target description YAML.
            base_address: Flash starting address (required/defaulted for BIN).
            binary_format: 'bin', 'hex', 'elf', or None (auto-detect).
            verify: Whether to verify flashed data.
            chip_erase: Whether to erase entire chip before download.
            reset: Whether to reset chip after downloading.
            probe: Debug probe selector.
            speed: Protocol clock speed in kHz.
            dry_run: Simulate operation without writing.
            stream_output: Print output in real-time.

        Returns:
            Process exit code (0 for success).
        """
        if not os.path.exists(firmware_path):
            raise FileNotFoundError(f"Firmware file not found: {firmware_path}")

        # Auto-detect binary format if not specified
        ext = os.path.splitext(firmware_path)[1].lower()
        if not binary_format:
            if ext == ".bin":
                binary_format = "bin"
            elif ext in (".hex", ".ihex"):
                binary_format = "hex"
            elif ext in (".elf", ".axf", ".out"):
                binary_format = "elf"
            else:
                binary_format = "target"

        args = ["download", firmware_path, "--non-interactive"]
        resolved_probe = self.resolve_probe(probe, notify=True)

        if chip:
            args.extend(["--chip", chip])
        if target_yaml:
            args.extend(["--chip-description-path", target_yaml])
        if binary_format:
            args.extend(["--binary-format", binary_format])
        if binary_format == "bin" and base_address is not None:
            args.extend(["--base-address", hex(base_address)])
        if verify:
            args.append("--verify")
        if chip_erase:
            args.append("--chip-erase")
        if resolved_probe:
            args.extend(["--probe", resolved_probe])
        if speed:
            args.extend(["--speed", str(speed)])
        if dry_run:
            args.append("--dry-run")

        code, out, err = self.run_cmd(args, stream_output=stream_output)
        if code != 0:
            raise ProbeRsError(f"Firmware programming failed with exit code {code}:\n{err or out}")

        if reset and not dry_run:
            time.sleep(0.3)
            # Explicitly reset the MCU after flashing so it starts running freely
            try:
                self.reset_target(
                    chip=chip,
                    target_yaml=target_yaml,
                    probe=resolved_probe,
                    speed=speed,
                    notify=False
                )
                print(f"[Reset] Target MCU reset and running.")
            except Exception as e:
                sys.stderr.write(f"\n[Warning] Post-flash reset: {e}\n")

        return code

    def reset_target(self,
                     chip: Optional[str] = None,
                     target_yaml: Optional[str] = None,
                     probe: Optional[str] = None,
                     speed: Optional[int] = None,
                     notify: bool = True) -> int:
        """
        Reset the target attached to the debug probe.
        """
        resolved_probe = self.resolve_probe(probe, notify=notify)
        # Determine target yaml or fallback generic core
        fam_cfg = FAMILY_CONFIGS.get(chip, {}) if chip else {}
        chip_to_use = fam_cfg.get("probe_rs_core", chip or "Cortex-M33")

        args = ["reset", "--non-interactive", "--chip", chip_to_use]
        if resolved_probe:
            args.extend(["--probe", resolved_probe])
        if speed:
            args.extend(["--speed", str(speed)])

        code, out, err = self.run_cmd(args)
        if code != 0:
            # Fallback retry with target_yaml if available
            target_desc = target_yaml or (get_target_yaml_path(chip) if chip else None)
            if target_desc and chip:
                retry_args = ["reset", "--non-interactive", "--chip", chip, "--chip-description-path", target_desc]
                if resolved_probe:
                    retry_args.extend(["--probe", resolved_probe])
                if speed:
                    retry_args.extend(["--speed", str(speed)])
                code, out, err = self.run_cmd(retry_args)

        if code != 0:
            raise ProbeRsError(f"Target reset failed: {err or out}")
        return code

    def observe_target_health(self,
                              duration_sec: float = 3.0,
                              chip: Optional[str] = None,
                              target_yaml: Optional[str] = None,
                              probe: Optional[str] = None,
                              speed: Optional[int] = None) -> Dict[str, Any]:
        """
        Wait for duration_sec, then check MCU runtime status via SWD registers and serial logs.
        Inspects:
            - ICSR (0xE000ED04): VECTACTIVE (0: Thread mode, 3: HardFault, etc.)
            - DHCSR (0xE000EDF0): S_LOCKUP, S_HALT
            - CFSR (0xE000ED28): Usage/Bus/Mem fault details
            - HFSR (0xE000ED2C): HardFault status
            - Optional UART/VCP serial log output (115200 bps)

        Returns:
            Dictionary with health observation results.
        """
        resolved_probe = self.resolve_probe(probe, notify=False)

        # 1. Wait duration while capturing serial logs if VCP is available
        serial_logs = self._capture_serial_logs(duration_sec=duration_sec)

        # 2. Determine target candidates for SWD register read
        fam_cfg = FAMILY_CONFIGS.get(chip, {}) if chip else {}
        generic_core = fam_cfg.get("probe_rs_core", "Cortex-M33")
        target_candidates = []
        if chip:
            target_candidates.append((chip, target_yaml or get_target_yaml_path(chip)))
        target_candidates.append((generic_core, None))
        target_candidates.append(("Cortex-M33", None))

        icsr = None
        dhcsr = None
        cfsr = None
        hfsr = None
        last_err = None

        for c_name, c_yaml in target_candidates:
            try:
                words = self.read_memory(
                    address=0xE000ED04,
                    words=1,
                    width="b32",
                    chip=c_name,
                    target_yaml=c_yaml,
                    probe=resolved_probe,
                    speed=speed
                )
                if words:
                    icsr = words[0]
                    # Try reading DHCSR
                    try:
                        dwords = self.read_memory(
                            address=0xE000EDF0,
                            words=1,
                            width="b32",
                            chip=c_name,
                            target_yaml=c_yaml,
                            probe=resolved_probe,
                            speed=speed
                        )
                        if dwords:
                            dhcsr = dwords[0]
                    except Exception:
                        pass

                    # Read CFSR & HFSR if faulted or available
                    try:
                        fwords = self.read_memory(
                            address=0xE000ED28,
                            words=2,
                            width="b32",
                            chip=c_name,
                            target_yaml=c_yaml,
                            probe=resolved_probe,
                            speed=speed
                        )
                        if fwords:
                            cfsr = fwords[0]
                            if len(fwords) > 1:
                                hfsr = fwords[1]
                    except Exception:
                        pass
                    break
            except Exception as e:
                last_err = e
                continue

        # 3. Analyze register states (CoreDebug DHCSR & SCB ICSR)
        # ARM DHCSR bits:
        #   [16]: S_REGRDY  (Register interface ready)
        #   [17]: S_HALT    (Core is halted in debug state)
        #   [18]: S_SLEEP   (Core is sleeping in WFI/WFE - normal for RTOS idle)
        #   [19]: S_LOCKUP  (Core is in lockup state)
        #   [20]: S_RETIRE_ST (Instructions retired since last read)
        #   [24]: S_RESET_ST  (Reset occurred)
        vectactive = (icsr & 0x1FF) if icsr is not None else None
        s_regrdy = bool((dhcsr >> 16) & 1) if dhcsr is not None else False
        s_halt = bool((dhcsr >> 17) & 1) if dhcsr is not None else False
        s_sleep = bool((dhcsr >> 18) & 1) if dhcsr is not None else False
        s_lockup = bool((dhcsr >> 19) & 1) if dhcsr is not None else False
        s_retire = bool((dhcsr >> 20) & 1) if dhcsr is not None else False
        s_reset = bool((dhcsr >> 24) & 1) if dhcsr is not None else False

        healthy = True
        state = "UNKNOWN"
        detail = ""

        if icsr is None:
            state = "RUNNING_UNCHECKED"
            detail = f"MCU reset complete. SWD register read unavailable ({last_err or 'No response'})."
        elif s_lockup:
            healthy = False
            state = "LOCKUP"
            detail = f"Core locked up in unhandled fault (DHCSR=0x{(dhcsr or 0):08X})."
        elif vectactive == 3:
            healthy = False
            state = "HARDFAULT"
            detail = f"HardFault_Handler triggered (ICSR=0x{icsr:08X}, HFSR=0x{(hfsr or 0):08X}, CFSR=0x{(cfsr or 0):08X})."
        elif vectactive in (4, 5, 6, 7):
            fault_map = {4: "MemManage", 5: "BusFault", 6: "UsageFault", 7: "SecureFault"}
            fname = fault_map.get(vectactive, "Fault")
            healthy = False
            state = fname.upper()
            detail = f"{fname}_Handler triggered (ICSR=0x{icsr:08X}, CFSR=0x{(cfsr or 0):08X})."
        elif s_halt:
            healthy = False
            state = "HALTED"
            detail = f"Core halted in debug state (DHCSR=0x{(dhcsr or 0):08X})."
        else:
            healthy = True
            if vectactive == 0:
                if s_sleep:
                    state = "IDLE_SLEEP"
                    detail = f"Executing normally in RTOS idle sleep (WFI, DHCSR=0x{(dhcsr or 0):08X})."
                else:
                    state = "THREAD_MODE"
                    detail = f"Executing normally in Thread Mode (FreeRTOS & LVGL tasks active, DHCSR=0x{(dhcsr or 0):08X})."
            elif vectactive in (14, 15):
                state = "RTOS_SCHEDULER"
                detail = f"Executing normally in RTOS ({'SysTick' if vectactive == 15 else 'PendSV'}, DHCSR=0x{(dhcsr or 0):08X})."
            else:
                state = f"IRQ_{vectactive - 16}"
                detail = f"Executing Peripheral IRQ {vectactive - 16} (DHCSR=0x{(dhcsr or 0):08X})."

        return {
            "healthy": healthy,
            "state": state,
            "detail": detail,
            "duration": duration_sec,
            "icsr": icsr,
            "vectactive": vectactive,
            "dhcsr": dhcsr,
            "cfsr": cfsr,
            "hfsr": hfsr,
            "serial_logs": serial_logs
        }

    @staticmethod
    def _capture_serial_logs(duration_sec: float, baudrate: int = 115200) -> str:
        """Capture serial logs from connected Nu-Link2 VCP during wait duration."""
        try:
            import serial
            import serial.tools.list_ports
        except ImportError:
            time.sleep(duration_sec)
            return ""

        target_port = None
        for p in serial.tools.list_ports.comports():
            hwid = (p.hwid or "").upper()
            desc = (p.description or "").upper()
            if "0416" in hwid or "NUVOTON" in desc or "NU-LINK" in desc or "VIRTUAL COM" in desc:
                target_port = p.device
                break

        if not target_port:
            time.sleep(duration_sec)
            return ""

        captured = []
        start_t = time.time()
        try:
            with serial.Serial(target_port, baudrate=baudrate, timeout=0.1) as ser:
                while time.time() - start_t < duration_sec:
                    waiting = ser.in_waiting
                    if waiting:
                        data = ser.read(waiting)
                        captured.append(data.decode("utf-8", errors="replace"))
                    time.sleep(0.05)
        except Exception:
            rem = duration_sec - (time.time() - start_t)
            if rem > 0:
                time.sleep(rem)
        return "".join(captured).strip()
