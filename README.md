# 🗳️ Online Voting System

A secure, web-based electronic voting platform built with Flask, featuring real-time results, candidate management, and comprehensive admin controls.

---

## 📖 Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Technology Stack](#technology-stack)
- [System Architecture](#system-architecture)
- [User Roles & Permissions](#user-roles--permissions)
- [Workflow](#workflow)
- [Security Features](#security-features)
- [Installation](#installation)
- [Screenshots](#screenshots)
- [Contributing](#contributing)
- [License](#license)

---

## 🌟 Overview

The Online Voting System is a modern, secure web application designed to facilitate democratic elections in organizations, institutions, and communities. It provides a complete voting infrastructure from voter registration to result publication, all through an intuitive web interface.

### Why This System?

- ✅ **Secure**: CSRF protection, rate limiting, session validation, password hashing
- ✅ **User-Friendly**: Clean, modern UI with responsive design
- ✅ **Transparent**: All voters see all elections with clear eligibility indicators
- ✅ **Flexible**: Support for multiple simultaneous elections
- ✅ **Auditable**: Comprehensive logging of all security events
- ✅ **Complete**: Full candidate application and approval workflow

---

## 🎯 Key Features

### For Administrators

#### Election Management
- ✅ **Create Elections** - Set title, description, start/end dates
- ✅ **Edit Elections** - Modify details before election starts
- ✅ **Delete Elections** - Soft delete with complete data preservation
- ✅ **Manage Eligible Voters** - Control who can vote in each election
- ✅ **View Results** - Real-time vote counts and percentages
- ✅ **Election Status** - Automatic status updates (Upcoming → Active → Closed)

#### Voter Management
- ✅ **Add Voters** - Register voters with CNIC, name, email
- ✅ **Edit Voter Profiles** - Update name, email
- ✅ **Reset Passwords** - Help voters who forgot passwords
- ✅ **Ban/Unban Voters** - Suspend compromised accounts
- ✅ **View Voter List** - See all registered voters with status

#### Candidate Management
- ✅ **Add Candidates Directly** - Nominate candidates as admin
- ✅ **Review Applications** - Approve/reject candidate applications
- ✅ **Remove Candidates** - Remove from upcoming elections
- ✅ **View Candidate Details** - See manifesto and votes

#### Security & Monitoring
- ✅ **Change Admin Password** - Secure password management
- ✅ **View Security Logs** - Monitor login attempts and bans
- ✅ **Session Management** - Automatic session validation
- ✅ **failed Login Tracking** - Rate limiting on authentication

### For Voters

#### Election Participation
- ✅ **View All Elections** - See all elections (with eligibility badges)
- ✅ **Vote in Active Elections** - Cast votes for eligible elections only
- ✅ **Apply as Candidate** - Submit candidacy applications
- ✅ **View Results** - See results after election closes
- ✅ **Track Application Status** - Monitor candidacy application

#### Account Management
- ✅ **Secure Login** - CNIC-based authentication
- ✅ **Session Security** - Auto-logout on suspicious activity
- ✅ **Profile Viewing** - See own CNIC and email

### Candidate Features

- ✅ **Application System** - Submit manifesto and apply
- ✅ **Status Tracking** - See application status (Pending/Approved/Rejected)
- ✅ **Manifesto Display** - Voters see candidate platforms

---

## 💻 Technology Stack

### Backend
- **Framework**: Flask 3.0.0
- **Database**: SQLAlchemy with SQLite (easily switchable to PostgreSQL/MySQL)
- **Authentication**: Werkzeug password hashing + custom decorators
- **Security**: Flask-WTF (CSRF), Flask-Limiter (Rate limiting)
- **Scheduling**: APScheduler for automatic status updates

### Frontend
- **HTML5**: Semantic markup
- **CSS3**: Custom design system with utilities
- **Styling**: Modern glassmorphism, gradients, animations
- **Responsive**: Mobile-first design with breakpoints
- **Icons**: Emoji + Unicode symbols

### Security
- **Password Hashing**: Bcrypt (Werkzeug)
- **CSRF Protection**: Flask-WTF
- **Rate Limiting**: Flask-Limiter
- **Session Security**: HTTP-only cookies, secure headers
- **Logging**: Separate app and security logs

---

## 🏗️ System Architecture

### Database Schema

```
┌─────────────┐
│    User     │
├─────────────┤
│ user_id (PK)│
│ name        │
│ cnic        │◄────────┐
│ email       │         │
│ password    │         │
│ is_admin    │         │
│ is_deleted  │         │
└─────────────┘         │
       │                │
       │                │
       ├────────────────┼────────────┐
       │                │            │
       ▼                │            ▼
┌─────────────┐         │     ┌─────────────┐
│    Vote     │         │     │ Candidate   │
├─────────────┤         │     ├─────────────┤
│ vote_id(PK) │         │     │candidate_id │
│ user_id(FK) │         │     │ user_id(FK) │
│election_id  │         │     │election_id  │
│candidate_id │         │     │ manifesto   │
│ voted_at    │         │     └─────────────┘
└─────────────┘         │            │
       │                │            │
       │                │            │
       │         ┌──────┴──────┐    │
       │         │  election_  │    │
       │         │   voters    │◄───┤
       │         │(Association)│    │
       │         └─────────────┘    │
       │                             │
       ▼                             ▼
┌──────────────────────────────────────┐
│            Election                  │
├──────────────────────────────────────┤
│ election_id (PK)                     │
│ title                                │
│ description                          │
│ start_date                           │
│ end_date                             │
│ status (upcoming/active/closed)      │
│ created_at                           │
│ is_deleted                           │
└──────────────────────────────────────┘
       ▲
       │
       │
┌─────────────────────┐
│CandidateApplication │
├─────────────────────┤
│ application_id (PK) │
│ user_id (FK)        │
│ election_id (FK)    │
│ manifesto           │
│ status              │
│ applied_at          │
└─────────────────────┘
```

### Application Flow

```
┌──────────────┐
│   Browser    │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  Flask App   │
│   (app.py)   │
└──────┬───────┘
       │
       ├─────► Authentication (routes/auth.py)
       │        ├─ Login (CNIC for voters, email for admin)
       │        ├─ Logout (session clear + no-cache)
       │        └─ Session validation
       │
       ├─────► Admin (routes/admin.py)
       │        ├─ Dashboard
       │        ├─ Election CRUD
       │        ├─ Voter management
       │        ├─ Candidate management
       │        ├─ Application review
       │        └─ Password change
       │
       ├─────► Voter (routes/voter.py)
       │        ├─ Dashboard (all elections)
       │        ├─ Election details
       │        ├─ Cast vote
       │        └─ Apply as candidate
       │
       └─────► Results (routes/results.py)
                └─ View election results
```

---

## 👥 User Roles & Permissions

### Administrator

**Full System Control**

| Feature | Permission |
|---------|-----------|
| Create/Edit/Delete Elections | ✅ Full Access |
| Add/Edit/Ban Voters | ✅ Full Access |
| Add/Remove Candidates | ✅ Full Access |
| Review Applications | ✅ Full Access |
| View All Results | ✅ Full Access |
| Access Security Logs | ✅ Full Access |
| Reset Voter Passwords | ✅ Full Access |
| Change Own Password | ✅ Full Access |
| Vote in Elections | ❌ No |

**Login**: Email-based (`admin@votingsystem.com`)

### Voter

**Participate in Elections**

| Feature | Permission |
|---------|-----------|
| View All Elections | ✅ Can See All |
| Vote | ✅ Only in Eligible Elections |
| Apply as Candidate | ✅ Only for Eligible Elections |
| View Results | ✅ All Closed Elections |
| View Own Profile | ✅ Read Only |
| Change Password | ❌ Admin Resets |
| View Other Voters | ❌ No |

**Login**: CNIC-based (13 digits)

### Candidate

**Same as Voter + Additional Visibility**

- Voters can see they are a candidate in election cards
- Special badge shown: "ℹ️ You are a candidate in this election"

---

## 🔄 Workflow

### Election Lifecycle

```
1. ELECTION CREATION (Admin)
   │
   ├─► Admin creates election with title, description, dates
   │
   ▼
2. VOTER ELIGIBILITY SETUP (Admin)
   │
   ├─► Admin adds eligible voters to election
   │
   ▼
3. CANDIDATE SETUP (Mixed)
   │
   ├─► Option A: Admin adds candidates directly
   │   └─► Select voter → Add manifesto → Save
   │
   ├─► Option B: Voters apply
   │   └─► Voter submits application → Admin reviews → Approve/Reject
   │
   ▼
4. UPCOMING STATUS (Automatic)
   │
   ├─► Election visible to all voters
   ├─► Eligibility badges shown (✓ ELIGIBLE / ✗ NOT ELIGIBLE)
   ├─► Eligible voters can apply as candidates
   ├─► No voting yet
   │
   ▼
5. ACTIVE STATUS (Automatic, when start_date reached)
   │
   ├─► Voting opens for eligible voters
   ├─► Vote button appears for eligible voters only
   ├─► Non-eligible voters see warning: "Not Eligible to Vote"
   ├─► One vote per voter enforced
   ├─► Real-time vote counting
   │
   ▼
6. CLOSED STATUS (Automatic, when end_date reached)
   │
   ├─► Voting closes
   ├─► Results published to ALL users
   ├─► View Results button appears
   ├─► Vote tallies and percentages shown
   │
   ▼
7. ARCHIVE (Optional, Admin)
   │
   └─► Admin can soft-delete election
       └─► Data preserved for auditing
```

### Voting Process

```
1. VOTER LOGIN
   │
   ├─► Enter CNIC (13 digits)
   ├─► Enter password
   └─► Session created
   │
   ▼
2. VIEW DASHBOARD
   │
   ├─► See ALL elections (with eligibility badges)
   ├─► Green badge (✓ ELIGIBLE) = Can vote
   ├─► Red badge (✗ NOT ELIGIBLE) = Cannot vote
   │
   ▼
3. SELECT ELECTION
   │
   ├─► Click "View Details"
   ├─► See election info, dates, candidates
   │
   ▼
4. VOTE (If Eligible & Active & Not Voted)
   │
   ├─► View candidate list with manifestos
   ├─► Select one candidate (radio button)
   ├─► Click "Cast Vote"
   ├─► Confirm vote
   │
   ▼
5. VOTE RECORDED
   │
   ├─► Vote saved to database
   ├─► Candidate vote count increments
   ├─► Voter marked as "voted" for this election
   ├─► Badge changes to: "✅ You have voted"
   └─► Cannot vote again
```

### Candidate Application Workflow

```
1. VOTER SEES UPCOMING ELECTION
   │
   ├─► Must be eligible voter
   ├─► Must not already be a candidate
   ├─► Election status = "Upcoming"
   │
   ▼
2. APPLY AS CANDIDATE
   │
   ├─► Click "Apply as Candidate" button
   ├─► Fill application form
   ├─► Write manifesto (platform/promises)
   ├─► Submit application
   │
   ▼
3. PENDING STATUS
   │
   ├─► Application stored in database
   ├─► Voter sees: "Application: PENDING"
   ├─► Admin sees in Applications list
   │
   ▼
4. ADMIN REVIEW
   │
   ├─► Admin reviews manifesto
   ├─► Decision: Approve OR Reject
   │
   ├──► APPROVED
   │    ├─► Candidate added to election
   │    ├─► Voter sees: "Application: APPROVED"
   │    └─► Badge: "ℹ️ You are a candidate"
   │
   └──► REJECTED
        ├─► Voter sees: "Application: REJECTED"
        └─► Can apply again if still upcoming
```

---

## 🔒 Security Features

### Authentication & Authorization

- **Password Hashing**: Bcrypt via Werkzeug (strong one-way hashing)
- **CNIC-Based Login**: Voters use 13-digit CNIC (not email)
- **Email-Based Login**: Admins use email (voters blocked from email login)
- **Role Validation**: `@admin_required` and `@login_required` decorators
- **Session Validation**: Every request checks user existence and ban status

### Session Security

- **HTTP-Only Cookies**: JavaScript cannot access session cookies
- **Secure Headers**: X-Content-Type-Options, X-Frame-Options, X-XSS-Protection
- **No-Cache Headers**: Prevents back-button access after logout
- **Session Invalidation**: Automatic logout on account ban/delete
- **CSRF Protection**: All forms protected with Flask-WTF tokens

### Input Validation

- **CNIC Validation**: 13-digit format enforcement
- **Email Validation**: Proper email format check
- **Password Strength**: Minimum length requirements
- **SQL Injection Prevention**: SQLAlchemy ORM (parameterized queries)
- **XSS Prevention**: Jinja2 auto-escaping

### Rate Limiting

- **Login Attempts**: Limited to prevent brute force attacks
- **Per-IP Limiting**: 100 requests per hour default
- **Authentication Endpoints**: Extra protection on /login and /register

### Audit Logging

- **Security Log** (`logs/security.log`):
  - Login attempts (success/failure)
  - Ban/unban events
  - Password resets
  - Admin password changes
  
- **Application Log** (`logs/app.log`):
  - Election creation/modification
  - Voter addition
  - Vote casting
  - Application submissions
  - Errors and exceptions

### Data Protection

- **Soft Deletes**: Elections and voters marked as deleted (not removed)
- **Vote Integrity**: One vote per voter per election enforced at DB level
- **CSRF Tokens**: All state-changing operations protected
- **Ban System**: Compromised accounts suspended, not deleted

---

## 🎨 User Interface

### Design Philosophy

- **Modern**: Glassmorphism, gradients, smooth animations
- **Clean**: Minimalist layout with ample whitespace
- **Professional**: Consistent color scheme and typography
- **Responsive**: Mobile-first design with breakpoints
- **Accessible**: High contrast, focus states, keyboard navigation

### Color System

- **Primary**: Indigo/Purple gradients (`#6366F1` → `#8B5CF6`)
- **Success**: Green (`#10B981`)
- **Danger**: Red (`#EF4444`)
- **Warning**: Amber (`#F59E0B`)
- **Info**: Blue (`#3B82F6`)
- **Gray Scale**: 50-900 shades

### Components

- **Cards**: Glassmorphic with backdrop blur
- **Buttons**: Gradient backgrounds with hover effects
- **Forms**: Clear labels, good spacing, focus states
- **Tables**: Hover rows, gradient headers
- **Badges**: Color-coded status indicators
- **Alerts**: Contextual colors with icons

---

## 📦 Installation

### Quick Start

```bash
# 1. Clone/Download project
cd "Voting System"

# 2. Create virtual environment
python -m venv venv

# 3. Activate virtual environment
# Windows:
venv\Scripts\activate
# macOS/Linux:
source venv/bin/activate

# 4. Install dependencies
pip install -r requirements.txt

# 5. Run application
python app.py

# 6. Open browser
http://localhost:5000
```

**Default Admin Login:**
- Email: `admin@votingsystem.com`
- Password: `Admin@123`

> ⚠️ **Change admin password immediately!**

For detailed setup instructions, see [SETUP_GUIDE.md](SETUP_GUIDE.md).

---

## 📸 Screenshots

*Coming soon: Screenshots of Dashboard, Election Details, Voting Interface, Results Page*

---

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

---

## 🙏 Acknowledgments

- Flask framework and extensions
- SQLAlchemy ORM
- All contributors and testers

---

## 📞 Support

For issues, questions, or feature requests:

1. Check [SETUP_GUIDE.md](SETUP_GUIDE.md) for installation help
2. Review logs in `logs/` directory
3. Open an issue on GitHub

---

## 🚀 Roadmap

Future enhancements:

- [ ] Multi-factor authentication (2FA)
- [ ] Email notifications for election events
- [ ] Vote verification receipts
- [ ] Advanced analytics dashboard
- [ ] Export results to PDF/CSV
- [ ] Multi-language support
- [ ] Dark mode toggle
- [ ] Blockchain vote verification

---

**Built with ❤️ for transparent, secure, and accessible democratic processes.**

**Version**: 1.0.0  
**Last Updated**: January 2026

# 🗳️ Online Voting System - Setup Guide

Complete installation and setup guide for new users.

---

## 📋 Table of Contents

1. [System Requirements](#system-requirements)
2. [Installation Steps](#installation-steps)
3. [Initial Configuration](#initial-configuration)
4. [Running the Application](#running-the-application)
5. [First-Time Setup](#first-time-setup)
6. [Troubleshooting](#troubleshooting)

---

## 🖥️ System Requirements

### Software Prerequisites

- **Python**: Version 3.8 or higher
- **pip**: Python package installer
- **Git** (optional): For cloning the repository
- **Web Browser**: Modern browser (Chrome, Firefox, Edge, Safari)

### Operating System

- ✅ Windows 10/11
- ✅ macOS 10.14+
- ✅ Linux (Ubuntu 18.04+, Debian, CentOS)

---

## 📥 Installation Steps

### Step 1: Download/Clone the Project

**Option A: Download ZIP**
1. Download the project ZIP file
2. Extract to your desired location
3. Open terminal/command prompt in the extracted folder

**Option B: Clone with Git**
```bash
git clone <repository-url>
cd "Voting System"
```

### Step 2: Create Virtual Environment

**On Windows:**
```bash
# Navigate to project directory
cd "C:\Path\To\Voting System"

# Create virtual environment
python -m venv venv

# Activate virtual environment
venv\Scripts\activate
```

**On macOS/Linux:**
```bash
# Navigate to project directory
cd "/path/to/Voting System"

# Create virtual environment
python3 -m venv venv

# Activate virtual environment
source venv/bin/activate
```

You should see `(venv)` prefix in your terminal.

### Step 3: Install Dependencies

```bash
# Upgrade pip
python -m pip install --upgrade pip

# Install all required packages
pip install -r requirements.txt
```

**Required Packages:**
- `Flask==3.0.0` - Web framework
- `Flask-SQLAlchemy==3.1.1` - Database ORM
- `Flask-Bcrypt==1.0.1` - Password hashing
- `Flask-WTF==1.2.1` - CSRF protection
- `Flask-Limiter==3.5.0` - Rate limiting
- `APScheduler==3.10.4` - Task scheduling
- `Werkzeug==3.0.1` - Security utilities
- `python-dotenv==1.0.0` - Environment variables

---

## ⚙️ Initial Configuration

### Step 1: Environment Variables (Optional)

You can create a `.env` file in the project root for custom configuration:

```ini
# Application Settings
FLASK_APP=app.py
FLASK_ENV=development
SECRET_KEY=your-secret-key-here-change-in-production

# Database
DATABASE_URI=sqlite:///voting_system.db

# Security
SESSION_COOKIE_SECURE=False
SESSION_COOKIE_HTTPONLY=True
SESSION_COOKIE_SAMESITE=Lax
```

> ⚠️ **IMPORTANT**: Change `SECRET_KEY` to a random string for production!

**Generate a secure SECRET_KEY:**
```python
# Run in Python shell
python
>>> import secrets
>>> print(secrets.token_hex(32))
>>> exit()
```

### Step 2: Initialize Database

The database will be created automatically on first run:

```bash
python app.py
```

This creates:
- `voting_system.db` - Main database file
- `logs/` directory - Application and security logs
- Default admin account

---

## 🚀 Running the Application

### Development Mode

```bash
# 1. Activate virtual environment (if not already)
# Windows:
venv\Scripts\activate

# macOS/Linux:
source venv/bin/activate

# 2. Run the application
python app.py
```

**Expected Output:**
```
 * Serving Flask app 'app'
 * Debug mode: on
INFO: Admin user created: admin@votingsystem.com
WARNING: This is a development server. Do not use it in production.
 * Running on http://127.0.0.1:5000
Press CTRL+C to quit
```

### Access the Application

Open your web browser and navigate to:
```
http://localhost:5000
```

### Stop the Application

Press `Ctrl+C` in the terminal to stop the server.

---

## 👤 First-Time Setup

### Step 1: Login as Admin

The system creates a default admin account automatically:

**Default Admin Credentials:**
- **Email**: `admin@votingsystem.com`
- **Password**: `Admin@123`

> 🔐 **CRITICAL**: Change the admin password immediately after first login!

### Step 2: Change Admin Password

1. Login at `http://localhost:5000/login`
2. Use default admin credentials above
3. Click **"🔒 Change Password"** in the navigation bar
4. Enter:
   - Current password: `Admin@123`
   - New password: (your secure password, min 8 characters)
   - Confirm new password
5. Click **"Change Password"**
6. You'll be logged out - login again with your new password

### Step 3: Add Your First Voter

1. Go to **Admin Dashboard** → **Voters** → **Add Voter**
2. Fill in voter details:
   - **Full Name**: e.g., "John Doe"
   - **CNIC**: 13 digits (e.g., 1234567890123)
   - **Email**: e.g., "john@example.com"
   - **Password**: Voter will use this to login (min 6 chars)
3. Click **"Add Voter"**

### Step 4: Create Your First Election

1. Go to **Admin Dashboard** → **Elections** → **Create Election**
2. Fill in election details:
   - **Title**: e.g., "Student Council Election 2024"
   - **Description**: Brief description
   - **Start Date & Time**: When voting begins
   - **End Date & Time**: When voting ends
3. Click **"Create Election"**

### Step 5: Add Eligible Voters to Election

1. Go to **Elections** → Click on your election → **Manage Voters**
2. Check boxes next to voters who can participate
3. Click **"Add Selected Voters"**

### Step 6: Add Candidates

**Option 1: Admin Adds Candidates Directly**
1. Go to election → **Manage Candidates** → **Add Candidate**
2. Select a voter from dropdown
3. Enter candidate manifesto
4. Click **"Add Candidate"**

**Option 2: Voters Apply as Candidates**
1. Eligible voters login and see upcoming elections
2. Click **"Apply as Candidate"** on election card
3. Fill application form with manifesto
4. Admin reviews in **Admin Dashboard** → **Applications**
5. Admin approves or rejects applications

---

## 🛠️ Troubleshooting

### Common Issues

#### ❌ Module Not Found Error
```
ModuleNotFoundError: No module named 'flask'
```

**Solution:**
```bash
# Ensure virtual environment is activated
# Then reinstall dependencies
pip install -r requirements.txt
```

#### ❌ Database Lock Error
```
sqlite3.OperationalError: database is locked
```

**Solution:**
- Close all other instances of the application
- If persists, stop the server and restart
- Last resort: Delete `voting_system.db` (⚠️ loses all data) and restart

#### ❌ Port Already in Use
```
OSError: [Errno 48] Address already in use
```

**Solution:**
```bash
# Find and kill process using port 5000

# Windows:
netstat -ano | findstr :5000
taskkill /PID <PID> /F

# macOS/Linux:
lsof -i :5000
kill -9 <PID>
```

#### ❌ CSRF Token Missing Error

**Solution:**
- Clear browser cache and cookies
- Hard refresh: `Ctrl+Shift+R` (Windows) or `Cmd+Shift+R` (Mac)
- Restart the Flask application

#### ❌ Can't Access Pages After Logout

**Solution:**
- This is normal (security feature prevents back-button access)
- Just login again normally

#### ❌ Voter Can't Login

**Solution:**
- Ensure voter is using CNIC (not email) to login
- CNIC should be 13 digits without dashes
- Admin can reset voter password in Edit Voter page

#### ❌ Import Errors on Windows

**Solution:**
```bash
# Use python instead of python3
python app.py
```

---

## 📁 Project Structure

```
Voting System/
├── app.py                  # Main application entry point
├── config.py              # Configuration settings
├── requirements.txt       # Python dependencies
├── models.py             # Database models
├── utils.py              # Helper functions
├── routes/
│   ├── admin.py         # Admin routes (elections, voters, candidates)
│   ├── auth.py          # Authentication (login, logout)
│   ├── voter.py         # Voter routes (dashboard, voting)
│   └── results.py       # Results viewing
├── templates/           # HTML templates
│   ├── admin/          # Admin-specific templates
│   ├── voter/          # Voter-specific templates
│   └── base.html       # Base template
├── static/             # Static files
│   └── css/           # Stylesheets
├── logs/              # Application logs (created at runtime)
│   ├── app.log        # General application logs
│   └── security.log   # Security-related events
└── voting_system.db   # SQLite database (created at runtime)
```

---

## 🔒 Security Best Practices

### For Administrators

1. ✅ **Change default admin password immediately**
2. ✅ **Use strong passwords** (8+ characters, mixed case, numbers, symbols)
3. ✅ **Don't share admin credentials**
4. ✅ **Regularly backup** `voting_system.db`
5. ✅ **Monitor logs** in `logs/` directory
6. ✅ **Ban compromised accounts** instead of deleting
7. ✅ **Review security logs** for suspicious activity

### For Production Deployment

1. ✅ Set `FLASK_ENV=production` in `.env`
2. ✅ Use a strong `SECRET_KEY` (64+ characters)
3. ✅ Enable HTTPS and set `SESSION_COOKIE_SECURE=True`
4. ✅ Use PostgreSQL/MySQL instead of SQLite
5. ✅ Set up proper firewall rules
6. ✅ Regular security updates
7. ✅ Use a production WSGI server (Gunicorn, uWSGI)

---

## 📞 Getting Help

### Log Files

Check logs for errors:
- `logs/app.log` - General application logs
- `logs/security.log` - Security events (login attempts, bans)

### Common Commands

```bash
# Activate virtual environment
# Windows:
venv\Scripts\activate
# macOS/Linux:
source venv/bin/activate

# Run application
python app.py

# Install/update dependencies
pip install -r requirements.txt

# Check Python version
python --version

# Deactivate virtual environment
deactivate
```

---

## ✅ Quick Start Checklist

- [ ] Install Python 3.8 or higher
- [ ] Create virtual environment (`python -m venv venv`)
- [ ] Activate virtual environment
- [ ] Install dependencies (`pip install -r requirements.txt`)
- [ ] Run application (`python app.py`)
- [ ] Access `http://localhost:5000`
- [ ] Login as admin (`admin@votingsystem.com` / `Admin@123`)
- [ ] **IMPORTANT: Change admin password!**
- [ ] Add voters
- [ ] Create elections
- [ ] Add eligible voters to elections
- [ ] Add or approve candidates
- [ ] Start voting!

---

## 🎉 You're Ready!

Your Online Voting System is now set up and ready to use. 

For detailed feature documentation, see [README.md](README.md).

**Happy Voting! 🗳️**

