# **NuMaker-HMI-N9H30**

Nuvoton offers the HMI platform which is embedded with Nuvoton N9H MPU. The N9H30 series with ARM926EJ-S core can operate at up to 240 MHz. It integrated TFT-LCD display controller, 2D graphics accelerator, H.264 and JPEG codec, supporting resolution up to 1024 x 768 pixels in parallel port, and the color depth up to 16.7 million colors ( 24-bit ) LCD screen output to deliver gorgeous display effects. It also offers built-in H.264 video decoder and voice decoder, which can streamline the peripheral circuits of HMI applications with sound playback. In addition, it embedded up to 64 MB DDR SDRAM, along with ample hardware storage and computing space for excellent design flexibility.

<p align="center">
<img src="https://direct.nuvoton.com/609-large_default/numaker-hmi-n9h30.jpg" alt="fishy" class="bg-primary">
</p>

## **Folder Structure**

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
| N9H30F6_800x480 | Use 800x480 LCD panel |
| N9H30F6_SD_800x480 | Use 800x480 LCD panel and SD storage |

## **GCC project**

User can select listed **Build configurations** to build target execution using Eclipse-base IDE.
| Target | Description |
|-|-|
| N9H30F6_800x480 | Use 800x480 LCD panel |
| N9H30F6_SD_800x480 | Use 800x480 LCD panel and SD storage |

## **Compiling options**

- The porting given CONFIG_LV_DISP_FULL_REFRESH to 0 by default, partial update approach is applied.

  ```bash
   #define CONFIG_LV_DISP_FULL_REFRESH      0
  ```

- The porting given LV_USE_DEMO_WIDGETS to 1 by default, LVGL Widgets will executed at startup.

  ```bash
  #define LV_USE_DEMO_WIDGETS             1
  //#define LV_USE_DEMO_MUSIC               1

  #if LV_USE_DEMO_MUSIC
    #define LV_COLOR_SCREEN_TRANSP        1
    #define LV_DEMO_MUSIC_AUTO_PLAY       1
  #endif
  ```

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/en/numaker-hmi-n9h30)

## **Resources**

[TurboWriter](../../bsp/n9h30/Tools/PC_Tools)
