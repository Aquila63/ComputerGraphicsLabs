@echo off
echo.
title Nvidia Driver Leftovers Cleaner by WhiteZero
echo.
echo Welcome to the Nvidia Driver Leftovers Cleaner script! Created by WhiteZero
echo.
echo Continue to cleaning files?
PAUSE
echo.
echo.
echo Removing Directory %SYSTEMDRIVE%\NVIDIA
rmdir /S /Q %SYSTEMDRIVE%\NVIDIA
echo.
echo Removing Directory %ProgramFiles%\Nvidia Corporation\Installer2
rmdir /S /Q "%ProgramFiles%\Nvidia Corporation\Installer2"
echo.
echo Removing EXEs from %ALLUSERSPROFILE%\NVIDIA Corporation\NetService
del /Q "%ALLUSERSPROFILE%\NVIDIA Corporation\NetService\*.exe"
echo.
echo.
echo Finished!
PAUSE