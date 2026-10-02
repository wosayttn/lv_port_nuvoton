# **NuMaker-HMI-M3334 GIF Playback on SPI NOR Flash (FAT)**

This application demonstrates LVGL GIF animation playback from an on-board SPI NOR Flash (FAT filesystem) on the NuMaker-HMI-M3334 board with **dual display buffers** providing **parallel rendering (GIF decoding) and display flush (SPI PDMA)**.

## **Architecture Overview**

```
+-------------------------------------------------------------+
|                      FreeRTOS / LVGL Task                   |
|                                                             |
|  Frame N+1:                                                 |
|  [ GIF Decode / Software Rendering ] ---> Write to Buffer 2 |
+-------------------------------------------------------------+
                              || (Concurrent Execution)
+-------------------------------------------------------------+
|                      Hardware SPI1 + PDMA                   |
|                                                             |
|  Frame N:                                                   |
|  Buffer 1 ---> SPI1 TX PDMA ---> ILI9341 LCD Panel          |
+-------------------------------------------------------------+
```

1. **Dual Display Buffer (`LV_DISPLAY_RENDER_MODE_PARTIAL`)**:
   - `s_au8FrameBuf1` (38,400 bytes, 60 lines)
   - `s_au8FrameBuf2` (38,400 bytes, 60 lines)
   - Registered via `lv_display_set_buffers()` and `lv_display_set_flush_wait_cb()`.
2. **Non-blocking SPI PDMA Flush**:
   - `lv_port_disp_flush_cb()` initiates hardware SPI PDMA transfer and immediately yields control back to LVGL.
   - When LVGL renders into the inactive buffer, SPI PDMA flushes the active buffer simultaneously.
   - When the PDMA transfer completes, the PDMA ISR calls `lv_display_flush_ready()` and releases the FreeRTOS semaphore.
3. **SPI NOR Flash FAT Filesystem**:
   - **NuFUN**: QSPI0 interface on pins PC0 (MOSI0), PC1 (MISO0), PC2 (CLK), PC3 (SS), PC4 (MOSI1), PC5 (MISO1).
   - **NuTFT**: QSPI0 interface on pins PA0 (MOSI0), PA1 (MISO0), PA2 (CLK), PA3 (SS), PA4 (MOSI1), PA5 (MISO1).
   - Uses `SpiFlash_QPI_FastRead` (Fast Read Quad I/O, Command 0xEB) with 4-bit data transfer for high-speed sector reading.
   - FatFs disk driver with 4096-byte sector size.
   - Logical drive `A:` mounted directly on SPI NOR Flash.
   - Automatically scans and carousels (輪播) all GIF animations found on drive `A:`.
   - Plays each GIF animation completely to its final frame before smoothly transitioning to the next GIF.
   - If mounting fails or no GIF file is found, the UI displays a `"Mount Fail!"` message.
   - Use [fat/make_fat_image.bat](fat/make_fat_image.bat) to build `fat_root.bin` from files in [fat/root/](fat/root/).

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK5.

| Target | Description |
|-|-|
| M3334KI_NUFUN | Use ILI9341 SPI LCD panel (SPI1) + QSPI0 NOR Flash on PC0~PC5 |
| M3334KI_NUTFT | Use ILI9341 SPI LCD panel (SPI2) + QSPI0 NOR Flash on PA0~PA5 |

## **Resources & Pin Map**

### **NuFUN**
- **ILI9341 SPI LCD**:
  - Controller: SPI1
  - MOSI: PE0, MISO: PE1, CLK: PH8, CS: PH9
  - DC: GH10, RST: GD14, Backlight: GH11
- **SPI NOR Flash (QSPI0)**:
  - Controller: QSPI0 (PC0..PC5)
  - MOSI0: PC0, MISO0: PC1, CLK: PC2, CS: PC3, MOSI1: PC4, MISO1: PC5

### **NuTFT**
- **ILI9341 SPI LCD**:
  - Controller: SPI2
  - MOSI: PA8, MISO: PA9, CLK: PA10, CS: PA11
  - DC: GB2, RST: GB3, Backlight: GB5
- **SPI NOR Flash (QSPI0)**:
  - Controller: QSPI0 (PA0..PA5)
  - MOSI0: PA0, MISO0: PA1, CLK: PA2, CS: PA3, MOSI1: PA4, MISO1: PA5

## **Resources**

- Technical Q&A: [QA.md](misc/numaker-hmi-m3334_gif/QA.md)
