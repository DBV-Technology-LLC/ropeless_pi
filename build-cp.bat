@echo off
REM Change directory to ./Build
cd /d "%~dp0Build"

REM Run the cmake build command
cmake --build . --config RelWithDebInfo --target tarball

REM Check if the build was successful
IF %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    cd /d ".."
    exit /b %ERRORLEVEL%
)

REM Copy *.dll and *.pdb files to the destination
xcopy "RelWithDebInfo\*.dll" "C:\Users\ColinVincent\AppData\Local\opencpn\plugins" /y
xcopy "RelWithDebInfo\*.pdb" "C:\Users\ColinVincent\AppData\Local\opencpn\plugins" /y

REM Confirm that files were copied
echo Files copied successfully!

cd /d ".."