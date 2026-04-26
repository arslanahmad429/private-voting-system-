from flask import Blueprint, render_template, request, redirect, url_for, flash, session, current_app
from models import db, Election, Candidate, Vote, CandidateApplication, User
from utils import login_required, get_election_status
from datetime import datetime
from sqlalchemy.exc import IntegrityError
from werkzeug.utils import secure_filename
import os

voter = Blueprint('voter', __name__)

@voter.route('/dashboard')
@login_required
def dashboard():
    """Voter dashboard showing all elections with eligibility status"""
    user_id = session.get('user_id')
    user = User.query.get(user_id)
    
    # Get ALL elections (not deleted)
    all_elections = Election.query.filter_by(is_deleted=False).order_by(Election.created_at.desc()).all()
    
    # Update election statuses
    for election in all_elections:
        election.status = get_election_status(election)
    db.session.commit()
    
    # Get user's eligible election IDs
    eligible_election_ids = [e.election_id for e in user.eligible_elections]
    
    # Get user's voted elections
    voted_election_ids = [vote.election_id for vote in 
                          Vote.query.filter_by(user_id=user_id).all()]
    
    # Get user's applications
    applications = CandidateApplication.query.filter_by(user_id=user_id).all()
    application_dict = {app.election_id: app for app in applications}
    
    # Get user's candidacies
    from models import Candidate
    candidacies = Candidate.query.filter_by(user_id=user_id).all()
    candidacy_election_ids = [c.election_id for c in candidacies]
    
    return render_template('voter_dashboard.html', 
                          elections=all_elections,
                          eligible_election_ids=eligible_election_ids,
                          voted_election_ids=voted_election_ids,
                          applications=application_dict,
                          candidacy_election_ids=candidacy_election_ids)


@voter.route('/election/<int:election_id>')
@login_required
def election_details(election_id):
    """View election details and candidates - visible to all voters"""
    user_id = session.get('user_id')
    user = User.query.get(user_id)
    election = Election.query.get_or_404(election_id)
    
    # Check if user is eligible for THIS election
    is_eligible = user in election.eligible_voters
    
    # Update election status
    election.status = get_election_status(election)
    db.session.commit()
    
    # Check if user has already voted
    has_voted = Vote.query.filter_by(user_id=user_id, election_id=election_id).first() is not None
    
    # Get candidates
    candidates = Candidate.query.filter_by(election_id=election_id).all()
    
    # Check if user has applied
    application = CandidateApplication.query.filter_by(
        user_id=user_id,
        election_id=election_id
    ).first()
    
    return render_template('election_details.html',
                          election=election,
                          candidates=candidates,
                          has_voted=has_voted,
                          is_eligible=is_eligible,
                          application=application)


@voter.route('/vote', methods=['POST'])
@login_required
def cast_vote():
    """Cast a vote for a candidate"""
    user_id = session.get('user_id')
    user = User.query.get(user_id)
    candidate_id = request.form.get('candidate_id', type=int)
    election_id = request.form.get('election_id', type=int)
    
    if not candidate_id or not election_id:
        flash('Invalid vote submission', 'danger')
        return redirect(url_for('voter.dashboard'))
    
    # Verify election exists and is active
    election = Election.query.get_or_404(election_id)
    election.status = get_election_status(election)
    db.session.commit()
    
    if election.status != 'active':
        flash('This election is not currently active', 'warning')
        return redirect(url_for('voter.dashboard'))
    
    # Check if user is eligible
    if user not in election.eligible_voters:
        current_app.security_logger.warning(
            f'Unauthorized vote attempt by user {user_id} in election {election_id}'
        )
        flash('You are not eligible to vote in this election', 'danger')
        return redirect(url_for('voter.dashboard'))
    
    # Verify candidate belongs to this election
    candidate = Candidate.query.get_or_404(candidate_id)
    if candidate.election_id != election_id:
        flash('Invalid candidate selection', 'danger')
        return redirect(url_for('voter.dashboard'))
    
    # Record the vote - database constraint prevents duplicates
    try:
        new_vote = Vote(
            user_id=user_id,
            candidate_id=candidate_id,
            election_id=election_id,
            ip_address=request.remote_addr
        )
        db.session.add(new_vote)
        db.session.commit()
        
        current_app.logger.info(
            f'Vote recorded - User: {user_id}, Election: {election_id}, '
            f'Candidate: {candidate_id}, IP: {request.remote_addr}'
        )
        flash(f'Your vote for {candidate.name} has been recorded successfully!', 'success')
        
    except IntegrityError as e:
        db.session.rollback()
        if 'unique_vote_per_election' in str(e).lower():
            flash('You have already voted in this election', 'warning')
        else:
            current_app.logger.error(f'Vote integrity error: {str(e)}')
            flash('An error occurred while recording your vote. Please try again.', 'danger')
    
    except Exception as e:
        db.session.rollback()
        current_app.logger.error(f'Vote error: {str(e)}')
        flash('An error occurred while recording your vote. Please try again.', 'danger')
    
    return redirect(url_for('voter.election_details', election_id=election_id))


