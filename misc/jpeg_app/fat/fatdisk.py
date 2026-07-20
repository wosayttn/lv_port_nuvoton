#!/usr/bin/env python3
"""
fatdisk.py - Pure Python FAT12/16 filesystem image creator.
Drop-in replacement for fatdisk.exe.

Creates a disk image with MBR + FAT12/16 partition matching
the layout produced by fatdisk.exe.

Reads fatdisk.xml and creates a FAT disk image containing
all files from the specified root directory.

Usage:
    python fatdisk.py [fatdisk.xml]
"""

import struct
import os
import sys
import math
import xml.etree.ElementTree as ET


HIDDEN_SECTORS = 63        # Sectors reserved for MBR + gap
SECTORS_PER_TRACK = 63
NUM_HEADS = 255
ROOT_ENTRY_COUNT = 512
MEDIA_TYPE = 0xF8


def parse_config(xml_path):
    tree = ET.parse(xml_path)
    root = tree.getroot()
    return {
        "disk_size": int(root.findtext("disk_size", "8192")),       # KB
        "sector_size": int(root.findtext("sector_size", "4096")),   # bytes per sector
        "root_dir": root.findtext("root_dir", "root"),
        "output": root.findtext("output", "fat_root.bin"),
        "strip": int(root.findtext("strip", "0")),
    }


def to_83_name(filename):
    name, _, ext = filename.upper().rpartition(".")
    if not name:
        name = ext
        ext = ""
    valid = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&'()-@^_`{}~"
    name = "".join(c if c in valid else "_" for c in name)
    ext = "".join(c if c in valid else "_" for c in ext)
    return name[:8].ljust(8) + ext[:3].ljust(3)


def lba_to_chs(lba):
    """Convert LBA to CHS tuple (head, sector, cylinder) for MBR."""
    temp = lba // SECTORS_PER_TRACK
    sector = (lba % SECTORS_PER_TRACK) + 1
    head = temp % NUM_HEADS
    cylinder = temp // NUM_HEADS
    return head, sector, cylinder


def encode_chs(head, sector, cylinder):
    """Encode CHS into 3-byte MBR format."""
    return bytes([
        head & 0xFF,
        (sector & 0x3F) | ((cylinder >> 2) & 0xC0),
        cylinder & 0xFF,
    ])


