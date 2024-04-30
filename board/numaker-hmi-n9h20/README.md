# **NuMaker-HMI-N9H20**

## **Folder Structure**

| Folder Name | Description |
|-|-|
| KEIL | MDK5 project file |
| lv_port | lvgl porting files |
| sls_files | User UI files SLS exported |
| lv_conf.h | Major lvgl configuration file |
| main.c | main routine |
| standalone.c | minimal C library bootstrap for MDK4 |
| wb_init.s | start-up routine for MDK4 |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK4.
| Target | Description |
|-|-|
| N9H20K5_NAND_480x272_FreeRTOS | Use 480x272 LCD panel, NAND storage and run on FreeRTOS environment |
| N9H20K5_NAND_800x480_FreeRTOS | Use 800x480 LCD panel, NAND storage and run on FreeRTOS environment |

## **Compiling options**

- The porting given CONFIG_LV_DISP_FULL_REFRESH to 0 by default, partial update approach is applied.

  ```c
   #define CONFIG_LV_DISP_FULL_REFRESH      0
  ```

- The porting given LV_USE_DEMO_WIDGETS to 1 by default, LVGL Widgets will executed at startup.

  ```c
  #define LV_USE_DEMO_WIDGETS             1
  //#define LV_USE_DEMO_MUSIC               1

  #if LV_USE_DEMO_MUSIC
    #define LV_COLOR_SCREEN_TRANSP        1
    #define LV_DEMO_MUSIC_AUTO_PLAY       1
  #endif
  ```

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/en/numaker-emwin-n9h20)

## **Resources**

[TurboWriter](../../bsp/n9h20/Tools/PC_Tools)
