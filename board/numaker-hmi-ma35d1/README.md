# **NuMaker-HMI-MA35D1**

## **Folder Structure**

| Folder Name | Description |
|-|-|
| GCC | GCC project file |
| lv_port | lvgl porting files |
| sls_files | User UI files SLS exported |
| nuwriter_scripts | User UI files SLS exported |
| lv_conf.h | Major lvgl configuration file |
| main.c | main routine |

## **GCC project**

User can select listed **Build configurations** to build target execution using Eclipse-base IDE.
| Target | Description |
|-|-|
| MA35D1_1024x600 | Use 1024x600 LCD panel |

## **Compiling options**

- The porting given CONFIG_LV_DISP_FULL_REFRESH to 1 by default.

  ```c
   #define CONFIG_LV_DISP_FULL_REFRESH      1
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

[Nuvoton Direct](https://direct.nuvoton.com/en/numaker-hmi-ma35d1-s1)

## **Resources**

[NuWriter](https://github.com/OpenNuvoton/MA35D1_NuWriter)
