# **NuMaker-HMI-M55M1-SLS-Examples**

| Major Folder | Description |
|-|-|
| KEIL | MDK5 project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK5.
Use LT7381 EBI LCD panel with capacitive touch screen(FT5316)

| Target | Description |
|-|-|
| [M55M1_Caffee_Machine](https://youtu.be/q52U7EFQdIg)  | SLSv1.6 Caffee Machine Example |
| [M55M1_3D_Printer](https://youtu.be/_j92tXy30pQ)      | SLSv1.6 3D Printer Example |
| [M55M1_EBike](https://youtu.be/b2ea5MImvhU)           | SLSv1.6 E-Bike Example |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to (LV_VER_RES_MAX/4) by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   (LV_VER_RES_MAX/4)
  ```

- The partial update approach is applied in this port.

## **Purchase**

## **Resources**
