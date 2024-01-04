# **tools**

## **pack_gzip.sh**

You can execute pack_gzip.sh script in git-bash shell to pack an u-boot header and compressed execution binary for spiloader_gzip loader.

```bash
$ ./pack_gzip.sh
+ gzip -f -9 -c ../KEIL/NAND_Data_FreeRTOS/lvgl_demo_nand_freertos.bin
+ python ./mkimage.py -n N9H20 -A arm -O linux -O linux -T standalone -C gzip -a 0 -e 0 -d lvgl_demo_nand_freertos.bin.gz conprog.gz
+ rm lvgl_demo_nand_freertos.bin.gz
+ ls -al
total 385
drwxr-xr-x 1 Wayne 197121      0 Dec 25 17:08 .
drwxr-xr-x 1 Wayne 197121      0 Dec 25 16:25 ..
-rw-r--r-- 1 Wayne 197121  42688 Dec 25 16:09 N9H20K5_SpiLoader_gzip_NoLCM_1225.bin
-rw-r--r-- 1 Wayne 197121    211 Dec 25 17:06 README.md
-rw-r--r-- 1 Wayne 197121 325525 Dec 25 17:08 conprog.gz
-rw-r--r-- 1 Wayne 197121   4625 Dec 20 11:43 mkimage.py
-rwxr-xr-x 1 Wayne 197121    309 Dec 25 17:08 pack_gzip.sh
```
