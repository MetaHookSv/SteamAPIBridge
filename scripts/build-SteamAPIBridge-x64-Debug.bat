@echo off
setlocal
set "Configuration=Debug"
call "%~dp0build-SteamAPIBridge-x64.bat" %*
exit /b %errorlevel%
