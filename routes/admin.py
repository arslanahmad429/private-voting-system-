from flask import Blueprint, render_template, request, redirect, url_for, flash, session, jsonify, current_app
from models import db, User, Election, Candidate, Vote, CandidateApplication, election_voters
from utils import admin_required, format_datetime, validate_cnic, clean_cnic, get_election_status
from datetime import datetime
from werkzeug.utils import secure_filename
from sqlalchemy.exc import IntegrityError
from sqlalchemy import or_
import os
from werkzeug.security import generate_password_hash, check_password_hash

admin = Blueprint('admin', __name__, url_prefix='/admin')

@admin.route('/dashboard')
@admin_required
def dashboard():
    """Admin dashboard with statistics"""
    total_voters = User.query.filter_by(is_admin=False, is_deleted=False).count()
    total_elections = Election.query.filter_by(is_deleted=False).count()
    total_votes = Vote.query.count()
    active_elections = Election.query.filter_by(status='active', is_deleted=False).count()
    
    recent_elections = Election.query.filter_by(is_deleted=False)\
        .order_by(Election.created_at.desc()).limit(5).all()
    
    for election in recent_elections:
        election.status = get_election_status(election)
    db.session.commit()
    
    return render_template('admin_dashboard.html',
                         total_voters=total_voters,
                         total_elections=total_elections,
                         total_votes=total_votes,
                         active_elections=active_elections,
                         recent_elections=recent_elections)


@admin.route('/change-password', methods=['GET', 'POST'])
@admin_required
def change_password():
    """Change admin password"""
    if request.method == 'POST':
        current_password = request.form.get('current_password', '')
        new_password = request.form.get('new_password', '')
        confirm_password = request.form.get('confirm_password', '')
        
        errors = []
        
        # Get current admin user
        admin_user = User.query.get(session.get('user_id'))
        
        # Verify current password
        if not check_password_hash(admin_user.password_hash, current_password):
            errors.append('Current password is incorrect')
        
        # Validate new password
        if len(new_password) < 8:
            errors.append('New password must be at least 8 characters')
        
        if new_password != confirm_password:
            errors.append('New passwords do not match')
        
        if current_password == new_password:
            errors.append('New password must be different from current password')
        
        if errors:
            for error in errors:
                flash(error, 'danger')
            return render_template('admin/change_password.html')
        
        try:
            # Update password
            admin_user.password_hash = generate_password_hash(new_password)
            db.session.commit()
            
            current_app.security_logger.warning(
                f'Admin password changed for user {admin_user.email} (ID: {admin_user.user_id})'
            )
            
            flash('Password changed successfully! Please login again.', 'success')
            
            # Clear session and redirect to login
            session.clear()
            return redirect(url_for('auth.login'))
            
        except Exception as e:
            db.session.rollback()
            current_app.logger.error(f'Error changing password: {str(e)}')
            flash('An error occurred while changing password', 'danger')
    
    return render_template('admin/change_password.html')


# ========== VOTER MANAGEMENT ==========

@admin.route('/voters')
@admin_required
def manage_voters():
    """List and manage all voters (including banned)"""
    search_query = request.args.get('search', '').strip()
    
    # Show all voters including banned (is_deleted can be True or False)
    query = User.query.filter_by(is_admin=False)
    
    if search_query:
        search_pattern = f'%{search_query}%'
        query = query.filter(
            or_(
                User.name.ilike(search_pattern),
                User.cnic.like(search_pattern),
                User.email.ilike(search_pattern)
            )
        )
    
    voters = query.order_by(User.is_deleted.asc(), User.created_at.desc()).all()
    
    return render_template('admin/manage_voters.html', voters=voters, search_query=search_query)


