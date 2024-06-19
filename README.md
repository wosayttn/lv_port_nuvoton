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

| **NuMaker Board Name** | **LVGL Demo Project Folder** |
| ------------------------- | ------------------------- |
| **NuMaker-HMI-N9H20** | [numaker-hmi-n9h20](./board/numaker-hmi-n9h20) |
| **NuMaker-HMI-N9H26** | [numaker-hmi-n9h26](./board/numaker-hmi-n9h26) |
| **NuMaker-HMI-N9H30** | [numaker-hmi-n9h30](./board/numaker-hmi-n9h30) |
| **NuMaker-HMI-N9H31** | [numaker-hmi-n9h31](./board/numaker-hmi-n9h31) |
| **NuMaker-HMI-MA35D1** | [numaker-hmi-ma35d1](./board/numaker-hmi-ma35d1) |
| **NuMaker-HMI-MA35H0** | [numaker-hmi-ma35h0](./board/numaker-hmi-ma35h0) |
| **NuMaker-IIoT-NUC980G1D** | [numaker-iiot-nuc980g1d](./board/numaker-iiot-nuc980g1d) |
| **NuMaker-IIoT-NUC980G2D** | [numaker-iiot-nuc980g2d](./board/numaker-iiot-nuc980g2d) |
| **NuMaker-HMI-M467** | [numaker-hmi-m467](./board/numaker-hmi-m467) |
| **NuMaker-HMI-M2354** | [numaker-hmi-m2354](./board/numaker-hmi-m2354) |
| **NuMaker-IoT-M2354** | [numaker-iot-m2354](./board/numaker-iot-m2354) |
| **NuMaker-HMI-M487** | [numaker-hmi-m487](./board/numaker-hmi-m487) |
| **NuMaker-HMI-M032** | [numaker-hmi-m032](./board/numaker-hmi-m032) |
| **NuMaker-HMI-M55M1** | [numaker-hmi-m55m1](./board/numaker-hmi-m55m1) |
