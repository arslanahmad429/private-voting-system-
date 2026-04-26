from flask import Flask, render_template, redirect, url_for, session, jsonify
from config import Config
from models import db, User
from utils import bcrypt, hash_password
from logging_config import setup_logging
from scheduler import init_scheduler
from flask_wtf.csrf import CSRFProtect
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address
from datetime import datetime
import os
import atexit

def create_app():
    """Application factory"""
    app = Flask(__name__)
    app.config.from_object(Config)
    
    # Initialize extensions
    db.init_app(app)
    bcrypt.init_app(app)
    
    # CSRF Protection
    csrf = CSRFProtect(app)
    
    # Rate Limiting
    limiter = Limiter(
        app=app,
        key_func=get_remote_address,
        default_limits=["200 per day", "50 per hour"],
        storage_uri="memory://"
    )
    app.limiter = limiter
    
    # Setup logging
    security_logger = setup_logging(app)
    app.security_logger = security_logger
    
    # Register blueprints
    from routes.auth import auth
    from routes.voter import voter
    from routes.admin import admin
    from routes.results import results
    from routes.candidate import candidate
    
    app.register_blueprint(auth)
    app.register_blueprint(voter)
    app.register_blueprint(admin)
    app.register_blueprint(results)
    app.register_blueprint(candidate)
    
    # Create database tables and initialize scheduler
    with app.app_context():
        # Create instance folder if it doesn't exist
        os.makedirs(os.path.join(app.config['BASE_DIR'], 'instance'), exist_ok=True)
        
        # Create upload folders
        os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)
        
        # Create logs and backups folders
        os.makedirs(os.path.join(app.config['BASE_DIR'], 'logs'), exist_ok=True)
        os.makedirs(os.path.join(app.config['BASE_DIR'], 'backups'), exist_ok=True)
        
        # Create all database tables
        db.create_all()
        
        # Create default admin user if not exists (only if tables are set up)
        try:
            if not User.query.filter_by(email='admin@votingsystem.com').first():
                admin_user = User(
                    name='System Admin',
                    cnic='0000000000000',  # Default admin CNIC
                    email='admin@votingsystem.com',
                    password_hash=hash_password('Admin@123'),
                    is_admin=True
                )
                db.session.add(admin_user)
                db.session.commit()
                print("✅ Default admin user created")
                print("   Email: admin@votingsystem.com")
                print("   Password: Admin@123")
                print("   CNIC: 0000000000000")
        except Exception:
            # Tables may not exist yet (during migration)
            pass
    
    # Initialize background scheduler
    scheduler = init_scheduler(app)
    atexit.register(lambda: scheduler.shutdown())
    
    # Routes
    @app.route('/')
    def index():
        """Landing page"""
        if 'user_id' in session:
            if session.get('is_admin'):
                return redirect(url_for('admin.dashboard'))
            # Check if user has candidacies
            from models import Candidate
            user_candidacies = Candidate.query.filter_by(user_id=session.get('user_id')).count()
            if user_candidacies > 0:
                # Show option to go to candidate portal or voter dashboard
                return render_template('index.html', has_candidacies=True)
            return redirect(url_for('voter.dashboard'))
        return render_template('index.html')
    
    # Health check endpoint
    @app.route('/health')
    def health_check():
        """Health check endpoint for monitoring"""
        health_status = {
            'status': 'healthy',
            'timestamp': datetime.now().isoformat(),
            'checks': {}
        }
        
        try:
            # Check database
            db.session.execute(db.text('SELECT 1'))
            health_status['checks']['database'] = 'ok'
        except Exception as e:
            health_status['status'] = 'unhealthy'
            health_status['checks']['database'] = f'error: {str(e)}'
        
        # Check scheduler
        try:
            if scheduler.running:
                health_status['checks']['scheduler'] = 'ok'
            else:
                health_status['checks']['scheduler'] = 'stopped'
        except:
            health_status['checks']['scheduler'] = 'error'
        
        status_code = 200 if health_status['status'] == 'healthy' else 503
        return jsonify(health_status), status_code
    
    # Error handlers
    @app.errorhandler(404)
    def not_found(error):
        return render_template('404.html'), 404
    
    @app.errorhandler(500)
    def internal_error(error):
        db.session.rollback()
        app.logger.error(f'Internal error: {str(error)}')
        return render_template('500.html'), 500
    
    @app.errorhandler(429)
    def ratelimit_error(error):
        return render_template('429.html'), 429
    
    # Custom Jinja2 filters
    @app.template_filter('format_cnic')
    def format_cnic_filter(cnic):
        from utils import format_cnic
        return format_cnic(cnic)
    
    @app.template_filter('format_datetime')
    def format_datetime_filter(dt):
        from utils import format_datetime
        return format_datetime(dt)
    
    return app


if __name__ == '__main__':
    app = create_app()
    print("\n" + "="*60)
    print("🗳️  ENHANCED ONLINE VOTING SYSTEM")
    print("="*60)
    print("🌐 Server: http://127.0.0.1:5000")
    print("\n📋 Default Admin Credentials:")
    print("   📧 Email: admin@votingsystem.com")
    print("   🔑 Password: Admin@123")
    print("   🆔 CNIC: 0000000000000")
    print("\n✨ New Features:")
    print("   ✅ CNIC-based voter identification")
    print("   ✅ Candidate portal")
    print("   ✅ Voter applications for candidacy")
    print("   ✅ Admin-controlled voter lists")
    print("   ✅ Enhanced security (CSRF, Rate Limiting)")
    print("   ✅ Automated status updates & backups")
    print("\n" + "="*60 + "\n")
    app.run(debug=True, host='0.0.0.0', port=5000)
