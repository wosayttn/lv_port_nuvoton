cd MA35D1_NuWriter

:forever_develop
py -3 nuwriter.py -a ddrimg\MA35D16A887C.bin
IF %ERRORLEVEL% EQU 0 (
   py -3 nuwriter.py -o execute -w ddr 0x28000000 ../leave_ibr.bin
)
pause

goto :forever_develop
