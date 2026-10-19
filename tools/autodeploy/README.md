# Nuvoton MCU Auto-Deploy Tool (基於 probe-rs)

此工具位於 `tools/autodeploy/`，專為 Nuvoton NuMicro Cortex-M 系列微控制器設計，透過調用開源硬體除錯工具 **probe-rs** 實現：
1. **Target Device ID Probe**：自動讀取硬體目標暫存器（`SYS_PDID` @ `0x40000000`），精確解碼晶片型號（如 `M3351LICAE`、`M3334KGGAE`、`M5531K2LJAE` 等）、MCU 家族、CPU 架構、Flash/SRAM 規格。
2. **Firmware APROM Program**：自動燒錄韌體（支援 `.bin`、`.hex`、`.elf` 格式）至 MCU APROM，支援自動定址、晶片抹除、寫入後校驗（Verify）及重置重啟。
3. **Target Reset**：透過除錯器重置 MCU。
4. **Flash Algorithm / Target YAML 轉換**：支援將 Keil CMSIS `.FLM` 演算法轉為 probe-rs 專屬的目標設定檔。

---

## 支援的 MCU 平台

| 平台系列 | CPU 核心架構 | APROM 基底位址 | 預設 Flash 大小 | Page / Sector 大小 | 範例晶片型號 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **M3331 / M3334** | Armv8-M (Cortex-M33) | `0x00000000` | 512 KB | 1024 B | M3334KGGAE, M3334LGGAE, M3333... |
| **M3351** | Armv8-M (Cortex-M33) | `0x00000000` | 512 KB / 1 MB | 8192 B | M3351LICAE, M3351LJCAE, M3351... |
| **M5531** | Armv8.1-M (Cortex-M55) | `0x00100000` | 2048 KB (2MB) | 512 B | M5531K2LJAE, M5531R2LJAE, M5531H2LJAE |
| **M55M1** | Armv8.1-M (Cortex-M55) | `0x00100000` | 2048 KB (2MB) | 512 B | M55M1H2LJAE, M55M1K2LJAE, M55M1R2LJAE |
| **M460 / M467** | Armv7E-M (Cortex-M4) | `0x00000000` | 1024 KB (1MB) | 4096 B | M467HJHAE, M463KJHAE, M460YGCAE |
| **M480 / M487** | Armv7E-M (Cortex-M4) | `0x00000000` | 512 KB | 4096 B | M487JIDAE, M481LGAAE, M482KGAAE |
| **M2354** | Armv8-M (Cortex-M23) | `0x00000000` | 1024 KB (1MB) | 2048 B | M2354SJFBE, M2354LJFAE, M2354... |
| **M2L31** | Armv8-M (Cortex-M23) | `0x00000000` | 512 KB | 128 B | M2L31KGDAE, M2L31LGDAE, M2L31... |
| **M031** | Armv6-M (Cortex-M0+) | `0x00000000` | 512 KB | 512 B | M031... |

---

## 前置條件

