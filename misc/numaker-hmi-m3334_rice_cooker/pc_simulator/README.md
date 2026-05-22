# Rice Cooker PC Simulator (WYSIWYG)

在 PC 上以 SDL2 視窗 **像素精確** 模擬 320×240 LCD，滑鼠模擬觸控、鍵盤模擬實體按鍵，達到所見即所得效果。

> UI 原始碼直接引用 `../ui_files/*.c`，與 MCU 目標板 **共用同一份程式碼、零修改**。

---

## 目錄結構

```
pc_simulator/
├── build_emulator.ps1  # 一鍵建置腳本 (自動安裝工具鏈 + SDL2)
├── CMakeLists.txt      # CMake 建構腳本
├── lv_conf.h           # PC 專用 LVGL 設定 (SDL2 driver, No OS)
├── main.c              # SDL2 視窗 + 鍵盤映射 + 主迴圈
├── sim_stubs.c         # 硬體相依函式的 stub (buzzer, clock, keypad)
└── README.md
```

---

## 快速開始 (Windows 一鍵建置)

```powershell
cd pc_simulator
.\build_emulator.ps1          # 建置
.\build_emulator.ps1 -Run     # 建置並啟動模擬器
.\build_emulator.ps1 -Clean   # 清除後重建
```

腳本會自動完成：
1. 安裝 **WinLibs** (MinGW-w64 GCC + CMake + Ninja)
2. 下載並安裝 **SDL2** 開發庫
3. 偵測 LVGL 原始碼路徑
4. CMake configure + build
5. 複製 `SDL2.dll` 至輸出目錄

---

## 手動建置

### Windows (已有 MinGW + SDL2)

```powershell
cd pc_simulator
mkdir build; cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DLVGL_PATH="C:/path/to/lvgl"
cmake --build .
.\rice_cooker_sim.exe
```

### Linux / macOS

```bash
sudo apt install libsdl2-dev   # Ubuntu
# brew install sdl2            # macOS

cd pc_simulator && mkdir build && cd build
cmake .. -DLVGL_PATH=/path/to/lvgl
make -j$(nproc)
./rice_cooker_sim
```

---

## 操作方式

| PC 輸入 | 對應功能 | 說明 |
|---------|---------|------|
| 滑鼠左鍵 | 觸控點擊 | 可測試按鈕位置、大小、回饋 |
| 滑鼠拖曳 | 觸控滑動 | 模擬手指滑動操作 |
| `M` 鍵 | Menu 鍵 | 進入/返回選單 |
| `↑` / `↓` | 上/下選擇 | 瀏覽模式清單 |
| `Enter` | Start/確認 | 開始烹飪或確認選項 |
| 關閉視窗 | 結束模擬器 | — |

---

## 設計優勢

| 特點 | 說明 |
|------|------|
| **所見即所得** | SDL2 視窗 1:1 像素對應 320×240 LCD |
| **快速迭代** | 修改 UI → 秒級重編 → 即時預覽，免燒錄 |
| **觸控驗證** | 滑鼠模擬觸控，驗證按鈕命中區域與互動邏輯 |
| **同一份原始碼** | `ui_files/` 在 PC 和 MCU 間共用，確保一致性 |
| **硬體隔離** | `sim_stubs.c` 提供 buzzer/clock/keypad 的 PC stub |
| **CI 友好** | 可整合到自動化 UI 測試流程 |

---

## 前置需求 (手動安裝時)

| 項目 | 來源 |
|------|------|
| LVGL v9.x 原始碼 | `https://github.com/lvgl/lvgl` |
| SDL2 開發庫 | [GitHub Releases](https://github.com/libsdl-org/SDL/releases) |
| CMake ≥ 3.16 | https://cmake.org/download/ |
| C compiler | WinLibs / MinGW / GCC / MSVC |

> 💡 使用 `build_emulator.ps1` 可跳過手動安裝步驟。
