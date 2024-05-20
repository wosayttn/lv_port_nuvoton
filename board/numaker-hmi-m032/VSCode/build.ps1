. ~/.vcpkg/vcpkg-init.ps1
vcpkg activate
cbuild "lvgl.csolution.yml" --rebuild --update-rte --packs --context "lvgl.Debug+M032KI320x240" -v
exit $LASTEXITCODE