@admin.route('/voters/add', methods=['GET', 'POST'])
@admin_required
def add_voter():
    """Add new voter"""
    if request.method == 'POST':
        name = request.form.get('name', '').strip()
        cnic = request.form.get('cnic', '').strip()
        email = request.form.get('email', '').strip().lower()
        password = request.form.get('password', '')
        
        errors = []
        
        if not name or len(name) < 2:
            errors.append('Name must be at least 2 characters')
        
        if not validate_cnic(cnic):
            errors.append('Invalid CNIC format')
        
        cnic_clean = clean_cnic(cnic)
        
        if User.query.filter_by(cnic=cnic_clean).first():
            errors.append('CNIC already exists')
        
        if User.query.filter_by(email=email).first():
            errors.append('Email already exists')
        
        if not password or len(password) < 8:
            errors.append('Password must be at least 8 characters')
        
        if errors:
            for error in errors:
                flash(error, 'danger')
            return render_template('admin/add_voter.html', name=name, cnic=cnic, email=email)
        
        try:
            from utils import hash_password
            new_voter = User(
                name=name,
                cnic=cnic_clean,
                email=email,
                password_hash=hash_password(password),
                is_admin=False
            )
            db.session.add(new_voter)
            db.session.commit()
            
            current_app.logger.info(f'Voter added by admin: {email}, CNIC: {cnic_clean}')
            flash(f'Voter {name} added successfully', 'success')
            return redirect(url_for('admin.manage_voters'))
            
        except IntegrityError:
            db.session.rollback()
            flash('CNIC or email already exists', 'danger')
        except Exception as e:
            db.session.rollback()
            current_app.logger.error(f'Error adding voter: {str(e)}')
            flash('An error occurred', 'danger')
    
    return render_template('admin/add_voter.html')


@admin.route('/voters/<int:user_id>/edit', methods=['GET', 'POST'])
@admin_required
def edit_voter(user_id):
    """Edit voter information and reset password if needed"""
    voter = User.query.get_or_404(user_id)
    
    if voter.is_admin:
        flash('Cannot edit admin users through this interface', 'danger')
        return redirect(url_for('admin.manage_voters'))
    
    if request.method == 'POST':
        name = request.form.get('name', '').strip()
        email = request.form.get('email', '').strip().lower()
        new_password = request.form.get('new_password', '').strip()
        
        errors = []
        
        if not name or len(name) < 2:
            errors.append('Name must be at least 2 characters')
        
        if not email or '@' not in email:
            errors.append('Valid email is required')
        
        # Check if email already exists for another user
        existing_email = User.query.filter(
            User.email == email,
            User.user_id != user_id
        ).first()
        if existing_email:
            errors.append('This email is already registered to another user')
        
        # Validate new password if provided
        if new_password:
            if len(new_password) < 6:
                errors.append('Password must be at least 6 characters')
        
        if errors:
            for error in errors:
                flash(error, 'danger')
            return render_template('admin/edit_voter.html', voter=voter)
        
        try:
            voter.name = name
            voter.email = email
            
            # Update password if provided
            if new_password:
                from werkzeug.security import generate_password_hash
                voter.password_hash = generate_password_hash(new_password)
                current_app.security_logger.warning(
                    f'Password reset for voter {voter.cnic} by admin {session.get("user_id")}'
                )
                flash(f'Password updated for {voter.name}', 'info')
            
            db.session.commit()
            
            current_app.logger.info(f'Voter updated: {voter.cnic}')
            flash(f'Voter profile updated successfully!', 'success')
            return redirect(url_for('admin.manage_voters'))
            
        except Exception as e:
            db.session.rollback()
            current_app.logger.error(f'Error updating voter: {str(e)}')
            flash('An error occurred while updating the voter', 'danger')
    
    return render_template('admin/edit_voter.html', voter=voter)


