from flask import Blueprint, render_template, request, redirect, url_for, flash, session, current_app
from models import db, User
from utils import hash_password, verify_password, validate_email, validate_password_strength, validate_cnic, clean_cnic
from sqlalchemy.exc import IntegrityError

auth = Blueprint('auth', __name__)

@auth.route('/register', methods=['GET', 'POST'])
def register():
    """Registration disabled - only admin can create voters"""
    flash('Public registration is disabled. Only administrators can create voter accounts.', 'warning')
    flash('Please contact your administrator to register as a voter.', 'info')
    return redirect(url_for('auth.login'))


@auth.route('/login', methods=['GET', 'POST'])
def login():
    """User login with CNIC (voters) or email (admin)"""
    if 'user_id' in session:
        if session.get('is_admin'):
            return redirect(url_for('admin.dashboard'))
        return redirect(url_for('voter.dashboard'))
    
    if request.method == 'POST':
        identifier = request.form.get('identifier', '').strip()
        password = request.form.get('password', '')
        
        if not identifier or not password:
            flash('Please provide both CNIC/Email and password', 'danger')
            return render_template('login.html', identifier=identifier)
        
        # Determine if identifier is email or CNIC
        user = None
        is_email_login = '@' in identifier
        
        # Check if it's an email (contains @)
        if is_email_login:
            user = User.query.filter_by(email=identifier.lower()).first()
            
            # ENFORCE: Email login only for admins
            if user and not user.is_admin:
                flash('Voters must login using CNIC, not email. Please use your 13-digit CNIC.', 'warning')
                current_app.security_logger.warning(f'Voter attempted email login: {identifier}')
                return render_template('login.html')
        else:
            # Treat as CNIC
            cnic_clean = clean_cnic(identifier)
            if len(cnic_clean) == 13:
                user = User.query.filter_by(cnic=cnic_clean).first()
            else:
                flash('Invalid CNIC format. CNIC must be 13 digits (e.g., 12345-6789012-3)', 'danger')
                return render_template('login.html', identifier=identifier)
        
        # Check if user exists
        if not user:
            if is_email_login:
                flash('Admin email not found. Please check your credentials.', 'danger')
            else:
                flash('You are not a registered voter. Please contact your administrator to register.', 'warning')
            
            current_app.security_logger.warning(f'Failed login attempt for: {identifier}')
            return render_template('login.html', identifier=identifier)
        
        # Check if account is deleted/banned
        if user.is_deleted:
            flash('This account has been banned. Please contact the administrator.', 'danger')
            current_app.security_logger.warning(f'Banned user attempted login: {identifier}')
            return render_template('login.html', identifier=identifier)
        
        # Verify password
        if not verify_password(user.password_hash, password):
            flash('Invalid password', 'danger')
            current_app.security_logger.warning(f'Invalid password attempt for user: {user.email}')
            return render_template('login.html', identifier=identifier)
        
        # Successful login - set session
        session.permanent = True
        session['user_id'] = user.user_id
        session['user_name'] = user.name
        session['user_cnic'] = user.cnic
        session['user_email'] = user.email
        session['is_admin'] = user.is_admin
        
        current_app.security_logger.info(f'Successful login: {user.email} (CNIC: {user.cnic})')
        
        flash(f'Welcome back, {user.name}!', 'success')
        
        if user.is_admin:
            return redirect(url_for('admin.dashboard'))
        return redirect(url_for('voter.dashboard'))
    
    return render_template('login.html')


@auth.route('/logout')
def logout():
    """User logout - clear session and prevent caching"""
    user_email = session.get('user_email', 'Unknown')
    
    # Clear all session data
    session.clear()
    
    current_app.security_logger.info(f'User logged out: {user_email}')
    
    flash('You have been logged out successfully.', 'success')
    
    # Create response and set no-cache headers
    response = redirect(url_for('auth.login'))
    response.headers['Cache-Control'] = 'no-store, no-cache, must-revalidate, post-check=0, pre-check=0, max-age=0'
    response.headers['Pragma'] = 'no-cache'
    response.headers['Expires'] = '-1'
    
    return response
