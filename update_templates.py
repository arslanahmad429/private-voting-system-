"""
Template Update Script
This script updates all existing templates to include CSRF tokens
"""

import os
import re

TEMPLATE_DIR = 'templates'

# Forms that need CSRF tokens
FORMS_TO_UPDATE = [
    'login.html',
    'register.html',
    'create_election.html',
    'edit_election.html',
    'election_details.html',
    'manage_candidates.html'
]

def add_csrf_token(content):
    """Add CSRF token after form tag if not already present"""
    
    # Check if already has CSRF token
    if 'csrf_token' in content:
        return content, False
    
    # Find all <form> tags
    form_pattern = r'(<form[^>]*>)'
    
    def add_token(match):
        form_tag = match.group(1)
        # Add CSRF token after form tag
        csrf = '\n    <input type="hidden" name="csrf_token" value="{{ csrf_token() }}"/>'
        return form_tag + csrf
    
    updated = re.sub(form_pattern, add_token, content)
    
    return updated, updated != content


def update_template(filepath):
    """Update a single template file"""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        updated_content, changed = add_csrf_token(content)
        
        if changed:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(updated_content)
            return True
        return False
        
    except Exception as e:
        print(f"Error updating {filepath}: {e}")
        return False


def main():
    """Update all templates"""
    print("="*60)
    print("TEMPLATE UPDATE SCRIPT")
    print("="*60)
    print("\nAdding CSRF tokens to forms...\n")
    
    updated_count = 0
    
    for filename in FORMS_TO_UPDATE:
        filepath = os.path.join(TEMPLATE_DIR, filename)
        
        if os.path.exists(filepath):
            if update_template(filepath):
                print(f"✓ Updated: {filename}")
                updated_count += 1
            else:
                print(f"○ Already has CSRF: {filename}")
        else:
            print(f"✗ Not found: {filename}")
    
    print(f"\n{updated_count} templates updated!")
    print("\n" + "="*60)


if __name__ == '__main__':
    main()
