# **NuMaker-HMI-M467**

| Major Folder | Description |
|-|-|
| KEIL | MDK5 project file |
| GCC | GCC project file |
| VSCode | VSCode CSolution project file |
| IAR | IAR project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK5.

| Target | Description |
|-|-|
| M467HJ_USE_NUMAKER_TFT_LCD43_V_1_0 | Use FSA506 EBI LCD panel with capacitive touch screen(ST1663I) |
| M467HJ_USE_NUMAKER_TFT_LCD43_V_1_1 | Use NV3041A EBI LCD panel with capacitive touch screen(GT911) |
| M467HJ_USE_NUTFT | Use ILI9431 SPI LCD panel with resistive touch screen(S/W ADC Sampling) |

## **GCC project**

To build target execution using Eclipse-base IDE(NuEclipse V1.02.023r).

| Configuration | Description |
|-|-|
| M467HJ_USE_NUMAKER_TFT_LCD43_V_1_0 | Use FSA506 EBI LCD panel with capacitive touch screen(ST1663I) |
| M467HJ_USE_NUMAKER_TFT_LCD43_V_1_1 | Use NV3041A EBI LCD panel with capacitive touch screen(GT911) |
| M467HJ_USE_NUTFT | Use ILI9431 SPI LCD panel with resistive touch screen(S/W ADC Sampling) |

## **VSCode project**

To build target execution using VSCode IDE or run the build.ps1 scripts.

| Context | Description |
|-|-|
| +M467HJ_USE_NUMAKER_TFT_LCD43_V_1_0 | Use FSA506 EBI LCD panel with capacitive touch screen(ST1663I) |
| +M467HJ_USE_NUMAKER_TFT_LCD43_V_1_1 | Use NV3041A EBI LCD panel with capacitive touch screen(GT911) |
| +M467HJ_USE_NUTFT | Use ILI9431 SPI LCD panel with resistive touch screen(S/W ADC Sampling) |

## **IAR project**

To build target execution using IAR IDE(IAR 9.40).

| Target | Description |
|-|-|
| Release | Use FSA506 EBI LCD panel with capacitive touch screen(ST1663I) |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to LV_VER_RES_MAX by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   LV_VER_RES_MAX
  ```

- The partial update approach is applied in this port.

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/tw/numaker-hmi-m467)

## **Resources**
