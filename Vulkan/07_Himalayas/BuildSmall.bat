@echo off
setlocal
rem Uses the shared Tools\Crinkler3.0b distribution through Build.ps1.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\..\Build.ps1" -Project "%~dp0GraphicsDemo.vcxproj" -Configuration Release -Compress %*
exit /b %errorlevel%
