@echo off
echo Building ropeless_pi plugin for Windows with DLL copy...

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo Error: CMakeLists.txt not found. Make sure you're in the ropeless_pi root directory.
    exit /b 1
)

REM Create build directory if it doesn't exist
if not exist "build" (
    echo Creating build directory...
    mkdir build
    cd build
    
    REM Set wxWidgets paths to use bundled version
    set WX_ROOT=%CD%\..\cache\wxWidgets
    
    echo Configuring build with CMake...
    REM Try different Visual Studio versions with Windows target
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

    REM Fall back to default generator
    echo Using default generator...
    cmake .. -A Win32 -DwxWidgets_ROOT_DIR="%WX_ROOT%" -DwxWidgets_LIB_DIR="%WX_ROOT%\lib\vc_dll" -DOCPN_TARGET_TUPLE="msvc-wx32;10;x86_64"
    
    :build_continue
    IF %ERRORLEVEL% NEQ 0 (
        echo Error: CMake configuration failed!
        cd ..
        exit /b %ERRORLEVEL%
    )
    
    cd ..
) else (
    echo Using existing build directory...
)

REM Change directory to ./build
cd /d "%~dp0build"

REM Run the cmake build command
echo Building plugin in RelWithDebInfo mode...
cmake --build . --config RelWithDebInfo --target ropeless_pi

REM Check if the build was successful
IF %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    cd /d ".."
    exit /b %ERRORLEVEL%
)

REM Copy *.dll and *.pdb files to the destination
echo Copying DLL and PDB files to OpenCPN plugins directory...
xcopy "RelWithDebInfo\*.dll" "C:\Users\ColinVincent\AppData\Local\opencpn\plugins" /y
xcopy "RelWithDebInfo\*.pdb" "C:\Users\ColinVincent\AppData\Local\opencpn\plugins" /y

REM Confirm that files were copied
echo Files copied successfully!
echo.
echo Plugin built and copied. You can now restart OpenCPN to use the updated plugin.

cd /d ".."