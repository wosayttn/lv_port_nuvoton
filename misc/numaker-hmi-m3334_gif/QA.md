# NuMaker-HMI-M3334 GIF 常見問題與技術解析 (Q&A)

---

### **Q1: 為什麼解析度為 320x240 的 GIF 檔案，LVGL 需要 allocate 一個 320x240x2 (153.6 KB) 的記憶體？**

主要有以下三大核心原因：

#### 1. GIF 規範的核心機制：影格增量更新與透明疊加 (Inter-frame Delta & Transparency)
- **局部區域更新 (Sub-rectangle Update)**：
  GIF 的第 $N$ 影格不一定是完整的 320x240。例如第 0 影格繪製整張背景，但第 1 影格可能只有一個位於 $(x=50, y=80)$、大小為 $100 \times 60$ 的局部變化區域。
- **透明色穿透 (Transparency)**：
  在局部更新區域中，某些像素被標記為「透明」，透明像素必須保留並顯示上一影格（甚至更早影格）留下來的畫面內容。
- **處置方式 (Disposal Method 1: Leave in Place)**：
  大部分動畫 GIF 採用 Disposal Method 1，後續影格直接疊加在上一影格的基礎上繪製。
- **結論**：如果不保留一張完整的 320x240 畫布 (Canvas)，當解碼後續影格時，未更新區域與透明像素點將無法取得上一幀的影像，會導致破圖或嚴重殘影。

#### 2. 交錯式編碼 (Interlaced GIF)
- GIF 格式支援「交錯儲存 (Interlaced)」，行資料不是由第 0 行依序解到第 239 行，而是分 4 個階段跳躍式解碼：
  - Pass 1：第 0, 8, 16, 24... 行
  - Pass 2：第 4, 12, 20, 28... 行
  - Pass 3：第 2, 6, 10, 14... 行
  - Pass 4：第 1, 3, 5, 7... 奇數行
- **結論**：垂直方向的解碼順序非連續，必須依賴一塊完整的記憶體供解碼器在不同 Pass 間把像素填入對應的行。

#### 3. LVGL 的物件與渲染管線 (Widget & Rendering Pipeline)
- `lv_gif` 是繼承自 `lv_image` 的 UI 物件，在 LVGL 架構中必須持有完整的 `lv_draw_buf_t` 畫布。
- **解碼與顯示解耦**：
  - **GIF Timer**：依影格延遲時間解碼，並將像素寫入 `draw_buf`。
  - **LVGL 渲染引擎 (`lv_refr.c`)**：在重繪週期中，從 `draw_buf` 依照目前螢幕更新區域、圖層層級與遮罩，以雙顯示緩衝區並行送往 LCD。
- 若沒有獨立完整的 `draw_buf`，LVGL 無法在多圖層、視窗拖動或遮罩混合時正常重繪該物件。

---

### **Q2: GIF Decoder 有支援 Line by Line 解碼嗎？**

**答案：底層解碼器「有支援」，但 LVGL 的包裝層將每行集中存入全幅畫布中。**

#### 1. 底層解碼核心 (`lvgl/src/libs/gif/gif.c` - AnimatedGIF)
底層實為逐行 (Line by Line) 解碼架構：
- 內部僅配置極小的單行緩衝區：
  ```c
  unsigned char ucLineBuf[MAX_WIDTH]; // 僅 480 位元組
  ```
- 當 `DecodeLZW()` 解完壓縮數據中的一行像素後，會立即觸發回調函式：
  ```c
  (*pPage->pfnDraw)(&gd); // 將該行的 y、寬度、像素指標傳給呼叫端
  ```
  這表示底層解碼器本質上具備每解出一行就丟給使用者的能力。

#### 2. LVGL 上層物件 (`lvgl/src/widgets/gif/lv_gif.c`)
當 LVGL 收到底層每解出一行的回調時，會將其寫入全幅畫布中：
```c
static inline void gif_blend_to_rgb565(GIFDRAW * pDraw, lv_draw_buf_t * draw_buf)
{
    // 將解出來的該行像素，複製並寫入全幅 draw_buf 的第 y 行
    uint16_t * dst = (uint16_t *)((uint8_t *)draw_buf->data + 
                     ((pDraw->iY + pDraw->y) * draw_buf->header.stride + pDraw->iX * 2));
    ...
}
```

---

### **總結與應用對比**

| 格式 / 機制 | 是否需要全幅記憶體 (320x240x2) | 能否 Line by Line 直接送 LCD | 原因 |
|---|---|---|---|
| **標準 LVGL GIF (`lv_gif`)** | **必須 (153.6 KB)** | 否 (需經過物件畫布) | 動畫前後幀疊加、透明色穿透、交錯式掃描、LVGL UI 管線架構需求。 |
| **裸機/特定 GIF 直刷 LCD** | 可不需要 (僅需單行緩衝區) | 可以 (限特定條件) | 需滿足：<br>1. 非交錯式<br>2. 每一影格皆為 320x240 無透明疊加<br>3. 直接調用 SPI 寫入 ILI9341 內部 GRAM (跳過 LVGL UI 物件系統)。 |
| **MJPEG / JPEG 播放** | **不需要** | **完全支援** | JPEG 每一影格都是獨立的完整影像，天然支援 16x16 MCU block / 逐行解碼，可以用兩個局部緩衝區 (如 30 行) 邊解邊送。 |

