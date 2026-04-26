"""
Database Cleanup Script - Remove Test Voters
This script removes all test voters from the database
"""

from app import create_app
from models import db, User, Vote, Candidate, CandidateApplication, election_voters

app = create_app()

with app.app_context():
    print("=" * 60)
    print("DATABASE CLEANUP - REMOVE TEST VOTERS")
    print("=" * 60)
    
    # Find all non-admin voters (potential test voters)
    all_voters = User.query.filter_by(is_admin=False).all()
    
    print(f"\nFound {len(all_voters)} voter(s) in database:\n")
    
    for i, voter in enumerate(all_voters, 1):
        print(f"{i}. Name: {voter.name}")
        print(f"   CNIC: {voter.cnic}")
        print(f"   Email: {voter.email}")
        print(f"   Banned: {voter.is_deleted}")
        print()
    
    if not all_voters:
        print("✅ No voters found in database!")
        print("=" * 60)
        exit()
    
    # Confirmation
    print("\n⚠️  WARNING: This will PERMANENTLY delete all selected voters!")
    print("This includes:")
    print("  - Voter profiles")
    print("  - Their votes (vote integrity maintained)")
    print("  - Their candidate applications")
    print("  - Their candidacies")
    print()
    
    choice = input("Do you want to delete ALL voters above? (yes/no): ").strip().lower()
    
    if choice != 'yes':
        print("\n❌ Operation cancelled.")
        exit()
    
    # Delete all voters
    try:
        deleted_count = 0
        
        for voter in all_voters:
            voter_name = voter.name
            voter_id = voter.user_id
            
            # Delete related records first
            # 1. Delete votes
            Vote.query.filter_by(user_id=voter_id).delete()
            
            # 2. Delete candidate applications
            CandidateApplication.query.filter_by(user_id=voter_id).delete()
            
            # 3. Delete candidacies
            Candidate.query.filter_by(user_id=voter_id).delete()
            
            # 4. Remove from election_voters association
            db.session.execute(
                election_voters.delete().where(
                    election_voters.c.user_id == voter_id
                )
            )
            
            # 5. Delete the voter
            db.session.delete(voter)
            
            deleted_count += 1
            print(f"✅ Deleted: {voter_name} (CNIC: {voter.cnic})")
        
        # Commit all changes
        db.session.commit()
        
        print()
        print("=" * 60)
        print(f"✅ Successfully deleted {deleted_count} voter(s)!")
        print("=" * 60)
        
    except Exception as e:
        db.session.rollback()
        print(f"\n❌ Error: {str(e)}")
        print("Database rolled back - no changes made.")