@admin.route('/voters/<int:user_id>/ban', methods=['POST'])
@admin_required
def ban_voter(user_id):
    """Ban voter (soft delete)"""
    voter = User.query.get_or_404(user_id)
    
    if voter.is_admin:
        flash('Cannot ban admin users', 'danger')
        return redirect(url_for('admin.manage_voters'))
    
    if voter.is_deleted:
        flash('Voter is already banned', 'warning')
        return redirect(url_for('admin.manage_voters'))
    
    try:
        voter.is_deleted = True
        voter.deleted_at = datetime.now()
        voter.deleted_by = session.get('user_id')
        db.session.commit()
        
        current_app.security_logger.warning(f'Voter banned: {voter.email} (CNIC: {voter.cnic}) by admin {session.get("user_id")}')
        flash(f'Voter {voter.name} has been banned', 'success')
    except Exception as e:
        db.session.rollback()
        flash('An error occurred while banning the voter', 'danger')
    
    return redirect(url_for('admin.manage_voters'))


@admin.route('/voters/<int:user_id>/unban', methods=['POST'])
@admin_required
def unban_voter(user_id):
    """Unban voter (restore deleted account)"""
    voter = User.query.get_or_404(user_id)
    
    if voter.is_admin:
        flash('Admin users cannot be banned/unbanned', 'danger')
        return redirect(url_for('admin.manage_voters'))
    
    if not voter.is_deleted:
        flash('Voter is not banned', 'warning')
        return redirect(url_for('admin.manage_voters'))
    
    try:
        voter.is_deleted = False
        voter.deleted_at = None
        voter.deleted_by = None
        db.session.commit()
        
        current_app.security_logger.info(f'Voter unbanned: {voter.email} (CNIC: {voter.cnic}) by admin {session.get("user_id")}')
        flash(f'Voter {voter.name} has been unbanned', 'success')
    except Exception as e:
        db.session.rollback()
        flash('An error occurred while unbanning the voter', 'danger')
    
    return redirect(url_for('admin.manage_voters'))


# ========== ELECTION MANAGEMENT ==========

@admin.route('/elections')
@admin_required
def elections():
    """List all elections"""
    elections = Election.query.filter_by(is_deleted=False)\
        .order_by(Election.created_at.desc()).all()
    return render_template('admin/elections.html', elections=elections)


@admin.route('/elections/create', methods=['GET', 'POST'])
@admin_required
def create_election():
    """Create a new election with voter selection"""
    if request.method == 'POST':
        title = request.form.get('title', '').strip()
        description = request.form.get('description', '').strip()
        start_date_str = request.form.get('start_date')
        end_date_str = request.form.get('end_date')
        selected_voter_ids = request.form.getlist('eligible_voters[]')
        
        # Validation
        errors = []
        
        if not title or len(title) < 3:
            errors.append('Title must be at least 3 characters long')
        
        if not selected_voter_ids:
            errors.append('Must select at least one eligible voter')
        
        try:
            start_date = datetime.strptime(start_date_str, '%Y-%m-%dT%H:%M')
            end_date = datetime.strptime(end_date_str, '%Y-%m-%dT%H:%M')
            
            if start_date >= end_date:
                errors.append('End date must be after start date')
            
        except (ValueError, TypeError):
            errors.append('Invalid date format')
            start_date = end_date = None
        
        if errors:
            for error in errors:
                flash(error, 'danger')
            all_voters = User.query.filter_by(is_admin=False, is_deleted=False).all()
            return render_template('admin/create_election.html',
                                 title=title,
                                 description=description,
                                 voters=all_voters,
                                 selected_voter_ids=[int(id) for id in selected_voter_ids])
        
        # Create election
        try:
            # Determine initial status
            now = datetime.now()
            if now < start_date:
                status = 'upcoming'
            elif now > end_date:
                status = 'closed'
            else:
                status = 'active'
            
            new_election = Election(
                title=title,
                description=description,
                start_date=start_date,
                end_date=end_date,
                status=status
            )
            db.session.add(new_election)
            db.session.flush()  # Get election ID
            
            # Add eligible voters
            for voter_id in selected_voter_ids:
                stmt = election_voters.insert().values(
                    election_id=new_election.election_id,
                    user_id=int(voter_id)
                )
                db.session.execute(stmt)
            
            db.session.commit()
            
            current_app.logger.info(
                f'Election created: {title} with {len(selected_voter_ids)} eligible voters'
            )
            flash(f'Election "{title}" created successfully!', 'success')
            return redirect(url_for('admin.elections'))
        
        except Exception as e:
            db.session.rollback()
            current_app.logger.error(f'Error creating election: {str(e)}')
            flash('An error occurred while creating the election', 'danger')
    
    # GET request
    all_voters = User.query.filter_by(is_admin=False, is_deleted=False)\
        .order_by(User.name).all()
    return render_template('admin/create_election.html', voters=all_voters)


