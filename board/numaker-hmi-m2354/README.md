# **NuMaker-HMI-M2354**

## **Folder Structure**

| Folder Name | Description |
|-|-|
| KEIL | MDK5 project file |
| lv_port | lvgl porting files |
| sls_files | User UI files SLS exported |
| lv_conf.h | Major lvgl configuration file |
| main.c | main routine |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK5.
| Target | Description |
|-|-|
| M2354KJ_320x240 | Use ILI9431 SPI LCD panel with resistive touch pad |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to (LV_VER_RES_MAX/4) by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   (LV_VER_RES_MAX/4)
  ```

- The partial update approach is applied in this port.

## **Notice**
- Initial fatfs and access SPI-NOR flash device on NuTFT board.
  - Due to `PA0, PA1, PA2, PA3` pins(SPI0) is in 1.8v power-domain by default. You should switch to 3.3v domain to access the W25X16 SPI NOR flash of Nu-TFT board.
  - Remove `R15` resistor and connect `VDDIO` pin to 3.3v.

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/tw/numaker-hmi-m2354)

## **Resources**
