#!/bin/bash

set -x

gzip -f -9 -c ../KEIL/NAND_Data_FreeRTOS/lvgl_demo_nand_freertos.bin > lvgl_demo_nand_freertos.bin.gz

python ./mkimage.py -n 'N9H20' -A arm -O linux -O linux -T standalone -C gzip -a 0 -e 0 -d lvgl_demo_nand_freertos.bin.gz conprog.gz

rm lvgl_demo_nand_freertos.bin.gz

ls -al