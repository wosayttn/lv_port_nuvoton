# **nuwriter_scripts**

Some scripts can help you program firmware into storages in this folder.

## **Requirement**

### [Python3 for Window](https://www.python.org/downloads/windows/)

After installing Python3, some plug-in as below must be installed.

```bash
pip3 install pyusb usb crypto ecdsa crcmod tqdm pycryptodome
```

Notice:

- Tested python3 version is 3.10.1.
- Please remember to add Python to PATH environment variable.

### [Libusb for Window](http://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.20/libusb-1.0.20.7z/download)

On Windows platform, it is required to install libusb manually.

```bash
Step 1: Download the library extract the download file.
Step 2: Copy MS64\dll\libusb-1.0.dll to C:\Windows\System32.
Step 3: Copy MS64\dll\libusb-1.0.lib to C:\Users\<user name>\AppData\Local\Programs\Python\<python ver>\Lib.
```

### [USB Driver for Window](https://github.com/OpenNuvoton/MA35D1_NuWriter/blob/master/driver/WinUSB4NuVCOM.exe?raw=true)

NuWriter must install **WinUSB4NuVCOM.exe** on the computer.

## **Double-click Scripts for Windows**

If your NuWriter_MA35 python running is ready, you can do following batch script files for Window directly.

### **nuwriter_ddr_download_and_run.bat**

Download GCC\MA35D1_1024x600\lvgl_ma35d1.bin binary file into DDR. The address is 0x80400000.

### **nuwriter_program_sd.bat**

Program header0, DDR timing setting and GCC\MA35D1_1024x600\lvgl_ma35d1.bin binary file into SD card or EMMC.

### **nuwriter_program_spinand.bat**

Program header0, DDR timing setting and GCC\MA35D1_1024x600\lvgl_ma35d1.bin binary file into SPI NAND flash.

### **nuwriter_program_rawnand.bat**

Program header0, DDR timing setting and GCC\MA35D1_1024x600\lvgl_ma35d1.bin binary file into NAND flash.

### **nuwriter_program_sd_pack1.bat**

Program header1, DDR timing setting and GCC\MA35D1_1024x600\lvgl_ma35d1.bin binary file into SD card or EMMC.

### **nuwriter_program_spinand_pack1.bat**

Program header1, DDR timing setting and GCC\MA35D1_1024x600\lvgl_ma35d1.bin binary file into SPI NAND flash.

### **nuwriter_program_rawnand_pack1.bat**

Program header1, DDR timing setting and GCC\MA35D1_1024x600\lvgl_ma35d1.bin binary file into NAND flash.

### **nuwriter_program_spinor_pack1.bat**

Program header1, DDR timing setting and GCC\MA35D1_1024x600\lvgl_ma35d1.bin binary file into SPI NOR flash.

## **Program firmware using NuWriter**

To switch SW4 dip-switch on NuMaker-BASE-MA35D1B1 to do corresponding actions as below.

**Power-on Setting**
L: OFF dip-switch
H: ON dip-switch

| Memory Storage | **Burn to** settings | **Boot from** settings |
|--|--|--|
| **DDR** | <ul><li>Switch 1(PG0) to ON.</li><li>Switch 3(PG2) to ON.</li><li>Switch 4(PG3) to ON.</li><li>Switch Others to OFF.</li></ul> |  |
| **EMMC**<br>(Select eMMC1 device and 8-bit mode) | <ul><li>Switch 1(PG0) to ON.</li><li>Switch 3(PG2) to ON.</li><li>Switch 4(PG3) to ON.</li><li>Switch 7(PG6) to ON.</li><li>Switch 8(PG7) to ON.</li><li>Switch Others to OFF.</li></ul> | <ul><li>Switch 1(PG0) to ON.</li><li>Switch 3(PG2) to ON.</li><li>Switch 7(PG6) to ON.</li><li>Switch 8(PG7) to ON.</li><li>Switch Others to OFF.</li></ul> |
| **Raw NAND**<br>(Select Ignore BCH and Page setting) | <ul><li>Switch 1(PG0) to ON.</li><li>Switch 3(PG2) to ON.</li><li>Switch 4(PG3) to ON.</li><li>Switch Others to OFF.</li></ul> | <ul><li>Switch 1(PG0) to ON.</li><li>Switch 4(PG3) to ON.</li><li>Switch Others to OFF.</li></ul> |
| **Serial NAND**<br>(Select 4-bit mode) | <ul><li>Switch 1(PG0) to ON.</li><li>Switch 3(PG2) to ON.</li><li>Switch 4(PG3) to ON.</li><li>Switch 7(PG6) to ON.</li><li>Switch Others to OFF.</li></ul> | <ul><li>Switch 1(PG0) to ON.</li><li>Switch 7(PG6) to ON.</li><li>Switch Others to OFF.</li></ul> |
| **Serial NOR**<br>(Select 4-bit mode) | <ul><li>Switch 1(PG0) to ON.</li><li>Switch 3(PG2) to ON.</li><li>Switch 4(PG3) to ON.</li><li>Switch 7(PG6) to ON.</li><li>Switch 8(PG7) to ON.</li><li>Switch Others to OFF.</li></ul> | <ul><li>Switch 1(PG0) to ON.</li><li>Switch 7(PG6) to ON.</li><li>Switch 8(PG7) to ON.</li><li>Switch Others to OFF.</li></ul> |

## **Troubleshoot**

### **Fail to create symbolic folder**

```bash
Create symbolic folder conv failed
```

- To switch **Developer Mode** in Window. To enter [Settings], [Update & Security], [For developers], [Developer Mode] page, then set it **On**
- Use **Administrator** permission to install python3, libusb and utilities.

### **Use NuWriter_MA35.exe**

Due to the python execution file size and saving network bandwidth, we just only release the python code in this repository. We also provide [NuWriter_MA35.exe](https://github.com/OpenNuvoton/MA35D1_NuWriter/blob/master/EXE/NuWriter_MA35.exe?raw=true) python execution. You can run the packaged app without installing a Python interpreter or any modules. You can use **NuWriter_MA35.exe** and do some modification.

```bash
To modify "py -3 nuwriter.py" to "NuWriter_MA35.exe"
```

For example, the **nuwriter_ddr_download_and_run.bat** modification is as following:

```bash
cd MA35D1_NuWriter

:forever_develop
NuWriter_MA35.exe -a ddrimg\enc_ddr3_winbond_256mb.bin
IF %ERRORLEVEL% EQU 0 (
   NuWriter_MA35.exe -o execute -w ddr 0x80400000 ../../GCC/MA35D1_1024x600/lvgl_ma35d1.bin
)
pause
goto :forever_develop
```
