@echo off

set ROOT=%cd%
set SELF=%~dp0

rd /q /s "_NRI_SDK"
mkdir "_NRI_SDK"
cd "_NRI_SDK"

mkdir "Include\Extensions"
mkdir "Lib\Debug"
mkdir "Lib\Release"

copy "%SELF%\Include\*" "Include"
copy "%SELF%\Include\Extensions\*" "Include\Extensions"
copy "%SELF%\LICENSE.txt" "."
copy "%SELF%\README.md" "."
copy "%SELF%\nri.natvis" "."

copy "%ROOT%\_Bin\Debug\NRI.dll" "Lib\Debug"
copy "%ROOT%\_Bin\Debug\NRI.lib" "Lib\Debug"
copy "%ROOT%\_Bin\Debug\NRI.pdb" "Lib\Debug"
copy "%ROOT%\_Bin\Release\NRI.dll" "Lib\Release"
copy "%ROOT%\_Bin\Release\NRI.lib" "Lib\Release"
copy "%ROOT%\_Bin\Release\NRI.pdb" "Lib\Release"

cd ..
