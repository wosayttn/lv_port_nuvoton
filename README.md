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
| **NuMaker-HMI-N9H30** | [n9h30](./bsp/n9h30) | [numaker-hmi-n9h30](./board/numaker-hmi-n9h30) |
