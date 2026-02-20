@echo off
setlocal

cd /d D:\work\c_projs\FitViber

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

set QT_DIR=D:\Qt\6.10.1\msvc2022_64
set PATH=%QT_DIR%\bin;%PATH%

if not exist build mkdir build
cd build

cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_DIR% ..
if errorlevel 1 exit /b 1

ninja
if errorlevel 1 exit /b 1

echo Build completed successfully!
