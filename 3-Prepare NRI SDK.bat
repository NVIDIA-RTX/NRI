@echo off

set NRI_DIR=.

rd /q /s "_NRI_SDK"

mkdir "_NRI_SDK\Include\Extensions"
mkdir "_NRI_SDK\Lib\Debug"
mkdir "_NRI_SDK\Lib\Release"

cd "_NRI_SDK"

copy "..\%NRI_DIR%\Include\*" "Include"
copy "..\%NRI_DIR%\Include\Extensions\*" "Include\Extensions"
copy "..\_Bin\Debug\NRI.dll" "Lib\Debug"
copy "..\_Bin\Debug\NRI.lib" "Lib\Debug"
copy "..\_Bin\Debug\NRI.pdb" "Lib\Debug"
copy "..\_Bin\Release\NRI.dll" "Lib\Release"
copy "..\_Bin\Release\NRI.lib" "Lib\Release"
copy "..\_Bin\Release\NRI.pdb" "Lib\Release"
copy "..\%NRI_DIR%\LICENSE.txt" "."

cd ..
