@echo off
setlocal
if not "%Configuration%"=="Debug" if not "%Configuration%"=="Release" exit /b 2
if not "%Arch%"=="x86" if not "%Arch%"=="x64" exit /b 2
if "%Arch%"=="x86" (set "Platform=Win32") else (set "Platform=x64")
for %%I in ("%~dp0..") do set "TaskSourceDir=%%~fI"
set "TaskBuildDir=%TaskSourceDir%\build\%Arch%\%Configuration%"
cmake -G "Visual Studio 17 2022" -A %Platform% -S "%TaskSourceDir%" -B "%TaskBuildDir%" -DCMAKE_INSTALL_PREFIX="%TaskSourceDir%\install\%Arch%\%Configuration%" %*
if errorlevel 1 exit /b %errorlevel%
cmake --build "%TaskBuildDir%" --config %Configuration% --parallel
if errorlevel 1 exit /b %errorlevel%
ctest --test-dir "%TaskBuildDir%" -C %Configuration% --output-on-failure
if errorlevel 1 exit /b %errorlevel%
cmake --install "%TaskBuildDir%" --config %Configuration%
exit /b %errorlevel%
