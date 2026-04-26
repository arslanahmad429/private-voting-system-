from flask import Blueprint, render_template, flash, redirect, url_for
from models import Election
from utils import get_election_status

results = Blueprint('results', __name__)

@results.route('/results/<int:election_id>')
def view_results(election_id):
    """View public election results (ONLY for closed elections)"""
    election = Election.query.get_or_404(election_id)
    
    # Update election status
    election.status = get_election_status(election)
    
    # STRICT: Only show results for CLOSED elections
    if election.status != 'closed':
        flash('Results are only available after the election closes', 'warning')
        return redirect(url_for('index'))
    
    results_data = election.get_results()
    total_votes = election.get_total_votes()
    
    return render_template('results.html',
                          election=election,
                          results=results_data,
                          total_votes=total_votes)
