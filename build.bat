@echo off
pushd bootsect
nasm bootsect.asm -O2 -o ..\bin\bootsect.bin
popd
pushd osldr16
nasm osldr.asm -O2 -o ..\bin\OSLDR
popd
copy res\SPLASH.IMG bin\SPLASH.IMG
if ERRORLEVEL 0 echo Build success.
pause
rem cd bin
rem cmd