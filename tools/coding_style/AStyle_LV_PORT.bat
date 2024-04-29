@echo on
@echo +=======================================================+
@echo +         Execute Astyle source code prettifier         +
@echo +=======================================================+
@echo off

@SET ASTYLE_BIN=%~dp0\AStyle\bin\AStyle.exe
@rem @SET ASTYLE_BIN="Astyle\AStyle.exe"
@SET ASTYLE_OPTION=%~dp0\AStyle_LV_PORT.txt

echo %1
echo %ASTYLE_BIN%
echo %ASTYLE_OPTION%
if exist "%1\" (
	%ASTYLE_BIN% --options=%ASTYLE_OPTION% --ascii --recursive "%1\*.c,*.h"
) else (
	%ASTYLE_BIN% --options=%ASTYLE_OPTION% --ascii %1
)

pause
