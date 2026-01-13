if (Test-Path "C:\\ArmPacks") {
    $env:CMSIS_PACK_ROOT = "C:\\ArmPacks"
}

. ~/.vcpkg/vcpkg-init.ps1
vcpkg activate
cbuild "lvgl.csolution.yml" --rebuild --update-rte --packs
exit $LASTEXITCODE

