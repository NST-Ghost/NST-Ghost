@echo off
setlocal

:: Get the directory of the script
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

:: Set PYTHONPATH to include the pylib directory
set "PYTHONPATH=%SCRIPT_DIR%;%PYTHONPATH%"

:: Path to the executable (adjust if needed for different build configurations)
if exist "build\Release\NST.exe" (
    set "EXECUTABLE=build\Release\NST.exe"
) else if exist "build\Debug\NST.exe" (
    set "EXECUTABLE=build\Debug\NST.exe"
) else if exist "build\NST.exe" (
    set "EXECUTABLE=build\NST.exe"
) else (
    echo Error: Executable not found.
    echo Please build the project first using CMake.
    pause
    exit /b 1
)

:: Run the executable with any arguments passed to this script
echo Starting NST...
"%EXECUTABLE%" %*

endlocal
