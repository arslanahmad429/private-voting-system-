"""
Quick test to verify CSRF token generation
"""
from app import create_app

app = create_app()

with app.test_request_context():
    from flask import render_template_string
    
    # Test if csrf_token() works
    test_template = """
    <form method="POST">
        <input type="hidden" name="csrf_token" value="{{ csrf_token() }}"/>
        <button>Submit</button>
    </form>
    """
    
    result = render_template_string(test_template)
    print("="*60)
    print("CSRF Token Test")
    print("="*60)
    print(result)
    print("="*60)
    
    if 'csrf_token' in result and 'value=""' not in result:
        print("✅ CSRF token is being generated correctly")
    else:
        print("❌ CSRF token is NOT being generated")
        print("\nThis means Flask-WTF is not properly initialized.")
