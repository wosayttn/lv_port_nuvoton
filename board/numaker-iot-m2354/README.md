# **NuMaker-IoT-M2354**

| Major Folder | Description |
|-|-|
| GCC | GCC project file |
| KEIL | MDK5 project file |
| IAR | IAR project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK5.
| Target | Description |
|-|-|
| M2354KJ_320x240 | Use ILI9431 SPI LCD panel with resistive touch screen(S/W ADC Sampling) |

## **GCC project**

To build target execution using Eclipse-base IDE(NuEclipse V1.02.023r).
| Target | Description |
|-|-|
| M2354KJ_320x240 | Use ILI9431 SPI LCD panel with resistive touch screen(S/W ADC Sampling) |

## **IAR project**

To build target execution using IAR IDE(IAR 9.40).
| Target | Description |
|-|-|
| Release | Use ILI9431 SPI LCD panel with resistive touch screen(S/W ADC Sampling) |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to (LV_VER_RES_MAX/2) by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   (LV_VER_RES_MAX/2)
  ```

- The partial update approach is applied in this port.


## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/tw/numaker-iot-m2354u)

## **Resources**
