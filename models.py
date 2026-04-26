from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
from sqlalchemy.orm import validates

db = SQLAlchemy()

# Association table for election-voter many-to-many relationship
election_voters = db.Table('election_voters',
    db.Column('id', db.Integer, primary_key=True),
    db.Column('election_id', db.Integer, db.ForeignKey('elections.election_id'), nullable=False),
    db.Column('user_id', db.Integer, db.ForeignKey('users.user_id'), nullable=False),
    db.Column('added_at', db.DateTime, default=datetime.now),
    db.UniqueConstraint('election_id', 'user_id', name='unique_voter_per_election')
)

class User(db.Model):
    """User model for voters and administrators"""
    __tablename__ = 'users'
    
    user_id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    cnic = db.Column(db.String(15), unique=True, nullable=False, index=True)
    email = db.Column(db.String(120), unique=True, nullable=False)
    password_hash = db.Column(db.String(200), nullable=False)
    is_admin = db.Column(db.Boolean, default=False)
    is_deleted = db.Column(db.Boolean, default=False)
    created_at = db.Column(db.DateTime, default=datetime.now)
    
    # Relationships
    votes = db.relationship('Vote', backref='voter', lazy=True, cascade='all, delete-orphan')
    eligible_elections = db.relationship(
        'Election',
        secondary=election_voters,
        back_populates='eligible_voters'
    )
    candidate_applications = db.relationship('CandidateApplication', foreign_keys='CandidateApplication.user_id', backref='applicant', lazy=True)
    candidacies = db.relationship('Candidate', foreign_keys='Candidate.user_id', backref='user', lazy=True)
    
    def __repr__(self):
        return f'<User {self.email}>'
    
    def has_voted_in_election(self, election_id):
        """Check if user has already voted in a specific election"""
        return Vote.query.filter_by(user_id=self.user_id, election_id=election_id).first() is not None
    
    def is_eligible_for_election(self, election_id):
        """Check if user is eligible to vote in an election"""
        election = Election.query.get(election_id)
        return election and self in election.eligible_voters
    
    def has_applied_for_election(self, election_id):
        """Check if user has applied as candidate for an election"""
        return CandidateApplication.query.filter_by(
            user_id=self.user_id,
            election_id=election_id
        ).first() is not None


class Election(db.Model):
    """Election model"""
    __tablename__ = 'elections'
    
    election_id = db.Column(db.Integer, primary_key=True)
    title = db.Column(db.String(200), nullable=False)
    description = db.Column(db.Text)
    status = db.Column(db.String(20), default='upcoming')  # upcoming, active, closed
    start_date = db.Column(db.DateTime, nullable=False)
    end_date = db.Column(db.DateTime, nullable=False)
    created_at = db.Column(db.DateTime, default=datetime.now)
    is_deleted = db.Column(db.Boolean, default=False)
    deleted_at = db.Column(db.DateTime, nullable=True)
    deleted_by = db.Column(db.Integer, db.ForeignKey('users.user_id'), nullable=True)
    
    # Relationships
    candidates = db.relationship('Candidate', backref='election', lazy=True, cascade='all, delete-orphan')
    votes = db.relationship('Vote', backref='election', lazy=True, cascade='all, delete-orphan')
    eligible_voters = db.relationship(
        'User',
        secondary=election_voters,
        back_populates='eligible_elections'
    )
    candidate_applications = db.relationship('CandidateApplication', foreign_keys='CandidateApplication.election_id', backref='election', lazy=True)
    
    def __repr__(self):
        return f'<Election {self.title}>'
    
    @validates('start_date', 'end_date')
    def validate_dates(self, key, value):
        """Validate election dates"""
        if key == 'end_date':
            if hasattr(self, 'start_date') and self.start_date:
                if value <= self.start_date:
                    raise ValueError("End date must be after start date")
        return value
    
    def get_total_votes(self):
        """Get total number of votes cast in this election"""
        return Vote.query.filter_by(election_id=self.election_id).count()
    
    def get_results(self):
        """Get election results as a list of candidates with vote counts"""
        results = []
        for candidate in self.candidates:
            vote_count = Vote.query.filter_by(candidate_id=candidate.candidate_id).count()
            results.append({
                'candidate': candidate,
                'votes': vote_count,
                'percentage': (vote_count / self.get_total_votes() * 100) if self.get_total_votes() > 0 else 0
            })
        # Sort by votes (descending)
        results.sort(key=lambda x: x['votes'], reverse=True)
        return results
    
    def can_edit_participants(self):
        """Check if voter/candidate list can be edited"""
        return self.status == 'upcoming'
    
    def can_delete(self):
        """Check if election can be deleted"""
        if self.get_total_votes() > 0:
            return False, "Cannot delete election with existing votes"
        if self.status == 'active':
            return False, "Cannot delete active election"
        return True, "Election can be deleted"
    
    def get_eligible_voter_count(self):
        """Get count of eligible voters"""
        return len(self.eligible_voters)
    
    def get_pending_applications_count(self):
        """Get count of pending candidate applications"""
        return CandidateApplication.query.filter_by(
            election_id=self.election_id,
            status='pending'
        ).count()


