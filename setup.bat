@echo off
REM ====================================================
REM   Online Voting System - Automatic Setup Script
REM   For Windows
REM ====================================================

echo.
echo ========================================
echo   VOTING SYSTEM - SETUP
echo ========================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python is not installed!
    echo.
    echo Please install Python 3.8 or higher from:
    echo https://www.python.org/downloads/
    echo.
    echo Make sure to check "Add Python to PATH" during installation.
    echo.
    pause
    exit /b 1
)

echo [OK] Python is installed
python --version
echo.

REM Check if virtual environment exists
if exist "venv\" (
    echo [INFO] Virtual environment already exists
    echo.
) else (
    echo [INFO] Creating virtual environment...
    python -m venv venv
    if errorlevel 1 (
        echo [ERROR] Failed to create virtual environment
        pause
        exit /b 1
    )
    echo [OK] Virtual environment created
    echo.
)

REM Activate virtual environment
echo [INFO] Activating virtual environment...
call venv\Scripts\activate.bat
if errorlevel 1 (
    echo [ERROR] Failed to activate virtual environment
    pause
    exit /b 1
)
echo [OK] Virtual environment activated
echo.

REM Upgrade pip
echo [INFO] Upgrading pip...
python -m pip install --upgrade pip --quiet
echo [OK] Pip upgraded
echo.

REM Install requirements
echo [INFO] Installing dependencies...
echo This may take a few minutes...
echo.
pip install -r requirements.txt
if errorlevel 1 (
    echo.
    echo [ERROR] Failed to install dependencies
    echo.
    echo Please try manually:
    echo   1. Open Command Prompt
    echo   2. Run: venv\Scripts\activate
    echo   3. Run: pip install -r requirements.txt
    echo.
    pause
    exit /b 1
)
echo.
echo [OK] All dependencies installed successfully!
echo.

REM Create .env file if it doesn't exist
if not exist ".env" (
    echo [INFO] Creating default .env file...
    (
        echo FLASK_APP=app.py
        echo FLASK_ENV=development
        echo SECRET_KEY=change-this-secret-key-in-production-use-secrets.token_hex
        echo DATABASE_URI=sqlite:///voting_system.db
        echo SESSION_COOKIE_SECURE=False
        echo SESSION_COOKIE_HTTPONLY=True
        echo SESSION_COOKIE_SAMESITE=Lax
    ) > .env
    echo [OK] .env file created
    echo.
)

REM Check if database exists and offer password reset
if exist "voting_system.db" (
    echo ========================================
    echo   ADMIN PASSWORD RESET
    echo ========================================
    echo.
    echo Database already exists.
    echo.
    echo Do you want to reset admin password?
    echo   1. Yes - Reset to default (Admin@123)
    echo   2. Yes - Set custom password
    echo   3. No - Skip password reset
    echo.
    choice /C 123 /N /M "Enter your choice (1, 2, or 3): "
    
    if errorlevel 3 goto skip_reset
    if errorlevel 2 goto custom_reset
    if errorlevel 1 goto default_reset
    
    :default_reset
    echo.
    echo [INFO] Resetting admin password to default...
    python reset_admin.py
    echo.
    goto skip_reset
    
    :custom_reset
    echo.
    set /p custom_pass="Enter new admin password (min 8 chars): "
    echo.
    echo [INFO] Setting custom admin password...
    python reset_admin.py "%custom_pass%"
    echo.
    echo [WARNING] Remember this password! Write it down.
    echo.
    goto skip_reset
    
    :skip_reset
    echo.
)

echo ========================================
echo   SETUP COMPLETE!
echo ========================================
echo.
echo Next steps:
echo   1. Click "run.bat" to start the application
echo   2. Browser will open automatically
echo   3. Login with admin credentials
echo   4. If using default: admin@votingsystem.com / Admin@123
echo   5. CHANGE ADMIN PASSWORD if using default!
echo.
echo ========================================
echo.
echo TIP: Run setup.bat anytime to reset admin password
echo.
pause
