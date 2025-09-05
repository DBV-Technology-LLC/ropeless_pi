@echo off
REM ===================================================================
REM Proper Windows build script for ropeless_pi OpenCPN plugin
REM This creates a proper tarball that OpenCPN can import correctly
REM ===================================================================

echo Building ropeless_pi plugin for Windows...

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo Error: CMakeLists.txt not found. Make sure you're in the ropeless_pi root directory.
    exit /b 1
)

REM Clean previous build and cache
if exist "build" (
    echo Cleaning previous build directory...
    rmdir /s /q "build"
)
if exist "cache" (
    echo Ensuring cache directory exists...
) else (
    echo Warning: cache directory not found - may need to run initial setup
)

REM Create build directory
echo Creating build directory...
mkdir build
cd build

REM Configure with CMake for Windows/MSVC
echo Configuring build with CMake...

REM Set wxWidgets paths to use bundled version
set WX_ROOT=%CD%\..\cache\wxWidgets
set wxWidgets_ROOT_DIR=%WX_ROOT%
set wxWidgets_LIB_DIR=%WX_ROOT%\lib\vc_dll

echo Using bundled wxWidgets from: %WX_ROOT%

REM Try different Visual Studio versions (newest first) with explicit wxWidgets path and Windows target
cmake .. -G "Visual Studio 17 2022" -A Win32 -DwxWidgets_ROOT_DIR="%WX_ROOT%" -DwxWidgets_LIB_DIR="%WX_ROOT%\lib\vc_dll" -DOCPN_TARGET_TUPLE="msvc-wx32;10;x86_64" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using Visual Studio 2022
    goto build_continue
)

cmake .. -G "Visual Studio 16 2019" -A Win32 -DwxWidgets_ROOT_DIR="%WX_ROOT%" -DwxWidgets_LIB_DIR="%WX_ROOT%\lib\vc_dll" -DOCPN_TARGET_TUPLE="msvc-wx32;10;x86_64" >nul 2>&1  
if %ERRORLEVEL% EQU 0 (
    echo Using Visual Studio 2019
    goto build_continue
)

cmake .. -G "Visual Studio 15 2017" -A Win32 -DwxWidgets_ROOT_DIR="%WX_ROOT%" -DwxWidgets_LIB_DIR="%WX_ROOT%\lib\vc_dll" -DOCPN_TARGET_TUPLE="msvc-wx32;10;x86_64" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using Visual Studio 2017
    goto build_continue
)

REM Fall back to default generator with wxWidgets path and Windows target
echo No specific Visual Studio version found, trying default generator...
cmake .. -A Win32 -DwxWidgets_ROOT_DIR="%WX_ROOT%" -DwxWidgets_LIB_DIR="%WX_ROOT%\lib\vc_dll" -DOCPN_TARGET_TUPLE="msvc-wx32;10;x86_64"

:build_continue
IF %ERRORLEVEL% NEQ 0 (
    echo Error: CMake configuration failed!
    cd ..
    exit /b %ERRORLEVEL%
)

REM Build the tarball (this is the proper way)
echo Building plugin tarball...
cmake --build . --config Release --target tarball
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Build failed!
    cd ..
    exit /b %ERRORLEVEL%
)

REM Find the generated tarball
echo.
echo Build completed successfully!
echo.
echo Generated tarball files:
dir /b *.tar.gz 2>nul
if %ERRORLEVEL% EQU 0 (
    echo.
    echo To install the plugin:
    echo 1. Open OpenCPN
    echo 2. Go to Settings -^> Plugins -^> Import Plugin
    echo 3. Select the .tar.gz file from the build directory
    echo 4. Restart OpenCPN
) else (
    echo Warning: No tarball files found in build directory
)

cd ..
echo.
echo Build script completed.