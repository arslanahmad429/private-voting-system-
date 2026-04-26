import logging
import os
from logging.handlers import RotatingFileHandler, TimedRotatingFileHandler


def setup_logging(app):
    """Configure application logging"""
    
    # Create logs directory
    log_dir = os.path.join(app.config.get('BASE_DIR', '.'), 'logs')
    os.makedirs(log_dir, exist_ok=True)
    
    # Application log
    app_log_file = os.path.join(log_dir, 'app.log')
    app_handler = RotatingFileHandler(
        app_log_file,
        maxBytes=10 * 1024 * 1024,  # 10MB
        backupCount=10
    )
    app_handler.setFormatter(logging.Formatter(
        '[%(asctime)s] %(levelname)s in %(module)s: %(message)s'
    ))
    app_handler.setLevel(logging.INFO)
    
    # Security audit log
    security_log_file = os.path.join(log_dir, 'security.log')
    security_handler = TimedRotatingFileHandler(
        security_log_file,
        when='midnight',
        interval=1,
        backupCount=90  # Keep 90 days
    )
    security_handler.setFormatter(logging.Formatter(
        '[%(asctime)s] %(levelname)s: %(message)s'
    ))
    security_handler.setLevel(logging.WARNING)
    
    # Error log
    error_log_file = os.path.join(log_dir, 'error.log')
    error_handler = RotatingFileHandler(
        error_log_file,
        maxBytes=10 * 1024 * 1024,
        backupCount=10
    )
    error_handler.setFormatter(logging.Formatter(
        '[%(asctime)s] %(levelname)s in %(pathname)s:%(lineno)d: %(message)s\n'
        'Exception: %(exc_info)s'
    ))
    error_handler.setLevel(logging.ERROR)
    
    # Add handlers
    if not app.debug:
        app.logger.addHandler(app_handler)
        app.logger.addHandler(security_handler)
        app.logger.addHandler(error_handler)
        app.logger.setLevel(logging.INFO)
    
    # Create security logger
    security_logger = logging.getLogger('security')
    security_logger.addHandler(security_handler)
    security_logger.setLevel(logging.WARNING)
    
    app.logger.info('Logging system initialized')
    
    return security_logger
