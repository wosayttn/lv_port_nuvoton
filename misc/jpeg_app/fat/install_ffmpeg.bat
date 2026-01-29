@echo off
setlocal enabledelayedexpansion

:: --- Configuration ---
set "URL=https://github.com/GyanD/codexffmpeg/releases/download/2025-12-24-git-abb1524138/ffmpeg-2025-12-24-git-abb1524138-essentials_build.zip"
set "ZIP_FILE=ffmpeg_temp.zip"
set "EXTRACT_DIR=ffmpeg_temp_folder"
set "DEST_BIN=bin"

echo ============================================
echo [1/4] Downloading FFmpeg (Zip format)...
echo ============================================
:: Download file using PowerShell (Supports TLS 1.2)
powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%URL%' -OutFile '%ZIP_FILE%'"

if not exist "%ZIP_FILE%" (
    echo [ERROR] Download failed. Please check your connection or URL.
    pause
    exit /b
)

echo.
echo [2/4] Extracting files...
:: Use built-in PowerShell command to extract
if exist "%EXTRACT_DIR%" rmdir /s /q "%EXTRACT_DIR%"
powershell -Command "Expand-Archive -Path '%ZIP_FILE%' -DestinationPath '%EXTRACT_DIR%'"

echo.
echo [3/4] Extracting binaries (bin) to local directory...
if not exist "%DEST_BIN%" mkdir "%DEST_BIN%"

:: Search for the extracted folder and locate the bin sub-directory
for /d %%i in ("%EXTRACT_DIR%\*") do (
    if exist "%%i\bin" (
        xcopy "%%i\bin\*.exe" "%DEST_BIN%\" /Y /S
    )
)

echo.
echo [4/4] Cleaning up temporary files...
del "%ZIP_FILE%"
rmdir /s /q "%EXTRACT_DIR%"

echo.
echo ============================================
echo Installation Complete! FFmpeg is located in .\%DEST_BIN%
echo ============================================
"%DEST_BIN%\ffmpeg.exe" -version | findstr "version"
pause