@echo off

set ROOT=%cd%
set SELF=%~dp0
set SDK=_NRI_SDK

echo %SDK%: ROOT=%ROOT%, SELF=%SELF%

rd /q /s "%SDK%"

mkdir "%SDK%\Include\Extensions"
mkdir "%SDK%\Lib\Debug"
mkdir "%SDK%\Lib\Release"

copy "%SELF%\Include\*" "%SDK%\Include"
copy "%SELF%\Include\Extensions\*" "%SDK%\Include\Extensions"
copy "%SELF%\LICENSE.txt" "%SDK%"
copy "%SELF%\README.md" "%SDK%"
copy "%SELF%\nri.natvis" "%SDK%"

copy "%ROOT%\_Bin\Debug\NRI.dll" "%SDK%\Lib\Debug"
copy "%ROOT%\_Bin\Debug\NRI.lib" "%SDK%\Lib\Debug"
copy "%ROOT%\_Bin\Debug\NRI.pdb" "%SDK%\Lib\Debug"
copy "%ROOT%\_Bin\Release\NRI.dll" "%SDK%\Lib\Release"
copy "%ROOT%\_Bin\Release\NRI.lib" "%SDK%\Lib\Release"
copy "%ROOT%\_Bin\Release\NRI.pdb" "%SDK%\Lib\Release"
