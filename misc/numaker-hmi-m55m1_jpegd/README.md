# **NuMaker-HMI-M55M1_JPEGD**

| Major Folder | Description |
|-|-|
| KEIL | MDK5 project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK5.

| Target | Description |
|-|-|
| JPG_SLIDESHOW | Play all jpeg files in SD card. (Use LT7381 EBI LCD panel with capacitive touch screen(FT5316)) |
| MJPG_PLAYBACK | Play movie.avi file in SD card. (Use LT7381 EBI LCD panel with capacitive touch screen(FT5316)) |
| JPG_SLIDESHOW_480X272 | Play all jpeg files in SD card. (Use NV3041A EBI LCD panel with capacitive touch screen(GT911)) |
| MJPG_PLAYBACK_480X272 | Play movie.avi file in SD card. (Use NV3041A EBI LCD panel with capacitive touch screen(GT911)) |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to LV_VER_RES_MAX by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   LV_VER_RES_MAX
  ```

- The partial update approach is applied in this port.

## **Purchase**

## **Resources**

