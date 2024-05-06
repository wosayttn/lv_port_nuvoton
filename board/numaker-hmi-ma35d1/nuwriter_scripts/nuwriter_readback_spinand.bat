cd MA35D1_NuWriter

py -3 nuwriter.py -a ddrimg\enc_ddr3_winbond_256mb.bin
IF %ERRORLEVEL% EQU 0 (
    py -3 nuwriter.py -r spinand 0x0 0x400000 output.bin
)
