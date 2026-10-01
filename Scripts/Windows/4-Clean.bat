@echo off
setlocal
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"

if exist "%ROOT%\build" (
    rd /q /s "%ROOT%\build" || exit /B 1
)
if exist "%ROOT%\_Bin" (
    rd /q /s "%ROOT%\_Bin" || exit /B 1
)
if exist "%ROOT%\_Build" (
    rd /q /s "%ROOT%\_Build" || exit /B 1
)
if exist "%ROOT%\_Shaders" (
    rd /q /s "%ROOT%\_Shaders" || exit /B 1
)
if exist "%ROOT%\_NRI_SDK" (
    rd /q /s "%ROOT%\_NRI_SDK" || exit /B 1
)

exit /B 0
