from flask import Blueprint, render_template, session, redirect, url_for, flash, current_app
from models import db, Candidate, Election, CandidateApplication
from utils import login_required

candidate = Blueprint('candidate', __name__, url_prefix='/candidate')


@candidate.route('/dashboard')
@login_required
def dashboard():
    """Candidate dashboard"""
    user_id = session.get('user_id')
    
    # Get all candidacies
    candidacies = Candidate.query.filter_by(user_id=user_id).all()
    
    # Get applications
    applications = CandidateApplication.query.filter_by(user_id=user_id)\
        .order_by(CandidateApplication.applied_at.desc()).all()
    
    current_app.logger.info(f'Candidate dashboard accessed by user {user_id}')
    
    return render_template('candidate/dashboard.html',
                          candidacies=candidacies,
                          applications=applications)


@candidate.route('/election/<int:election_id>/results')
@login_required
def view_results(election_id):
    """View results for election where user is a candidate"""
    user_id = session.get('user_id')
    
    # Verify user is candidate in this election
    candidacy = Candidate.query.filter_by(
        user_id=user_id,
        election_id=election_id
    ).first()
    
    if not candidacy:
        flash('You are not a candidate in this election', 'danger')
        return redirect(url_for('candidate.dashboard'))
    
    election = Election.query.get_or_404(election_id)
    
    # Only show results if election is closed
    if election.status != 'closed':
        flash('Results will be available after the election closes', 'info')
        return redirect(url_for('candidate.dashboard'))
    
    results = election.get_results()
    total_votes = election.get_total_votes()
    
    # Find candidate's position
    candidate_position = None
    candidate_result = None
    for idx, result in enumerate(results, 1):
        if result['candidate'].candidate_id == candidacy.candidate_id:
            candidate_position = idx
            candidate_result = result
            break
    
    current_app.logger.info(f'Candidate {user_id} viewed results for election {election_id}')
    
    return render_template('candidate/results.html',
                          election=election,
                          results=results,
                          total_votes=total_votes,
                          candidacy=candidacy,
                          candidate_position=candidate_position,
                          candidate_result=candidate_result)