---

### **Q3: 能依據 bss_end 和 SRAM 邊界自動計算 HEAP SIZE 嗎？**

**答案：完全可以，且本專案已在 Keil 散列加載文件 (`m3331.sct`) 實作自動計算！**

#### 1. 傳統硬編碼 (Hardcoded) 的痛點
在傳統的 Keil Scatter File (`.sct`) 中，通常手動固定一個尺寸：
```sct
#define __HEAP_SIZE  0x00048000  /* 固定 288 KB */
#define __RW_SIZE    (__RAM_SIZE - __STACK_SIZE - __HEAP_SIZE) /* 剩餘僅 28 KB 給靜態變數 */
```
- **缺點 A（浪費內存）**：若全域變數只用 8 KB，未使用的 20 KB 空間將無法被 Heap 使用。
- **缺點 B（維護不易）**：若全域變數增加超過 28 KB，編譯器立即報錯 `Region RW_RAM has overflowed`，必須手動微調算術。

#### 2. 本專案採用的動態自動計算架構 (`m3331.sct`)
利用 Armlink 的符號計算引擎與 `ImageLimit(RW_RAM)`（即 `bss_end`，全域靜態變數的結束位址）：
```sct
#define __RAM_BASE      0x20000000
#define __RAM_SIZE      0x00050000
#define __RAM_END       (__RAM_BASE + __RAM_SIZE)   /* 0x20050000 (SRAM 頂部) */

#define __STACK_SIZE    0x00001000
#define __HEAP_SIZE     (__RAM_END - AlignExpr(ImageLimit(RW_RAM), 8)) /* 自動延伸至 SRAM 頂部 */

LR_ROM __RO_BASE __RO_SIZE {
  ...
  ARM_LIB_STACK (__STACK_TOP) EMPTY -(__STACK_SIZE) {   ; 4 KB 堆疊位於 SRAM 底部
  }

  RW_RAM __RW_BASE (__RAM_SIZE - __STACK_SIZE) {        ; 靜態變數區 (RW + ZI / BSS)
   .ANY (+RW +ZI)
  }

  ARM_LIB_HEAP (AlignExpr(+0, 8)) EMPTY __HEAP_SIZE {   ; Heap 起點為 bss_end，終點為 SRAM_END
  }
}
```

#### 3. 自動計算效果與實測數據
- **`RW_RAM` 彈性成長**：全域/靜態變數不再受限於 28 KB，最多可靈活使用達 316 KB。
- **`ARM_LIB_HEAP` 最大化**：
  - 起始位址 `Base`：自動緊接在 `RW_RAM` 之後（`0x20002F30`，即 `bss_end` 8位元組對齊處）。
  - 終止位址 `Limit`：精確對齊至 `0x20050000`（`__RAM_END`）。
  - **實際可用 Heap 大小**：由原先死板的 288 KB 自動擴展至 **308.2 KB (315,600 Bytes)**，靜態變數越少，Heap 自動獲得越多！
- **C 語言運行時完全同步**：
  `task_lv.c` 中的 Heap 監控函式可直接透過 Arm C Runtime 的 `Image$$ARM_LIB_HEAP$$ZI$$Base` 與 `Image$$ARM_LIB_HEAP$$ZI$$Limit` 自動讀出當前編譯後的精確全幅大小，無需任何手動常數設定。

---

### **Q4: 當 SPI NOR Flash 掛載失敗時，如何透過 CherryUSB MSC 匯出成隨身碟給 PC 存取？**

**答案：本專案已整合 CherryUSB 裝置端堆疊與 M3331 HSUSBD 高速驅動，在掛載失敗時自動啟動 MSC 隨身碟功能。**

#### 1. 架構與觸發機制
- **條件觸發**：在 `ui_init()` 呼叫 `fatfs_spinor_init()` 時，若 SPI NOR Flash 尚未格式化或未找到有效 FAT 檔案系統（回傳非 0）：
  1. 螢幕顯示 `"Mount Fail!"` 與指示訊息 `"USB MSC: Connect USB to PC to write disk"`。
  2. 自動呼叫 `msc_spinor_init()`，啟動 CherryUSB Mass Storage Class 裝置。
- **硬體層 (`misc/CherryUSB-port`)**：
  - `usb_glue_m3331.c`：配置 M3331 專屬 High-Speed PHY（`SYS->USBPHY`）、重置與致能 `HSUSBD_MODULE` 時鐘。
  - `usb_dc_hs.c`：控制 M3331 HSUSBD 引擎，處理 `USBD20_IRQHandler` 中斷。
  - `glue_nuvoton.c`：轉接 CherryUSB Device 核心與底層控制器回呼。
- **儲存層 (`components/msc_spinor.c`)**：
  - 區塊大小 (`scsi_blk_size`) 設為 4096 位元組，與 SPI NOR Flash 物理抹除區塊及 FAT 檔案系統完美 1:1 對齊。
  - 讀取回呼：調用 `SpiFlash_QPI_FastRead()` 透過 QSPI 快速讀出。
  - 寫入回呼：調用 `SpiFlash_WriteSector()` 抹除並寫入對應 4KB 區塊。
  - 啟用 `CONFIG_USBDEV_MSC_THREAD`：將讀寫操作自中斷移至 FreeRTOS 任務中執行，確保 QSPI PDMA 等待信號量合法且不阻塞中斷。
