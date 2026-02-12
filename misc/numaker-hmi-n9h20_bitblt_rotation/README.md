# **NuMaker-HMI-N9H20**

| Major Folder | Description |
|-|-|
| KEIL | MDK4 project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK4.
| Target | Description |
|-|-|
| N9H20K5_NAND_480x272_FreeRTOS | Use 480x272 LCD panel with resistive touch screen(H/W ADC Sampling) |

## **Compiling options**

- The porting given CONFIG_LV_DISP_FULL_REFRESH to 0 by default, partial update approach is applied.

  ```c
   #define CONFIG_LV_DISP_FULL_REFRESH      0
  ```

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/en/numaker-emwin-n9h20)

## **Resources**

[TurboWriter](../../bsp/n9h20/Tools/PC_Tools)
