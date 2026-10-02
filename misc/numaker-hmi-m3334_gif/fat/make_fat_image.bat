@echo off
setlocal

pushd "%~dp0"

echo [FATDISK] Generating FAT filesystem image...

set PYTHON_CMD=python
where python >nul 2>nul
if %ERRORLEVEL% neq 0 (
    set PYTHON_CMD=py -3
)

%PYTHON_CMD% fatdisk.py %*

if %ERRORLEVEL% equ 0 (
    echo.
    echo [FATDISK] Done: FAT image generated successfully.
) else (
    echo.
    echo [FATDISK] Error: Failed to generate FAT image.
)

popd
pause
endlocal