class FATImage:
    def __init__(self, disk_size_kb, sector_size):
        self.bps = sector_size                     # bytes per sector
        self.spc = 1                               # sectors per cluster
        self.cluster_size = self.bps * self.spc

        # Total sectors for the entire disk (MBR + gap + partition)
        self.total_disk_sectors = disk_size_kb * 1024 // self.bps
        # Partition sectors (excluding MBR + gap)
        self.part_sectors = self.total_disk_sectors - HIDDEN_SECTORS
        self.reserved_sectors = 1                  # VBR only
        self.num_fats = 1                          # Single FAT table

        # Root directory size
        self.root_dir_sectors = (
            ROOT_ENTRY_COUNT * 32 + self.bps - 1
        ) // self.bps

        # Compute FAT size and determine FAT type
        # Data clusters available (iterative)
        self.fat_sectors = 1
        for _ in range(20):
            data_sectors = (
                self.part_sectors
                - self.reserved_sectors
                - self.num_fats * self.fat_sectors
                - self.root_dir_sectors
            )
            data_clusters = data_sectors // self.spc

            # Determine FAT type by cluster count
            if data_clusters < 4085:
                # FAT12: 1.5 bytes per entry
                needed = math.ceil((data_clusters + 2) * 3 / 2 / self.bps)
            else:
                # FAT16: 2 bytes per entry
                needed = math.ceil((data_clusters + 2) * 2 / self.bps)
            if needed == self.fat_sectors:
                break
            self.fat_sectors = needed

        self.max_clusters = data_clusters
        self.is_fat12 = (data_clusters < 4085)

        # Offsets within partition (in sectors, relative to partition start)
        self.fat_start = self.reserved_sectors
        self.root_dir_start = self.fat_start + self.num_fats * self.fat_sectors
        self.data_start = self.root_dir_start + self.root_dir_sectors

        # Allocate full disk image: MBR gap filled with 0xFF, rest with 0x00
        mbr_sector_bytes = self.bps  # First sector (MBR) is zeros
        gap_bytes = (HIDDEN_SECTORS - 1) * self.bps  # Sectors 1..62 are 0xFF
        part_bytes = self.part_sectors * self.bps

        self.image = bytearray(mbr_sector_bytes)       # MBR: zeros
        self.image += bytearray(b'\xFF' * gap_bytes)    # Gap: 0xFF
        self.image += bytearray(part_bytes)             # Partition: zeros

        # Partition data offset in image
        self.part_offset = HIDDEN_SECTORS * self.bps

        # FAT table (in-memory)
        self.fat = [0] * (self.max_clusters + 2)
        if self.is_fat12:
            self.fat[0] = 0xFF8
            self.fat[1] = 0xFFF
        else:
            self.fat[0] = 0xFFF8
            self.fat[1] = 0xFFFF
        self.next_free_cluster = 2

        # Root directory entries
        self.root_entries = []

        # Track highest used cluster for strip
        self.highest_cluster = 1

        # Shared read buffer (mimics fatdisk.exe's single-buffer I/O behavior)
        # Leftover bytes from previous reads persist in unused positions
        self._io_buf = bytearray(self.cluster_size)

    def _abs_offset(self, part_sector):
        """Convert partition-relative sector to absolute byte offset."""
        return self.part_offset + part_sector * self.bps

    def _cluster_offset(self, cluster):
        """Absolute byte offset for a data cluster."""
        return self._abs_offset(self.data_start + (cluster - 2) * self.spc)

    def _alloc_cluster(self):
        if self.next_free_cluster >= self.max_clusters + 2:
            raise RuntimeError("Disk image full - no free clusters")
        c = self.next_free_cluster
        self.next_free_cluster += 1
        self.fat[c] = 0xFFF if self.is_fat12 else 0xFFFF  # EOC
        if c > self.highest_cluster:
            self.highest_cluster = c
        return c

    def _write_chain(self, data):
        if not data:
            return 0
        clusters_needed = math.ceil(len(data) / self.cluster_size)
        first_cluster = 0
        prev_cluster = 0
        for i in range(clusters_needed):
            c = self._alloc_cluster()
            if first_cluster == 0:
                first_cluster = c
            if prev_cluster != 0:
                self.fat[prev_cluster] = c
            offset = self._cluster_offset(c)
            chunk = data[i * self.cluster_size : (i + 1) * self.cluster_size]
            self.image[offset : offset + len(chunk)] = chunk
            prev_cluster = c
        return first_cluster

    def _write_file_clusters(self, filepath):
        """Read file in cluster-sized chunks using a shared buffer and write
        full clusters (including residual slack from prior reads)."""
        first_cluster = 0
        prev_cluster = 0
        with open(filepath, "rb") as f:
            while True:
                chunk = f.read(self.cluster_size)
                if not chunk:
                    break
                n = len(chunk)
                self._io_buf[0:n] = chunk  # Only overwrite bytes actually read

                c = self._alloc_cluster()
                if first_cluster == 0:
                    first_cluster = c
                if prev_cluster != 0:
                    self.fat[prev_cluster] = c
                offset = self._cluster_offset(c)
                # Write the FULL buffer (including leftover slack)
                self.image[offset : offset + self.cluster_size] = self._io_buf
                prev_cluster = c
        return first_cluster

    def add_file(self, filepath, dir_entries_list=None):
        filename = os.path.basename(filepath)
        file_size = os.path.getsize(filepath)
        first_cluster = self._write_file_clusters(filepath) if file_size else 0
        entry = self._make_dir_entry(filename, 0x20, first_cluster, file_size)
        if dir_entries_list is None:
            self.root_entries.append(entry)
        else:
            dir_entries_list.append(entry)

    def add_directory(self, dirpath, dir_entries_list=None):
        dirname = os.path.basename(dirpath)
        sub_entries = []
        for item in sorted(os.listdir(dirpath)):
            item_path = os.path.join(dirpath, item)
            if os.path.isfile(item_path):
                self.add_file(item_path, sub_entries)
            elif os.path.isdir(item_path):
                self.add_directory(item_path, sub_entries)

        if sub_entries:
            dir_data = b"".join(sub_entries)
            padded = math.ceil(len(dir_data) / self.cluster_size) * self.cluster_size
            dir_data = dir_data.ljust(padded, b"\x00")
            first_cluster = self._write_chain(dir_data)
        else:
            first_cluster = self._alloc_cluster()
            offset = self._cluster_offset(first_cluster)
            self.image[offset : offset + self.cluster_size] = b"\x00" * self.cluster_size

        entry = self._make_dir_entry(dirname, 0x10, first_cluster, 0)
        if dir_entries_list is None:
            self.root_entries.append(entry)
        else:
            dir_entries_list.append(entry)

    def _make_dir_entry(self, name, attr, first_cluster, file_size):
        name83 = to_83_name(name)
        # NtRes=0x18: bit3(name lowercase) | bit4(ext lowercase)
        nt_res = 0x18
        # All timestamps zeroed (matching fatdisk.exe)
        return struct.pack(
            "<11sBBBHHHHHHHL",
            name83.encode("ascii"),
            attr,           # attributes
            nt_res,         # NT reserved (lowercase flags)
            0,              # create time tenths
            0,              # create time
            0,              # create date
            0,              # last access date
            0,              # first cluster high
            0,              # write time
            0,              # write date
            first_cluster,  # first cluster low
            file_size,      # file size
        )

    def _write_mbr(self):
        """Write MBR with partition table at sector 0."""
        # Partition entry 1 at offset 446
        start_chs = lba_to_chs(HIDDEN_SECTORS)
        # Use max CHS for end (matching fatdisk.exe behavior)
        end_chs_bytes = bytes([0xFE, 0x3F, 0x00])

        pe = bytearray(16)
        pe[0] = 0x80                                    # Boot indicator (active)
        pe[1:4] = encode_chs(*start_chs)                # Start CHS
        pe[4] = 0x04                                    # Partition type (FAT16 <32MB)
        pe[5:8] = end_chs_bytes                         # End CHS
        struct.pack_into("<I", pe, 8, HIDDEN_SECTORS)   # LBA start
        struct.pack_into("<I", pe, 12, self.part_sectors)  # LBA size

        self.image[446:462] = pe
        self.image[510] = 0x55
        self.image[511] = 0xAA

    def _write_vbr(self):
        """Write Volume Boot Record at partition start."""
        o = self.part_offset

        # Jump instruction
        self.image[o:o+3] = b"\xEB\xFE\x90"
        # OEM name
        self.image[o+3:o+11] = b"MSDOS5.0"
        # BPB
        struct.pack_into("<H", self.image, o+11, self.bps)
        self.image[o+13] = self.spc
        struct.pack_into("<H", self.image, o+14, self.reserved_sectors)
        self.image[o+16] = self.num_fats
        struct.pack_into("<H", self.image, o+17, ROOT_ENTRY_COUNT)
        if self.part_sectors < 0x10000:
            struct.pack_into("<H", self.image, o+19, self.part_sectors)
        else:
            struct.pack_into("<H", self.image, o+19, 0)
            struct.pack_into("<I", self.image, o+32, self.part_sectors)
        self.image[o+21] = MEDIA_TYPE
        struct.pack_into("<H", self.image, o+22, self.fat_sectors)
        struct.pack_into("<H", self.image, o+24, SECTORS_PER_TRACK)
        struct.pack_into("<H", self.image, o+26, NUM_HEADS)
        struct.pack_into("<I", self.image, o+28, HIDDEN_SECTORS)
        # Extended boot record
        self.image[o+36] = 0x80       # drive number
        self.image[o+37] = 0          # reserved
        self.image[o+38] = 0x29       # boot signature
        struct.pack_into("<I", self.image, o+39, 0x00000000)  # volume serial
        self.image[o+43:o+54] = b"NO NAME    "
        self.image[o+54:o+62] = b"FAT     "     # Generic FAT type string
        # Boot signature
        self.image[o+510] = 0x55
        self.image[o+511] = 0xAA

    def _write_fat(self):
        """Write FAT table."""
        fat_off = self._abs_offset(self.fat_start)
        fat_data = bytearray(self.fat_sectors * self.bps)

        if self.is_fat12:
            for i, val in enumerate(self.fat):
                offset = i + i // 2  # 1.5 bytes per entry
                if offset + 1 >= len(fat_data):
                    break
                if i % 2 == 0:
                    fat_data[offset] = val & 0xFF
                    fat_data[offset + 1] = (fat_data[offset + 1] & 0xF0) | ((val >> 8) & 0x0F)
                else:
                    fat_data[offset] = (fat_data[offset] & 0x0F) | ((val & 0x0F) << 4)
                    fat_data[offset + 1] = (val >> 4) & 0xFF
        else:
            for i, val in enumerate(self.fat):
                if i * 2 + 1 < len(fat_data):
                    struct.pack_into("<H", fat_data, i * 2, val)

        self.image[fat_off : fat_off + len(fat_data)] = fat_data

    def _write_root_directory(self):
        """Write root directory entries."""
        root_off = self._abs_offset(self.root_dir_start)
        for i, entry in enumerate(self.root_entries):
            off = root_off + i * 32
            self.image[off : off + 32] = entry

    def build(self):
        self._write_mbr()
        self._write_vbr()
        self._write_root_directory()
        self._write_fat()

    def save(self, output_path, strip=False):
        if strip:
            # Trim to end of last used cluster
            last_byte = self._cluster_offset(self.highest_cluster) + self.cluster_size
            # Ensure metadata area is included
            min_byte = self._abs_offset(self.data_start)
            last_byte = max(last_byte, min_byte)
            # Align to sector boundary
            last_byte = ((last_byte + self.bps - 1) // self.bps) * self.bps
            data = self.image[:last_byte]
        else:
            data = self.image

        with open(output_path, "wb") as f:
            f.write(data)
        return len(data)


def main():
    xml_path = sys.argv[1] if len(sys.argv) > 1 else "fatdisk.xml"

    if not os.path.exists(xml_path):
        print(f"Error: {xml_path} not found")
        sys.exit(1)

    config = parse_config(xml_path)
    base_dir = os.path.dirname(os.path.abspath(xml_path))
    root_dir = os.path.join(base_dir, config["root_dir"])
    output_path = os.path.join(base_dir, config["output"])

    print(f"fatdisk.py - FAT Image Creator")
    print(f"  Config:       {xml_path}")
    print(f"  Disk size:    {config['disk_size']} KB")
    print(f"  Sector size:  {config['sector_size']} bytes")
    print(f"  Root dir:     {root_dir}")
    print(f"  Output:       {output_path}")
    print(f"  Strip:        {'yes' if config['strip'] else 'no'}")

    if not os.path.isdir(root_dir):
        print(f"Error: root directory '{root_dir}' not found")
        sys.exit(1)

    img = FATImage(config["disk_size"], config["sector_size"])
    fat_type = "FAT12" if img.is_fat12 else "FAT16"
    print(f"  FAT type:     {fat_type} ({img.max_clusters} data clusters)")

    file_count = 0
    for item in sorted(os.listdir(root_dir)):
        item_path = os.path.join(root_dir, item)
        if os.path.isfile(item_path):
            fsize = os.path.getsize(item_path)
            print(f"  Adding file: {item} ({fsize} bytes)")
            img.add_file(item_path)
            file_count += 1
        elif os.path.isdir(item_path):
            print(f"  Adding dir:  {item}/")
            img.add_directory(item_path)
            file_count += 1

    img.build()
    written = img.save(output_path, strip=bool(config["strip"]))

    print(f"\nDone: {file_count} entries written, image size {written} bytes "
          f"({written // 1024} KB)")


if __name__ == "__main__":
    main()
