# **NuMaker-HMI-M032**

| Major Folder | Description |
|-|-|
| KEIL | MDK5 project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK5.
| Target | Description |
|-|-|
| M032KI_320x240 | Use ILI9431 SPI LCD panel with resistive touch screen(S/W ADC Sampling) |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to 12 by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   12
  ```

- The partial update approach is applied in this port.

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/tw/numaker-hmi-m032)

## **Resources**
