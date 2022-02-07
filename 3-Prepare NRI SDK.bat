@echo off

set NRI_DIR=.

mkdir "_NRI_SDK"
pushd "_NRI_SDK"

copy "..\%NRI_DIR%\LICENSE.txt" "."

mkdir "Include"
copy "..\%NRI_DIR%\Include\*" "Include"

mkdir "Include\Extensions"
copy "..\%NRI_DIR%\Include\Extensions\*" "Include\Extensions"

mkdir "Lib"
mkdir "Lib\Debug"
copy "..\_Build\Debug\NRI.dll" "Lib\Debug"
copy "..\_Build\Debug\NRI.lib" "Lib\Debug"
copy "..\_Build\Debug\NRI.pdb" "Lib\Debug"
mkdir "Lib\Release"
copy "..\_Build\Release\NRI.dll" "Lib\Release"
copy "..\_Build\Release\NRI.lib" "Lib\Release"

popd
