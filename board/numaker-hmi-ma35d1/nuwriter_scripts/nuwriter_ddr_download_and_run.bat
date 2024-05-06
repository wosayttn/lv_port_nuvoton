cd MA35D1_NuWriter

:forever_develop
py -3 nuwriter.py -a ddrimg\enc_ddr3_winbond_256mb.bin
IF %ERRORLEVEL% EQU 0 (
   py -3 nuwriter.py -o execute -w ddr 0x80400000 ../../GCC/MA35D1_1024x600/lvgl_ma35d1.bin
)
pause

goto :forever_develop
