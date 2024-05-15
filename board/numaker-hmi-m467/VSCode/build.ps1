. ~/.vcpkg/vcpkg-init.ps1
vcpkg activate
cbuild "lvgl.csolution.yml" --rebuild --update-rte --packs --context "lvgl.Debug+M467HJ480x272FSA506"
cbuild "lvgl.csolution.yml" --rebuild --update-rte --packs --context "lvgl.Debug+M467HJ480x272SSD1963"
exit $LASTEXITCODE

