#################################################################
# build_emulator.ps1 - Rice Cooker PC Emulator build script
# Usage: powershell -ExecutionPolicy Bypass -File build_emulator.ps1
#################################################################
param(
    [switch]$Clean,
    [switch]$Run
)

$ErrorActionPreference = "Stop"
$SimDir = $PSScriptRoot
$BuildDir = Join-Path $SimDir "build"

Write-Host "=== Rice Cooker PC Emulator Build ===" -ForegroundColor Cyan

# ---------- Toolchain: WinLibs MinGW-w64 ----------
$MINGW = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64"
if (-not (Test-Path "$MINGW\bin\gcc.exe")) {
    Write-Host "[1/5] Installing WinLibs (MinGW-w64 GCC)..." -ForegroundColor Yellow
    winget install --id BrechtSanders.WinLibs.POSIX.UCRT --accept-package-agreements --accept-source-agreements --silent --source winget
    if (-not (Test-Path "$MINGW\bin\gcc.exe")) {
        throw "Failed to install WinLibs. Please install manually."
    }
}
# Add to PATH for this session
$env:Path = "$MINGW\bin;$env:Path"
Write-Host "[1/5] GCC: $(gcc --version | Select-Object -First 1)" -ForegroundColor Green

# ---------- SDL2 ----------
if (-not (Test-Path "$MINGW\include\SDL2\SDL.h")) {
    Write-Host "[2/5] Downloading SDL2 development library..." -ForegroundColor Yellow
    $sdlVer = "2.32.10"
    $sdlUrl = "https://github.com/libsdl-org/SDL/releases/download/release-$sdlVer/SDL2-devel-$sdlVer-mingw.tar.gz"
    $sdlZip = "$env:TEMP\SDL2-mingw.tar.gz"
    [System.Net.ServicePointManager]::SecurityProtocol = 'Tls12'
    Invoke-WebRequest -Uri $sdlUrl -OutFile $sdlZip -UseBasicParsing
    tar -xzf $sdlZip -C $env:TEMP
    $sdlSrc = "$env:TEMP\SDL2-$sdlVer\x86_64-w64-mingw32"
    Copy-Item "$sdlSrc\include\SDL2" "$MINGW\include\SDL2" -Recurse -Force
    Copy-Item "$sdlSrc\lib\*" "$MINGW\lib\" -Recurse -Force
    Copy-Item "$sdlSrc\bin\SDL2.dll" "$MINGW\bin\" -Force
    Remove-Item $sdlZip -Force -ErrorAction SilentlyContinue
}
Write-Host "[2/5] SDL2 OK" -ForegroundColor Green

# ---------- LVGL path ----------
$LVGL_PATH = (Resolve-Path "$SimDir\..\..\..\lvgl").Path
if (-not (Test-Path "$LVGL_PATH\src")) {
    throw "LVGL source not found at $LVGL_PATH. Set LVGL_PATH manually."
}
Write-Host "[3/5] LVGL: $LVGL_PATH" -ForegroundColor Green

# ---------- CMake configure & build ----------
Write-Host "[4/5] Building emulator..." -ForegroundColor Yellow

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "  Cleaning build directory..."
    Remove-Item -Recurse -Force $BuildDir
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Push-Location $BuildDir
try {
    # Configure
    Write-Host "  CMake configure..."
    cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DLVGL_PATH="$LVGL_PATH" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

    # Build
    Write-Host "  CMake build..."
    cmake --build . --config Release
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed" }

    Write-Host "[5/5] Build successful!" -ForegroundColor Green

    # Copy SDL2.dll next to exe
    $exe = Get-ChildItem -Filter "rice_cooker_sim.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($exe) {
        Copy-Item "$MINGW\bin\SDL2.dll" $exe.DirectoryName -Force -ErrorAction SilentlyContinue
        Write-Host "  Executable: $($exe.FullName)" -ForegroundColor Cyan
        if ($Run) {
            Write-Host "`nLaunching emulator..." -ForegroundColor Cyan
            & $exe.FullName
        } else {
            Write-Host "  Run with: .\build_emulator.ps1 -Run" -ForegroundColor Gray
        }
    }
} finally {
    Pop-Location
}
