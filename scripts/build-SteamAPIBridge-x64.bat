@echo off
setlocal
set "Arch=x64"
call "%~dp0build-SteamAPIBridge-windows.bat" %*
exit /b %errorlevel%
