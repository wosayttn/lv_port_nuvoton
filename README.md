# **lv_port_nuvoton**

This repository contains the LVGLv9 demo port for the NuMaker development board from Nuvoton.

## Clone Repositories

Clone the main lv_port_nuvoton repository and the necessary git modules separately. For example, you only need the lvgl and n9h20 git submodules, execute the following command for cloning.

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

## Supported porting on NuMaker Boards

| **NuMaker Board Name** | **BSP Folder** | **LVGL Demo Project Folder** |
| ------------------------- | ------------------------- | -------------------------- |
| **NuMaker-HMI-N9H20** | [n9h20](./bsp/n9h20) | [numaker-hmi-n9h20](./board/numaker-hmi-n9h20) |
| **NuMaker-HMI-N9H26** | [n9h26](./bsp/n9h26) | [numaker-hmi-n9h26](./board/numaker-hmi-n9h26) |
| **NuMaker-HMI-N9H30** | [n9h30](./bsp/n9h30) | [numaker-hmi-n9h30](./board/numaker-hmi-n9h30) |
| **NuMaker-HMI-MA35D1** | [ma35d1](./bsp/ma35d1) | [numaker-hmi-ma35d1](./board/numaker-hmi-ma35d1) |
| **NuMaker-IIoT-NUC980G2D** | [nuc980](./bsp/nuc980) | [numaker-iiot-nuc980g2d](./board/numaker-iiot-nuc980g2d) |
| **NuMaker-HMI-M467** | [m460](./bsp/m460) | [numaker-hmi-m467](./board/numaker-hmi-m467) |
| **NuMaker-HMI-M2354** | [m2354](./bsp/m2354) | [numaker-hmi-m2354](./board/numaker-hmi-m2354) |
| **NuMaker-HMI-M487** | [m480](./bsp/m480) | [numaker-hmi-m487](./board/numaker-hmi-m487) |
| **NuMaker-HMI-M032** | [m031](./bsp/m031) | [numaker-hmi-m032](./board/numaker-hmi-m032) |
| **NuMaker-HMI-M55M1** | [m55m1es](./bsp/m55m1) | [numaker-hmi-m55m1](./board/numaker-hmi-m55m1) |
