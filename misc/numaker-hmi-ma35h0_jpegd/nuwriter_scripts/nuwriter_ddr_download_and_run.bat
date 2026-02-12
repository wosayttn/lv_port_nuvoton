cd MA35D1_NuWriter

:forever_develop
py -3 nuwriter.py -a ddrimg\MA35H04F764C.bin
IF %ERRORLEVEL% EQU 0 (
   py -3 nuwriter.py -o execute -w ddr 0x80000000 ../../GCC/MA35H0_1024x600/lvgl_ma35h0.bin
)
pause

goto :forever_develop
