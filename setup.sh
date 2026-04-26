#!/bin/bash
# ====================================================
#   Online Voting System - Automatic Setup Script
#   For macOS/Linux
# ====================================================

echo ""
echo "========================================"
echo "   VOTING SYSTEM - SETUP"
echo "========================================"
echo ""

# Check if Python is installed
if ! command -v python3 &> /dev/null; then
    echo "[ERROR] Python 3 is not installed!"
    echo ""
    echo "Please install Python 3.8 or higher:"
    echo "  - macOS: brew install python3"
    echo "  - Ubuntu/Debian: sudo apt install python3 python3-venv python3-pip"
    echo "  - CentOS/RHEL: sudo yum install python3"
    echo ""
    read -p "Press Enter to exit..."
    exit 1
fi

echo "[OK] Python is installed"
python3 --version
echo ""

# Check if virtual environment exists
if [ -d "venv" ]; then
    echo "[INFO] Virtual environment already exists"
    echo ""
else
    echo "[INFO] Creating virtual environment..."
    python3 -m venv venv
    if [ $? -ne 0 ]; then
        echo "[ERROR] Failed to create virtual environment"
        read -p "Press Enter to exit..."
        exit 1
    fi
    echo "[OK] Virtual environment created"
    echo ""
fi

# Activate virtual environment
echo "[INFO] Activating virtual environment..."
source venv/bin/activate
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to activate virtual environment"
    read -p "Press Enter to exit..."
    exit 1
fi
echo "[OK] Virtual environment activated"
echo ""

# Upgrade pip
echo "[INFO] Upgrading pip..."
python -m pip install --upgrade pip --quiet
echo "[OK] Pip upgraded"
echo ""

# Install requirements
echo "[INFO] Installing dependencies..."
echo "This may take a few minutes..."
echo ""
pip install -r requirements.txt
if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Failed to install dependencies"
    echo ""
    echo "Please try manually:"
    echo "  1. Run: source venv/bin/activate"
    echo "  2. Run: pip install -r requirements.txt"
    echo ""
    read -p "Press Enter to exit..."
    exit 1
fi
echo ""
echo "[OK] All dependencies installed successfully!"
echo ""

# Create .env file if it doesn't exist
if [ ! -f ".env" ]; then
    echo "[INFO] Creating default .env file..."
    cat > .env << EOL
FLASK_APP=app.py
FLASK_ENV=development
SECRET_KEY=change-this-secret-key-in-production-use-secrets.token_hex
DATABASE_URI=sqlite:///voting_system.db
SESSION_COOKIE_SECURE=False
SESSION_COOKIE_HTTPONLY=True
SESSION_COOKIE_SAMESITE=Lax
EOL
    echo "[OK] .env file created"
    echo ""
fi

# Make run script executable
chmod +x run.sh

echo "========================================"
echo "   SETUP COMPLETE!"
echo "========================================"
echo ""
echo "Next steps:"
echo "  1. Run: ./run.sh (or double-click run.sh)"
echo "  2. Browser will open automatically"
echo "  3. Login with: admin@votingsystem.com / Admin@123"
echo "  4. CHANGE ADMIN PASSWORD IMMEDIATELY!"
echo ""
echo "========================================"
echo ""
read -p "Press Enter to exit..."
