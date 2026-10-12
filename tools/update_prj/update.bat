echo on

:forever_develop

if exist "..\..\.venv\Scripts\python.exe" (
	"..\..\.venv\Scripts\python.exe" update_v9.6.py
) else (
	py -3 update_v9.6.py
)

pause

goto :forever_develop