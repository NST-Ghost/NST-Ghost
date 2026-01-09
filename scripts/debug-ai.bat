@echo off
:: =============================================================================
:: NST Debugger Tool
:: =============================================================================
:: This script helps diagnose "WinError 126" and other AI feature issues.
:: It inspects the Python environment and attempts to load AI libraries
:: with verbose feedback.
:: =============================================================================

title NST Debugger

:: Find NST root and bundled python
set "SCRIPT_DIR=%~dp0"
set "NST_ROOT=%SCRIPT_DIR%.."
set "PYTHON_EXE=%NST_ROOT%\python\python.exe"

echo -------------------------------------------------------------------
echo  NST Debugger Tool
echo -------------------------------------------------------------------
echo.

if not exist "%PYTHON_EXE%" (
    echo [ERROR] Bundled Python not found at:
    echo "%PYTHON_EXE%"
    echo.
    echo Please make sure you have extracted the full NST package.
    pause
    exit /b 1
)

echo Bundled Python found. Running diagnostic script...
echo.

:: Run the debug script using bundled Python
"%PYTHON_EXE%" "%SCRIPT_DIR%debug_ai.py"

if errorlevel 1 (
    echo.
    echo [ERROR] The debugger script encountered an error.
    pause
    exit /b 1
)
