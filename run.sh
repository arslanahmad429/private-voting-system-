#!/bin/bash
# ====================================================
#   Online Voting System - Run Script
#   For macOS/Linux
# ====================================================

echo ""
echo "========================================"
echo "   STARTING VOTING SYSTEM"
echo "========================================"
echo ""

# Check if virtual environment exists
if [ ! -d "venv" ]; then
    echo "[ERROR] Virtual environment not found!"
    echo ""
    echo "Please run './setup.sh' first to set up the system."
    echo ""
    read -p "Press Enter to exit..."
    exit 1
fi

# Activate virtual environment
source venv/bin/activate

# Check if requirements are installed
python -c "import flask" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "[ERROR] Dependencies not installed!"
    echo ""
    echo "Please run './setup.sh' first to install dependencies."
    echo ""
    read -p "Press Enter to exit..."
    exit 1
fi

echo "[OK] Starting application..."
echo ""
echo "========================================"
echo "   Server will start on:"
echo "   http://localhost:5000"
echo "========================================"
echo ""
echo "Default Admin Login:"
echo "  Email: admin@votingsystem.com"
echo "  Password: Admin@123"
echo ""
echo "[!] CHANGE ADMIN PASSWORD AFTER LOGIN!"
echo ""
echo "========================================"
echo ""
echo "Press Ctrl+C to stop the server"
echo ""
echo "========================================"
echo ""

# Open browser after 3 seconds
(sleep 3 && open http://localhost:5000 2>/dev/null || xdg-open http://localhost:5000 2>/dev/null) &

# Run the application
python app.py

# If app stops, keep terminal open
echo ""
echo "========================================"
echo "   Server stopped"
echo "========================================"
read -p "Press Enter to exit..."
