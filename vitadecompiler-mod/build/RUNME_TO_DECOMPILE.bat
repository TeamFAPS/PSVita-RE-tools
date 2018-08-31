@echo off
cd %cd%

for %%i in (*.elf *.velf) do (
	vitadecompiler.exe %%i db.yml %%~ni/%%i.c
	echo.
)

pause