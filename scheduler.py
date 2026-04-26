from apscheduler.schedulers.background import BackgroundScheduler
from models import db, Election
from utils import get_election_status
import logging
import shutil
import os
from datetime import datetime

logger = logging.getLogger(__name__)


def update_election_statuses(app):
    """Update all election statuses based on current time"""
    with app.app_context():
        try:
            elections = Election.query.filter_by(is_deleted=False).all()
            updated_count = 0
            
            for election in elections:
                new_status = get_election_status(election)
                if election.status != new_status:
                    old_status = election.status
                    election.status = new_status
                    updated_count += 1
                    logger.info(
                        f'Election {election.election_id} status changed: '
                        f'{old_status} -> {new_status}'
                    )
            
            if updated_count > 0:
                db.session.commit()
                logger.info(f'Updated {updated_count} election statuses')
                
        except Exception as e:
            logger.error(f'Error updating election statuses: {str(e)}')
            db.session.rollback()


def backup_database(app):
    """Create daily database backup"""
    with app.app_context():
        try:
            db_uri = app.config['SQLALCHEMY_DATABASE_URI']
            
            # Only backup SQLite databases
            if not db_uri.startswith('sqlite:///'):
                logger.info('Skipping backup for non-SQLite database')
                return
            
            db_path = db_uri.replace('sqlite:///', '')
            if not os.path.exists(db_path):
                logger.warning(f'Database file not found: {db_path}')
                return
            
            # Create backup directory
            backup_dir = os.path.join(app.config.get('BASE_DIR', '.'), 'backups')
            os.makedirs(backup_dir, exist_ok=True)
            
            # Create backup with timestamp
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            backup_filename = f'voting_system_{timestamp}.db'
            backup_path = os.path.join(backup_dir, backup_filename)
            
            # Copy database file
            shutil.copy2(db_path, backup_path)
            logger.info(f'Database backed up to {backup_path}')
            
            # Keep only last 30 backups
            backups = sorted([
                f for f in os.listdir(backup_dir)
                if f.startswith('voting_system_') and f.endswith('.db')
            ])
            
            if len(backups) > 30:
                for old_backup in backups[:-30]:
                    old_path = os.path.join(backup_dir, old_backup)
                    os.remove(old_path)
                    logger.info(f'Removed old backup: {old_backup}')
                    
        except Exception as e:
            logger.error(f'Error backing up database: {str(e)}')


def init_scheduler(app):
    """Initialize background scheduler"""
    scheduler = BackgroundScheduler()
    
    # Update election statuses every minute
    scheduler.add_job(
        func=lambda: update_election_statuses(app),
        trigger="interval",
        minutes=1,
        id='update_elections',
        name='Update election statuses',
        replace_existing=True
    )
    
    # Daily database backup at 3 AM
    scheduler.add_job(
        func=lambda: backup_database(app),
        trigger="cron",
        hour=3,
        minute=0,
        id='backup_db',
        name='Backup database',
        replace_existing=True
    )
    
    scheduler.start()
    logger.info('Background scheduler started')
    app.logger.info('Background scheduler initialized')
    
    return scheduler
