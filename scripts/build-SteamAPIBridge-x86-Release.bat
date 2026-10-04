@echo off
setlocal
set "Configuration=Release"
call "%~dp0build-SteamAPIBridge-x86.bat" %*
exit /b %errorlevel%
