@echo off
setlocal
set "Configuration=Debug"
call "%~dp0build-SteamAPIBridge-x86.bat" %*
exit /b %errorlevel%
