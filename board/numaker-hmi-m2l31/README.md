# **NuMaker-HMI-M2L31**

| Major Folder | Description |
|-|-|
| KEIL | MDK5 project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK5.
| Target | Description |
|-|-|
| M2354KJ_320x240 | Use ILI9431 SPI LCD panel with resistive touch screen(S/W ADC Sampling) |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to (LV_VER_RES_MAX/4) by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   (LV_VER_RES_MAX/4)
  ```

- The partial update approach is applied in this port.

## **Purchase**

## **Resources**
