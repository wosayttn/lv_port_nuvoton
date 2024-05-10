cd MA35D1_NuWriter

py -3 nuwriter.py -c ../header-spinor.json
py -3 nuwriter.py -p ../pack-spinor.json

py -3 nuwriter.py -a ddrimg\MA35H04F764C.bin
IF %ERRORLEVEL% EQU 0 (
    py -3 nuwriter.py -w spinor pack/pack.bin
)
PAUSE