@voter.route('/election/<int:election_id>/apply', methods=['GET', 'POST'])
@login_required
def apply_as_candidate(election_id):
    """Apply to be a candidate in an election"""
    user_id = session.get('user_id')
    user = User.query.get(user_id)
    election = Election.query.get_or_404(election_id)
    
    # Check if user is eligible voter for this election
    if user not in election.eligible_voters:
        flash('You must be an eligible voter to apply as a candidate', 'danger')
        return redirect(url_for('voter.dashboard'))
    
    # Check election status
    if election.status != 'upcoming':
        flash('Applications are only accepted for upcoming elections', 'warning')
        return redirect(url_for('voter.dashboard'))
    
    # Check if already applied
    existing_application = CandidateApplication.query.filter_by(
        user_id=user_id,
        election_id=election_id
    ).first()
    
    if existing_application:
        flash('You have already applied for this election', 'info')
        return redirect(url_for('voter.view_application', application_id=existing_application.application_id))
    
    if request.method == 'POST':
        description = request.form.get('description', '').strip()
        
        if not description or len(description) < 10:
            flash('Please provide a description (at least 10 characters)', 'danger')
            return render_template('voter/apply_candidate.html', election=election, user=user)
        
        # Handle image upload
        image_path = 'default-candidate.png'
        if 'image' in request.files:
            file = request.files['image']
            if file and file.filename:
                from utils import allowed_file
                if allowed_file(file.filename, {'png', 'jpg', 'jpeg', 'gif'}):
                    filename = secure_filename(f"{user.cnic}_{election_id}_{file.filename}")
                    upload_folder = os.path.join('static', 'images', 'candidates')
                    os.makedirs(upload_folder, exist_ok=True)
                    file_path = os.path.join(upload_folder, filename)
                    file.save(file_path)
                    image_path = f"candidates/{filename}"
        
        try:
            application = CandidateApplication(
                user_id=user_id,
                election_id=election_id,
                description=description,
                image_path=image_path,
                status='pending'
            )
            
            db.session.add(application)
            db.session.commit()
            
            current_app.logger.info(
                f'Candidate application submitted - User: {user_id}, Election: {election_id}'
            )
            flash('Your application has been submitted for admin review', 'success')
            return redirect(url_for('voter.dashboard'))
            
        except IntegrityError:
            db.session.rollback()
            flash('You have already applied for this election', 'warning')
        except Exception as e:
            db.session.rollback()
            current_app.logger.error(f'Application error: {str(e)}')
            flash('An error occurred. Please try again.', 'danger')
    
    return render_template('voter/apply_candidate.html', election=election, user=user)


@voter.route('/applications/<int:application_id>')
@login_required
def view_application(application_id):
    """View application status"""
    application = CandidateApplication.query.get_or_404(application_id)
    
    # Security check
    if application.user_id != session.get('user_id'):
        flash('Access denied', 'danger')
        return redirect(url_for('voter.dashboard'))
    
    return render_template('voter/application_status.html', application=application)
