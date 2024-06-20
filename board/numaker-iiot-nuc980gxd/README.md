# **NuMaker-IIoT-NUC980G1D / NuMaker-IIoT-NUC980G2D**

| Major Folder | Description |
|-|-|
| GCC | GCC project file |
| KEIL | MDK4 project file |

## **KEIL project**

User can select listed **Target Name** to build target execution using uVision MDK4.
| Target | Description |
|-|-|
| G1D_NUC980DK_320x240 | Use ILI9431 SPI LCD panel with RTP(H/W ADC Sampling), for G1D board |
| G2D_NUC980DK_320x240 | Use ILI9431 SPI LCD panel with RTP(H/W ADC Sampling), for G2D board |

## **GCC project**

To build target execution using Eclipse-base IDE(NuEclipse V1.02.023r).
| Target | Description |
|-|-|
| G1D_NUC980DK_320x240 | Use ILI9431 SPI LCD panel with RTP(H/W ADC Sampling), for G1D board |
| G2D_NUC980DK_320x240 | Use ILI9431 SPI LCD panel with RTP(H/W ADC Sampling), for G2D board |

## **Compiling options**

- The porting given CONFIG_DISP_LINE_BUFFER_NUMBER to LV_VER_RES_MAX by default. If you need save the memory size, you can re-define it.

  ```c
  #define CONFIG_DISP_LINE_BUFFER_NUMBER   LV_VER_RES_MAX
  ```

- The porting given LV_USE_DEMO_WIDGETS to 1 by default, LVGL Widgets will executed at startup.

  ```c
  #define LV_USE_DEMO_WIDGETS             1
  //#define LV_USE_DEMO_MUSIC               1
  ```

- The partial update approach is applied in this port.

## **SPI NAND flash using NuWriter**

You can use NuWriter to program lvgl_demo.bin into SPI NAND flash.
<br>
Choose type: SPINAND<br>
<< Press Re-Connect >><br>

<< Press Erase >><br>
<< Select Erase All >><br>
<< Press OK >><br>

Choose file: Specify your uboot-spl.bin file.<br>
Image Type: Loader<br>
Execute Address: 0x200<br>
<< Press Program >><br>

Choose file: Specify your uboot.bin file.<br>
Image Type: Data<br>
Image start address: 0x100000<br>
<< Press Program >><br>

Choose file: Specify your lvgl_demo.bin file.<br>
Image Type: Data<br>
Image start address: 0x200000<br>
<< Press Program >><br>

Choose file: Specify your env.txt file.<br>
Image Type: Environment<br>
Image start address: 0x80000<br>
<< Press Program >><br>

<< Press OK & Wait it down >><br>
<< Set Power-on setting to SPI NAND booting >><br>
<< Press Reset button on board >><br>
Enjoy!! <br>
<br>

## **Purchase**

[Nuvoton Direct - NuMaker-IIoT-NUC980G1D](https://direct.nuvoton.com/tw/numaker-iiot-nuc980g1d)
[Nuvoton Direct - NuMaker-IIoT-NUC980G2D](https://direct.nuvoton.com/tw/numaker-iiot-nuc980g2d)

## **Resources**

[NuWriter](https://github.com/OpenNuvoton/NUC980_NuWriter)
