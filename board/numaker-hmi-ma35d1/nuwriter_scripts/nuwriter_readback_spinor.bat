cd MA35D1_NuWriter

py -3 nuwriter.py -a ddrimg\enc_ddr3_winbond_256mb.bin
IF %ERRORLEVEL% EQU 0 (
    py -3 nuwriter.py -r spinor 0x00840000 0x1000 output.bin
)
