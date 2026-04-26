"""
Database Cleanup Script - Remove All Elections
This script removes ALL elections and related data from the database
"""

from app import create_app
from models import db, Election, Candidate, Vote, CandidateApplication, election_voters

app = create_app()

with app.app_context():
    print("=" * 60)
    print("DATABASE CLEANUP - REMOVE ALL ELECTIONS")
    print("=" * 60)
    
    # Find all elections
    all_elections = Election.query.all()
    
    print(f"\nFound {len(all_elections)} election(s) in database:\n")
    
    for i, election in enumerate(all_elections, 1):
        print(f"{i}. Title: {election.title}")
        print(f"   Status: {election.status}")
        print(f"   Created: {election.created_at.strftime('%Y-%m-%d')}")
        print(f"   Candidates: {len(election.candidates)}")
        print(f"   Total Votes: {election.get_total_votes()}")
        print()
    
    if not all_elections:
        print("✅ No elections found in database!")
        print("Database is already clean!")
        print("=" * 60)
        exit()
    
    # Confirmation
    print("\n⚠️  WARNING: This will PERMANENTLY delete ALL elections!")
    print("This includes:")
    print("  - Elections")
    print("  - All candidates")
    print("  - All votes")
    print("  - All candidate applications")
    print("  - All eligible voter associations")
    print()
    
    choice = input("Do you want to delete ALL elections above? (yes/no): ").strip().lower()
    
    if choice != 'yes':
        print("\n❌ Operation cancelled.")
        exit()
    
    # Delete all elections and related data
    try:
        deleted_elections = 0
        deleted_candidates = 0
        deleted_votes = 0
        deleted_applications = 0
        
        for election in all_elections:
            election_title = election.title
            election_id = election.election_id
            
            print(f"\nProcessing: {election_title}...")
            
            # 1. Delete all votes for this election
            votes_count = Vote.query.filter_by(election_id=election_id).count()
            Vote.query.filter_by(election_id=election_id).delete()
            deleted_votes += votes_count
            print(f"  ✓ Deleted {votes_count} vote(s)")
            
            # 2. Delete all candidate applications
            apps_count = CandidateApplication.query.filter_by(election_id=election_id).count()
            CandidateApplication.query.filter_by(election_id=election_id).delete()
            deleted_applications += apps_count
            print(f"  ✓ Deleted {apps_count} application(s)")
            
            # 3. Delete all candidates
            candidates_count = Candidate.query.filter_by(election_id=election_id).count()
            Candidate.query.filter_by(election_id=election_id).delete()
            deleted_candidates += candidates_count
            print(f"  ✓ Deleted {candidates_count} candidate(s)")
            
            # 4. Remove eligible voter associations
            db.session.execute(
                election_voters.delete().where(
                    election_voters.c.election_id == election_id
                )
            )
            print(f"  ✓ Removed eligible voter associations")
            
            # 5. Delete the election itself
            db.session.delete(election)
            deleted_elections += 1
            print(f"  ✓ Deleted election: {election_title}")
        
        # Commit all changes
        db.session.commit()
        
        print()
        print("=" * 60)
        print("✅ CLEANUP COMPLETE!")
        print("=" * 60)
        print(f"Deleted:")
        print(f"  • {deleted_elections} election(s)")
        print(f"  • {deleted_candidates} candidate(s)")
        print(f"  • {deleted_votes} vote(s)")
        print(f"  • {deleted_applications} application(s)")
        print()
        print("✅ Database is now clean and ready for production!")
        print("=" * 60)
        
    except Exception as e:
        db.session.rollback()
        print(f"\n❌ Error: {str(e)}")
        print("Database rolled back - no changes made.")