1. **Python 3.8+**
2. **probe-rs**：
   - 安裝方式（若尚未安裝）：
     ```bash
     cargo install probe-rs-tools --locked
     ```
     或至 [probe-rs GitHub Releases](https://github.com/probe-rs/probe-rs/releases) 下載執行檔並加入環境變數 `PATH`。
3. **除錯器**：
   - Nu-Link2（支援 CMSIS-DAP 模式）、J-Link、DAPLink 或其他 probe-rs 支援之除錯器。

---

## CLI 命令使用說明

所有功能均可透過 `deploy.py` 執行：

### 1. 偵測連接之除錯器 (`list-probes`)
```bash
python tools/autodeploy/deploy.py list-probes
```
若系統連接了多支除錯器（例如同時插上 M3331 與 M5531 開發板）：
```text
Connected Debug Probes (2 found):
  [0] Nu-Link2 CMSIS-DAP
      Selector: 0416:2004-0:13010000000A25020450395D00001023
      Serial  : 13010000000A25020450395D00001023
      Details : CMSIS-DAP
  [1] Nu-Link2 CMSIS-DAP
      Selector: 0416:2004-0:13150000000A25020450395D00000EFE
      Serial  : 13150000000A25020450395D00000EFE
      Details : CMSIS-DAP

Hint: You can pass --probe 0 or --probe 1 to select by index.
```

### 2. 探測連接的 MCU 目標裝置 (`probe`)
自動透過 SWD 讀取 Nuvoton MCU 的 `SYS->PDID`（`0x40000000`）並解析完整規格。
當系統連接多支除錯器時，預設會**自動遍歷並列出所有連接的目標 MCU**：
```bash
# 自動探測所有已連接的目標裝置
python tools/autodeploy/deploy.py probe

# 指定特定除錯器（例如 probe 0 或 probe 1）
python tools/autodeploy/deploy.py probe --probe 0
python tools/autodeploy/deploy.py probe --probe 1
```
多裝置輸出範例：
```text
Target Probe Successful! Found 2 connected target devices:

[Device 1/2] M3334KGGAE (Nuvoton NuMicro M3331/M3334 Series)
  Debug Probe     : [0] Nu-Link2 CMSIS-DAP (0416:2004-0:13010000000A25020450395D00001023)
  Part Number     : M3334KGGAE
  MCU Family      : Nuvoton NuMicro M3331/M3334 Series (M3331)
  Device ID (PDID): 0x03300040 (SYS_PDID @ 0x40000000)
  CPU Core        : Cortex-M33 (armv8m)
  APROM Base Addr : 0x00000000
  APROM Flash Size: 512 KB
  Flash Page Size : 1024 Bytes
  SRAM Base Addr  : 0x20000000
  SRAM Size       : 128 KB
  Target Profile  : tools/autodeploy/targets/M3331.yaml

[Device 2/2] M5531K2LJAE (Nuvoton NuMicro M5531 Series)
  Debug Probe     : [1] Nu-Link2 CMSIS-DAP (0416:2004-0:13150000000A25020450395D00000EFE)
  Part Number     : M5531K2LJAE
  MCU Family      : Nuvoton NuMicro M5531 Series (M5531)
  Device ID (PDID): 0xA2000060 (SYS_PDID @ 0x40000000)
  CPU Core        : Cortex-M55 (armv8m)
  APROM Base Addr : 0x00100000
  APROM Flash Size: 2048 KB
  Flash Page Size : 512 Bytes
  SRAM Base Addr  : 0x20100000
  SRAM Size       : 1024 KB
  Target Profile  : tools/autodeploy/targets/M5531.yaml
```

若需機器可讀格式（如 CI/CD 自動化整合），可加上 `--json`（會輸出所有探測裝置的 JSON 陣列）：
```bash
python tools/autodeploy/deploy.py probe --json
```

### 3. 多板自動配對與批次燒錄 (`auto-deploy`)
當電腦上同時插有 $N$ 塊不同 MCU 開發板（例如 M3331、M5531 等），而編譯目錄已產生多個不同平台的韌體二進位檔時，本命令會：
1. **自動探測所有連接之板子**（取得各自的 `probe_selector`、`part_name` 與晶片系列）。
2. **自動在指定的韌體目錄中搜尋匹配的二進位檔**（依照晶片型號如 `m3334`、`m5531`、板名與版本比對）。
3. **依序自動將正確的二進位檔燒錄到正確的板子上**，並完成校驗與重置。
4. **自動產出 GitHub Actions Job Summary**（若在 CI/CD 環境執行）。

```bash
# 預覽配對矩陣（Dry-run 模式，不實際寫入）
python tools/autodeploy/deploy.py auto-deploy --firmware-dir board --dry-run

# 自動尋找 board/ 目錄下所有韌體，並依序燒錄至對應的開發板
python tools/autodeploy/deploy.py auto-deploy --firmware-dir board

# 搭配關鍵字過濾（例如只燒錄 nutft 顯示螢幕版本的韌體）
python tools/autodeploy/deploy.py auto-deploy --firmware-dir board --filter nutft

# 指定自定義韌體輸出目錄（例如 CI artifact 目錄）
python tools/autodeploy/deploy.py auto-deploy --firmware-dir ./artifacts
```

執行範例輸出：
```text
Scanning for firmware binaries in: C:\D\PRJ\lv_port_nuvoton\board...
Found 213 firmware files in search path.
Probing all connected Nuvoton MCU target boards...
Detected 2 connected target boards.

#   Probe    Part Number      Family   Status             Firmware Binary
--------------------------------------------------------------------------------
0   [0]      M3334KGGAE       M3331    PENDING            numaker-hmi-m3334\Bin\lvgl_m3334ki_nutft.bin
1   [1]      M5531K2LJAE      M5531    PENDING            numaker-hmi-m5531\Bin\lvgl_m5531_nutft.bin

>>> Flashing Board [0]: M3334KGGAE (M3331) <<<
    Binary : board\numaker-hmi-m3334\Bin\lvgl_m3334ki_nutft.bin
    Probe  : 0416:2004-0:13010000000A25020450395D00001023
    Address: 0x00000000
Successfully programmed M3334KGGAE!

>>> Flashing Board [1]: M5531K2LJAE (M5531) <<<
    Binary : board\numaker-hmi-m5531\Bin\lvgl_m5531_nutft.bin
    Probe  : 0416:2004-0:13150000000A25020450395D00000EFE
    Address: 0x00100000
Successfully programmed M5531K2LJAE!
```

---

### 4. 單一韌體手動燒錄至 APROM (`program` / `flash`)
本工具支援 **全自動偵測燒錄**：若未指定 `--chip`，會先主動探測硬體取得晶片型號，自動選取對應的 Flash 演算法與基底位址！

- **自動偵測並燒錄 BIN 檔**（自動使用該晶片的 APROM 基底位址，M5531 自動為 `0x00100000`，M3331/M3351 為 `0x00000000`）：
  ```bash
  python tools/autodeploy/deploy.py program build/output.bin
  ```

- **手動指定晶片系列**：
  ```bash
  python tools/autodeploy/deploy.py program build/output.bin --chip M3331
  python tools/autodeploy/deploy.py program build/output.bin --chip M5531
  python tools/autodeploy/deploy.py program build/output.bin --chip M3351
  ```

- **燒錄 ELF 或 HEX 檔案**：
  ```bash
  python tools/autodeploy/deploy.py program build/output.elf
  python tools/autodeploy/deploy.py program build/output.hex
  ```

- **進階燒錄參數**：
  ```bash
  python tools/autodeploy/deploy.py program build/output.bin \
      --chip M5531 \
      --address 0x00100000 \
      --speed 4000 \
      --chip-erase \
      --no-verify \
      --no-reset
  ```
  - `--speed <kHz>`: 設定 SWD 時脈速度（例如 4000 kHz）。
  - `--chip-erase`: 燒錄前執行全晶片抹除。
  - `--no-verify`: 略過燒錄後校驗。
  - `--no-reset`: 燒錄後不重置 MCU。
  - `--probe <VID:PID / Serial>`: 指定多支除錯器時的目標 probe。
  - `--dry-run`: 模擬執行而不實際寫入 Flash。

### 4. 重置目標晶片 (`reset`)
```bash
# 重置預設或指定的目標晶片
python tools/autodeploy/deploy.py reset --chip M3331

# 指定特定除錯器進行重置
python tools/autodeploy/deploy.py reset --chip M3331 --probe 0
python tools/autodeploy/deploy.py reset --chip M5531 --probe 1
```

### 5. 查看所有支援之平台列表 (`list-targets`)
```bash
python tools/autodeploy/deploy.py list-targets
```

### 6. 從 Keil .FLM 產生 probe-rs 目標設定檔 (`gen-target`)
若需要為新晶片由 Keil FLM 演算法產生 probe-rs 的 Target YAML：
```bash
python tools/autodeploy/deploy.py gen-target C:\Keil_v5\ARM\Flash\M3331_AP_512.FLM \
    -o tools/autodeploy/targets/M3331.yaml \
    --family M3331 \
    --core armv8m
```

---

## 整合至 GitHub Actions Deploy Stage

因為燒錄實體板子需要存取主機的實體 USB 介面，在 GitHub Actions 工作流程中需配置 **Self-hosted Runner**（安裝於本台連接除錯器之主機）：

### Workflow 範例 (`.github/workflows/deploy.yml`)

```yaml
name: Deploy Firmware to Hardware

on:
  workflow_run:
    workflows: ["VSCode"]
    types: [completed]
  workflow_dispatch:

jobs:
  hardware-deploy:
    runs-on: [self-hosted, windows]
    if: ${{ github.event.workflow_run.conclusion == 'success' || github.event_name == 'workflow_dispatch' }}

    steps:
      - name: Checkout Code
        uses: actions/checkout@v4

      - name: Download Build Artifacts
        uses: actions/download-artifact@v4
        with:
          path: ./artifacts
        continue-on-error: true

      - name: Check Connected Nu-Link Probes
        run: |
          python tools/autodeploy/deploy.py list-probes

      - name: Auto-Deploy to Connected Boards
        run: |
          # 自動探測連接的 N 塊板子，搜尋 ./artifacts 或 ./board，並將對應韌體燒錄至各板
          python tools/autodeploy/deploy.py auto-deploy \
            --firmware-dir board \
            --filter nutft
```

---

## Python API 使用範例

您亦可在其他 Python 腳本或測試程式中直接引用：

```python
from tools.autodeploy import ProbeRsWrapper, decode_pdid

wrapper = ProbeRsWrapper()

# 1. 探測連接的晶片
device_info = wrapper.probe_target()
print(f"Detected: {device_info['part_name']} ({device_info['family_name']})")
print(f"PDID: {device_info['pdid']}")
print(f"Flash Base: 0x{device_info['flash_base']:08X}")

# 2. 燒錄韌體
wrapper.download_firmware(
    firmware_path="build/firmware.bin",
    chip=device_info["family"],
    target_yaml=f"tools/autodeploy/targets/{device_info['family']}.yaml",
    base_address=device_info["flash_base"],
    verify=True,
    reset=True
)

# 3. 重置晶片
wrapper.reset_target(chip=device_info["family"])
```

---

## 架構說明

```
tools/autodeploy/
├── __init__.py           # Package 導出介面
├── deploy.py             # CLI 程式進入點 (probe, program, reset, list 等命令)
├── probe_rs.py           # probe-rs 執行檔封裝 (list, read, download, reset)
├── nuvoton_db.py         # Nuvoton SYS_PDID 資料庫與晶片解碼器
├── devices_db.py         # 內建 800+ 款 Nuvoton MCU 晶片型號對照表
├── flm_parser.py         # Keil .FLM ELF 演算法解析與 Target YAML 產生器
├── README.md             # 本說明文件
└── targets/              # 預先產生的 probe-rs Target YAML 設定檔
    ├── M3331.yaml        # M3331/M3334 512KB APROM 演算法
    ├── M3351.yaml        # M3351 512KB APROM 演算法
    ├── M5531.yaml        # M5531 2MB APROM @ 0x00100000 演算法
    ├── M55M1.yaml        # M55M1 2MB APROM @ 0x00100000 演算法
    ├── M460.yaml         # M460 1MB APROM 演算法
    ├── M480.yaml         # M480 512KB APROM 演算法
    ├── M2354.yaml        # M2354 1MB APROM 演算法
    └── M2L31.yaml        # M2L31 512KB APROM 演算法
```