@admin.route('/elections/<int:election_id>/edit', methods=['GET', 'POST'])
@admin_required
def edit_election(election_id):
    """Edit an existing election"""
    election = Election.query.get_or_404(election_id)
    
    if request.method == 'POST':
        title = request.form.get('title', '').strip()
        description = request.form.get('description', '').strip()
        start_date_str = request.form.get('start_date')
        end_date_str = request.form.get('end_date')
        
        errors = []
        
        if not title or len(title) < 3:
            errors.append('Title must be at least 3 characters long')
        
        try:
            start_date = datetime.strptime(start_date_str, '%Y-%m-%dT%H:%M')
            end_date = datetime.strptime(end_date_str, '%Y-%m-%dT%H:%M')
            
            if start_date >= end_date:
                errors.append('End date must be after start date')
        
        except (ValueError, TypeError):
            errors.append('Invalid date format')
            start_date = end_date = None
        
        if errors:
            for error in errors:
                flash(error, 'danger')
            return render_template('admin/edit_election.html', election=election)
        
        try:
            election.title = title
            election.description = description
            election.start_date = start_date
            election.end_date = end_date
            db.session.commit()
            
            current_app.logger.info(f'Election updated: {title}')
            flash(f'Election "{title}" updated successfully!', 'success')
            return redirect(url_for('admin.elections'))
        
        except Exception as e:
            db.session.rollback()
            current_app.logger.error(f'Error updating election: {str(e)}')
            flash('An error occurred while updating the election', 'danger')
    
    return render_template('admin/edit_election.html', election=election)


@admin.route('/elections/<int:election_id>/voters', methods=['GET', 'POST'])
@admin_required
def manage_election_voters(election_id):
    """Manage eligible voters for an election"""
    election = Election.query.get_or_404(election_id)
    
    # Cannot edit after election starts
    if election.status != 'upcoming':
        flash('Cannot modify voter list after election has started', 'warning')
        return redirect(url_for('admin.elections'))
    
    if request.method == 'POST':
        selected_voter_ids = request.form.getlist('eligible_voters[]')
        
        if not selected_voter_ids:
            flash('Must select at least one eligible voter', 'danger')
        else:
            try:
                # Clear existing
                db.session.execute(
                    election_voters.delete().where(
                        election_voters.c.election_id == election_id
                    )
                )
                
                # Add new
                for voter_id in selected_voter_ids:
                    stmt = election_voters.insert().values(
                        election_id=election_id,
                        user_id=int(voter_id)
                    )
                    db.session.execute(stmt)
                
                db.session.commit()
                
                current_app.logger.info(
                    f'Voter list updated for election {election_id}: {len(selected_voter_ids)} voters'
                )
                flash('Eligible voters updated successfully', 'success')
                return redirect(url_for('admin.elections'))
                
            except Exception as e:
                db.session.rollback()
                current_app.logger.error(f'Error updating voters: {str(e)}')
                flash('An error occurred', 'danger')
    
    all_voters = User.query.filter_by(is_admin=False, is_deleted=False)\
        .order_by(User.name).all()
    eligible_voter_ids = [v.user_id for v in election.eligible_voters]
    
    return render_template('admin/manage_election_voters.html',
                          election=election,
                          all_voters=all_voters,
                          eligible_voter_ids=eligible_voter_ids)


