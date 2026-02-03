# **NuMaker-HMI-N9H30-SLS-Examples**

| Major Folder | Description |
|-|-|
| KEIL | MDK4 project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK4.
Use 800x480 LCD panel with resistive touch screen(H/W ADC Sampling)

| Target | Description |
|-|-|
| N9H30_3D_Printer      | SLS 3D Printer Example |
| N9H30_EBike           | SLS E-Bike Example |

## **Compiling options**

- The porting given CONFIG_LV_DISP_FULL_REFRESH to 0 by default, partial update approach is applied.

  ```c
  #define CONFIG_LV_DISP_FULL_REFRESH      0
  ```

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/en/numaker-hmi-n9h30)

## **Resources**

[NuWriter](https://github.com/OpenNuvoton/NUC970_NuWriter)
