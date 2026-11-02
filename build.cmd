@echo off
rem Builds build\Release\PandaTool.dll (Win32) with MSVC Build Tools 2026.
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul || exit /b 1
cd /d "%~dp0"
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32 || exit /b 1
cmake --build build --config Release || exit /b 1
if /i "%~1"=="install" copy /y "build\Release\PandaTool.dll" "C:\Games\Wow-Octo\PandaTool.dll" || exit /b 1
