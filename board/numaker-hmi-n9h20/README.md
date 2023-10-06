# **NuMaker-HMI-N9H20**

## Folder Structure

| Folder Name | Description |
|-|-|
| GCC | GCC project file |
| KEIL | MDK4 project file |
| lv_port | lvgl porting files |
| lv_conf.h | Major lvgl configuration file |
| main.c | main routine |
| standalone.c | minimal C library bootstrap for MDK4 |
| wb_init.s | start-up routine for MDK4 |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK4.
| Target | Description |
|-|-|
| N9H20K5_NAND_480x272 | Use 480x272 LCD panel and NAND  storage |
| N9H20K5_NAND_800x480 | Use 800x480 LCD panel and NAND  storage |
| N9H20K5_SD_480x272 | Use 480x272 LCD panel and SD storage |
| N9H20K5_SD_800x480 | Use 800x480 LCD panel and SD storage |
| N9H20K5_NAND_480x272_FreeRTOS | Use 480x272 LCD panel, NAND storage and run on FreeRTOS environment |

## **GCC project**

User can select listed **Build configurations** to build target execution using Eclipse-base IDE.
| Target | Description |
|-|-|
| N9H20K5_480x272 | Use 480x272 LCD panel and NAND  storage |
| N9H20K5_SD_480x272 | Use 480x272 LCD panel and SD storage |

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
