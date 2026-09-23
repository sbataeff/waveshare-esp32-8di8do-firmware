// Web UI HTML/CSS/JS, kept in its own header on purpose: Arduino IDE's
// automatic function-prototype generator scans .ino files with a simple
// tokenizer that doesn't understand C++ raw string literals, so JS code
// like a plain "function foo(){" inside a raw string literal in the .ino
// itself gets misread as real C++ and breaks the build ("'function' does
// not name a type"). Prototype generation only runs on .ino files, so
// moving this content into a .h file (included, not compiled standalone)
// sidesteps the bug entirely. Keep any future edits to this string in this
// file, not back in the .ino.

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 8DI/8DO Console</title>
<style>
 body{font-family:system-ui,Segoe UI,Arial,sans-serif;background:#12161c;color:#e6e6e6;margin:0;padding:16px}
 h1{font-size:18px;margin:0 0 12px}
 .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:14px}
 .card{background:#1b212b;border:1px solid #2a3140;border-radius:8px;padding:12px}
 .card h2{font-size:13px;text-transform:uppercase;letter-spacing:.06em;color:#93a1b7;margin:0 0 10px}
 .chRow{display:flex;align-items:center;justify-content:space-between;padding:6px 0;border-bottom:1px solid #232a36}
 .chRow:last-child{border-bottom:none}
 .lbl{font-family:ui-monospace,Consolas,monospace;font-size:13px}
 button,.ind{background:#2a3140;color:#e6e6e6;border:1px solid #3a4256;border-radius:6px;padding:6px 12px;font-size:12px;font-family:ui-monospace,Consolas,monospace;display:inline-block;min-width:38px;text-align:center}
 button{cursor:pointer}
 .ind{cursor:default;user-select:none}
 button.on,.ind.on{background:#245c3d;border-color:#3ddc84;color:#3ddc84}
 #log{font-family:ui-monospace,Consolas,monospace;font-size:11px;white-space:pre-wrap;max-height:220px;overflow-y:auto;background:#0d1117;border-radius:6px;padding:8px}
 .kv{display:flex;justify-content:space-between;font-family:ui-monospace,Consolas,monospace;font-size:12px;padding:3px 0}
 .kv span:first-child{color:#93a1b7}
 .status-pill{display:inline-block;padding:2px 8px;border-radius:10px;font-size:11px}
 .status-pill.up{background:#1e3d2c;color:#3ddc84}
 .status-pill.down{background:#3d1e1e;color:#e0776d}
 #connBanner{display:none;background:#3d1e1e;color:#ffb4a8;border:1px solid #7a3a2f;border-radius:8px;padding:8px 12px;margin-bottom:12px;font-family:ui-monospace,Consolas,monospace;font-size:12px}
 .camCard{grid-column:1/-1}
 .camRow{display:flex;gap:6px;margin-bottom:8px}
 .camRow input{flex:1;background:#0d1117;color:#e6e6e6;border:1px solid #3a4256;border-radius:6px;padding:6px 8px;font-family:ui-monospace,Consolas,monospace;font-size:12px}
 #camWrap{position:relative;background:#0d1117;border-radius:6px;min-height:120px;display:flex;align-items:center;justify-content:center;overflow:hidden}
 #camImg{max-width:100%;display:block}
 #camError{display:none;color:#e0776d;font-family:ui-monospace,Consolas,monospace;font-size:12px;padding:16px;text-align:center}
</style></head><body>
<h1>WaveShare ESP32 8DI/8DO Console</h1>
<div id="connBanner"></div>
<div class="grid">
  <div class="card"><h2>Digital Inputs</h2><div id="diList"></div></div>
  <div class="card"><h2>Digital Outputs</h2><div id="doList"></div></div>
  <div class="card"><h2>Device / Connected Client</h2><div id="deviceInfo"></div></div>
  <div class="card"><h2>GUI Clients</h2><div id="clientsList"></div></div>
  <div class="card camCard"><h2>IP Camera Feed</h2>
    <div class="camRow">
      <input id="camUrlInput" type="text" placeholder="http://phone-ip:8080/video">
      <button id="camSetBtn">Set</button>
      <button id="camRetryBtn">Reconnect</button>
    </div>
    <div id="camWrap">
      <img id="camImg" alt="camera feed">
      <div id="camError">No camera feed — set the IP Webcam URL above and make sure the app is running and reachable from this browser.</div>
    </div>
  </div>
  <div class="card"><h2>Status / Debug Log</h2><div id="log"></div></div>
</div>
<script>
// Rows are built once and updated in place on every poll — avoids
// innerHTML churn (which was re-creating all 16 rows 4x/sec and adding
// to the perceived lag) and lets output clicks update instantly instead
// of waiting on a network round trip.
const diInd = [], doBtn = [];
let doOverride = [false,false,false,false,false,false,false,false]; // optimistic local state
let doOverrideUntil = [0,0,0,0,0,0,0,0];

function buildRows(){
  const di = document.getElementById('diList');
  for(let i=0;i<8;i++){
    const row=document.createElement('div'); row.className='chRow';
    const lbl=document.createElement('span'); lbl.className='lbl'; lbl.textContent='DI'+(i+1);
    const ind=document.createElement('span'); ind.textContent='OFF'; ind.className='ind';
    row.appendChild(lbl); row.appendChild(ind); di.appendChild(row);
    diInd.push(ind);
  }
  const doL = document.getElementById('doList');
  for(let i=0;i<8;i++){
    const row=document.createElement('div'); row.className='chRow';
    const lbl=document.createElement('span'); lbl.className='lbl'; lbl.textContent='DO'+(i+1);
    const btn=document.createElement('button'); btn.textContent='OFF';
    const ch=i+1;
    btn.onclick=()=>{
      // Optimistic update: flip the button immediately, don't wait on the
      // network. doOverride briefly wins over polled state so a slow poll
      // response in flight can't stomp the click back to its old value.
      const next = !btn.classList.contains('on');
      applyDoState(i, next);
      doOverride[i] = true;
      doOverrideUntil[i] = Date.now() + 1500;
      fetch('/api/output', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'ch='+ch+'&state='+(next?'on':'off')})
        .catch(()=>{ doOverride[i]=false; applyDoState(i, !next); });
    };
    row.appendChild(lbl); row.appendChild(btn); doL.appendChild(row);
    doBtn.push(btn);
  }
}
function applyDoState(i, on){
  doBtn[i].textContent = on?'ON':'OFF';
  doBtn[i].className = on?'on':'';
}

// Watchdog: tracks the last time /api/status actually succeeded. If this
// gets stale, the banner makes it obvious you're looking at a frozen or
// cached page rather than a live connection to the board — distinct from
// a poll simply being in flight.
let lastGoodPollMs = Date.now();
const STALE_AFTER_MS = 2000;

function updateConnBanner(){
  const staleFor = Date.now() - lastGoodPollMs;
  const banner = document.getElementById('connBanner');
  if (staleFor > STALE_AFTER_MS) {
    banner.style.display = 'block';
    banner.textContent = '⚠ NOT LIVE — no response from ESP32 for ' + Math.round(staleFor/1000) +
      's. You may be viewing a cached/offline page — reload to reconnect.';
  } else {
    banner.style.display = 'none';
  }
}

let statusInflight = false;
async function refreshStatus(){
  if (statusInflight) return; // don't let requests queue up on the device
  statusInflight = true;
  try{
    const r = await fetch('/api/status'); const d = await r.json();
    lastGoodPollMs = Date.now();
    window.__myIp = d.client_ip;
    const now = Date.now();
    d.di.forEach((v,i)=>{
      diInd[i].textContent = v?'ON':'OFF';
      diInd[i].className = 'ind'+(v?' on':'');
    });
    d.do.forEach((v,i)=>{
      if (doOverride[i] && now < doOverrideUntil[i]) return; // trust the click for a moment
      doOverride[i] = false;
      applyDoState(i, v);
    });
    document.getElementById('deviceInfo').innerHTML = `
      <div class="kv"><span>Hostname</span><span>${d.hostname}</span></div>
      <div class="kv"><span>Ethernet link</span><span class="status-pill ${d.eth_link?'up':'down'}">${d.eth_link?'UP':'DOWN'}</span></div>
      <div class="kv"><span>Device IP (DHCP)</span><span>${d.eth_ip}</span></div>
      <div class="kv"><span>Subnet mask</span><span>${d.eth_subnet}</span></div>
      <div class="kv"><span>Gateway</span><span>${d.eth_gateway}</span></div>
      <div class="kv"><span>MAC</span><span>${d.mac}</span></div>
      <div class="kv"><span>Uptime</span><span>${d.uptime_s}s</span></div>
      <div class="kv"><span>Free heap</span><span>${d.free_heap}B</span></div>
      <div class="kv"><span>Your (client) IP</span><span>${d.client_ip}</span></div>
      <div class="kv"><span>Your User-Agent</span><span style="max-width:180px;overflow:hidden;text-overflow:ellipsis">${d.client_ua}</span></div>`;
  }catch(e){ document.getElementById('log').textContent = 'status fetch failed: '+e; }
  finally{ statusInflight = false; }
}
let logInflight = false;
async function refreshLog(){
  if (logInflight) return;
  logInflight = true;
  try{
    const r = await fetch('/api/log'); const d = await r.json();
    document.getElementById('log').textContent = d.lines.join('\n');
  }catch(e){}
  finally{ logInflight = false; }
}
let clientsInflight = false;
async function refreshClients(){
  if (clientsInflight) return;
  clientsInflight = true;
  try{
    const r = await fetch('/api/clients'); const d = await r.json();
    const el = document.getElementById('clientsList');
    el.innerHTML = '';
    if (d.clients.length === 0) {
      el.innerHTML = '<div class="lbl" style="color:#93a1b7">No clients seen yet</div>';
    }
    d.clients.forEach(c=>{
      const row=document.createElement('div'); row.className='chRow';
      const left=document.createElement('span'); left.className='lbl';
      left.textContent = c.ip + (c.ip===window.__myIp ? ' (you)' : '');
      const right=document.createElement('span');
      right.className = 'ind' + (c.active?' on':'');
      right.textContent = c.active ? 'LIVE' : (Math.round(c.ago_ms/1000)+'s ago');
      row.appendChild(left); row.appendChild(right); el.appendChild(row);
    });
  }catch(e){}
  finally{ clientsInflight = false; }
}
// IP camera feed (e.g. an Android "IP Webcam" app's MJPEG /video URL).
// Purely browser-side: the ESP32 never touches this traffic, your browser
// connects to the camera directly, so it only works if this browser can
// reach that address on your network. The URL is remembered per-browser
// (localStorage), not stored on the device.
const CAM_DEFAULT_URL = 'http://100.69.34.95:8080/video';
const camImg = document.getElementById('camImg');
const camError = document.getElementById('camError');
const camUrlInput = document.getElementById('camUrlInput');

function loadCamUrl(url){
  if (!url) return;
  camError.style.display = 'none';
  camImg.style.display = 'block';
  // Cache-bust so "Reconnect" actually opens a fresh stream instead of
  // reusing a dead one the browser thinks is still the same resource.
  camImg.src = url + (url.includes('?') ? '&' : '?') + '_=' + Date.now();
}
camImg.onerror = () => { camImg.style.display = 'none'; camError.style.display = 'block'; };

let savedCamUrl;
try { savedCamUrl = localStorage.getItem('camUrl'); } catch(e) {}
camUrlInput.value = savedCamUrl || CAM_DEFAULT_URL;
loadCamUrl(camUrlInput.value);

document.getElementById('camSetBtn').onclick = () => {
  const url = camUrlInput.value.trim();
  try { localStorage.setItem('camUrl', url); } catch(e) {}
  loadCamUrl(url);
};
document.getElementById('camRetryBtn').onclick = () => loadCamUrl(camUrlInput.value.trim());

buildRows();
setInterval(refreshStatus, 150);
setInterval(refreshLog, 1000);
setInterval(refreshClients, 1000);
setInterval(updateConnBanner, 500);
// A bfcache restore or a background tab regaining focus can leave stale
// data on screen with timers paused/throttled — force an immediate
// re-check the moment the page is actually looked at again.
document.addEventListener('visibilitychange', () => { if (!document.hidden) refreshStatus(); });
window.addEventListener('pageshow', () => { refreshStatus(); refreshClients(); refreshLog(); });
refreshStatus(); refreshLog(); refreshClients();
</script></body></html>
)HTML";
