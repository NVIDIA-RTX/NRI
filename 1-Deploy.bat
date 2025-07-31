@echo off

mkdir "_Build"

cd "_Build"

cmake .. %*
if %ERRORLEVEL% NEQ 0 exit /B %ERRORLEVEL%

cd ..
