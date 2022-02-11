@echo off

set /P M=Do you want to delete PACKMAN repository? [y/n]
if /I "%M%" neq "y" goto KEEP_PACKMAN

:DELETE_PACKMAN
rd /q /s "%PM_PACKAGES_ROOT%"

:KEEP_PACKMAN
rd /q /s "_Build"
rd /q /s "_Compiler"
rd /q /s "_NRI_SDK"

rd /q /s "build"
