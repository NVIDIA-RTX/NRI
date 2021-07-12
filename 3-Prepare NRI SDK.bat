@echo off

rd /q /s "_NRI_SDK"

mkdir "_NRI_SDK"
cd "_NRI_SDK"
copy "..\LICENSE.txt" "."
copy "..\README.md" "."

mkdir "Lib"

mkdir "Lib\Debug"
copy "..\_Build\Debug\NRI.dll" "Lib\Debug"
copy "..\_Build\Debug\NRI.lib" "Lib\Debug"
copy "..\_Build\Debug\NRI.pdb" "Lib\Debug"

mkdir "Lib\Release"
copy "..\_Build\Release\NRI.dll" "Lib\Release"
copy "..\_Build\Release\NRI.lib" "Lib\Release"
copy "..\_Build\Release\NRI.pdb" "Lib\Release"

mkdir "Include"
copy "..\Include\*" "Include"
mkdir "Include\Extensions"
copy "..\Include\Extensions\*" "Include\Extensions"
