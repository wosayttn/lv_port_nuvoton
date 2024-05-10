# **NuMaker-HMI-MA35H0**

## **Folder Structure**

| Folder Name | Description |
|-|-|
| GCC | GCC project file |
| lv_port | lvgl porting files |
| sls_files | User UI files SLS exported |
| [nuwriter_scripts](./nuwriter_scripts) | User UI files SLS exported |
| lv_conf.h | Major lvgl configuration file |
| main.c | main routine |

## **GCC project**

User can select listed **Build configurations** to build target execution using Eclipse-base IDE.
| Target | Description |
|-|-|
| MA35H0_1024x600 | Use 1024x600 LCD panel |

## **Compiling options**

- The porting given CONFIG_LV_DISP_FULL_REFRESH to 0 by default, partial update approach is applied.

  ```c
   #define CONFIG_LV_DISP_FULL_REFRESH      0
  ```

- The porting given LV_USE_DEMO_WIDGETS to 1 by default, LVGL Widgets will executed at startup.

  ```c
  #define LV_USE_DEMO_WIDGETS             1
  //#define LV_USE_DEMO_MUSIC               1
  ```

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/tw/numaker-hmi-ma35h0-a1)

## **Resources**

[NuWriter](https://github.com/OpenNuvoton/MA35H0_NuWriter)
