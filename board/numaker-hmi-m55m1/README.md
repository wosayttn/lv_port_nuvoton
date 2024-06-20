# **NuMaker-HMI-M55M1**

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
| M55M1_800x480 | Use LT7381 EBI LCD panel with capacitive touch screen(FT5316) |
| M55M1_480x272 | Use FSA506 EBI LCD panel with capacitive touch screen(ST1663I) |

## **GCC project**

To build target execution using Eclipse-base IDE(NuEclipse V1.02.023r).
| Target | Description |
|-|-|
| M55M1_800x480 | Use LT7381 EBI LCD panel with capacitive touch screen(FT5316) |

## **IAR project**

To build target execution using IAR IDE(IAR 9.40).
| Target | Description |
|-|-|
| Release | Use LT7381 EBI LCD panel with capacitive touch screen(FT5316) |

## **VSCode project**

To build target execution using VSCode IDE or run the build.ps1 scripts.
| Target | Description |
|-|-|
| M55M1800x480 | Use LT7381 EBI LCD panel with capacitive touch screen(FT5316) |
| M55M1480x272 | Use FSA506 EBI LCD panel with capacitive touch screen(ST1663I) |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to LV_VER_RES_MAX by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   LV_VER_RES_MAX
  ```

- The partial update approach is applied in this port.

## **Purchase**

[Nuvoton Direct]()

## **Resources**
