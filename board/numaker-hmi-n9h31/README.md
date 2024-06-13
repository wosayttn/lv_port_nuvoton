# **NuMaker-HMI-N9H31**

| Major Folder | Description |
|-|-|
| GCC | GCC project file |
| KEIL | MDK4 project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK4.
| Target | Description |
|-|-|
| N9H31K5_800x480 | Use 800x480 LCD panel with resistive touch screen(H/W ADC Sampling) |

## **GCC project**

To build target execution using Eclipse-base IDE(NuEclipse V1.02.023r).
| Target | Description |
|-|-|
| N9H31K5_800x480 | Use 800x480 LCD panel with resistive touch screen(H/W ADC Sampling) |

## **Compiling options**

- The porting given CONFIG_LV_DISP_FULL_REFRESH to 0 by default, partial update approach is applied.

```c
  #define CONFIG_LV_DISP_FULL_REFRESH      0
```

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/en/numaker-hmi-n9h31)

## **Resources**

[NuWriter](https://github.com/OpenNuvoton/NUC970_NuWriter)
