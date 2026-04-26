from functools import wraps
from flask import session, redirect, url_for, flash
from flask_bcrypt import Bcrypt
import re

bcrypt = Bcrypt()

def login_required(f):
    """Decorator to require login for routes"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'user_id' not in session:
            flash('Please log in to access this page.', 'warning')
            return redirect(url_for('auth.login'))
        return f(*args, **kwargs)
    return decorated_function


def admin_required(f):
    """Decorator to require admin privileges for routes"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'user_id' not in session:
            flash('Please log in to access this page.', 'warning')
            return redirect(url_for('auth.login'))
        if not session.get('is_admin', False):
            flash('Access denied. Admin privileges required.', 'danger')
            return redirect(url_for('voter.dashboard'))
        return f(*args, **kwargs)
    return decorated_function


def hash_password(password):
    """Hash a password using bcrypt"""
    return bcrypt.generate_password_hash(password).decode('utf-8')


def verify_password(password_hash, password):
    """Verify a password against its hash"""
    return bcrypt.check_password_hash(password_hash, password)


def validate_email(email):
    """Validate email format"""
    pattern = r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$'
    return re.match(pattern, email) is not None


def validate_password_strength(password):
    """
    Validate password strength
    Returns (bool, str) - (is_valid, message)
    """
    if len(password) < 8:
        return False, "Password must be at least 8 characters long"
    if not re.search(r'[A-Z]', password):
        return False, "Password must contain at least one uppercase letter"
    if not re.search(r'[a-z]', password):
        return False, "Password must contain at least one lowercase letter"
    if not re.search(r'\d', password):
        return False, "Password must contain at least one number"
    return True, "Password is strong"


def allowed_file(filename, allowed_extensions):
    """Check if file extension is allowed"""
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in allowed_extensions


def format_datetime(dt):
    """Format datetime for display"""
    if dt is None:
        return ""
    return dt.strftime("%B %d, %Y at %I:%M %p")


def get_election_status(election):
    """Determine current status of an election based on dates"""
    from datetime import datetime
    now = datetime.now()
    
    if now < election.start_date:
        return 'upcoming'
    elif now > election.end_date:
        return 'closed'
    else:
        return 'active'


def validate_cnic(cnic):
    """
    Validate CNIC format (13 digits, optionally with dashes)
    Accepts formats: 1234567890123 or 12345-6789012-3
    """
    if not cnic:
        return False
    
    # Remove dashes if present
    cnic_clean = cnic.replace('-', '').replace(' ', '')
    
    # Check if exactly 13 digits
    if not re.match(r'^\d{13}$', cnic_clean):
        return False
    
    return True


def format_cnic(cnic):
    """
    Format CNIC as XXXXX-XXXXXXX-X for display
    Handles input with or without dashes
    """
    if not cnic:
        return ""
    
    cnic_clean = cnic.replace('-', '').replace(' ', '')
    
    if len(cnic_clean) == 13:
        return f"{cnic_clean[:5]}-{cnic_clean[5:12]}-{cnic_clean[12]}"
    
    return cnic


def clean_cnic(cnic):
    """
    Clean CNIC by removing dashes and spaces
    Returns 13-digit string
    """
    if not cnic:
        return ""
    
    return cnic.replace('-', '').replace(' ', '')


def allowed_file(filename, allowed_extensions):
    """Check if file extension is allowed"""
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in allowed_extensions

