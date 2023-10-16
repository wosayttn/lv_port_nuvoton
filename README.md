# **lv_port_nuvoton**

This reposistory includes lvgl porting for Nuvoton NuMaker boards.

## Clone Repositories

Clone major lv_port_nuvoton and necessary submodule, respectively. For example, only clone lvgl and n9h20 BSP repositories as following command.

```bash
git clone https://github.com/OpenNuvoton/lv_port_nuvoton
git submodule update --progress --init -- "lvgl"
git submodule update --progress --init -- "bsp/n9h20"
```

or, clone lv_port_nuvoton with recurse-sumbmodules option for clone-all repositories directly.

```bash
git clone --recurse-submodules https://github.com/OpenNuvoton/lv_port_nuvoton
```

## Supported porting on NuMaker Boards

| **Board Name** | **CPU Arch.** | **Project Folder** |
| ------------------------- | ------------------------- | -------------------------- |
| **NuMaker-HMI-N9H20** | ARM926EJS | [numaker-hmi-n9h20](./board/numaker-hmi-n9h20) |
| **NuMaker-HMI-N9H26** | ARM926EJS | [numaker-hmi-n9h26](./board/numaker-hmi-n9h26) |
| **NuMaker-HMI-N9H30** | ARM926EJS | [numaker-hmi-n9h30](./board/numaker-hmi-n9h30) |
| **NuMaker-HMI-MA35D1** | Cortex-A35 | [numaker-hmi-ma35d1](./board/numaker-hmi-ma35d1) |
| **NuMaker-IIoT-NUC980G2D** | ARM926EJS | **Coming soon** |
| **NuMaker-HMI-M467** | Cortex-M4 | **Coming soon** |
| **NuMaker-HMI-M2354** | Cortex-M23 | **Coming soon** |
| **NuMaker-HMI-M487** | Cortex-M4 | **Coming soon** |
| **NuMaker-HMI-M032** | Cortex-M0 | **Coming soon** |
