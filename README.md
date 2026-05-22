# **lv_port_nuvoton**

![Eclipse Build](https://github.com/wosayttn/lv_port_nuvoton/actions/workflows/NuEclipse.yml/badge.svg)
![VSCode Build](https://github.com/wosayttn/lv_port_nuvoton/actions/workflows/VSCode.yml/badge.svg)

Currently, the LVGL version is synchronized with **release/v9.4**.

This repository contains the LVGLv9 demo port for the NuMaker development board from Nuvoton.

## Clone Repositories

Clone the main lv_port_nuvoton repository and the necessary git modules separately. For example, you only need the lvgl and n9h20 git submodules, execute the following command for cloning in git bash console.

```bash
git clone https://github.com/OpenNuvoton/lv_port_nuvoton
cd lv_port_nuvoton
git submodule update --progress --init -- "lvgl"
git submodule update --progress --init -- "bsp/n9h20"
```

or, clone lv_port_nuvoton with recurse-sumbmodules option for cloning all repositories directly.

```bash
git clone --recurse-submodules https://github.com/OpenNuvoton/lv_port_nuvoton
```

## **Supported porting on NuMaker Boards**

| **NuMaker Board Name** | **Demo Project Folder** |
| ------------------------- | ------------------------- |
| **NuMaker-HMI-N9H20** | [LVGL Widget](./board/numaker-hmi-n9h20)<br>[Bitblt Screen Rotation](./misc/numaker-hmi-n9h20_bitblt_rotation)<br>[JPEG Decoder](./misc/numaker-hmi-n9h20_jpegd) |
| **NuMaker-HMI-N9H26** | [LVGL Widget](./board/numaker-hmi-n9h26) |
| **NuMaker-HMI-N9H30** | [LVGL Widget](./board/numaker-hmi-n9h30)<br>[JPEG Decoder](./misc/numaker-hmi-n9h30_jpegd)<br>[SquareLine Studio Examples](./misc/numaker-hmi-n9h30_sls_examples) |
| **NuMaker-HMI-N9H31** | [LVGL Widget](./board/numaker-hmi-n9h31) |
| **NuMaker-IIoT-NUC980G1D** | [LVGL Widget](./board/numaker-iiot-nuc980gxd) |
| **NuMaker-IIoT-NUC980G2D** | [LVGL Widget](./board/numaker-iiot-nuc980gxd) |
| **NuMaker-HMI-M2354** | [LVGL Widget](./board/numaker-hmi-m2354) |
| **NuMaker-IoT-M2354** | [LVGL Widget](./board/numaker-iot-m2354) |
| **NuMaker-HMI-M2L31** | [LVGL Widget](./board/numaker-hmi-m2l31) |
| **NuMaker-HMI-M3334** | [LVGL Widget](./board/numaker-hmi-m3334)<br>[Smart Rice Cooker](./misc/numaker-hmi-m3334_rice_cooker) |
| **NuMaker-HMI-M3351** | [LVGL SimpleUI](./board/numaker-hmi-m3351) |
| **NuMaker-HMI-M487** | [LVGL Widget](./board/numaker-hmi-m487) |
| **NuMaker-HMI-M467** | [LVGL Widget](./board/numaker-hmi-m467) |
| **NuMaker-HMI-M55M1** | [LVGL Widget](./board/numaker-hmi-m55m1)<br>[JPEG Decoder](./misc/numaker-hmi-m55m1_jpegd)<br>[SquareLine Studio Examples](./misc/numaker-hmi-m55m1_sls_examples) |
| **NuMaker-HMI-M5531** | [LVGL Widget](./board/numaker-hmi-m5531) |
| **NuMaker-HMI-MA35D1** | [LVGL Widget](./board/numaker-hmi-ma35d1) |
| **NuMaker-HMI-MA35H0** | [LVGL Widget](./board/numaker-hmi-ma35h0)<br>[JPEG Decoder](./misc/numaker-hmi-ma35h0_jpegd)  |
