"""
Database Reset Script
Deletes and recreates the database with proper admin account
"""

import os
import sys

# Get the database path
db_path = 'voting_system.db'

print("=" * 60)
print("   DATABASE RESET UTILITY")
print("=" * 60)
print()
print("WARNING: This will DELETE the database and all data!")
print("  - All elections will be lost")
print("  - All voters will be lost")
print("  - All votes will be lost")
print()
print("This is useful for:")
print("  - Fixing 'Invalid salt' password errors")
print("  - Starting fresh with clean database")
print("  - Resolving database corruption issues")
print()

choice = input("Are you SURE you want to continue? (yes/no): ").strip().lower()

if choice != 'yes':
    print()
    print("Operation cancelled.")
    sys.exit(0)

# Delete database if it exists
if os.path.exists(db_path):
    try:
        os.remove(db_path)
        print()
        print(f"[OK] Deleted {db_path}")
    except Exception as e:
        print()
        print(f"[ERROR] Could not delete database: {str(e)}")
        print("Make sure the Flask app is not running.")
        sys.exit(1)
else:
    print()
    print("[INFO] Database file not found, will create new one")

# Import and initialize app
print("[INFO] Creating new database...")

try:
    from app import create_app
    app = create_app()
    
    print("[OK] Database created successfully!")
    print()
    print("=" * 60)
    print("   RESET COMPLETE!")
    print("=" * 60)
    print()
    print("Default admin credentials:")
    print("  Email: admin@votingsystem.com")
    print("  Password: Admin@123")
    print()
    print("IMPORTANT: Change admin password after first login!")
    print()
    print("=" * 60)
    
except Exception as e:
    print()
    print(f"[ERROR] Failed to create database: {str(e)}")
    print()
    print("Try:")
    print("  1. Close all terminal windows")
    print("  2. Run this script again")
    sys.exit(1)
