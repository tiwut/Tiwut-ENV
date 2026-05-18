function updateClock() {
    const clockElement = document.getElementById('clock');
    const now = new Date();
    let hours = now.getHours();
    let minutes = now.getMinutes();
    const ampm = hours >= 12 ? 'PM' : 'AM';
    hours = hours % 12;
    hours = hours ? hours : 12;
    minutes = minutes < 10 ? '0' + minutes : minutes;
    clockElement.textContent = hours + ':' + minutes + ' ' + ampm;
}
async function updateSystemStats() {
    try {
        const res = await fetch('http:
        if (!res.ok) return;
        const stats = await res.json();
        const batteryIcon = document.querySelector('.status-icon.battery');
        if (batteryIcon) {
            batteryIcon.title = `Battery: ${stats.battery}% (${stats.status})`;
            if (stats.status === "Charging") {
                batteryIcon.style.background = '#27c93f';
            } else if (stats.battery < 20) {
                batteryIcon.style.background = '#ff5f56';
            } else {
                batteryIcon.style.background = '#fff';
            }
        }
    } catch(e) {
    }
}
updateClock();
updateSystemStats();
setInterval(updateClock, 1000);
setInterval(updateSystemStats, 5000); 
const launcherBtn = document.getElementById('launcher-btn');
const appMenu = document.getElementById('app-menu');
launcherBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    if (appMenu.classList.contains('hidden')) {
        appMenu.classList.remove('hidden');
        setTimeout(() => appMenu.classList.add('visible'), 10);
    } else {
        appMenu.classList.remove('visible');
        setTimeout(() => appMenu.classList.add('hidden'), 400);
    }
});
document.addEventListener('click', (e) => {
    if (!appMenu.contains(e.target) && !launcherBtn.contains(e.target) && appMenu.classList.contains('visible')) {
        appMenu.classList.remove('visible');
        setTimeout(() => appMenu.classList.add('hidden'), 400);
    }
});
const windowLayer = document.getElementById('window-layer');
let highestZIndex = 100;
let windowCount = 0;
function createWindow(title, url, iconClass) {
    windowCount++;
    const winId = `win-${windowCount}`;
    highestZIndex++;
    const win = document.createElement('div');
    win.className = 'os-window glass-heavy';
    win.id = winId;
    win.style.zIndex = highestZIndex;
    const offset = (windowCount * 30) % 200;
    win.style.top = `${50 + offset}px`;
    win.style.left = `${100 + offset}px`;
    win.innerHTML = `
        <div class="window-header">
            <div class="window-controls">
                <span class="btn close"></span>
                <span class="btn minimize"></span>
                <span class="btn maximize"></span>
            </div>
            <div class="window-title">${title}</div>
        </div>
        <div class="window-content">
            <iframe src="${url}"></iframe>
            <div class="resize-handle"></div>
        </div>
    `;
    windowLayer.appendChild(win);
    win.addEventListener('mousedown', () => {
        highestZIndex++;
        win.style.zIndex = highestZIndex;
    });
    const closeBtn = win.querySelector('.btn.close');
    const minBtn = win.querySelector('.btn.minimize');
    const maxBtn = win.querySelector('.btn.maximize');
    closeBtn.addEventListener('click', () => {
        win.style.opacity = '0';
        win.style.transform = 'scale(0.9)';
        setTimeout(() => win.remove(), 200);
    });
    minBtn.addEventListener('click', () => {
        win.classList.toggle('minimized');
    });
    maxBtn.addEventListener('click', () => {
        win.classList.toggle('maximized');
    });
    const header = win.querySelector('.window-header');
    let isDragging = false;
    let startX, startY, initialX, initialY;
    header.addEventListener('mousedown', (e) => {
        if (e.target.classList.contains('btn')) return; 
        if (win.classList.contains('maximized')) return; 
        isDragging = true;
        startX = e.clientX;
        startY = e.clientY;
        initialX = win.offsetLeft;
        initialY = win.offsetTop;
        win.querySelector('iframe').style.pointerEvents = 'none';
    });
    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const dx = e.clientX - startX;
        const dy = e.clientY - startY;
        win.style.left = `${initialX + dx}px`;
        win.style.top = `${initialY + dy}px`;
    });
    document.addEventListener('mouseup', () => {
        if (isDragging) {
            isDragging = false;
            win.querySelector('iframe').style.pointerEvents = 'auto';
        }
    });
    const resizer = win.querySelector('.resize-handle');
    let isResizing = false;
    let startW, startH;
    resizer.addEventListener('mousedown', (e) => {
        isResizing = true;
        startX = e.clientX;
        startY = e.clientY;
        startW = win.offsetWidth;
        startH = win.offsetHeight;
        win.querySelector('iframe').style.pointerEvents = 'none';
        e.stopPropagation();
    });
    document.addEventListener('mousemove', (e) => {
        if (!isResizing) return;
        const w = startW + (e.clientX - startX);
        const h = startH + (e.clientY - startY);
        win.style.width = `${Math.max(300, w)}px`; 
        win.style.height = `${Math.max(200, h)}px`; 
    });
    document.addEventListener('mouseup', () => {
        if (isResizing) {
            isResizing = false;
            win.querySelector('iframe').style.pointerEvents = 'auto';
        }
    });
    if (appMenu.classList.contains('visible')) {
        appMenu.classList.remove('visible');
        setTimeout(() => appMenu.classList.add('hidden'), 400);
    }
}
async function loadNativeApps() {
    try {
        const response = await fetch('apps.json');
        if (!response.ok) throw new Error('apps.json not found. Run backend first.');
        const apps = await response.json();
        const appGrid = document.querySelector('.app-grid');
        appGrid.innerHTML = ''; 
        const displayApps = apps.slice(0, 18);
        displayApps.forEach(app => {
            const item = document.createElement('div');
            item.className = 'app-item';
            item.title = app.exec;
            let iconStyle = 'background: linear-gradient(45deg, #333, #666);';
            if (app.icon && !app.icon.includes('%') && !app.icon.includes('/')) {
                iconStyle = `background: linear-gradient(45deg, #11998e, #38ef7d);`;
            }
            item.innerHTML = `<div class="app-icon" style="${iconStyle}"></div><span>${app.name}</span>`;
            item.addEventListener('click', () => {
                console.log(`Sending launch request for: ${app.exec}`);
                let cleanExec = app.exec.replace(/%[a-zA-Z]/g, '').trim();
                fetch(`http:
                    .catch(e => console.warn("IPC Server not running or connection refused", e));
                createWindow(app.name, 'data:text/html,<body style="background:%23222;color:%23fff;font-family:sans-serif;padding:20px;"><h3>Waiting for Wayland Video Stream...</h3><p>App requested: ' + cleanExec + '</p><p>Check your host PC screen! The native app should have successfully launched!</p></body>');
            });
            appGrid.appendChild(item);
        });
    } catch (e) {
        console.warn("Could not load native apps:", e);
    }
}
document.querySelectorAll('.dock-item').forEach(el => {
    if (el.classList.contains('launcher')) return;
    el.addEventListener('click', () => {
        const title = el.getAttribute('title');
        createWindow(title, 'data:text/html,<body style="background:%23222;color:%23fff;padding:20px;"><h3>Loading Native App...</h3></body>');
    });
});
setTimeout(() => {
    createWindow('Welcome', 'data:text/html,<body style="font-family:sans-serif;padding:30px;text-align:center;"><h1>Welcome to Tiwut-ENV!</h1><p>You can drag this window around, resize it using the bottom right corner, or maximize it.</p><p>Click the icons in the dock to open more apps.</p></body>');
    loadNativeApps();
}, 500);
