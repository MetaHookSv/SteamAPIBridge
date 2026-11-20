@echo off
setlocal
set "Configuration=Release"
call "%~dp0build-SteamAPIBridge-x64.bat" %*
exit /b %errorlevel%
