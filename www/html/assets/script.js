document.getElementById('mode-switch').addEventListener('click', function() {
    document.body.classList.toggle('light-mode');
    const icon = this.querySelector('i');
    if (document.body.classList.contains('light-mode')) {
        icon.classList.remove('fa-sun');
        icon.classList.add('fa-moon');
        localStorage.setItem('mode', 'light');
    } else {
        icon.classList.remove('fa-moon');
        icon.classList.add('fa-sun');
        localStorage.setItem('mode', 'dark');
    }
});

// Set initial mode based on localStorage
document.addEventListener('DOMContentLoaded', function() {
    const savedMode = localStorage.getItem('mode');
    const modeSwitchIcon = document.getElementById('mode-switch').querySelector('i');
    if (savedMode === 'light') {
        document.body.classList.add('light-mode');
        modeSwitchIcon.classList.remove('fa-sun');
        modeSwitchIcon.classList.add('fa-moon');
    } else {
        document.body.classList.remove('light-mode');
        modeSwitchIcon.classList.remove('fa-moon');
        modeSwitchIcon.classList.add('fa-sun');
    }
});

document.getElementById('nav-toggle').addEventListener('click', function() {
    const navList = document.getElementById('nav-list');
    navList.style.display = navList.style.display === 'flex' ? 'none' : 'flex';
});
