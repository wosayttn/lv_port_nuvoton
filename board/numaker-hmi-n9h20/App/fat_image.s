        AREA    |.rodata|, DATA, READONLY, ALIGN=4
        EXPORT  incbin_fatimg_start
        EXPORT  incbin_fatimg_end

incbin_fatimg_start
        INCBIN  fat_root.bin
incbin_fatimg_end

		END