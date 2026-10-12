#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generate a comprehensive, beautiful HTML matrix of all supported boards,
their project build targets across IDEs (Keil MDK, IAR, NuEclipse GCC, VSCode),
and their test/demo items and hardware acceleration features.
"""

import os
import sys
import glob
import json
import xml.etree.ElementTree as ET
try:
    import yaml
except ImportError:
    yaml = None

def get_keil_info(proj_path):
    targets = []
    compiler = 'ARMCLANG (V6)'
    try:
        tree = ET.parse(proj_path)
        for t in tree.findall('.//Target'):
            name = t.find('TargetName')
            if name is not None and name.text:
                targets.append(name.text.strip())
            pcc = t.find('.//pCCUsed')
            if pcc is not None and pcc.text:
                if pcc.text.startswith('5'):
                    compiler = 'ARMCC (V5)'
                elif pcc.text.startswith('6'):
                    compiler = 'ARMCLANG (V6)'
        uac6 = tree.find('.//uAC6')
        if uac6 is not None and uac6.text == '1':
            compiler = 'ARMCLANG (V6)'
        elif proj_path.endswith('.uvproj'):
            compiler = 'ARMCC (V5)'
    except Exception:
        pass
    return targets, compiler

def get_iar_info(proj_path):
    configs = []
    try:
        tree = ET.parse(proj_path)
        for c in tree.findall('.//configuration/name'):
            if c.text:
                configs.append(c.text.strip())
    except Exception:
        pass
    return configs

def get_gcc_info(proj_path):
    configs = []
    try:
        tree = ET.parse(proj_path)
        for c in tree.findall('.//cconfiguration'):
            for sm in c.findall('storageModule'):
                if 'name' in sm.attrib:
                    configs.append(sm.attrib['name'])
                    break
    except Exception:
        pass
    return configs

def get_vscode_info(proj_path):
    target_types = []
    try:
        if yaml:
            with open(proj_path, 'r', encoding='utf-8') as f:
                d = yaml.safe_load(f)
                solution = d.get('solution', {})
                target_types = [t.get('type') for t in solution.get('target-types', []) if t.get('type')]
        else:
            with open(proj_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            for line in lines:
                if 'type:' in line and '-' in line:
                    parts = line.split('type:')
                    if len(parts) > 1:
                        target_types.append(parts[1].strip())
    except Exception:
        pass
    return target_types

# Detailed database of hardware, MCU and test features for each project
PROJECT_METADATA = {
    'board/numaker-hmi-m2354': {
        'name': 'NuMaker-HMI-M2354',
        'category': 'board',
        'arch_group': 'cortex-m',
        'mcu': 'M2354KJFAE',
        'core': 'Arm Cortex-M23 (64 MHz)',
        'arch': 'Armv8-M Baseline',
        'os': 'FreeRTOS',
        'display': '3.2" NuMaker-NuTFT (320x240, ILI9341 SPI)',
        'touch': '4-Wire ADC 類比電阻式觸控 (ADC Resistive)',
        'accel': ['軟體渲染 (SW Render)'],
        'memory': '1MB Flash, 256KB SRAM, TrustZone 安全隔離',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            '系統效能監控器 (Perf Monitor & Sysmon)',
            'FreeRTOS 多工作業系統排程測試'
        ]
    },
    'board/numaker-hmi-m2l31': {
        'name': 'NuMaker-HMI-M2L31',
        'category': 'board',
        'arch_group': 'cortex-m',
        'mcu': 'M2L31KI6AE',
        'core': 'Arm Cortex-M23 (72 MHz)',
        'arch': 'Armv8-M Baseline',
        'os': 'FreeRTOS',
        'display': '3.2" NuMaker-NuTFT (320x240, ILI9341 SPI)',
        'touch': '4-Wire ADC 類比電阻式觸控 (ADC Resistive)',
        'accel': ['軟體渲染 (SW Render)'],
        'memory': '512KB Flash, 96KB SRAM',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            '系統效能監控器 (Perf Monitor)',
            'FreeRTOS 多工作業系統排程測試'
        ]
    },
    'board/numaker-hmi-m3334': {
        'name': 'NuMaker-HMI-M3334',
        'category': 'board',
        'arch_group': 'cortex-m',
        'mcu': 'M3334KI6AE',
        'core': 'Arm Cortex-M33 with FPU/DSP (180 MHz)',
        'arch': 'Armv8-M Mainline',
        'os': 'FreeRTOS',
        'display': '1.3" NuFun (240x240, ST7789V SPI) / 3.2" NuTFT (320x240, ILI9341 SPI)',
        'touch': '4-Wire ADC 類比電阻式觸控 (NuTFT)',
        'accel': ['硬體 DSP 數學運算加速'],
        'memory': '512KB Flash, 128KB SRAM',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            '圓形/方形面板適配 (NuFun 240x240 & NuTFT 320x240)',
            'FreeRTOS 多工作業系統排程測試'
        ]
    },
    'board/numaker-hmi-m3351': {
        'name': 'NuMaker-HMI-M3351',
        'category': 'board',
        'arch_group': 'cortex-m',
        'mcu': 'M3351KJ6AE',
        'core': 'Arm Cortex-M33 with FPU/DSP (120 MHz)',
        'arch': 'Armv8-M Mainline',
        'os': 'FreeRTOS',
        'display': '3.2" NuMaker-NuTFT (320x240, ILI9341 SPI)',
        'touch': '4-Wire ADC 類比電阻式觸控 (ADC Resistive)',
        'accel': ['硬體 DSP 數學運算加速'],
        'memory': '256KB Flash, 64KB SRAM',
        'test_items': [
            'LVGL 互動式圖表展示 (lv_example_entry: 儀表指針與動態刻度)',
            '觸控位置回饋與互動 LED 效果 (Screen Touch Event Callback)',
            'FreeRTOS 多工作業系統排程測試'
        ]
    },
    'board/numaker-hmi-m467': {
        'name': 'NuMaker-HMI-M467',
        'category': 'board',
        'arch_group': 'cortex-m',
        'mcu': 'M467HJHAN',
        'core': 'Arm Cortex-M4 with FPU/DSP (200 MHz)',
        'arch': 'Armv7E-M',
        'os': 'FreeRTOS',
        'display': '5" 800x480 (LT7381 EBI) / 4.3" 480x272 (NV3041A/FSA506 EBI) / 3.2" 320x240 (NuTFT SPI)',
        'touch': '電容式 FT5316/GT911 CTP、電阻式 ST1663I/ADC RTP',
        'accel': ['EBI 外部匯流排高速傳輸', '硬體 DSP 運算加速'],
        'memory': '1024KB Flash, 512KB SRAM, 外部 HyperRAM 擴充',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            'LVGL 互動範例展示 (lv_examples: 儀表、滑桿、多功能觸控)',
            '支援多重螢幕與雙觸控模式切換驗證 (4.3" / 5" / 3.2")',
            'FreeRTOS 多工作業系統排程測試'
        ]
    },
    'board/numaker-hmi-m487': {
        'name': 'NuMaker-HMI-M487',
        'category': 'board',
        'arch_group': 'cortex-m',
        'mcu': 'M487JIDAE',
        'core': 'Arm Cortex-M4 with FPU/DSP (192 MHz)',
        'arch': 'Armv7E-M',
        'os': 'FreeRTOS',
        'display': '3.2" NuMaker-NuTFT (320x240, ILI9341 SPI)',
        'touch': '4-Wire ADC 類比電阻式觸控 (ADC Resistive)',
        'accel': ['EBI 高速傳輸', '硬體 DSP 運算加速'],
        'memory': '512KB Flash, 160KB SRAM',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            '系統效能監控器 (Perf Monitor)',
            'FreeRTOS 多工作業系統排程測試'
        ]
    },
    'board/numaker-hmi-m5531': {
        'name': 'NuMaker-HMI-M5531',
        'category': 'board',
        'arch_group': 'cortex-m',
        'mcu': 'M5531H2LJAE',
        'core': 'Arm Cortex-M55 with Helium MVE (150 MHz)',
        'arch': 'Armv8.1-M Mainline',
        'os': 'FreeRTOS',
        'display': '3.2" NuMaker-NuTFT (320x240, ILI9341 SPI)',
        'touch': '4-Wire ADC 類比電阻式觸控 (ADC Resistive)',
        'accel': ['GDMA 2D 硬體加速 (Fill/Blit/Blend)', 'Arm-2D (Helium MVE 向量最佳化)'],
        'memory': '2MB Flash, 1.5MB SRAM, SPIM HyperRAM',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            'GDMA 2D 晶片硬體色塊填滿與圖層混合加速測試',
            'Arm-2D Helium (MVE) 向量繪圖核心運算驗證 (SW_ARM2D)',
            'FreeRTOS 多工作業系統排程測試'
        ]
    },
    'board/numaker-hmi-ma35d1': {
        'name': 'NuMaker-HMI-MA35D1',
        'category': 'board',
        'arch_group': 'cortex-a',
        'mcu': 'MA35D1',
        'core': 'Dual 64-bit Arm Cortex-A35 (800 MHz)',
        'arch': 'Armv8-A 64-bit',
        'os': 'FreeRTOS (AArch64)',
        'display': '7" 1024x600 RGB/LVDS 液晶螢幕',
        'touch': 'Goodix GT911 電容式多點觸控 (I2C CTP)',
        'accel': ['GFX 2D 晶片圖形控制器', '64-bit Neon SIMD 向量加速 (SW_NEON)'],
        'memory': '內建 128MB/256MB/512MB DDR SDRAM, SD Card / eMMC 開機',
        'test_items': [
            'LVGL Widgets 綜合元件高解析度展示 (1024x600)',
            'GFX 2D 硬體圖形控制器雙緩衝區幀同步換頁 (Task Sync)',
            '64-bit AArch64 Neon 向量 SIMD 混合渲染效能驗證',
            'FreeRTOS 64-bit 核心排程'
        ]
    },
    'board/numaker-hmi-ma35h0': {
        'name': 'NuMaker-HMI-MA35H0',
        'category': 'board',
        'arch_group': 'cortex-a',
        'mcu': 'MA35H0',
        'core': '64-bit Arm Cortex-A35 (650 MHz)',
        'arch': 'Armv8-A 64-bit',
        'os': 'FreeRTOS (AArch64)',
        'display': '7" 1024x600 RGB 液晶螢幕',
        'touch': '4-Wire ADC 電阻式觸控 / Goodix GT911 電容式觸控',
        'accel': ['GFX 2D 晶片圖形控制器', '64-bit Neon SIMD 向量加速 (SW_NEON)'],
        'memory': '內建 128MB DDR SDRAM',
        'test_items': [
            'LVGL Widgets 綜合元件高解析度展示 (1024x600)',
            '支援電阻式 RTP 與電容式 CTP 雙觸控目標配置',
            'GFX 2D 硬體換頁與 Neon 向量混合渲染驗證',
            'FreeRTOS 64-bit 核心排程'
        ]
    },
    'board/numaker-hmi-n9h20': {
        'name': 'NuMaker-HMI-N9H20',
        'category': 'board',
        'arch_group': 'arm9',
        'mcu': 'N9H20K51N',
        'core': 'ARM926EJ-S (200 MHz)',
        'arch': 'ARMv5TEJ',
        'os': 'FreeRTOS',
        'display': '4.3" (480x272) / 7" (800x480) Sync RGB 螢幕',
        'touch': '4-Wire ADC 類比電阻式觸控 (ADC Resistive)',
        'accel': ['BitBLT 2D 硬體區塊搬移與 Alpha 混合加速器'],
        'memory': '封裝內疊 32MB SDRAM, NAND Flash 開機',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            'BitBLT 2D 硬體加速區塊填滿與影像混合測試',
            'NAND Flash 系統韌體開機與 FreeRTOS 排程'
        ]
    },
    'board/numaker-hmi-n9h26': {
        'name': 'NuMaker-HMI-N9H26',
        'category': 'board',
        'arch_group': 'arm9',
        'mcu': 'N9H26K61N',
        'core': 'ARM926EJ-S (264 MHz)',
        'arch': 'ARMv5TEJ',
        'os': 'FreeRTOS',
        'display': '3.2" (320x240) / 7" (800x480) Sync RGB 螢幕',
        'touch': '4-Wire ADC 類比電阻式觸控 (ADC Resistive)',
        'accel': ['BitBLT 2D 硬體區塊搬移與 Alpha 混合加速器'],
        'memory': '封裝內疊 64MB SDRAM, SD Card / NAND 開機',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            '支援 320x240 與 800x480 雙解析度配置',
            'BitBLT 2D 硬體加速與 FreeRTOS 排程'
        ]
    },
    'board/numaker-hmi-n9h30': {
        'name': 'NuMaker-HMI-N9H30',
        'category': 'board',
        'arch_group': 'arm9',
        'mcu': 'N9H30F61IEC',
        'core': 'ARM926EJ-S (300 MHz)',
        'arch': 'ARMv5TEJ',
        'os': 'FreeRTOS',
        'display': '7" 800x480 Sync RGB 液晶螢幕',
        'touch': '4-Wire ADC 類比電阻式觸控 (ADC Resistive)',
        'accel': ['2DGE (2D Graphics Engine) 硬體圖形引擎'],
        'memory': '封裝內疊 64MB DDR2 SDRAM, SPI NAND Flash 開機',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            '2DGE 硬體繪圖引擎色塊填滿與區塊搬移測試',
            'FreeRTOS 多工作業系統排程測試'
        ]
    },
    'board/numaker-hmi-n9h31': {
        'name': 'NuMaker-HMI-N9H31',
        'category': 'board',
        'arch_group': 'arm9',
        'mcu': 'N9H31K51N',
        'core': 'ARM926EJ-S (300 MHz)',
        'arch': 'ARMv5TEJ',
        'os': 'FreeRTOS',
        'display': '7" 800x480 Sync RGB 液晶螢幕',
        'touch': '電阻式觸控 (V1.0) / 電容式觸控 (V1.1)',
        'accel': ['2DGE (2D Graphics Engine) 硬體圖形引擎'],
        'memory': '封裝內疊 32MB DDR SDRAM',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            '支援硬體修訂版雙目標切換 (V1.0 電阻式 / V1.1 電容式)',
            '2DGE 硬體加速繪圖與 FreeRTOS 排程'
        ]
    },
    'board/numaker-iiot-nuc980gxd': {
        'name': 'NuMaker-IIoT-NUC980G1D / G2D',
        'category': 'board',
        'arch_group': 'arm9',
        'mcu': 'NUC980DK61Y',
        'core': 'ARM926EJ-S (300 MHz)',
        'arch': 'ARMv5TEJ',
        'os': 'FreeRTOS',
        'display': '3.2" NuMaker-NuTFT (320x240, ILI9341)',
        'touch': '4-Wire ADC 類比電阻式觸控 (ADC Resistive)',
        'accel': ['G2D 硬體 2DGE 圖形引擎加速 / G1D 軟體搬移'],
        'memory': '封裝內疊 64MB DDR SDRAM, SPI NOR/NAND Flash',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            'G2D 硬體加速 vs G1D 軟體搬移之雙效能模式驗證',
            'FreeRTOS 多工作業系統排程測試'
        ]
    },
    'board/numaker-iot-m2354': {
        'name': 'NuMaker-IoT-M2354',
        'category': 'board',
        'arch_group': 'cortex-m',
        'mcu': 'M2354KJFAE',
        'core': 'Arm Cortex-M23 (64 MHz)',
        'arch': 'Armv8-M Baseline',
        'os': 'FreeRTOS',
        'display': '3.2" NuMaker-NuTFT (320x240, ILI9341 SPI)',
        'touch': '4-Wire ADC 類比電阻式觸控 (ADC Resistive)',
        'accel': ['軟體渲染 (SW Render)'],
        'memory': '1MB Flash, 256KB SRAM, Wi-Fi & 物聯網安全元件',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            '物聯網低功耗安全架構整合驗證',
            'FreeRTOS 多工作業系統排程測試'
        ]
    },
    'board/numaker-x-m55m1d': {
        'name': 'NuMaker-X-M55M1D',
        'category': 'board',
        'arch_group': 'cortex-m',
        'mcu': 'M55M1H2LJAE',
        'core': 'Arm Cortex-M55 with Helium MVE (200 MHz)',
        'arch': 'Armv8.1-M Mainline',
        'os': 'FreeRTOS',
        'display': '5" 800x480 (LT7381) / 4.3" 480x272 (NV3041A/FSA506) / 3.2" 320x240 (NuTFT)',
        'touch': '電容式 FT5316/GT911 CTP、電阻式 ST1663I/ADC RTP',
        'accel': ['GDMA 2D 晶片硬體加速', 'Arm-2D (Helium MVE 向量指令最佳化)'],
        'memory': '2MB Dual-bank Flash, 1.5MB SRAM, 外部 SPIM HyperRAM',
        'test_items': [
            'LVGL Widgets 綜合元件展示 (lv_demo_widgets)',
            'GDMA 2D 晶片硬體繪圖加速測試 (Fill/Blit/Blend/Layer)',
            'Arm-2D 同步加速整合 (SW_ARM2D: 向量旋轉/縮放/Mask/抗鋸齒)',
            '4 大液晶面板與多種觸控晶片相容性測試 (800x480/480x272/320x240)'
        ]
    },
    'misc/m55m1_xip': {
        'name': 'M55M1 XiP (eXecute-In-Place) 評估專案',
        'category': 'misc',
        'arch_group': 'cortex-m',
        'mcu': 'M55M1H2LJAE',
        'core': 'Arm Cortex-M55 with Helium MVE (200 MHz)',
        'arch': 'Armv8.1-M Mainline',
        'os': 'FreeRTOS',
        'display': 'NuMaker-X-M55M1D 顯示平台 (800x480)',
        'touch': '電容式 FT5316 觸控',
        'accel': ['GDMA 2D 硬體加速', 'Arm-2D Helium 向量加速'],
        'memory': 'SPIM0 外部 NORFlash / HyperFlash 晶片原地執行 (XiP)',
        'test_items': [
            'APROM 內建 Flash vs SPIM 外部 Flash 原地執行效能評測 (Benchmark)',
            'XiP Loader 開機引導與 SPIM 高速快顯設定',
            'ITCM / DTCM 記憶體邊界與 Scatter 載入最佳化'
        ]
    },
    'misc/numaker-hmi-m3334_gif': {
        'name': 'NuMaker-HMI-M3334 動態 GIF 播放專案',
        'category': 'misc',
        'arch_group': 'cortex-m',
        'mcu': 'M3334KI6AE',
        'core': 'Arm Cortex-M33 with FPU/DSP (180 MHz)',
        'arch': 'Armv8-M Mainline',
        'os': 'FreeRTOS',
        'display': '1.3" NuFun (240x240) / 3.2" NuTFT (320x240)',
        'touch': '4-Wire ADC 類比電阻式觸控',
        'accel': ['軟體解碼與 DSP 向量運算'],
        'memory': 'SPI NOR Flash 搭載 FAT 檔案系統 (FatFs)',
        'test_items': [
            '從外部 SPI NOR Flash FAT 檔案系統即時讀取 GIF 動畫檔',
            'LVGL 官方 GIF 解碼器渲染與循環播放驗證',
            '圓形與方形面板雙顯示輸出'
        ]
    },
    'misc/numaker-hmi-m3334_rice_cooker': {
        'name': 'NuMaker-HMI-M3334 智能電子鍋 UI 專案',
        'category': 'misc',
        'arch_group': 'cortex-m',
        'mcu': 'M3334KI6AE / PC 模擬器',
        'core': 'Arm Cortex-M33 (180 MHz) & x86_64 PC',
        'arch': 'Armv8-M Mainline / x86_64',
        'os': 'FreeRTOS / Windows MinGW',
        'display': '1.3" NuFun 圓形螢幕 (240x240, ST7789V)',
        'touch': '實體按鍵 / 圓形旋鈕控制介面',
        'accel': ['PC 模擬器與 MCU 雙平台即時渲染'],
        'memory': '內部 Flash & SRAM',
        'test_items': [
            '智能家電商業化 UI 換頁與多重烹飪模式選單切換',
            '動態加熱動畫與即時時鐘 (RTC / Software Clock) 狀態管理',
            '跨平台雙軌測試：PC Simulator (MinGW/GCC) 與 MCU 韌體一體化'
        ]
    },
    'misc/numaker-hmi-ma35h0_jpegd': {
        'name': 'NuMaker-HMI-MA35H0 硬體 JPEG 解碼幻燈片',
        'category': 'misc',
        'arch_group': 'cortex-a',
        'mcu': 'MA35H0',
        'core': '64-bit Arm Cortex-A35 (650 MHz)',
        'arch': 'Armv8-A 64-bit',
        'os': 'FreeRTOS (AArch64)',
        'display': '7" 1024x600 RGB 液晶螢幕',
        'touch': '4-Wire ADC 電阻式觸控',
        'accel': ['硬體 JPEG 解碼引擎 (HW JPEGD)', 'GFX 2D 圖形控制器', 'Neon SIMD'],
        'memory': '128MB DDR SDRAM, SD Card 圖片讀取',
        'test_items': [
            'MA35H0 晶片硬體 JPEG 解碼引擎高速解碼與圖片呈現',
            '高解析度 (1024x600) 照片幻燈片輪播 (Slideshow)',
            'LVGL 自訂 Image Decoder 驅動橋接整合'
        ]
    },
    'misc/numaker-hmi-n9h20_bitblt_rotation': {
        'name': 'NuMaker-HMI-N9H20 BitBLT 旋轉與圖塊搬移評測',
        'category': 'misc',
        'arch_group': 'arm9',
        'mcu': 'N9H20K51N',
        'core': 'ARM926EJ-S (200 MHz)',
        'arch': 'ARMv5TEJ',
        'os': 'FreeRTOS',
        'display': '4.3" 480x272 Sync RGB 液晶螢幕',
        'touch': '4-Wire ADC 類比電阻式觸控',
        'accel': ['BitBLT 硬體圖形搬移器', '硬體 90/180/270 度旋轉'],
        'memory': '32MB 內疊 SDRAM',
        'test_items': [
            'N9H20 BitBLT 硬體 90° / 180° / 270° 螢幕旋轉功能評測',
            '硬體區塊搬移與透明度 Alpha 混合速度測試',
            'FreeRTOS 繪圖排程與效能監控'
        ]
    },
    'misc/numaker-hmi-n9h20_jpegd': {
        'name': 'NuMaker-HMI-N9H20 硬體 JPEG 幻燈片專案',
        'category': 'misc',
        'arch_group': 'arm9',
        'mcu': 'N9H20K51N',
        'core': 'ARM926EJ-S (200 MHz)',
        'arch': 'ARMv5TEJ',
        'os': 'FreeRTOS',
        'display': '4.3" 480x272 Sync RGB 液晶螢幕',
        'touch': '4-Wire ADC 類比電阻式觸控',
        'accel': ['N9H20 晶片硬體 JPEG 解碼引擎 (HW JPEGD)', 'BitBLT 加速'],
        'memory': '32MB 內疊 SDRAM',
        'test_items': [
            'N9H20 專屬硬體 JPEG 解碼引擎驅動整合',
            'SD Card 圖片讀取與 480x272 相簿幻燈片輪播',
            '硬體解碼與 LVGL 快取記憶體生命週期測試'
        ]
    },
    'misc/numaker-hmi-n9h30_jpegd': {
        'name': 'NuMaker-HMI-N9H30 硬體 JPEG 幻燈片專案',
        'category': 'misc',
        'arch_group': 'arm9',
        'mcu': 'N9H30F61IEC',
        'core': 'ARM926EJ-S (300 MHz)',
        'arch': 'ARMv5TEJ',
        'os': 'FreeRTOS',
        'display': '7" 800x480 Sync RGB 液晶螢幕',
        'touch': '4-Wire ADC 類比電阻式觸控',
        'accel': ['N9H30 硬體 JPEG 解碼引擎 (HW JPEGD)', '2DGE 圖形引擎'],
        'memory': '64MB 內疊 DDR2 SDRAM',
        'test_items': [
            'N9H30 晶片硬體 JPEG 解碼引擎高速解碼',
            '800x480 大尺寸液晶螢幕相片展示與輪播',
            '2DGE 硬體混合顯示流暢度測試'
        ]
    },
    'misc/numaker-hmi-n9h30_sls_examples': {
        'name': 'NuMaker-HMI-N9H30 SquareLine Studio 商業範例',
        'category': 'misc',
        'arch_group': 'arm9',
        'mcu': 'N9H30F61IEC',
        'core': 'ARM926EJ-S (300 MHz)',
        'arch': 'ARMv5TEJ',
        'os': 'FreeRTOS',
        'display': '7" 800x480 Sync RGB 液晶螢幕',
        'touch': '4-Wire ADC 類比電阻式觸控',
        'accel': ['2DGE 硬體圖形引擎 (Fill/Blit/Blend)'],
        'memory': '64MB 內疊 DDR2 SDRAM',
        'test_items': [
            '3D Printer (3D 列印機操控面板商業 UI 展示)',
            'Coffee Machine (智慧咖啡機選單與調製進度展示)',
            'E-Bike Dashboard (電動自行車儀表板、時速與電量監控展示)',
            '2DGE 晶片硬體圖形加速渲染驗證'
        ]
    },
    'misc/numaker-x-m55m1d_jpegd': {
        'name': 'NuMaker-X-M55M1D 硬體 JPEG 幻燈片與 MJPEG 影片播放',
        'category': 'misc',
        'arch_group': 'cortex-m',
        'mcu': 'M55M1H2LJAE',
        'core': 'Arm Cortex-M55 with Helium MVE (200 MHz)',
        'arch': 'Armv8.1-M Mainline',
        'os': 'FreeRTOS',
        'display': '5" 800x480 (LT7381) / 4.3" 480x272 (NV3041A/FSA506)',
        'touch': '電容式 FT5316/GT911 CTP、電阻式 ST1663I RTP',
        'accel': ['M55M1 晶片硬體 JPEG 解碼引擎 (HW JPEGD)', 'GDMA 2D 硬體加速', 'Arm-2D Helium 向量加速'],
        'memory': '2MB Dual-bank Flash, 1.5MB SRAM, SPIM HyperRAM, SD Card',
        'test_items': [
            '硬體 JPEG 解碼相簿幻燈片輪播 (JPG Slideshow)',
            'Motion JPEG (MJPEG) 動態影音播放：整合 AVILib AVI 容器解封裝與即時解碼串流',
            '多重解析度 (800x480 及 480x272) 硬體解碼幀率測試',
            'GDMA 2D 及 Arm-2D 雙硬體加速圖層輸出驗證'
        ]
    },
    'misc/numaker-x-m55m1d_sls_examples': {
        'name': 'NuMaker-X-M55M1D SquareLine Studio 商業範例',
        'category': 'misc',
        'arch_group': 'cortex-m',
        'mcu': 'M55M1H2LJAE',
        'core': 'Arm Cortex-M55 with Helium MVE (200 MHz)',
        'arch': 'Armv8.1-M Mainline',
        'os': 'FreeRTOS',
        'display': '5" 800x480 (LT7381) / 4.3" 480x272 (NV3041A/FSA506)',
        'touch': '電容式 FT5316/GT911 CTP、電阻式 ST1663I RTP',
        'accel': ['GDMA 2D 晶片硬體加速', 'Arm-2D (Helium MVE 向量指令最佳化)'],
        'memory': '2MB Dual-bank Flash, 1.5MB SRAM, 外部 SPIM HyperRAM',
        'test_items': [
            '3D Printer (3D 列印機商業 UI 介面, 800x480)',
            'Coffee Machine (智慧咖啡機觸控 UI 介面, 800x480)',
            'E-Bike Dashboard (電動自行車儀表板, 800x480 及 480x272)',
            'GDMA 2D 繪圖加速與 Arm-2D Helium 向量圖形渲染綜合評測'
        ]
    }
}

def collect_project_data(repo_root):
    data = []
    projects = sorted(glob.glob(os.path.join(repo_root, 'board', '*')) + 
                      glob.glob(os.path.join(repo_root, 'misc', '*')))
    
    idx = 1
    for proj_dir in projects:
        if not os.path.isdir(proj_dir):
            continue
        rel_path = os.path.relpath(proj_dir, repo_root).replace('\\', '/')
        if rel_path not in PROJECT_METADATA:
            continue
        
        meta = PROJECT_METADATA[rel_path]
        
        # Discover build projects
        keil_files = [f for f in glob.glob(os.path.join(proj_dir, 'Keil', '*.uvproj*')) if not f.endswith('.log')]
        iar_files = glob.glob(os.path.join(proj_dir, 'IAR', '*.ewp'))
        gcc_files = glob.glob(os.path.join(proj_dir, 'GCC', '.cproject'))
        vscode_files = glob.glob(os.path.join(proj_dir, 'VSCode', '*.csolution.yml'))
        
        build_items = {}
        if keil_files:
            keil_list = []
            for k in keil_files:
                targets, compiler = get_keil_info(k)
                keil_list.append({
                    'file': os.path.basename(k),
                    'compiler': compiler,
                    'targets': targets
                })
            build_items['keil'] = keil_list
        
        if iar_files:
            iar_list = []
            for i in iar_files:
                configs = get_iar_info(i)
                iar_list.append({
                    'file': os.path.basename(i),
                    'compiler': 'IAR ICCARM',
                    'configs': configs
                })
            build_items['iar'] = iar_list
            
        if gcc_files:
            gcc_list = []
            for g in gcc_files:
                configs = get_gcc_info(g)
                compiler_name = 'NuEclipse GCC (AArch64)' if 'aarch64' in meta.get('arch', '').lower() or 'a35' in meta.get('core', '').lower() else 'NuEclipse GCC (arm-none-eabi)'
                gcc_list.append({
                    'file': '.cproject',
                    'compiler': compiler_name,
                    'targets': configs
                })
            build_items['gcc'] = gcc_list
            
        if vscode_files:
            vscode_list = []
            for v in vscode_files:
                targets = get_vscode_info(v)
                vscode_list.append({
                    'file': os.path.basename(v),
                    'toolchain': 'CMSIS-Toolbox (AC6 / vcpkg)',
                    'targets': targets
                })
            build_items['vscode'] = vscode_list
            
        entry = {
            'index': idx,
            'rel_path': rel_path,
            'meta': meta,
            'build_items': build_items
        }
        data.append(entry)
        idx += 1
        
    return data

def render_html(data):
    total_projects = len(data)
    total_targets = 0
    cortex_m_cnt = sum(1 for d in data if d['meta']['arch_group'] == 'cortex-m')
    cortex_a_cnt = sum(1 for d in data if d['meta']['arch_group'] == 'cortex-a')
    arm9_cnt = sum(1 for d in data if d['meta']['arch_group'] == 'arm9')
    
    for d in data:
        bi = d['build_items']
        for k in bi.get('keil', []):
            total_targets += len(k['targets'])
        for g in bi.get('gcc', []):
            total_targets += len(g['targets'])
        for v in bi.get('vscode', []):
            total_targets += len(v['targets'])
        for i in bi.get('iar', []):
            total_targets += len(i['configs'])

    # Build rows
    rows_html = []
    for d in data:
        idx = d['index']
        m = d['meta']
        b = d['build_items']
        
        # Architecture badge
        if m['arch_group'] == 'cortex-m':
            arch_badge = f'<span class="badge badge-cm">{m["core"]}</span>'
        elif m['arch_group'] == 'cortex-a':
            arch_badge = f'<span class="badge badge-ca">{m["core"]}</span>'
        else:
            arch_badge = f'<span class="badge badge-arm9">{m["core"]}</span>'
            
        cat_badge = '<span class="badge badge-board">Board</span>' if m['category'] == 'board' else '<span class="badge badge-misc">Misc Demo</span>'
        
        # Build Items column HTML
        build_blocks = []
        if 'keil' in b:
            for k in b['keil']:
                tg_tags = ''.join([f'<span class="target-tag target-keil">{t}</span>' for t in k['targets']])
                build_blocks.append(f'''
                <div class="ide-block">
                    <div class="ide-title"><span class="ide-icon keil-icon">MDK</span> <strong>Keil MDK</strong> <small class="text-muted">({k["compiler"]})</small></div>
                    <div class="ide-file"><code>{k["file"]}</code></div>
                    <div class="target-container">{tg_tags}</div>
                </div>
                ''')
                
        if 'iar' in b:
            for i in b['iar']:
                cfg_tags = ''.join([f'<span class="target-tag target-iar">{c}</span>' for c in i['configs']])
                build_blocks.append(f'''
                <div class="ide-block">
                    <div class="ide-title"><span class="ide-icon iar-icon">IAR</span> <strong>IAR EWARM</strong> <small class="text-muted">({i["compiler"]})</small></div>
                    <div class="ide-file"><code>{i["file"]}</code></div>
                    <div class="target-container">{cfg_tags}</div>
                </div>
                ''')
                
        if 'gcc' in b:
            for g in b['gcc']:
                tg_tags = ''.join([f'<span class="target-tag target-gcc">{t}</span>' for t in g['targets']])
                build_blocks.append(f'''
                <div class="ide-block">
                    <div class="ide-title"><span class="ide-icon gcc-icon">GCC</span> <strong>NuEclipse</strong> <small class="text-muted">({g["compiler"]})</small></div>
                    <div class="ide-file"><code>{g["file"]}</code></div>
                    <div class="target-container">{tg_tags}</div>
                </div>
                ''')
                
        if 'vscode' in b:
            for v in b['vscode']:
                tg_tags = ''.join([f'<span class="target-tag target-vscode">{t}</span>' for t in v['targets']])
                build_blocks.append(f'''
                <div class="ide-block">
                    <div class="ide-title"><span class="ide-icon vscode-icon">VS</span> <strong>VSCode</strong> <small class="text-muted">({v["toolchain"]})</small></div>
                    <div class="ide-file"><code>{v["file"]}</code></div>
                    <div class="target-container">{tg_tags}</div>
                </div>
                ''')
                
        build_html = ''.join(build_blocks)
        
        # Test & Features column HTML
        accel_tags = ''.join([f'<span class="accel-tag">{a}</span>' for a in m['accel']])
        tests_li = ''.join([f'<li>{t}</li>' for t in m['test_items']])
        
        test_html = f'''
        <div class="feature-item">
            <span class="feature-label">📺 顯示螢幕：</span> {m["display"]}
        </div>
        <div class="feature-item">
            <span class="feature-label">👆 觸控介面：</span> {m["touch"]}
        </div>
        <div class="feature-item">
            <span class="feature-label">⚡ 硬體加速：</span> <div class="accel-container">{accel_tags}</div>
        </div>
        <div class="feature-item">
            <span class="feature-label">💾 記憶體：</span> {m["memory"]}
        </div>
        <div class="test-item-box">
            <div class="test-title">🧪 測試與範例項目：</div>
            <ul class="test-list">{tests_li}</ul>
        </div>
        '''
        
        row = f'''
        <tr class="project-row" data-arch="{m['arch_group']}" data-category="{m['category']}" data-search="{d['rel_path']} {m['name']} {m['mcu']} {m['core']} {m['display']} {' '.join(m['accel'])}">
            <td class="text-center font-bold text-muted">{idx}</td>
            <td class="board-col">
                <div class="board-name">{m['name']}</div>
                <div class="board-path"><code>{d['rel_path']}</code></div>
                <div class="badge-row">
                    {cat_badge}
                    <span class="badge badge-mcu">{m['mcu']}</span>
                    {arch_badge}
                    <span class="badge badge-os">{m['os']}</span>
                </div>
            </td>
            <td class="build-col">
                {build_html}
            </td>
            <td class="test-col">
                {test_html}
            </td>
        </tr>
        '''
        rows_html.append(row)
        
    all_rows = ''.join(rows_html)

    html = f'''<!DOCTYPE html>
<html lang="zh-Hant">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Nuvoton LVGL v9.6.0 專案建置與測試矩陣表</title>
    <style>
        :root {{
            --primary: #1e3a8a;
            --primary-light: #3b82f6;
            --accent: #0ea5e9;
            --bg: #f8fafc;
            --card-bg: #ffffff;
            --text-main: #0f172a;
            --text-muted: #64748b;
            --border: #e2e8f0;
            --border-dark: #cbd5e1;
            
            --cm-color: #2563eb;
            --cm-bg: #dbeafe;
            --ca-color: #7c3aed;
            --ca-bg: #ede9fe;
            --arm9-color: #d97706;
            --arm9-bg: #fef3c7;
            
            --keil-color: #0284c7;
            --keil-bg: #e0f2fe;
            --iar-color: #d97706;
            --iar-bg: #fef3c7;
            --gcc-color: #dc2626;
            --gcc-bg: #fee2e2;
            --vscode-color: #9333ea;
            --vscode-bg: #f3e8ff;
            
            --accel-bg: #ecfdf5;
            --accel-text: #059669;
            --accel-border: #a7f3d0;
        }}
        
        * {{
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }}
        
        body {{
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            background-color: var(--bg);
            color: var(--text-main);
            line-height: 1.5;
            padding: 24px 16px;
        }}
        
        .container {{
            max-width: 1440px;
            margin: 0 auto;
        }}
        
        /* Header */
        header {{
            background: linear-gradient(135deg, #1e3a8a 0%, #0369a1 100%);
            color: white;
            padding: 32px 28px;
            border-radius: 16px;
            box-shadow: 0 10px 25px -5px rgba(30, 58, 138, 0.25);
            margin-bottom: 24px;
        }}
        
        .header-title-row {{
            display: flex;
            align-items: center;
            justify-content: space-between;
            flex-wrap: wrap;
            gap: 16px;
        }}
        
        h1 {{
            font-size: 1.85rem;
            font-weight: 700;
            letter-spacing: -0.02em;
        }}
        
        .version-tag {{
            display: inline-block;
            background: rgba(255, 255, 255, 0.2);
            backdrop-filter: blur(8px);
            padding: 6px 14px;
            border-radius: 9999px;
            font-size: 0.875rem;
            font-weight: 600;
            border: 1px solid rgba(255, 255, 255, 0.3);
        }}
        
        .subtitle {{
            margin-top: 8px;
            color: #e0f2fe;
            font-size: 1rem;
            max-width: 900px;
        }}
        
        /* Stats Dashboard */
        .stats-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
            gap: 16px;
            margin-bottom: 24px;
        }}
        
        .stat-card {{
            background: var(--card-bg);
            padding: 20px;
            border-radius: 12px;
            border: 1px solid var(--border);
            box-shadow: 0 2px 4px rgba(0,0,0,0.02);
            transition: transform 0.15s ease, box-shadow 0.15s ease;
        }}
        
        .stat-card:hover {{
            transform: translateY(-2px);
            box-shadow: 0 6px 12px rgba(0,0,0,0.05);
        }}
        
        .stat-label {{
            font-size: 0.85rem;
            color: var(--text-muted);
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 0.04em;
        }}
        
        .stat-value {{
            font-size: 2rem;
            font-weight: 800;
            color: var(--primary);
            margin: 4px 0;
            line-height: 1.1;
        }}
        
        .stat-desc {{
            font-size: 0.8rem;
            color: var(--text-muted);
        }}
        
        /* Filter Controls */
        .controls-card {{
            background: var(--card-bg);
            padding: 18px 24px;
            border-radius: 12px;
            border: 1px solid var(--border);
            margin-bottom: 24px;
            display: flex;
            flex-wrap: wrap;
            gap: 16px;
            align-items: center;
            justify-content: space-between;
        }}
        
        .search-box {{
            flex: 1;
            min-width: 280px;
            position: relative;
        }}
        
        .search-input {{
            width: 100%;
            padding: 10px 16px 10px 38px;
            border: 1px solid var(--border-dark);
            border-radius: 8px;
            font-size: 0.95rem;
            outline: none;
            transition: border-color 0.15s ease, box-shadow 0.15s ease;
        }}
        
        .search-input:focus {{
            border-color: var(--primary-light);
            box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.15);
        }}
        
        .search-icon {{
            position: absolute;
            left: 12px;
            top: 50%;
            transform: translateY(-50%);
            color: var(--text-muted);
            font-size: 1rem;
        }}
        
        .filter-buttons {{
            display: flex;
            flex-wrap: wrap;
            gap: 8px;
        }}
        
        .filter-btn {{
            background: #f1f5f9;
            border: 1px solid var(--border);
            padding: 8px 14px;
            border-radius: 8px;
            font-size: 0.85rem;
            font-weight: 600;
            cursor: pointer;
            color: var(--text-muted);
            transition: all 0.15s ease;
        }}
        
        .filter-btn:hover {{
            background: #e2e8f0;
            color: var(--text-main);
        }}
        
        .filter-btn.active {{
            background: var(--primary);
            color: white;
            border-color: var(--primary);
        }}
        
        /* Table */
        .table-container {{
            background: var(--card-bg);
            border-radius: 12px;
            border: 1px solid var(--border);
            overflow: hidden;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.03);
        }}
        
        table {{
            width: 100%;
            border-collapse: collapse;
            text-align: left;
            font-size: 0.9rem;
        }}
        
        th {{
            background: #f8fafc;
            color: #334155;
            font-weight: 700;
            padding: 14px 16px;
            border-bottom: 2px solid var(--border);
            white-space: nowrap;
        }}
        
        td {{
            padding: 16px;
            border-bottom: 1px solid var(--border);
            vertical-align: top;
        }}
        
        tr:hover td {{
            background-color: #f8fafc;
        }}
        
        .text-center {{ text-align: center; }}
        .font-bold {{ font-weight: 700; }}
        
        /* Columns */
        .board-col {{
            min-width: 260px;
            max-width: 320px;
        }}
        
        .build-col {{
            min-width: 340px;
            max-width: 440px;
        }}
        
        .test-col {{
            min-width: 380px;
        }}
        
        .board-name {{
            font-size: 1.05rem;
            font-weight: 700;
            color: var(--primary);
            margin-bottom: 4px;
        }}
        
        .board-path {{
            margin-bottom: 8px;
        }}
        
        code {{
            background: #f1f5f9;
            padding: 2px 6px;
            border-radius: 4px;
            font-size: 0.8rem;
            color: #475569;
            font-family: Consolas, Monaco, monospace;
        }}
        
        .badge-row {{
            display: flex;
            flex-wrap: wrap;
            gap: 6px;
            margin-top: 6px;
        }}
        
        .badge {{
            display: inline-block;
            font-size: 0.72rem;
            font-weight: 700;
            padding: 2px 8px;
            border-radius: 6px;
            letter-spacing: 0.02em;
        }}
        
        .badge-board {{ background: #e0f2fe; color: #0369a1; border: 1px solid #bae6fd; }}
        .badge-misc {{ background: #fef3c7; color: #b45309; border: 1px solid #fde68a; }}
        .badge-mcu {{ background: #f1f5f9; color: #334155; border: 1px solid #cbd5e1; }}
        .badge-cm {{ background: var(--cm-bg); color: var(--cm-color); }}
        .badge-ca {{ background: var(--ca-bg); color: var(--ca-color); }}
        .badge-arm9 {{ background: var(--arm9-bg); color: var(--arm9-color); }}
        .badge-os {{ background: #ecfdf5; color: #047857; border: 1px solid #a7f3d0; }}
        
        /* IDE Block */
        .ide-block {{
            background: #f8fafc;
            border: 1px solid var(--border);
            border-radius: 8px;
            padding: 10px 12px;
            margin-bottom: 10px;
        }}
        
        .ide-block:last-child {{
            margin-bottom: 0;
        }}
        
        .ide-title {{
            font-size: 0.85rem;
            margin-bottom: 4px;
            display: flex;
            align-items: center;
            gap: 6px;
        }}
        
        .ide-icon {{
            font-size: 0.68rem;
            font-weight: 800;
            padding: 1px 5px;
            border-radius: 4px;
            display: inline-block;
        }}
        
        .keil-icon {{ background: var(--keil-bg); color: var(--keil-color); }}
        .iar-icon {{ background: var(--iar-bg); color: var(--iar-color); }}
        .gcc-icon {{ background: var(--gcc-bg); color: var(--gcc-color); }}
        .vscode-icon {{ background: var(--vscode-bg); color: var(--vscode-color); }}
        
        .ide-file {{
            font-size: 0.78rem;
            margin-bottom: 6px;
        }}
        
        .target-container {{
            display: flex;
            flex-wrap: wrap;
            gap: 4px;
        }}
        
        .target-tag {{
            font-size: 0.75rem;
            font-weight: 600;
            padding: 2px 8px;
            border-radius: 4px;
            font-family: Consolas, Monaco, monospace;
        }}
        
        .target-keil {{ background: #e0f2fe; color: #0369a1; border: 1px solid #bae6fd; }}
        .target-iar {{ background: #fef3c7; color: #92400e; border: 1px solid #fde68a; }}
        .target-gcc {{ background: #fee2e2; color: #991b1b; border: 1px solid #fecaca; }}
        .target-vscode {{ background: #f3e8ff; color: #6b21a8; border: 1px solid #e9d5ff; }}
        
        /* Features & Tests */
        .feature-item {{
            font-size: 0.85rem;
            margin-bottom: 6px;
        }}
        
        .feature-label {{
            font-weight: 700;
            color: #475569;
        }}
        
        .accel-container {{
            display: inline-flex;
            flex-wrap: wrap;
            gap: 4px;
            vertical-align: middle;
        }}
        
        .accel-tag {{
            background: var(--accel-bg);
            color: var(--accel-text);
            border: 1px solid var(--accel-border);
            font-size: 0.72rem;
            font-weight: 700;
            padding: 2px 7px;
            border-radius: 4px;
        }}
        
        .test-item-box {{
            margin-top: 8px;
            background: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 6px;
            padding: 8px 12px;
        }}
        
        .test-title {{
            font-size: 0.8rem;
            font-weight: 700;
            color: #1e293b;
            margin-bottom: 4px;
        }}
        
        .test-list {{
            list-style-type: none;
            padding-left: 0;
            font-size: 0.82rem;
            color: #334155;
        }}
        
        .test-list li {{
            position: relative;
            padding-left: 14px;
            margin-bottom: 2px;
        }}
        
        .test-list li::before {{
            content: "•";
            position: absolute;
            left: 2px;
            color: var(--primary-light);
            font-weight: bold;
        }}
        
        /* Footer */
        footer {{
            margin-top: 32px;
            text-align: center;
            font-size: 0.85rem;
            color: var(--text-muted);
            padding: 16px;
            border-top: 1px solid var(--border);
        }}
        
        /* Empty Search Notice */
        .empty-notice {{
            display: none;
            text-align: center;
            padding: 40px;
            font-size: 1rem;
            color: var(--text-muted);
        }}
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <header>
            <div class="header-title-row">
                <div>
                    <h1>新唐科技 NuMicro® LVGL 專案建置與測試矩陣</h1>
                    <div class="subtitle">全面彙整支援之開發板與應用專案、IDE 建置目標 (Keil MDK / IAR / GCC / VSCode)、硬體 2D 加速與測試項目</div>
                </div>
                <div>
                    <span class="version-tag">LVGL v9.6.0</span>
                </div>
            </div>
        </header>

        <!-- Stats Dashboard -->
        <div class="stats-grid">
            <div class="stat-card">
                <div class="stat-label">支援板子與專案總數</div>
                <div class="stat-value">{total_projects}</div>
                <div class="stat-desc">16 主板專案 + 10 應用範例</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">編譯目標組合總數</div>
                <div class="stat-value">{total_targets}+</div>
                <div class="stat-desc">涵蓋不同螢幕與觸控驅動</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">處理器架構涵蓋</div>
                <div class="stat-value">6 種</div>
                <div class="stat-desc">M55, A35 (64b), M4, M33, M23, ARM9</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">支援之 IDE 工具鏈</div>
                <div class="stat-value">4 款</div>
                <div class="stat-desc">Keil MDK, IAR, NuEclipse, VSCode</div>
            </div>
        </div>

        <!-- Filter Controls -->
        <div class="controls-card">
            <div class="search-box">
                <span class="search-icon">🔍</span>
                <input type="text" id="searchInput" class="search-input" placeholder="即時搜尋：板名、MCU、核心、解析度、硬體加速 (如 Helium, Neon, GDMA)...">
            </div>
            <div class="filter-buttons">
                <button class="filter-btn active" data-filter="all">全部專案 ({total_projects})</button>
                <button class="filter-btn" data-filter="cm">Cortex-M ({cortex_m_cnt})</button>
                <button class="filter-btn" data-filter="ca">Cortex-A 64-bit ({cortex_a_cnt})</button>
                <button class="filter-btn" data-filter="arm9">ARM9 ({arm9_cnt})</button>
                <button class="filter-btn" data-filter="board">主開發板 (16)</button>
                <button class="filter-btn" data-filter="misc">應用展示 (10)</button>
            </div>
        </div>

        <!-- Matrix Table -->
        <div class="table-container">
            <table>
                <thead>
                    <tr>
                        <th style="width: 40px;" class="text-center">#</th>
                        <th style="width: 280px;">開發板 / 專案資訊</th>
                        <th style="width: 380px;">支援之 IDE 專案建置項目 (Targets)</th>
                        <th>測試項目與硬體功能特性</th>
                    </tr>
                </thead>
                <tbody id="matrixBody">
                    {all_rows}
                </tbody>
            </table>
            <div id="emptyNotice" class="empty-notice">無符合搜尋條件的專案項目</div>
        </div>

        <!-- Footer -->
        <footer>
            新唐科技 Nuvoton Technology Corporation &bull; NuMicro® Graphical User Interface (GUI) Solution for LVGL v9.6.0
        </footer>
    </div>

    <!-- Client-Side Search and Filter Logic -->
    <script>
        document.addEventListener('DOMContentLoaded', function() {{
            const searchInput = document.getElementById('searchInput');
            const filterBtns = document.querySelectorAll('.filter-btn');
            const rows = document.querySelectorAll('.project-row');
            const emptyNotice = document.getElementById('emptyNotice');
            
            let currentFilter = 'all';
            let currentSearch = '';
            
            function filterTable() {{
                let visibleCount = 0;
                rows.forEach(row => {{
                    const arch = row.getAttribute('data-arch');
                    const category = row.getAttribute('data-category');
                    const searchContent = row.getAttribute('data-search').toLowerCase();
                    
                    let matchFilter = true;
                    if (currentFilter === 'cm') matchFilter = (arch === 'cortex-m');
                    else if (currentFilter === 'ca') matchFilter = (arch === 'cortex-a');
                    else if (currentFilter === 'arm9') matchFilter = (arch === 'arm9');
                    else if (currentFilter === 'board') matchFilter = (category === 'board');
                    else if (currentFilter === 'misc') matchFilter = (category === 'misc');
                    
                    let matchSearch = true;
                    if (currentSearch) {{
                        matchSearch = searchContent.includes(currentSearch);
                    }}
                    
                    if (matchFilter && matchSearch) {{
                        row.style.display = '';
                        visibleCount++;
                    }} else {{
                        row.style.display = 'none';
                    }}
                }});
                
                if (visibleCount === 0) {{
                    emptyNotice.style.display = 'block';
                }} else {{
                    emptyNotice.style.display = 'none';
                }}
            }}
            
            searchInput.addEventListener('input', function(e) {{
                currentSearch = e.target.value.trim().toLowerCase();
                filterTable();
            }});
            
            filterBtns.forEach(btn => {{
                btn.addEventListener('click', function() {{
                    filterBtns.forEach(b => b.classList.remove('active'));
                    this.classList.add('active');
                    currentFilter = this.getAttribute('data-filter');
                    filterTable();
                }});
            }});
        }});
    </script>
</body>
</html>
'''
    return html

def main():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
    data = collect_project_data(repo_root)
    html_content = render_html(data)
    
    out_dir = os.path.abspath(os.path.join(os.path.dirname(__file__)))
    target_html = os.path.join(out_dir, 'project_matrix.html')
    index_html = os.path.join(out_dir, 'index.html')
    
    with open(target_html, 'w', encoding='utf-8') as f:
        f.write(html_content)
    with open(index_html, 'w', encoding='utf-8') as f:
        f.write(html_content)
        
    print(f'Successfully generated {target_html} and {index_html} with {len(data)} projects.')

if __name__ == '__main__':
    main()
