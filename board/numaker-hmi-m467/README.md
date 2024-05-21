# **NuMaker-HMI-M467**

| Major Folder | Description |
|-|-|
| GCC | GCC project file |
| KEIL | MDK5 project file |
| IAR | IAR project file |
| VSCode | VSCode CSolution project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK5.
| Target | Description |
|-|-|
| M467HJ_480x272_FSA506 | Use FSA506 EBI LCD panel with capacitive touch screen(ST1663I) |
| M467HJ_480x272_SSD1963 | Use SSD1963 EBI LCD panel |

## **GCC project**

To build target execution using Eclipse-base IDE(NuEclipse V1.02.023r).
| Target | Description |
|-|-|
| M467HJ_480x272 | Use FSA506 EBI LCD panel with capacitive touch screen(ST1663I) |

## **IAR project**

To build target execution using IAR IDE(IAR 9.40).
| Target | Description |
|-|-|
| Release | Use FSA506 EBI LCD panel with capacitive touch screen(ST1663I) |

## **VSCode project**

To build target execution using VSCode IDE or run the build.ps1 scripts.
| Target | Description |
|-|-|
| M467HJ480x272FSA506 | Use FSA506 EBI LCD panel with capacitive touch screen(ST1663I) |
| M467HJ480x272SSD1963 | Use SSD1963 EBI LCD panel |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to LV_VER_RES_MAX by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   LV_VER_RES_MAX
  ```

- The partial update approach is applied in this port.

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/tw/numaker-hmi-m467)

## **Resources**
