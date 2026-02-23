# **NuMaker-HMI-M55M1-SLS-Examples**

| Major Folder | Description |
|-|-|
| KEIL | MDK5 project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK5.

| Target | Description |
|-|-|
| [M55M1_Caffee_Machine](https://youtu.be/q52U7EFQdIg)    | SLSv1.6 Caffee Machine Example, LT7381 EBI LCD panel with FT5316 TPC board. |
| [M55M1_3D_Printer](https://youtu.be/_j92tXy30pQ)        | SLSv1.6 3D Printer Example, LT7381 EBI LCD panel with FT5316 TPC board. |
| [M55M1_EBike](https://youtu.be/b2ea5MImvhU)             | SLSv1.6 E-Bike Example, LT7381 EBI LCD panel with FT5316 TPC board. |
| [M55M1_EBike_LCD43_V1_1](https://youtu.be/EKddwPokQMg)  | SLSv1.6 E-Bike 480x272 Example, NV3041A LCD panel with GT911 TPC board. |
| [M55M1_EBike_LCD43_V1_0]()                              | SLSv1.6 E-Bike 480x272 Example, FSA506 LCD panel with ST1663I TPC board. |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to (LV_VER_RES_MAX/2) by default. If you need save the memory size, you can re-define it.

  ```c
  /*Define off-screen line buffer number,  Range: 10~LV_VER_RES_MAX*/

  # if defined(**480x272**)
      #define CONFIG_DISP_LINE_BUFFER_NUMBER  (LV_VER_RES_MAX)
  # else
      #define CONFIG_DISP_LINE_BUFFER_NUMBER  (LV_VER_RES_MAX/2)
  # endif
  ```

- The partial update approach is applied in this port.

## **Purchase**

## **Resources**

[Port SLS_Example Steps](./Port_SLS_Example.md)
