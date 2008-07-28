@echo off

C:\cygwin\bin\bash --login -i /usr/src/myos/build

echo Running Qemu...
c:\qemu\qemu -L c:\qemu -snapshot c:\cygwin\usr\src\myos\hdd.img
echo Simulation terminated.