class Candidate(db.Model):
    """Candidate model"""
    __tablename__ = 'candidates'
    
    candidate_id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey('users.user_id'), nullable=False)
    cnic = db.Column(db.String(15), nullable=False)
    name = db.Column(db.String(100), nullable=False)
    description = db.Column(db.Text)
    election_id = db.Column(db.Integer, db.ForeignKey('elections.election_id'), nullable=False, index=True)
    image_path = db.Column(db.String(200), default='default-candidate.png')
    application_id = db.Column(db.Integer, db.ForeignKey('candidate_applications.application_id'), nullable=True)
    created_at = db.Column(db.DateTime, default=datetime.now)
    
    # Relationships
    votes = db.relationship('Vote', backref='candidate', lazy=True, cascade='all, delete-orphan')
    
    def __repr__(self):
        return f'<Candidate {self.name}>'
    
    def get_vote_count(self):
        """Get number of votes for this candidate"""
        return Vote.query.filter_by(candidate_id=self.candidate_id).count()
    
    def get_vote_percentage(self):
        """Get percentage of votes"""
        total = self.election.get_total_votes()
        if total == 0:
            return 0
        return (self.get_vote_count() / total) * 100


class CandidateApplication(db.Model):
    """Candidate applications from voters"""
    __tablename__ = 'candidate_applications'
    
    application_id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey('users.user_id'), nullable=False)
    election_id = db.Column(db.Integer, db.ForeignKey('elections.election_id'), nullable=False)
    description = db.Column(db.Text)
    image_path = db.Column(db.String(200), default='default-candidate.png')
    status = db.Column(db.String(20), default='pending')  # pending, approved, rejected
    applied_at = db.Column(db.DateTime, default=datetime.now)
    reviewed_at = db.Column(db.DateTime)
    reviewed_by = db.Column(db.Integer, db.ForeignKey('users.user_id'))
    rejection_reason = db.Column(db.Text)
    
    # Relationship to the candidate created from this application
    created_candidate = db.relationship('Candidate', backref='application', uselist=False)
    
    __table_args__ = (
        db.UniqueConstraint('user_id', 'election_id', name='unique_application_per_election'),
        db.Index('idx_application_status', 'status'),
        db.Index('idx_application_election', 'election_id'),
    )
    
    def __repr__(self):
        return f'<CandidateApplication {self.user_id} for Election {self.election_id}>'


class Vote(db.Model):
    """Vote model to record all votes"""
    __tablename__ = 'votes'
    
    vote_id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey('users.user_id'), nullable=False, index=True)
    candidate_id = db.Column(db.Integer, db.ForeignKey('candidates.candidate_id'), nullable=False, index=True)
    election_id = db.Column(db.Integer, db.ForeignKey('elections.election_id'), nullable=False, index=True)
    voted_at = db.Column(db.DateTime, default=datetime.now)
    ip_address = db.Column(db.String(50))  # For logging purposes
    
    # Unique constraint: one vote per user per election
    __table_args__ = (
        db.UniqueConstraint('user_id', 'election_id', name='unique_vote_per_election'),
        db.Index('idx_vote_election_candidate', 'election_id', 'candidate_id'),
    )
    
    def __repr__(self):
        return f'<Vote by User {self.user_id} for Candidate {self.candidate_id}>'
