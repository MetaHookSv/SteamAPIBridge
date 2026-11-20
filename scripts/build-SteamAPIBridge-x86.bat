@echo off
setlocal
set "Arch=x86"
call "%~dp0build-SteamAPIBridge-windows.bat" %*
exit /b %errorlevel%
