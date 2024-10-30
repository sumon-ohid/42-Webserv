document.getElementById('mode-switch').addEventListener('click', function() {
    document.body.classList.toggle('light-mode');
    const icon = this.querySelector('i');
    if (document.body.classList.contains('light-mode')) {
        icon.classList.remove('fa-moon');
        icon.classList.add('fa-sun');
    } else {
        icon.classList.remove('fa-sun');
        icon.classList.add('fa-moon');
    }
});

// Set initial button icon based on the current mode
const modeSwitchIcon = document.getElementById('mode-switch').querySelector('i');
if (document.body.classList.contains('light-mode')) {
    modeSwitchIcon.classList.remove('fa-moon');
    modeSwitchIcon.classList.add('fa-sun');
} else {
    modeSwitchIcon.classList.remove('fa-sun');
    modeSwitchIcon.classList.add('fa-moon');
}

document.getElementById('nav-toggle').addEventListener('click', function() {
    const navList = document.getElementById('nav-list');
    navList.style.display = navList.style.display === 'flex' ? 'none' : 'flex';
});
