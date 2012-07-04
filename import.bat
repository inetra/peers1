@echo off
if "%1" == "" goto no_args
cvs import -I ! -I CVS -I .svn -I Settings -I *.log -I *.dof -I *.cfg -W "*.gif -k 'b'" -W "*.wav -k 'b'" -W "*.bz2 -k 'b'" -W "*.bmp -k 'b'" -W "*.res -k 'b'" -W "*.ico -k 'b'" -W "*.dll -k 'b'" -m "импорт %1" -- Peers\src FLY_LINK_DC FLY_LINK_DC_%1

:no_args
echo Error: No arguments
echo Usage:
echo %0 {import-date} (e.g. 2007-12-31)

:end
