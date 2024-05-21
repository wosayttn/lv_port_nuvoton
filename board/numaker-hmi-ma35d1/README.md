# **NuMaker-HMI-MA35D1**

| Major Folder | Description |
|-|-|
| GCC | GCC project file |

## **GCC project**

To build target execution using Eclipse-base IDE(NuEclipse V1.02.023r).
| Target | Description |
|-|-|
| MA35D1_1024x600 | Use 1024x600 LCD panel with resistive touch screen(H/W ADC Sampling) |

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

[Nuvoton Direct](https://direct.nuvoton.com/en/numaker-hmi-ma35d1-s1)

## **Resources**

[NuWriter](https://github.com/OpenNuvoton/MA35D1_NuWriter)