@admin.route('/elections/<int:election_id>/delete', methods=['POST'])
@admin_required
def delete_election(election_id):
    """Soft delete an election"""
    election = Election.query.get_or_404(election_id)
    
    # Prevent deletion if votes exist
    if election.get_total_votes() > 0:
        flash('Cannot delete election with existing votes', 'warning')
        return redirect(url_for('admin.elections'))
    
    if election.status == 'active':
        flash('Cannot delete an active election', 'danger')
        return redirect(url_for('admin.elections'))
    
    try:
        election.is_deleted = True
        election.deleted_at = datetime.now()
        election.deleted_by = session.get('user_id')
        db.session.commit()
        
        current_app.security_logger.warning(
            f'Election deleted: {election.title} by admin {session.get("user_id")}'
        )
        flash(f'Election "{election.title}" has been archived', 'success')
    except Exception as e:
        db.session.rollback()
        flash('An error occurred', 'danger')
    
    return redirect(url_for('admin.elections'))


# ========== CANDIDATE APPLICATION MANAGEMENT ==========

@admin.route('/applications')
@admin_required
def view_all_applications():
    """View all candidate applications"""
    status_filter = request.args.get('status', 'all')
    
    query = CandidateApplication.query
    
    if status_filter == 'pending':
        query = query.filter_by(status='pending')
    elif status_filter == 'approved':
        query = query.filter_by(status='approved')
    elif status_filter == 'rejected':
        query = query.filter_by(status='rejected')
    
    applications = query.order_by(CandidateApplication.applied_at.desc()).all()
    
    return render_template('admin/all_applications.html',
                          applications=applications,
                          status_filter=status_filter)


@admin.route('/elections/<int:election_id>/applications')
@admin_required
def view_applications(election_id):
    """View candidate applications for an election"""
    election = Election.query.get_or_404(election_id)
    applications = CandidateApplication.query.filter_by(election_id=election_id)\
        .order_by(CandidateApplication.applied_at.desc()).all()
    
    return render_template('admin/applications.html',
                          election=election,
                          applications=applications)


@admin.route('/applications/<int:application_id>/approve', methods=['POST'])
@admin_required
def approve_application(application_id):
    """Approve candidate application"""
    application = CandidateApplication.query.get_or_404(application_id)
    election = application.election
    
    # Cannot approve after election starts
    if election.status != 'upcoming':
        flash('Cannot approve applications after election has started', 'warning')
        return redirect(request.referrer or url_for('admin.dashboard'))
    
    try:
        # Create candidate from application
        candidate = Candidate(
            user_id=application.user_id,
            cnic=application.applicant.cnic,
            name=application.applicant.name,
            description=application.description,
            election_id=application.election_id,
            image_path=application.image_path,
            application_id=application.application_id
        )
        
        application.status = 'approved'
        application.reviewed_at = datetime.now()
        application.reviewed_by = session.get('user_id')
        
        db.session.add(candidate)
        db.session.commit()
        
        current_app.logger.info(
            f'Application approved: {application_id} by admin {session.get("user_id")}'
        )
        flash(f'Application from {application.applicant.name} approved', 'success')
        
    except Exception as e:
        db.session.rollback()
        current_app.logger.error(f'Error approving application: {str(e)}')
        flash('An error occurred', 'danger')
    
    return redirect(request.referrer or url_for('admin.dashboard'))


