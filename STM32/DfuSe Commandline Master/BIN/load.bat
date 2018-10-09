@ echo off
for /f %%a in ('dir /B /S *.hex') do %~dp0DfuFileMgr.exe %%a %%a.dfu >nul
for /f %%a in ('dir /B /S *.dfu') do %~dp0DfuSeCommand.exe -c -d --fn %%a -l >nul