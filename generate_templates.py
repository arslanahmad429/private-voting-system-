"""
Generate missing template placeholders
This creates simple working templates for all required routes
"""

import os

TEMPLATES = {
    'admin/manage_election_voters.html': '''{% extends "base.html" %}
{% block title %}Manage Election Voters{% endblock %}
{% block content %}
<div class="container py-6">
    <h1>Manage Eligible Voters - {{ election.title }}</h1>
    <p class="text-warning">⚠️ Voter list can only be edited before election starts</p>
    <form method="POST">
        <input type="hidden" name="csrf_token" value="{{ csrf_token() }}"/>
        <div class="card-glass p-6">
            {% for voter in all_voters %}
            <div class="form-check">
                <input class="form-check-input" type="checkbox" name="eligible_voters[]" value="{{ voter.user_id }}"
                       {% if voter.user_id in eligible_voter_ids %}checked{% endif %}>
                <label class="form-check-label">{{ voter.name }} ({{ voter.cnic|format_cnic }})</label>
            </div>
            {% endfor %}
        </div>
        <button type="submit" class="btn btn-primary mt-4">Update Eligible Voters</button>
        <a href="{{ url_for('admin.elections') }}" class="btn btn-secondary mt-4">Cancel</a>
    </form>
</div>
{% endblock %}''',

    'admin/manage_candidates.html': '''{% extends "base.html" %}
{% block title %}Manage Candidates{% endblock %}
{% block content %}
<div class="container py-6">
    <h1>Manage Candidates - {{ election.title }}</h1>
    <div class="card-glass p-6 mb-4">
        <h2 class="h4">Current Candidates ({{ candidates|length }})</h2>
        {% for candidate in candidates %}
        <div class="d-flex justify-content-between align-items-center p-3 border-bottom">
            <div>
                <strong>{{ candidate.name }}</strong> ({{ candidate.cnic|format_cnic }})
                <br><small class="text-gray">{{ candidate.description }}</small>
            </div>
            {% if election.status == 'upcoming' %}
            <form method="POST" action="{{ url_for('admin.delete_candidate', candidate_id=candidate.candidate_id) }}" style="display:inline;">
                <input type="hidden" name="csrf_token" value="{{ csrf_token() }}"/>
                <button type="submit" class="btn btn-sm btn-danger">Remove</button>
            </form>
            {% endif %}
        </div>
        {% endfor %}
    </div>
    {% if election.status == 'upcoming' %}
    <div class="card-glass p-6">
        <h2 class="h4">Add Candidate Manually</h2>
        <form method="POST">
            <input type="hidden" name="csrf_token" value="{{ csrf_token() }}"/>
            <div class="form-group">
                <select name="user_id" class="form-control" required>
                    <option value="">Select Voter...</option>
                    {% for voter in eligible_voters %}
                    <option value="{{ voter.user_id }}">{{ voter.name }} ({{ voter.cnic|format_cnic }})</option>
                    {% endfor %}
                </select>
            </div>
            <div class="form-group">
                <textarea name="description" class="form-control" placeholder="Candidate description..." required></textarea>
            </div>
            <button type="submit" class="btn btn-primary">Add Candidate</button>
        </form>
    </div>
    {% endif %}
</div>
{% endblock %}''',

    'admin/applications.html': '''{% extends "base.html" %}
{% block title %}Candidate Applications{% endblock %}
{% block content %}
<div class="container py-6">
    <h1>Candidate Applications - {{ election.title }}</h1>
    <div class="card-glass p-6">
        {% for app in applications %}
        <div class="card-glass p-4 mb-3">
            <div class="d-flex justify-content-between">
                <div>
                    <h3 class="h5">{{ app.applicant.name }}</h3>
                    <p class="text-gray">CNIC: {{ app.applicant.cnic|format_cnic }}</p>
                    <p>{{ app.description }}</p>
                    <small class="text-gray">Applied: {{ app.applied_at.strftime('%Y-%m-%d %H:%M') }}</small>
                </div>
                <div>
                    <span class="badge badge-{{ 'warning' if app.status == 'pending' else ('success' if app.status == 'approved' else 'danger') }}">
                        {{ app.status|upper }}
                    </span>
                </div>
            </div>
            {% if app.status == 'pending' %}
            <div class="mt-3">
                <form method="POST" action="{{ url_for('admin.approve_application', application_id=app.application_id) }}" style="display:inline;">
                    <input type="hidden" name="csrf_token" value="{{ csrf_token() }}"/>
                    <button type="submit" class="btn btn-success btn-sm">Approve</button>
                </form>
                <button class="btn btn-danger btn-sm" onclick="rejectApp({{ app.application_id }})">Reject</button>
            </div>
            {% endif %}
        </div>
        {% else %}
        <p class="text-gray">No applications yet.</p>
        {% endfor %}
    </div>
</div>
<script>
function rejectApp(id) {
    const reason = prompt('Rejection reason:');
    if (reason) {
        const form = document.createElement('form');
        form.method = 'POST';
        form.action = '/admin/applications/' + id + '/reject';
        const csrf = document.createElement('input');
        csrf.type = 'hidden';
        csrf.name = 'csrf_token';
        csrf.value = '{{ csrf_token() }}';
        const reasonInput = document.createElement('input');
        reasonInput.type = 'hidden';
        reasonInput.name = 'reason';
        reasonInput.value = reason;
        form.appendChild(csrf);
        form.appendChild(reasonInput);
        document.body.appendChild(form);
        form.submit();
    }
}
</script>
{% endblock %}''',

    'admin/all_applications.html': '''{% extends "base.html" %}
{% block title %}All Applications{% endblock %}
{% block content %}
<div class="container py-6">
    <h1>All Candidate Applications</h1>
    <div class="mb-4">
        <a href="?status=all" class="btn btn-sm {% if status_filter == 'all' %}btn-primary{% else %}btn-outline{% endif %}">All</a>
        <a href="?status=pending" class="btn btn-sm {% if status_filter == 'pending' %}btn-warning{% else %}btn-outline{% endif %}">Pending</a>
        <a href="?status=approved" class="btn btn-sm {% if status_filter == 'approved' %}btn-success{% else %}btn-outline{% endif %}">Approved</a>
        <a href="?status=rejected" class="btn btn-sm {% if status_filter == 'rejected' %}btn-danger{% else %}btn-outline{% endif %}">Rejected</a>
    </div>
    <div class="card-glass p-6">
        {% for app in applications %}
        <div class="border-bottom p-3">
            <strong>{{ app.applicant.name }}</strong> for <em>{{ app.election.title }}</em>
            <span class="badge badge-{{ 'warning' if app.status == 'pending' else ('success' if app.status == 'approved' else 'danger') }}">{{ app.status }}</span>
        </div>
        {% else %}
        <p>No applications found.</p>
        {% endfor %}
    </div>
</div>
{% endblock %}''',

    'admin/results.html': '''{% extends "base.html" %}
{% block title %}Results - {{ election.title }}{% endblock %}
{% block content %}
<div class="container py-6">
    <h1>{{ election.title }} - Results</h1>
    <p class="lead">Total Votes: {{ total_votes }}</p>
    <div class="card-glass p-6">
        {% for result in results %}
        <div class="p-4 border-bottom">
            <h3 class="h4">{{ loop.index }}. {{ result.candidate.name }}</h3>
            <div class="progress" style="height: 30px;">
                <div class="progress-bar bg-success" style="width: {{ result.percentage }}%;">
                    {{ result.votes }} votes ({{ "%.1f"|format(result.percentage) }}%)
                </div>
            </div>
        </div>
        {% endfor %}
    </div>
</div>
{% endblock %}''',

    'voter/apply_candidate.html': '''{% extends "base.html" %}
{% block title %}Apply as Candidate{% endblock %}
{% block content %}
<div class="container-sm py-6">
    <div class="card-glass p-8" style="max-width: 600px; margin: 0 auto;">
        <h1 class="h2 mb-4">Apply as Candidate</h1>
        <h2 class="h4 mb-6">{{ election.title }}</h2>
        <form method="POST" enctype="multipart/form-data">
            <input type="hidden" name="csrf_token" value="{{ csrf_token() }}"/>
            <div class="form-group">
                <label>Your Name</label>
                <input type="text" class="form-control" value="{{ user.name }}" disabled>
            </div>
            <div class="form-group">
                <label>Your CNIC</label>
                <input type="text" class="form-control" value="{{ user.cnic|format_cnic }}" disabled>
            </div>
            <div class="form-group">
                <label for="description">Why should voters choose you?</label>
                <textarea id="description" name="description" class="form-control" rows="5" required></textarea>
            </div>
            <div class="form-group">
                <label for="image">Photo (optional)</label>
                <input type="file" id="image" name="image" class="form-control" accept="image/*">
            </div>
            <button type="submit" class="btn btn-primary btn-lg">Submit Application</button>
            <a href="{{ url_for('voter.dashboard') }}" class="btn btn-secondary">Cancel</a>
        </form>
    </div>
</div>
{% endblock %}''',

    'voter/application_status.html': '''{% extends "base.html" %}
{% block title %}Application Status{% endblock %}
{% block content %}
<div class="container-sm py-6">
    <div class="card-glass p-8" style="max-width: 600px; margin: 0 auto;">
        <h1 class="h2 mb-6">Application Status</h1>
        <div class="alert alert-{{ 'warning' if application.status == 'pending' else ('success' if application.status == 'approved' else 'danger') }}">
            Status: {{ application.status|upper }}
        </div>
        <p><strong>Election:</strong> {{ application.election.title }}</p>
        <p><strong>Applied:</strong> {{ application.applied_at.strftime('%Y-%m-%d %H:%M') }}</p>
        <p><strong>Description:</strong></p>
        <p>{{ application.description }}</p>
        {% if application.status == 'rejected' %}
        <div class="alert alert-danger">
            <strong>Rejection Reason:</strong> {{ application.rejection_reason }}
        </div>
        {% endif %}
        <a href="{{ url_for('voter.dashboard') }}" class="btn btn-primary">Back to Dashboard</a>
    </div>
</div>
{% endblock %}''',

    'candidate/dashboard.html': '''{% extends "base.html" %}
{% block title %}Candidate Portal{% endblock %}
{% block content %}
<div class="container py-6">
    <h1 class="mb-6">Candidate Portal</h1>
    <div class="card-glass p-6 mb-4">
        <h2 class="h3">My Candidacies</h2>
        {% for candidacy in candidacies %}
        <div class="border-bottom p-3">
            <h3 class="h5">{{ candidacy.election.title }}</h3>
            <p class="text-gray">{{ candidacy.description }}</p>
            {% if candidacy.election.status == 'closed' %}
            <a href="{{ url_for('candidate.view_results', election_id=candidacy.election_id) }}" class="btn btn-sm btn-success">View Results</a>
            {% else %}
            <span class="badge badge-{{ 'success' if candidacy.election.status == 'active' else 'warning' }}">{{ candidacy.election.status }}</span>
            {% endif %}
        </div>
        {% else %}
        <p class="text-gray">You are not a candidate in any elections yet.</p>
        {% endfor %}
    </div>
    <div class="card-glass p-6">
        <h2 class="h3">My Applications</h2>
        {% for app in applications %}
        <div class="border-bottom p-3">
            <strong>{{ app.election.title }}</strong>
            <span class="badge badge-{{ 'warning' if app.status == 'pending' else ('success' if app.status == 'approved' else 'danger') }}">{{ app.status }}</span>
        </div>
        {% else %}
        <p class="text-gray">No applications submitted.</p>
        {% endfor %}
    </div>
</div>
{% endblock %}''',

    'candidate/results.html': '''{% extends "base.html" %}
{% block title %}Candidate Results{% endblock %}
{% block content %}
<div class="container py-6">
    <h1>{{ election.title }} - Results</h1>
    <div class="card-glass p-6 mb-4">
        <h2 class="h4">Your Performance</h2>
        <p><strong>Position:</strong> #{{ candidate_position }}</p>
        <p><strong>Votes Received:</strong> {{ candidate_result.votes }}</p>
        <p><strong>Vote Share:</strong> {{ "%.1f"|format(candidate_result.percentage) }}%</p>
    </div>
    <div class="card-glass p-6">
        <h2 class="h4">All Results</h2>
        {% for result in results %}
        <div class="p-3 border-bottom {% if result.candidate.candidate_id == candidacy.candidate_id %}bg-light{% endif %}">
            <strong>{{ loop.index }}. {{ result.candidate.name }}</strong>: {{ result.votes }} votes ({{ "%.1f"|format(result.percentage) }}%)
        </div>
        {% endfor %}
    </div>
</div>
{% endblock %}'''
}


def create_templates():
    """Create all missing templates"""
    print("="*60)
    print("TEMPLATE GENERATOR")
    print("="*60)
    print(f"\nCreating {len(TEMPLATES)} templates...\n")
    
    created = 0
    skipped = 0
    
    for template_path, content in TEMPLATES.items():
        full_path = os.path.join('templates', template_path)
        dir_path = os.path.dirname(full_path)
        
        # Create directory if needed
        os.makedirs(dir_path, exist_ok=True)
        
        # Create file if doesn't exist
        if not os.path.exists(full_path):
            with open(full_path, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"✓ Created: {template_path}")
            created += 1
        else:
            print(f"○ Exists: {template_path}")
            skipped += 1
    
    print(f"\n{created} templates created, {skipped} already existed.")
    print("="*60)


if __name__ == '__main__':
    create_templates()
