# **NuMaker-HMI-MA35H0**

| Major Folder | Description |
|-|-|
| GCC | GCC project file |

## **GCC project**

To build target execution using Eclipse-base IDE(NuEclipse V1.02.023r).
| Target | Description |
|-|-|
| MA35H0_1024x600 | Use 1024x600 LCD panel with resistive touch screen(H/W ADC Sampling) |
| MA35H0_1024x600_GT911_CTP | Use 1024x600 LCD panel with captive touch screen(1024x600 GT911) |

## **Compiling options**

- The porting sets `CONFIG_DISP_DIRECT_REFRESH` to 1 by default (Direct refresh approach). Set to 0 to use Partial refresh approach.

  ```c
  #define CONFIG_DISP_DIRECT_REFRESH       1
  ```

- The porting given LV_USE_DEMO_WIDGETS to 1 by default, LVGL Widgets will executed at startup.

  ```c
  #define LV_USE_DEMO_WIDGETS             1
  //#define LV_USE_DEMO_BENCHMARK           1
  ```

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/tw/numaker-hmi-ma35h0-a1)

## **Resources**

[NuWriter](https://github.com/OpenNuvoton/MA35H0_NuWriter)
