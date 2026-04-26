/* ================================================================
   ONLINE VOTING SYSTEM - COMPLETE JAVASCRIPT
   Developed by: Anas Farooq
   BS Computer Science - Ziauddin University
   ================================================================ */

/* ---- SOURCE: utils.js ---- */
// Notification Toast System
const Toast = {
    show(message, type = 'success') {
        const container = document.getElementById('toast-container');
        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        toast.textContent = message;
        
        container.appendChild(toast);
        
        // Trigger reflow for transition
        void toast.offsetWidth;
        toast.classList.add('show');
        
        setTimeout(() => {
            toast.classList.remove('show');
            setTimeout(() => toast.remove(), 400); // Wait for exit animation
        }, 3000);
    }
};

// Common Fetch Wrapper for JSON APIs
const API = {
    async post(url, data) {
        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(data)
            });
            const result = await response.json();
            if (!response.ok) throw new Error(result.error || 'Server error');
            return result;
        } catch (error) {
            Toast.show(error.message, 'error');
            throw error;
        }
    },
    
    async get(url) {
        try {
            const response = await fetch(url);
            const result = await response.json();
            if (!response.ok) throw new Error(result.error || 'Server error');
            return result;
        } catch (error) {
            Toast.show(error.message, 'error');
            throw error;
        }
    }
};


/* ---- SOURCE: ui_controller.js ---- */
// DOM Elements
const views = {
    login: document.getElementById('view-login'),
    dashboard: document.getElementById('view-dashboard'),
    admin: document.getElementById('view-admin')
};

const modal = {
    overlay: document.getElementById('modal-overlay'),
    title: document.getElementById('modal-title'),
    body: document.getElementById('modal-body'),
    confirm: document.getElementById('btn-modal-confirm'),
    close: document.getElementById('btn-modal-close')
};

// State
let session = {
    user_id: null,
    is_admin: 0,
    cnic: null
};

// View Navigation
function switchView(viewName) {
    Object.values(views).forEach(v => {
        v.classList.remove('active-view');
        v.classList.add('hidden-view');
    });
    views[viewName].classList.remove('hidden-view');
    views[viewName].classList.add('active-view');
}

// Modal Control
function openModal(title, contentHTML, onConfirm) {
    modal.title.textContent = title;
    modal.body.innerHTML = contentHTML;
    modal.overlay.classList.add('active-view');
    
    modal.confirm.onclick = async () => {
        const btn = modal.confirm;
        btn.disabled = true;
        try {
            await onConfirm();
            closeModal();
        } catch(e) {
            console.error(e);
        } finally {
            btn.disabled = false;
        }
    };
}

function closeModal() {
    modal.overlay.classList.remove('active-view');
}
modal.close.onclick = closeModal;

// Authentication
document.getElementById('login-form').addEventListener('submit', async (e) => {
    e.preventDefault();
    const cnic = document.getElementById('cnic').value;
    const password = document.getElementById('password').value;
    
    const btn = e.target.querySelector('button');
    btn.disabled = true;
    
    try {
        const res = await API.post('/api/auth/login', { cnic, password });
        session.user_id = res.user_id;
        session.is_admin = res.is_admin;
        session.cnic = cnic;
        
        Toast.show('Authentication successful.', 'success');
        
        if (session.is_admin) {
            switchView('admin');
            loadAdminData();
        } else {
            switchView('dashboard');
            document.getElementById('nav-user-cnic').textContent = cnic;
            loadVoterData();
        }
    } catch (e) {
        // Error already handled by Toast in utils.js
    } finally {
        btn.disabled = false;
    }
});

// Admin Features
function loadAdminData() {
    // Demo implementation
    const adminPanel = document.getElementById('admin-data-view');
    adminPanel.innerHTML = '<h4>System status: Active</h4><p>Use the administrative controls on the left to manage the NextGen Voting system.</p>';
}

document.getElementById('btn-create-voter').addEventListener('click', () => {
    openModal('Register New Voter', `
        <div class="input-group">
            <label>Voter CNIC</label>
            <input type="text" id="new-voter-cnic" placeholder="0000000000000">
        </div>
        <div class="input-group">
            <label>Name</label>
            <input type="text" id="new-voter-name" placeholder="John Doe">
        </div>
        <div class="input-group">
            <label>Email</label>
            <input type="email" id="new-voter-email" placeholder="john@example.com">
        </div>
        <div class="input-group">
            <label>Initial Password</label>
            <input type="text" id="new-voter-pass" placeholder="GeneratedPass123">
        </div>
    `, async () => {
        const cnic = document.getElementById('new-voter-cnic').value;
        const name = document.getElementById('new-voter-name').value;
        const email = document.getElementById('new-voter-email').value;
        const password = document.getElementById('new-voter-pass').value;
        
        await API.post('/api/admin/users/create', { cnic, name, email, password });
        Toast.show('Voter registered successfully.', 'success');
    });
});

// Logout handlers
document.getElementById('btn-logout').addEventListener('click', () => { session = {}; switchView('login'); });
document.getElementById('btn-admin-logout').addEventListener('click', () => { session = {}; switchView('login'); });


/* ---- SOURCE: main.js ---- */
/* Online Voting System - Main JS */
document.addEventListener('DOMContentLoaded',function(){
    // Auto-dismiss alerts after 5 seconds
    const alerts=document.querySelectorAll('.alert');
    alerts.forEach(a=>{
        setTimeout(()=>{
            a.style.opacity='0';
            a.style.transform='translateY(-20px)';
            a.style.transition='all 0.3s';
            setTimeout(()=>a.remove(),300);
        },5000);
    });
    // Clear flash cookies
    document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';
    document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';
    // Smooth scroll
    document.querySelectorAll('a[href^="#"]').forEach(anchor=>{
        anchor.addEventListener('click',function(e){
            const target=document.querySelector(this.getAttribute('href'));
            if(target){e.preventDefault();target.scrollIntoView({behavior:'smooth',block:'start'});}
        });
    });
    // Animate progress bars on results page
    if(document.querySelector('.progress-bar')){
        setTimeout(animateResults,500);
    }
    // Radio buttons visual enhancement
    document.querySelectorAll('input[type="radio"]').forEach(radio=>{
        radio.addEventListener('change',function(){
            const siblings=document.querySelectorAll('input[name="'+this.name+'"]');
            siblings.forEach(s=>{const p=s.closest('.candidate-card,.radio-option');if(p)p.classList.remove('selected');});
            const parent=this.closest('.candidate-card,.radio-option');
            if(parent)parent.classList.add('selected');
        });
    });
});

function animateResults(){
    document.querySelectorAll('.progress-bar').forEach(bar=>{
        const tw=parseFloat(bar.getAttribute('data-width'))||0;
        bar.style.width='0%';
        let w=0;
        const iv=setInterval(()=>{
            if(w>=tw)clearInterval(iv);
            else{w=Math.min(w+1,tw);bar.style.width=w+'%';}
        },10);
    });
}

function confirmVote(candidateName,candidateId,electionId){
    return confirm('Are you sure you want to vote for '+candidateName+'?\n\nThis action cannot be undone.');
}

function confirmDelete(name){
    return confirm('Are you sure you want to delete "'+name+'"?\n\nThis action cannot be undone.');
}


