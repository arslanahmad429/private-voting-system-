/* ============================================
   ONLINE VOTING SYSTEM - JAVASCRIPT
   Interactive Features & Functionality
   ============================================ */

// Flash message auto-dismiss
document.addEventListener('DOMContentLoaded', function () {
    const alerts = document.querySelectorAll('.alert');
    alerts.forEach(alert => {
        setTimeout(() => {
            alert.style.opacity = '0';
            alert.style.transform = 'translateY(-20px)';
            setTimeout(() => alert.remove(), 300);
        }, 5000);
    });
});

// Form Validation
function validateForm(formId) {
    const form = document.getElementById(formId);
    if (!form) return true;

    const inputs = form.querySelectorAll('input[required], textarea[required]');
    let isValid = true;

    inputs.forEach(input => {
        if (!input.value.trim()) {
            input.style.borderColor = 'var(--danger-500)';
            isValid = false;
        } else {
            input.style.borderColor = 'var(--gray-200)';
        }
    });

    return isValid;
}

// Password Strength Indicator
function checkPasswordStrength(password) {
    let strength = 0;
    const feedback = [];

    if (password.length >= 8) strength++;
    else feedback.push('At least 8 characters');

    if (/[A-Z]/.test(password)) strength++;
    else feedback.push('One uppercase letter');

    if (/[a-z]/.test(password)) strength++;
    else feedback.push('One lowercase letter');

    if (/\d/.test(password)) strength++;
    else feedback.push('One number');

    if (/[^A-Za-z0-9]/.test(password)) strength++;

    return { strength, feedback };
}

function updatePasswordStrength() {
    const passwordInput = document.getElementById('password');
    const strengthMeter = document.getElementById('password-strength');

    if (!passwordInput || !strengthMeter) return;

    const password = passwordInput.value;
    const { strength, feedback } = checkPasswordStrength(password);

    const colors = ['#ef4444', '#f59e0b', '#eab308', '#84cc16', '#22c55e'];
    const labels = ['Weak', 'Fair', 'Good', 'Strong', 'Very Strong'];

    strengthMeter.style.width = (strength * 20) + '%';
    strengthMeter.style.background = colors[strength - 1] || colors[0];
    strengthMeter.textContent = labels[strength - 1] || labels[0];
}

// Vote Confirmation Modal
function confirmVote(candidateName, candidateId, electionId) {
    const confirmed = confirm(`Are you sure you want to vote for ${candidateName}?\n\nThis action cannot be undone.`);

    if (confirmed) {
        const form = document.createElement('form');
        form.method = 'POST';
        form.action = '/vote';

        const candidateInput = document.createElement('input');
        candidateInput.type = 'hidden';
        candidateInput.name = 'candidate_id';
        candidateInput.value = candidateId;

        const electionInput = document.createElement('input');
        electionInput.type = 'hidden';
        electionInput.name = 'election_id';
        electionInput.value = electionId;

        form.appendChild(candidateInput);
        form.appendChild(electionInput);
        document.body.appendChild(form);
        form.submit();
    }
}

// Countdown Timer for Elections
function startCountdown(elementId, endDate) {
    const element = document.getElementById(elementId);
    if (!element) return;

    const countdownInterval = setInterval(() => {
        const now = new Date().getTime();
        const distance = new Date(endDate).getTime() - now;

        if (distance < 0) {
            clearInterval(countdownInterval);
            element.innerHTML = "ELECTION CLOSED";
            return;
        }

        const days = Math.floor(distance / (1000 * 60 * 60 * 24));
        const hours = Math.floor((distance % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60));
        const minutes = Math.floor((distance % (1000 * 60 * 60)) / (1000 * 60));
        const seconds = Math.floor((distance % (1000 * 60)) / 1000);

        element.innerHTML = `${days}d ${hours}h ${minutes}m ${seconds}s`;
    }, 1000);
}

// Smooth Animations for Election Results
function animateResults() {
    const progressBars = document.querySelectorAll('.progress-bar');

    progressBars.forEach(bar => {
        const targetWidth = bar.getAttribute('data-width');
        let currentWidth = 0;

        const interval = setInterval(() => {
            if (currentWidth >= targetWidth) {
                clearInterval(interval);
            } else {
                currentWidth += 1;
                bar.style.width = currentWidth + '%';
            }
        }, 10);
    });
}

// Image Preview for Candidate Upload
function previewImage(input) {
    const preview = document.getElementById('image-preview');
    if (!preview || !input.files || !input.files[0]) return;

    const reader = new FileReader();
    reader.onload = function (e) {
        preview.src = e.target.result;
        preview.style.display = 'block';
    };
    reader.readAsDataURL(input.files[0]);
}

// Delete Confirmation
function confirmDelete(itemName) {
    return confirm(`Are you sure you want to delete "${itemName}"?\n\nThis action cannot be undone and will remove all associated data.`);
}

// Radio Button Visual Enhancement
document.addEventListener('DOMContentLoaded', function () {
    const radioInputs = document.querySelectorAll('input[type="radio"]');

    radioInputs.forEach(radio => {
        radio.addEventListener('change', function () {
            // Remove selection from all siblings
            const siblings = document.querySelectorAll(`input[name="${this.name}"]`);
            siblings.forEach(sibling => {
                const parent = sibling.closest('.candidate-card, .radio-option');
                if (parent) {
                    parent.classList.remove('selected');
                }
            });

            // Add selection to current
            const parent = this.closest('.candidate-card, .radio-option');
            if (parent) {
                parent.classList.add('selected');
            }
        });
    });
});

// Search Functionality
function searchTable(inputId, tableId) {
    const input = document.getElementById(inputId);
    const table = document.getElementById(tableId);

    if (!input || !table) return;

    const filter = input.value.toUpperCase();
    const rows = table.getElementsByTagName('tr');

    for (let i = 1; i < rows.length; i++) {
        const row = rows[i];
        const cells = row.getElementsByTagName('td');
        let found = false;

        for (let j = 0; j < cells.length; j++) {
            const cell = cells[j];
            if (cell.textContent.toUpperCase().indexOf(filter) > -1) {
                found = true;
                break;
            }
        }

        row.style.display = found ? '' : 'none';
    }
}

// Toast Notification System
function showToast(message, type = 'info') {
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.textContent = message;
    toast.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        padding: 16px 24px;
        background: white;
        border-radius: 12px;
        box-shadow: 0 10px 25px rgba(0,0,0,0.1);
        z-index: 9999;
        animation: slideInRight 0.3s ease;
    `;

    document.body.appendChild(toast);

    setTimeout(() => {
        toast.style.animation = 'slideOutRight 0.3s ease';
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

// Initialize everything when DOM is ready
document.addEventListener('DOMContentLoaded', function () {
    // Animate results if on results page
    if (document.querySelector('.progress-bar')) {
        setTimeout(animateResults, 500);
    }

    // Add smooth scroll to all anchor links
    document.querySelectorAll('a[href^="#"]').forEach(anchor => {
        anchor.addEventListener('click', function (e) {
            const target = document.querySelector(this.getAttribute('href'));
            if (target) {
                e.preventDefault();
                target.scrollIntoView({ behavior: 'smooth', block: 'start' });
            }
        });
    });
});
