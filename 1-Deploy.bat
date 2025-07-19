@echo off

git submodule update --init --recursive

mkdir "_Build"

cd "_Build"

cmake .. -A x64
if %ERRORLEVEL% NEQ 0 exit /B %ERRORLEVEL%

cd ..
