@echo off
REM ──────────────────────────────────────────────────────────────────
REM nst-mcp-bridge.bat — stdio-to-Named-Pipe bridge for NST MCP Server
REM
REM This script bridges stdin/stdout to the NST TUI's embedded MCP
REM server running on a Windows Named Pipe.
REM
REM Usage:
REM   1. Start NST TUI:  NST.exe -t
REM   2. In your MCP client config:
REM      {
REM        "mcpServers": {
REM          "nst": {
REM            "command": "C:\\path\\to\\nst-mcp-bridge.bat"
REM          }
REM        }
REM      }
REM ──────────────────────────────────────────────────────────────────

setlocal

REM Named pipe path (must match NST TUI's embedded server)
set PIPE_NAME=\\.\pipe\nst-mcp

REM Check if NST executable is available for pipe bridging
where NST.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    REM Use NST's own --mcp-bridge mode if available
    NST.exe --mcp-bridge %PIPE_NAME%
    exit /b %ERRORLEVEL%
)

REM Fallback: Try using PowerShell for pipe communication
echo Error: Named Pipe bridging requires NST.exe in PATH or PowerShell 5.0+. >&2
echo Make sure NST TUI is running (NST.exe -t) >&2
exit /b 1