@admin.route('/applications/<int:application_id>/reject', methods=['POST'])
@admin_required
def reject_application(application_id):
    """Reject candidate application"""
    application = CandidateApplication.query.get_or_404(application_id)
    reason = request.form.get('reason', '').strip()
    
    try:
        application.status = 'rejected'
        application.reviewed_at = datetime.now()
        application.reviewed_by = session.get('user_id')
        application.rejection_reason = reason
        
        db.session.commit()
        
        current_app.security_logger.warning(
            f'Application rejected: {application_id}, Reason: {reason}'
        )
        flash('Application rejected', 'info')
        
    except Exception as e:
        db.session.rollback()
        flash('An error occurred', 'danger')
    
    return redirect(request.referrer or url_for('admin.dashboard'))


# ========== CANDIDATE MANAGEMENT ==========

@admin.route('/elections/<int:election_id>/candidates', methods=['GET', 'POST'])
@admin_required
def manage_candidates(election_id):
    """Manage candidates for an election"""
    election = Election.query.get_or_404(election_id)
    
    # Cannot add candidates after election starts
    if election.status != 'upcoming' and request.method == 'POST':
        flash('Cannot modify candidates after election has started', 'warning')
        return redirect(url_for('admin.manage_candidates', election_id=election_id))
    
    if request.method == 'POST':
        # Manual candidate addition (admin can add directly)
        user_id = request.form.get('user_id', type=int)
        description = request.form.get('description', '').strip()
        
        if not user_id:
            flash('Must select a voter', 'danger')
        else:
            user = User.query.get(user_id)
            
            # Check if user is eligible voter
            if user not in election.eligible_voters:
                flash('Selected user is not an eligible voter for this election', 'danger')
            # Check if already a candidate
            elif Candidate.query.filter_by(user_id=user_id, election_id=election_id).first():
                flash('User is already a candidate', 'warning')
            else:
                try:
                    candidate = Candidate(
                        user_id=user_id,
                        cnic=user.cnic,
                        name=user.name,
                        description=description,
                        election_id=election_id
                    )
                    db.session.add(candidate)
                    db.session.commit()
                    
                    current_app.logger.info(f'Candidate manually added: {user.name} to election {election_id}')
                    flash(f'{user.name} added as candidate', 'success')
                except Exception as e:
                    db.session.rollback()
                    flash('An error occurred', 'danger')
    
    candidates = Candidate.query.filter_by(election_id=election_id).all()
    eligible_voters = election.eligible_voters
    
    return render_template('admin/manage_candidates.html',
                          election=election,
                          candidates=candidates,
                          eligible_voters=eligible_voters)


@admin.route('/candidates/<int:candidate_id>/delete', methods=['POST'])
@admin_required
def delete_candidate(candidate_id):
    """Delete a candidate"""
    candidate = Candidate.query.get_or_404(candidate_id)
    election_id = candidate.election_id
    election = candidate.election
    
    # Cannot delete after election starts
    if election.status != 'upcoming':
        flash('Cannot remove candidates after election has started', 'warning')
        return redirect(url_for('admin.manage_candidates', election_id=election_id))
    
    try:
        db.session.delete(candidate)
        db.session.commit()
        
        current_app.logger.info(f'Candidate deleted: {candidate.name}')
        flash(f'Candidate "{candidate.name}" removed successfully', 'success')
    except Exception as e:
        db.session.rollback()
        flash('An error occurred', 'danger')
    
    return redirect(url_for('admin.manage_candidates', election_id=election_id))


# ========== RESULTS ==========

@admin.route('/results/<int:election_id>')
@admin_required
def view_results(election_id):
    """View detailed election results"""
    election = Election.query.get_or_404(election_id)
    
    # Only show results if election is closed (configurable)
    from flask import current_app
    if current_app.config.get('RESULTS_VISIBILITY') == 'after_close':
        if election.status != 'closed':
            flash('Results viewing is disabled during active elections', 'info')
            return redirect(url_for('admin.elections'))
    
    results = election.get_results()
    total_votes = election.get_total_votes()
    
    return render_template('admin/results.html',
                          election=election,
                          results=results,
                          total_votes=total_votes)
