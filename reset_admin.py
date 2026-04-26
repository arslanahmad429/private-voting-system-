"""
Admin Password Reset Utility
Resets admin password to default or custom password
"""

from app import create_app
from models import db, User
from werkzeug.security import generate_password_hash
import sys

def reset_admin_password(new_password=None):
    """Reset admin password"""
    app = create_app()
    
    with app.app_context():
        # Find admin user
        admin = User.query.filter_by(email='admin@votingsystem.com').first()
        
        if not admin:
            print("[ERROR] Admin user not found!")
            print("Creating admin user...")
            try:
                admin = User(
                    name='System Administrator',
                    cnic='0000000000000',
                    email='admin@votingsystem.com',
                    password_hash=generate_password_hash('Admin@123'),
                    is_admin=True,
                    is_deleted=False
                )
                db.session.add(admin)
                db.session.commit()
                print("[OK] Admin user created with default password: Admin@123")
                return True
            except Exception as e:
                print(f"[ERROR] Failed to create admin: {str(e)}")
                return False
        
        # Reset password
        if new_password:
            password = new_password
        else:
            password = 'Admin@123'
        
        try:
            admin.password_hash = generate_password_hash(password)
            db.session.commit()
            print("[OK] Admin password reset successfully!")
            print(f"[OK] Email: admin@votingsystem.com")
            print(f"[OK] Password: {password}")
            return True
        except Exception as e:
            db.session.rollback()
            print(f"[ERROR] Failed to reset password: {str(e)}")
            return False

if __name__ == '__main__':
    # Check if custom password provided
    if len(sys.argv) > 1:
        custom_password = sys.argv[1]
        reset_admin_password(custom_password)
    else:
        reset_admin_password()
