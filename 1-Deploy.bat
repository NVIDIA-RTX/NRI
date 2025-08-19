@echo off

mkdir "_Build"

cd "_Build"

cmake .. %*
if %ERRORLEVEL% NEQ 0 goto END

:END
cd ..
exit /B %ERRORLEVEL%
