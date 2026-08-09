let userEnergy = 150;
let adTimerInterval = null;

// Tab Switcher
function switchTab(tabId) {
    document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
    document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));

    event.target.classList.add('active');
    document.getElementById(tabId).classList.add('active');
}

// Update Energy UI
function updateEnergy(amount) {
    userEnergy += amount;
    document.getElementById('energyCount').innerText = userEnergy;
    document.getElementById('tabEnergyCount').innerText = `${userEnergy} ⚡`;
}

// Meta Audience Network Rewarded Ad Modal
function openAdModal() {
    const modal = document.getElementById('adModal');
    const timerElem = document.getElementById('adTimer');
    const closeBtn = document.getElementById('closeAdBtn');

    modal.classList.add('active');
    closeBtn.disabled = true;
    closeBtn.classList.remove('enabled');
    closeBtn.innerText = '⏳ กรุณารับชมจนจบ (5s)';

    let timeLeft = 5;
    timerElem.innerText = `${timeLeft}s`;

    clearInterval(adTimerInterval);
    adTimerInterval = setInterval(() => {
        timeLeft--;
        if (timeLeft > 0) {
            timerElem.innerText = `${timeLeft}s`;
            closeBtn.innerText = `⏳ กรุณารับชมจนจบ (${timeLeft}s)`;
        } else {
            clearInterval(adTimerInterval);
            timerElem.innerText = '✅ SUCCESS';
            closeBtn.disabled = false;
            closeBtn.classList.add('enabled');
            closeBtn.innerText = '🎉 รับพลังงานฟรี +50 ⚡ (ปิด)';
        }
    }, 1000);
}

function closeAdModal(granted) {
    document.getElementById('adModal').classList.remove('active');
    if (granted) {
        updateEnergy(50);
    }
}

// Live Test Translation with NST Go Server (Port 8080)
async function testTranslate() {
    if (userEnergy < 1) {
        alert('พลังงานไม่พอ! กรุณาดูโฆษณา Meta Audience Network เพื่อรับพลังงานฟรี');
        openAdModal();
        return;
    }

    const input = document.getElementById('sampleInput').value.trim();
    if (!input) return;

    const resultBox = document.getElementById('testResultBox');
    const resultText = document.getElementById('testResultText');
    const cacheBadge = document.getElementById('testCacheBadge');

    resultBox.style.display = 'block';
    resultText.innerText = '⏳ กำลังส่งแปลไปยัง NST Go Server...';
    cacheBadge.innerText = '';

    try {
        const response = await fetch('http://localhost:8080/api/v1/translate', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ text: input })
        });

        const data = await response.json();
        if (data.status === 'success') {
            resultText.innerText = data.translated;
            cacheBadge.innerText = data.cached ? '⚡ Hit Cache (0ms latency)' : '🌐 Translated via Go Server';
            updateEnergy(-1);
        } else {
            resultText.innerText = '❌ เกิดข้อผิดพลาดในการแปล';
        }
    } catch (err) {
        resultText.innerText = '⚠️ ไม่สามารถเชื่อมต่อกับ NST Go Server (localhost:8080)';
        cacheBadge.innerText = err.message;
    }
}

// Game Folder Inspection Data
const gamesData = {
    1: {
        title: "FRAGILE PRINCESS Ver 1.1.1",
        path: "/home/jop/Downloads/New Folder/[Ryuugames] RY-RJ01647202_V1.1/RJ01647202/FRAGILE_PRINCESS_Ver_1_1_1/",
        files: [
            { name: "data/Actors.json", status: "แปลไทยแล้ว (100%)" },
            { name: "data/Armors.json", status: "แปลไทยแล้ว (100%)" },
            { name: "data/CommonEvents.json", status: "แปลไทยแล้ว (6,443 lines)" },
            { name: "data/Items.json", status: "แปลไทยแล้ว (100%)" },
            { name: "data/Skills.json", status: "แปลไทยแล้ว (100%)" },
            { name: "data/System.json", status: "แปลไทยแล้ว (100%)" },
            { name: "data/Map001.json - Map015.json", status: "แปลไทยแล้ว (100%)" },
            { name: "package.json", status: "Valid RPG Maker MV" }
        ]
    },
    2: {
        title: "悪堕ち魔法少女クリッカー (Akudochi Mahou Shoujo Clicker)",
        path: "/home/jop/Downloads/New Folder (1)/[Ryuugames] RY-RJ01399977/RJ01399977/悪堕ち魔法少女クリッカー/",
        files: [
            { name: "data/Actors.json", status: "กำลังยิงแปลผ่าน Go Server..." },
            { name: "data/CommonEvents.json", status: "กำลังยิงแปลผ่าน Go Server (2,689KB)..." },
            { name: "data/Skills.json", status: "กำลังยิงแปลผ่าน Go Server..." },
            { name: "data/System.json", status: "กำลังยิงแปลผ่าน Go Server..." },
            { name: "data/Map001.json - Map024.json", status: "กำลังยิงแปลผ่าน Go Server..." },
            { name: "悪堕ち魔法少女クリッカー.exe", status: "Valid RPG Maker MZ Executable" }
        ]
    }
};

function inspectGame(id) {
    const game = gamesData[id];
    if (!game) return;

    document.getElementById('inspectTitle').innerText = game.title;
    document.getElementById('inspectPath').innerText = game.path;

    const listElem = document.getElementById('inspectFileList');
    listElem.innerHTML = '';

    game.files.forEach(f => {
        const item = document.createElement('div');
        item.className = 'file-item';
        item.innerHTML = `
            <span class="file-name">${f.name}</span>
            <span class="file-status">${f.status}</span>
        `;
        listElem.appendChild(item);
    });

    document.getElementById('gameInspectModal').classList.add('active');
}

function closeInspectModal() {
    document.getElementById('gameInspectModal').classList.remove('active');
}

function scanFolder() {
    alert('🔄 สแกนโฟลเดอร์ในเครื่องเรียบร้อยแล้ว พบเกม RPG Maker 2 โฟลเดอร์');
}
