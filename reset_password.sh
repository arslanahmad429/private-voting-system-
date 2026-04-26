#!/bin/bash
# ====================================================
#   Admin Password Reset Utility
#   For macOS/Linux
# ====================================================

echo ""
echo "========================================"
echo "   ADMIN PASSWORD RESET"
echo "========================================"
echo ""

# Check if virtual environment exists
if [ ! -d "venv" ]; then
    echo "[ERROR] Virtual environment not found!"
    echo "Please run setup.sh first."
    echo ""
    read -p "Press Enter to exit..."
    exit 1
fi

# Activate virtual environment
source venv/bin/activate

# Check if database exists
if [ ! -f "voting_system.db" ]; then
    echo "[WARNING] Database not found."
    echo "Password reset will create admin user with default password."
    echo ""
fi

# Ask for password choice
echo "Choose reset option:"
echo "  1. Reset to default password (Admin@123)"
echo "  2. Set custom password"
echo ""
read -p "Enter choice (1 or 2): " choice

if [ "$choice" = "2" ]; then
    echo ""
    read -s -p "Enter new admin password (min 8 chars): " custom_pass
    echo ""
    read -s -p "Confirm password: " confirm_pass
    echo ""
    
    if [ "$custom_pass" != "$confirm_pass" ]; then
        echo "[ERROR] Passwords do not match!"
        read -p "Press Enter to exit..."
        exit 1
    fi
    
    echo ""
    echo "[INFO] Setting custom password..."
    python reset_admin.py "$custom_pass"
else
    echo ""
    echo "[INFO] Resetting to default password..."
    python reset_admin.py
fi

echo ""
echo "========================================"
echo "   PASSWORD RESET COMPLETE"
echo "========================================"
echo ""
read -p "Press Enter to exit..."
