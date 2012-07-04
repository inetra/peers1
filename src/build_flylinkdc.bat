rem -- call build_stlport.bat
del .\compiled\FlylinkDC.exe
msbuild.exe StrongDC.sln /t:Rebuild /p:Configuration=Release
rem -- cd setup
rem -- call build_setup.bat