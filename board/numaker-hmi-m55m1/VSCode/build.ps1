. ~/.vcpkg/vcpkg-init.ps1
vcpkg activate
cbuild "lvgl.csolution.yml" --rebuild --update-rte --packs --context "lvgl.Debug+M55M1480x272"
cbuild "lvgl.csolution.yml" --rebuild --update-rte --packs --context "lvgl.Debug+M55M1800x480"
exit $LASTEXITCODE

