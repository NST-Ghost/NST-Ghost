@echo off
:: =============================================================================
:: NST AI Features Installer Launcher for Windows
:: =============================================================================
:: This batch file launches the PowerShell installer script with the correct
:: execution policy to avoid "running scripts is disabled" errors.
::
:: Usage:
::   install-ai-features.bat          - Install CPU version (default)
::   install-ai-features.bat -GPU     - Install GPU version (requires CUDA)
:: =============================================================================

title NST AI Features Installer

:: Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"

:: Check if -GPU parameter was passed
set "GPU_FLAG="
if /i "%~1"=="-GPU" set "GPU_FLAG=-GPU"
if /i "%~1"=="--GPU" set "GPU_FLAG=-GPU"
if /i "%~1"=="/GPU" set "GPU_FLAG=-GPU"

:: Run the PowerShell script with Bypass execution policy
PowerShell -ExecutionPolicy Bypass -NoProfile -File "%SCRIPT_DIR%install-ai-features.ps1" %GPU_FLAG%

:: Keep window open if there was an error
if errorlevel 1 (
    echo.
    echo Installation failed. Press any key to close...
    pause >nul
    exit /b 1
)

:: Keep window open briefly on success
echo.
echo Press any key to close...
pause >nul
