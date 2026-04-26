"""
Database Migration Script for Enhanced Voting System
This script migrates the existing database to support new features:
- CNIC fields for users and candidates
- Election-voter relationships
- Candidate applications
"""

import os
import sys
import shutil
from datetime import datetime

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from app import create_app
from models import db, User, Election, Candidate
import random


def backup_database(app):
    """Create backup of existing database"""
    db_uri = app.config['SQLALCHEMY_DATABASE_URI']
    
    if not db_uri.startswith('sqlite:///'):
        print("⚠️  Non-SQLite database detected. Please backup manually.")
        return None
    
    db_path = db_uri.replace('sqlite:///', '')
    
    if not os.path.exists(db_path):
        print("ℹ️  No existing database found. Will create new one.")
        return None
    
    # Create backup
    backup_dir = 'backups'
    os.makedirs(backup_dir, exist_ok=True)
    
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    backup_path = os.path.join(backup_dir, f'pre_migration_{timestamp}.db')
    
    shutil.copy2(db_path, backup_path)
    print(f"✅ Database backed up to: {backup_path}")
    
    return backup_path


def migrate_database(app):
    """Perform database migration"""
    
    print("\n" + "="*60)
    print("🔄 DATABASE MIGRATION STARTED")
    print("="*60)
    
    with app.app_context():
        # Step 1: Backup
        print("\n📦 Step 1: Creating backup...")
        backup_path = backup_database(app)
        
        # Step 2: Check for existing data
        print("\n🔍 Step 2: Checking existing data...")
        try:
            existing_users = User.query.count()
            existing_elections = Election.query.count()
            print(f"   Found {existing_users} users")
            print(f"   Found {existing_elections} elections")
            has_data = existing_users > 0 or existing_elections > 0
        except:
            has_data = False
            print("   No existing data found")
        
        # Step 3: Recreate tables with new schema
        print("\n🏗️  Step 3: Updating database schema...")
        
        if has_data:
            print("\n⚠️  WARNING: Existing data detected!")
            print("   The migration will drop and recreate all tables.")
            print("   A backup has been created at:", backup_path)
            
            response = input("\n   Continue with migration? (yes/no): ")
            if response.lower() != 'yes':
                print("\n❌ Migration cancelled.")
                return False
        
        # Drop all tables
        db.drop_all()
        print("   ✓ Old tables dropped")
        
        # Create new tables
        db.create_all()
        print("   ✓ New tables created")
        
        #Step 4: Create default admin
        print("\n👤 Step 4: Creating default admin user...")
        
        admin = User.query.filter_by(email='admin@votingsystem.com').first()
        if not admin:
            from utils import hash_password
            admin = User(
                name='System Admin',
                cnic='0000000000000',
                email='admin@votingsystem.com',
                password_hash=hash_password('Admin@123'),
                is_admin=True
            )
            db.session.add(admin)
            db.session.commit()
            print("   ✓ Admin user created")
            print("      Email: admin@votingsystem.com")
            print("      Password: Admin@123")
            print("      CNIC: 0000000000000")
        else:
            print("   ✓ Admin user already exists")
        
        # Step 5: Create sample voter (optional)
        print("\n📝 Creating sample voter...")
        sample_voter = User(
            name='Test Voter',
            cnic='1234567890123',
            email='voter@test.com',
            password_hash=hash_password('Voter@123'),
            is_admin=False
        )
        db.session.add(sample_voter)
        db.session.commit()
        print("   ✓ Sample voter created")
        print("      Email: voter@test.com")
        print("      Password: Voter@123")
        print("      CNIC: 1234567890123")
        
    print("\n" + "="*60)
    print("✅ MIGRATION COMPLETED SUCCESSFULLY")
    print("="*60)
    print("\n📋 Next Steps:")
    print("   1. Start the application: python app.py")
    print("   2. Login as admin to configure elections")
    print("   3. Add voters and set up election-specific eligibility")
    print("\n")
    
    return True


if __name__ == '__main__':
    print("""
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║     🗳️  Enhanced Voting System - Database Migration         ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
    """)
    
    app = create_app()
    
    print("\n📌 This script will:")
    print("   • Backup your existing database")
    print("   • Update schema to support CNIC-based identification")
    print("   • Add support for election-voter relationships")
    print("   • Add candidate application system")
    print("   • Create default admin and sample voter")
    
    print("\n⚠️  IMPORTANT: Make sure the application is not running!")
    
    input("\nPress Enter to continue or Ctrl+C to cancel...")
    
    success = migrate_database(app)
    
    if success:
        print("\n🎉 You can now start the enhanced voting system!")
    else:
        print("\n❌ Migration failed or was cancelled.")
