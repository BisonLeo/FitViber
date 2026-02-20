@echo off
setlocal

REM Change to script directory (project root)
cd /d "%~dp0"

REM Setup MSVC environment (VS 2022)
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

set QT_DIR=D:\Qt\6.10.1\msvc2022_64
set PATH=%QT_DIR%\bin;%PATH%

echo ========================================
echo FitViber - GPS Activity Overlay Editor
echo Building with Ninja + MSVC Compiler
echo ========================================
echo.

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake using Ninja generator
echo Configuring project with Ninja...
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_DIR% ..

if errorlevel 1 (
    echo.
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Building project...
ninja

if errorlevel 1 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Executable: bin\FitViber.exe
echo ========================================
echo.

pause
