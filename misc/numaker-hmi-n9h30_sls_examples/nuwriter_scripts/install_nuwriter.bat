echo on
git.exe clone https://github.com/OpenNuvoton/NUC970_NuWriter
IF %ERRORLEVEL% EQU 0 (
    echo "Failed to download NuWriter"
    exit 1
)
pip3 install pyusb usb crypto ecdsa crcmod tqdm pycryptodome
IF %ERRORLEVEL% EQU 0 (
    echo "Failed to download NuWriter"
    exit 1
)
