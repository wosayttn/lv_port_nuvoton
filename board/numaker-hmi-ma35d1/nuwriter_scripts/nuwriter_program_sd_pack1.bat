cd MA35D1_NuWriter

py -3 nuwriter.py -c ../header1-sd.json
py -3 nuwriter.py -p ../pack1-sd.json

py -3 nuwriter.py -a ddrimg/enc_ddr3_winbond_256mb.bin
IF %ERRORLEVEL% EQU 0 (
    py -3 nuwriter.py -w sd pack/pack.bin
)

PAUSE
