@echo off
setlocal
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"

cmake -S "%ROOT%" -B "%ROOT%\_Build" %*
exit /B %ERRORLEVEL%
