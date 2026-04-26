@echo off
REM ====================================================
REM   Online Voting System - Run Script
REM   For Windows
REM ====================================================

echo.
echo ========================================
echo   STARTING VOTING SYSTEM
echo ========================================
echo.

REM Check if virtual environment exists
if not exist "venv\" (
    echo [ERROR] Virtual environment not found!
    echo.
    echo Please run "setup.bat" first to set up the system.
    echo.
    pause
    exit /b 1
)

REM Activate virtual environment
call venv\Scripts\activate.bat

REM Check if requirements are installed
python -c "import flask" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Dependencies not installed!
    echo.
    echo Please run "setup.bat" first to install dependencies.
    echo.
    pause
    exit /b 1
)

echo [OK] Starting application...
echo.
echo ========================================
echo   Server will start on:
echo   http://localhost:5000
echo ========================================
echo.
echo Default Admin Login:
echo   Email: admin@votingsystem.com
echo   Password: Admin@123
echo.
echo [!] CHANGE ADMIN PASSWORD AFTER LOGIN!
echo.
echo ========================================
echo.
echo Press Ctrl+C to stop the server
echo.
echo ========================================
echo.

REM Open browser after 3 seconds
start /B timeout /t 3 /nobreak >nul && start http://localhost:5000

REM Run the application
python app.py

REM If app stops, keep window open
echo.
echo ========================================
echo   Server stopped
echo ========================================
pause
