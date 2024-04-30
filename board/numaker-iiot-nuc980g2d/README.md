# **NuMaker-IIoT-NUC980G2D**

## **Folder Structure**

| Folder Name | Description |
|-|-|
| KEIL | MDK4 project file |
| lv_port | lvgl porting files |
| sls_files | User UI files SLS exported |
| lv_conf.h | Major lvgl configuration file |
| main.c | main routine |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK4.
| Target | Description |
|-|-|
| NUC980DK_320x240 | Use ILI9431 SPI LCD panel with resistive touch screen |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to LV_VER_RES_MAX by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   LV_VER_RES_MAX
  ```

- The porting given LV_USE_DEMO_WIDGETS to 1 by default, LVGL Widgets will executed at startup.

  ```c
  #define LV_USE_DEMO_WIDGETS             1
  //#define LV_USE_DEMO_MUSIC               1
  ```

- The partial update approach is applied in this port.

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/tw/numaker-iiot-nuc980g2d)

## **Resources**

[NuWriter](https://github.com/OpenNuvoton/NUC980_NuWriter)
