# **NuMaker-HMI-M55M1_JPEGD**

| Major Folder | Description |
|-|-|
| KEIL | MDK5 project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK5.

| Target | Description |
|-|-|
| JPG_SLIDESHOW | Play all jpeg files in SD card. |
| MJPG_PLAYBACK | Play movie.avi file in SD card. |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to LV_VER_RES_MAX by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   LV_VER_RES_MAX
  ```

- The partial update approach is applied in this port.

## **Purchase**

[Nuvoton Direct]()

## **Resources**

