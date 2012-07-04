@echo off
del build\lib\obj /S /Q
cd build\lib
call "%ProgramFiles%\microsoft visual studio 8\VC\bin\vcvars32.bat"
call configure.bat -c msvc8
nmake /fmsvc.mak
nmake /fmsvc.mak install
