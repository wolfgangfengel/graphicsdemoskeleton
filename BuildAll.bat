@echo off
setlocal
rem Build every Win32 demo and pack Release with Tools\Crinkler3.0b.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build.ps1" -Configuration All -Compress %*
exit /b %errorlevel%
