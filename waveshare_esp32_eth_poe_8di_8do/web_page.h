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
 select{background:#0d1117;color:#e6e6e6;border:1px solid #3a4256;border-radius:6px;padding:6px 8px;font-family:ui-monospace,Consolas,monospace;font-size:12px}
 .camRow input[type=number]{flex:0 0 130px}
 #pingResult{font-family:ui-monospace,Consolas,monospace;font-size:12px;padding:8px;background:#0d1117;border-radius:6px;margin-bottom:8px}
 #pingNote{font-size:11px;color:#93a1b7;margin-top:8px;line-height:1.4}
 .bwCard{grid-column:1/-1}
 .bwLegend{display:flex;gap:18px;margin-bottom:8px;font-family:ui-monospace,Consolas,monospace;font-size:12px}
 .bwLegend .key{display:inline-flex;align-items:center;gap:6px}
 .bwLegend .swatch{width:14px;height:2px;display:inline-block}
 #bwChartWrap{background:#0d1117;border-radius:6px;padding:6px}
 #bwChart{width:100%;height:140px;display:block}
 .bwCrosshair{stroke:#3a4256;stroke-width:1}
 .bwTooltip{font-family:ui-monospace,Consolas,monospace;font-size:11px}
 .statGrid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:8px;margin-top:10px}
 .statTile{background:#0d1117;border-radius:6px;padding:8px}
 .statTile .statLabel{color:#93a1b7;font-size:11px;margin-bottom:2px}
 .statTile .statVal{font-family:ui-monospace,Consolas,monospace;font-size:13px}
 .latCard{grid-column:1/-1}
 #latChartWrap{background:#0d1117;border-radius:6px;padding:6px;position:relative}
 #latChart{width:100%;height:120px;display:block}
 #latEmpty{position:absolute;inset:0;display:flex;align-items:center;justify-content:center;color:#93a1b7;font-size:12px;text-align:center;padding:0 20px}
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
  <div class="card"><h2>Connection Speed Test</h2>
    <div class="camRow">
      <select id="pingSizeSelect"></select>
      <input id="pingCustomSize" type="number" min="1" max="1048576" placeholder="bytes (1-1048576)" style="display:none">
      <button id="pingRunBtn">Run Test</button>
    </div>
    <div id="pingResult">No test run yet.</div>
    <div class="chRow">
      <span class="lbl">Live latency ticker</span>
      <span style="display:flex;gap:8px;align-items:center">
        <span id="pingTickerVal" class="ind">OFF</span>
        <button id="pingTickerToggle">Enable</button>
      </span>
    </div>
    <div id="pingNote">Round trip = time for the ESP32 to send back the chosen payload. The board serves one request at a time, so other live panels (DI/DO, GUI Clients) may pause while a large test is in flight — that's the embedded server's real behavior under load, not a bug.</div>
  </div>
  <div class="card latCard"><h2>Latency Trend</h2>
    <div class="chRow">
      <span class="lbl">Update rate (ping interval)</span>
      <select id="latRateSelect"></select>
    </div>
    <div id="latChartWrap">
      <svg id="latChart" viewBox="0 0 600 120" preserveAspectRatio="none"></svg>
      <div id="latEmpty">Enable the live latency ticker above to start recording a trend.</div>
    </div>
    <div class="statGrid">
      <div class="statTile"><div class="statLabel">Current</div><div class="statVal" id="latCur">—</div></div>
      <div class="statTile"><div class="statLabel">Average (window)</div><div class="statVal" id="latAvg">—</div></div>
      <div class="statTile"><div class="statLabel">Max / jitter (window)</div><div class="statVal" id="latMax">—</div></div>
    </div>
    <div id="pingNote">One point per latency-ticker ping — a 0-byte round trip, same measurement as the ticker value above. This rate <strong>is</strong> the ticker's ping interval, so a faster rate sends more real pings (more data used); the "save data" toggle is still the Enable/Disable button. ~60s rolling window at the current rate. A gap in the line means that ping failed. History is kept when you disable the ticker; new points resume once you re-enable it.</div>
  </div>
  <div class="card bwCard"><h2>Bandwidth Trend</h2>
    <div class="chRow">
      <span class="lbl">Update rate (chart re-sample)</span>
      <select id="bwRateSelect"></select>
    </div>
    <div class="bwLegend">
      <span class="key"><span class="swatch" style="background:#3987e5"></span>ESP32 Interface</span>
      <span class="key"><span class="swatch" style="background:#d95926"></span>IP Camera Feed</span>
    </div>
    <div id="bwChartWrap"><svg id="bwChart" viewBox="0 0 600 140" preserveAspectRatio="none"></svg></div>
    <div class="statGrid">
      <div class="statTile"><div class="statLabel">ESP32 interface ↓ (download)</div><div class="statVal" id="bwEspRx">0.0 KB/s</div></div>
      <div class="statTile"><div class="statLabel">ESP32 interface ↑ (upload)</div><div class="statVal" id="bwEspTx">0.0 KB/s</div></div>
      <div class="statTile"><div class="statLabel">Camera feed ↓ (download)</div><div class="statVal" id="bwCamRx">0.0 KB/s</div></div>
    </div>
    <div id="pingNote">Measured client-side from actual bytes transferred (response Content-Length for the ESP32 interface; real bytes read from the camera's MJPEG stream) — not a synthetic estimate. Includes ~200B/request assumed HTTP header overhead per direction since the browser can't see raw TCP/HTTP framing. This rate only changes how often the chart re-samples and redraws — it doesn't add network traffic, since it's just reading counters that existing traffic already produces. ~60s rolling window at the current rate.</div>
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
      espFetch('/api/output', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'},
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

// Bandwidth accounting for the ESP32 web interface. Accumulates real
// transferred bytes (response Content-Length; request body length for
// POSTs) between 1s bandwidth-chart samples, plus a rough fixed overhead
// estimate per direction since the browser can't see raw HTTP/TCP framing.
const HTTP_OVERHEAD_EST_BYTES = 200;
let espRxBytes = 0, espTxBytes = 0;
async function espFetch(url, opts){
  const body = opts && opts.body;
  espTxBytes += (typeof body === 'string' ? body.length : 0) + HTTP_OVERHEAD_EST_BYTES;
  const r = await fetch(url, opts);
  const cl = r.headers.get('content-length');
  if (cl !== null) {
    espRxBytes += parseInt(cl, 10) + HTTP_OVERHEAD_EST_BYTES;
  } else {
    r.clone().arrayBuffer().then(buf => { espRxBytes += buf.byteLength + HTTP_OVERHEAD_EST_BYTES; }).catch(()=>{});
  }
  return r;
}

let statusInflight = false;
async function refreshStatus(){
  if (statusInflight) return; // don't let requests queue up on the device
  statusInflight = true;
  try{
    const r = await espFetch('/api/status'); const d = await r.json();
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
    const r = await espFetch('/api/log'); const d = await r.json();
    document.getElementById('log').textContent = d.lines.join('\n');
  }catch(e){}
  finally{ logInflight = false; }
}
let clientsInflight = false;
async function refreshClients(){
  if (clientsInflight) return;
  clientsInflight = true;
  try{
    const r = await espFetch('/api/clients'); const d = await r.json();
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
//
// Fetched (not <img src>) and parsed by hand as multipart/x-mixed-replace
// so real bytes-received can be counted for the bandwidth chart — a plain
// <img> tag never exposes byte-level progress for an open MJPEG connection,
// so that's the only way to get real numbers instead of a chart that reads
// zero the whole time the camera is actually streaming fine.
const CAM_DEFAULT_URL = 'http://100.69.34.95:8080/video';
const camImg = document.getElementById('camImg');
const camError = document.getElementById('camError');
const camUrlInput = document.getElementById('camUrlInput');

let camConnected = false;      // at least one frame currently on screen
let camAttemptInFlight = false; // a connect attempt is already running — don't stack another
let camStreamGeneration = 0;    // bumped on every (re)connect so a superseded attempt's
                                 // cleanup can't stomp a newer one's state
let camAbortController = null;
let camRxBytes = 0; // accumulated since the last 1s bandwidth sample

function indexOfBytes(buf, pattern, from){
  const limit = buf.length - pattern.length;
  outer:
  for (let i = from; i <= limit; i++){
    for (let j = 0; j < pattern.length; j++){
      if (buf[i+j] !== pattern[j]) continue outer;
    }
    return i;
  }
  return -1;
}

async function connectCamStream(url){
  if (!url) return;
  camStreamGeneration++;
  const myGen = camStreamGeneration;
  if (camAbortController) camAbortController.abort();
  const controller = new AbortController();
  camAbortController = controller;
  camAttemptInFlight = true;

  try {
    const resp = await fetch(url, { signal: controller.signal, cache: 'no-store' });
    if (myGen !== camStreamGeneration) return; // superseded while awaiting the response
    if (!resp.ok || !resp.body) throw new Error('HTTP ' + resp.status);
    const ct = resp.headers.get('content-type') || '';
    const bm = ct.match(/boundary=(?:"([^"]+)"|([^;]+))/i);
    if (!bm) throw new Error('not a multipart MJPEG response (Content-Type: ' + ct + ')');
    const boundaryBytes = new TextEncoder().encode('--' + (bm[1] || bm[2]).trim());
    const dblCrlf = new TextEncoder().encode('\r\n\r\n');

    const reader = resp.body.getReader();
    let buf = new Uint8Array(0);
    let lastObjectUrl = null;

    while (myGen === camStreamGeneration) {
      const { value, done } = await reader.read();
      if (done) break;
      camRxBytes += value.byteLength;

      const merged = new Uint8Array(buf.length + value.length);
      merged.set(buf, 0); merged.set(value, buf.length);
      buf = merged;

      // Pull out as many complete frames as have already arrived.
      for (;;) {
        const bIdx = indexOfBytes(buf, boundaryBytes, 0);
        if (bIdx === -1) {
          if (buf.length > 5 * 1024 * 1024) throw new Error('no MJPEG boundary found in stream, giving up');
          break;
        }
        const headerStart = bIdx + boundaryBytes.length;
        const hdrEnd = indexOfBytes(buf, dblCrlf, headerStart);
        if (hdrEnd === -1) break; // part header hasn't fully arrived yet

        const headerText = new TextDecoder().decode(buf.subarray(headerStart, hdrEnd));
        const clMatch = headerText.match(/content-length:\s*(\d+)/i);
        const frameStart = hdrEnd + dblCrlf.length;

        let frameEnd, advanceTo;
        if (clMatch) {
          const len = parseInt(clMatch[1], 10);
          if (buf.length < frameStart + len) break; // frame body not fully arrived yet
          frameEnd = frameStart + len;
          advanceTo = frameEnd;
        } else {
          // No Content-Length in this part: fall back to "next boundary ends this frame".
          const nextB = indexOfBytes(buf, boundaryBytes, frameStart);
          if (nextB === -1) break;
          frameEnd = nextB;
          advanceTo = nextB;
        }

        const frameBytes = buf.subarray(frameStart, frameEnd);
        if (frameBytes.length > 0) {
          const objUrl = URL.createObjectURL(new Blob([frameBytes], { type: 'image/jpeg' }));
          camImg.src = objUrl;
          if (lastObjectUrl) URL.revokeObjectURL(lastObjectUrl);
          lastObjectUrl = objUrl;
          camConnected = true;
          camError.style.display = 'none';
          camImg.style.display = 'block';
        }
        buf = buf.subarray(advanceTo);
      }
    }
  } catch (e) {
    // Falls through to the state reset below.
  } finally {
    if (myGen === camStreamGeneration) {
      camAttemptInFlight = false;
      camConnected = false;
      camImg.style.display = 'none';
      camError.style.display = 'block';
    }
  }
}

let savedCamUrl;
try { savedCamUrl = localStorage.getItem('camUrl'); } catch(e) {}
camUrlInput.value = savedCamUrl || CAM_DEFAULT_URL;
connectCamStream(camUrlInput.value);

document.getElementById('camSetBtn').onclick = () => {
  const url = camUrlInput.value.trim();
  try { localStorage.setItem('camUrl', url); } catch(e) {}
  connectCamStream(url);
};
document.getElementById('camRetryBtn').onclick = () => connectCamStream(camUrlInput.value.trim());
// Auto-reconnect: every 1s, if not currently connected (and no attempt is
// already in flight — a slow initial connect shouldn't get restarted every
// second before it even has a chance to finish), try again. This IS the
// "is the camera active" test: the connect attempt itself is the probe.
setInterval(() => {
  if (!camConnected && !camAttemptInFlight) connectCamStream(camUrlInput.value.trim());
}, 1000);

// Connection speed test: fetches /api/ping?size=N (the board streams back
// exactly N bytes) and times the full round trip client-side. Payload is
// user-chosen, 1 byte to 1 MiB, via presets or a custom entry.
const PING_SIZES = [
  ['1 B', 1], ['64 B', 64], ['256 B', 256], ['1 KB', 1024], ['4 KB', 4096],
  ['16 KB', 16384], ['64 KB', 65536], ['256 KB', 262144], ['1 MB', 1048576],
];
const pingSizeSelect = document.getElementById('pingSizeSelect');
PING_SIZES.forEach(([label, val]) => {
  const opt = document.createElement('option'); opt.value = val; opt.textContent = label;
  pingSizeSelect.appendChild(opt);
});
const customOpt = document.createElement('option'); customOpt.value = 'custom'; customOpt.textContent = 'Custom…';
pingSizeSelect.appendChild(customOpt);
pingSizeSelect.value = '1024';

const pingCustomSize = document.getElementById('pingCustomSize');
pingSizeSelect.onchange = () => {
  pingCustomSize.style.display = pingSizeSelect.value === 'custom' ? 'inline-block' : 'none';
};

function formatBytes(n){
  if (n >= 1048576) return (n/1048576).toFixed(2) + 'MB';
  if (n >= 1024) return (n/1024).toFixed(1) + 'KB';
  return n + 'B';
}
function currentPingSize(){
  if (pingSizeSelect.value === 'custom') {
    let n = parseInt(pingCustomSize.value, 10);
    if (!Number.isFinite(n) || n < 1) n = 1;
    if (n > 1048576) n = 1048576;
    return n;
  }
  return parseInt(pingSizeSelect.value, 10);
}
async function runPingOnce(size){
  const t0 = performance.now();
  const r = await espFetch('/api/ping?size=' + size + '&_=' + Date.now());
  const buf = await r.arrayBuffer();
  const ms = performance.now() - t0;
  return { bytes: buf.byteLength, ms };
}
document.getElementById('pingRunBtn').onclick = async () => {
  const size = currentPingSize();
  const resultEl = document.getElementById('pingResult');
  resultEl.textContent = 'Testing ' + formatBytes(size) + '...';
  try {
    const { bytes, ms } = await runPingOnce(size);
    const kbps = ms > 0 ? (bytes / 1024) / (ms / 1000) : 0;
    resultEl.textContent = formatBytes(bytes) + ' round trip in ' + ms.toFixed(1) + 'ms  (' +
      kbps.toFixed(1) + ' KB/s, ' + (kbps * 8 / 1024).toFixed(2) + ' Mbps)';
  } catch (e) {
    resultEl.textContent = 'Test failed: ' + e;
  }
};

// Shared rate dropdown, used by both trend panels. Each panel keeps its
// own rolling window at roughly a constant ~60s of *time* rather than a
// fixed sample *count*, so the chart reads the same "last minute or so"
// regardless of which rate is picked.
const RATE_OPTIONS = [['0.1s', 100], ['0.25s', 250], ['0.5s', 500], ['1s', 1000], ['2s', 2000]];
function historyLenForRate(intervalMs){
  return Math.max(10, Math.min(600, Math.round(60000 / intervalMs)));
}
function populateRateSelect(selectEl, defaultMs){
  RATE_OPTIONS.forEach(([label, ms]) => {
    const opt = document.createElement('option'); opt.value = ms; opt.textContent = label;
    selectEl.appendChild(opt);
  });
  selectEl.value = String(defaultMs);
}

// Latency ticker: a separate, toggle-able lightweight 0-byte round trip,
// off by default so it costs no data unless explicitly enabled. Each
// successful (or failed) ping is also recorded into latencyHistory for the
// Latency Trend chart below — same measurement, just plotted. The rate
// selector on that chart IS the ticker's actual ping interval — unlike the
// bandwidth chart's rate, this one changes how much real traffic is sent.
let tickerEnabled = false;
let tickerTimer = null;
let tickerIntervalMs = 2000;
let latHistoryLen = historyLenForRate(tickerIntervalMs);
let latencyHistory = []; // ms, or null for a failed ping (shows as a gap)

async function tickerOnce(){
  const val = document.getElementById('pingTickerVal');
  let ms = null;
  try {
    const r = await runPingOnce(0);
    ms = r.ms;
    val.textContent = ms.toFixed(0) + 'ms';
    val.className = 'ind on';
  } catch (e) {
    val.textContent = 'ERR';
    val.className = 'ind';
  }
  latencyHistory.push(ms);
  while (latencyHistory.length > latHistoryLen) latencyHistory.shift();
  drawLatencyChart();
}
function startTicker(){
  if (tickerTimer) clearInterval(tickerTimer);
  tickerOnce();
  tickerTimer = setInterval(tickerOnce, tickerIntervalMs);
}
document.getElementById('pingTickerToggle').onclick = () => {
  tickerEnabled = !tickerEnabled;
  document.getElementById('pingTickerToggle').textContent = tickerEnabled ? 'Disable' : 'Enable';
  if (tickerEnabled) {
    startTicker();
  } else {
    clearInterval(tickerTimer);
    const val = document.getElementById('pingTickerVal');
    val.textContent = 'OFF';
    val.className = 'ind';
  }
};
const latRateSelect = document.getElementById('latRateSelect');
populateRateSelect(latRateSelect, tickerIntervalMs);
latRateSelect.onchange = () => {
  tickerIntervalMs = parseInt(latRateSelect.value, 10);
  latHistoryLen = historyLenForRate(tickerIntervalMs);
  while (latencyHistory.length > latHistoryLen) latencyHistory.shift();
  if (tickerEnabled) startTicker(); // restart at the new cadence without losing history
  drawLatencyChart();
};

function drawLatencyChart(){
  const svg = document.getElementById('latChart');
  const empty = document.getElementById('latEmpty');
  svg.textContent = '';
  const samples = latencyHistory.filter(v => v !== null);
  document.getElementById('latCur').textContent = samples.length ? samples[samples.length - 1].toFixed(0) + 'ms' : '—';
  document.getElementById('latAvg').textContent = samples.length ? (samples.reduce((a,b)=>a+b,0) / samples.length).toFixed(0) + 'ms' : '—';
  document.getElementById('latMax').textContent = samples.length ? Math.max(...samples).toFixed(0) + 'ms' : '—';

  if (samples.length === 0) { empty.style.display = 'flex'; return; }
  empty.style.display = 'none';

  const W = 600, H = 120, padL = 34, padT = 10, padB = 8, padR = 8;
  const plotW = W - padL - padR, plotH = H - padT - padB;
  const niceMax = Math.max(10, Math.ceil(Math.max(...samples) / 10) * 10);

  for (let i = 0; i <= 2; i++){
    const frac = i / 2;
    const y = padT + plotH * (1 - frac);
    svg.appendChild(svgEl('line', { x1:padL, x2:W-padR, y1:y, y2:y, stroke:'#232a36', 'stroke-width':1 }));
    const t = svgEl('text', { x:padL-6, y:y+3, 'text-anchor':'end', fill:'#93a1b7', 'font-size':9, 'font-family':'ui-monospace,Consolas,monospace' });
    t.textContent = Math.round(niceMax * frac);
    svg.appendChild(t);
  }

  function xFor(i){ return padL + plotW * (i / (latHistoryLen - 1)); }
  function yFor(v){ return padT + plotH * (1 - Math.min(v, niceMax) / niceMax); }

  // Break the line at nulls (failed pings) instead of interpolating over them.
  let d = '', drawing = false, lastX = 0, lastY = 0, lastV = 0;
  latencyHistory.forEach((v, i) => {
    if (v === null) { drawing = false; return; }
    const x = xFor(i), y = yFor(v);
    d += (drawing ? 'L' : 'M') + x.toFixed(1) + ',' + y.toFixed(1) + ' ';
    drawing = true;
    lastX = x; lastY = y; lastV = v;
  });
  svg.appendChild(svgEl('path', { d, fill: 'none', stroke: '#3987e5', 'stroke-width': 2, 'stroke-linejoin': 'round', 'stroke-linecap': 'round' }));
  svg.appendChild(svgEl('circle', { cx: lastX, cy: lastY, r: 4, fill: '#3987e5', stroke: '#0d1117', 'stroke-width': 2 }));
  const lbl = svgEl('text', { x: lastX + 7, y: lastY + 3, fill: '#93a1b7', 'font-size': 10, 'font-family': 'ui-monospace,Consolas,monospace' });
  lbl.textContent = lastV.toFixed(0) + 'ms';
  svg.appendChild(lbl);
}

// Bandwidth trend: samples the espRxBytes/espTxBytes/camRxBytes counters
// (already accumulated for free by espFetch and the camera stream reader
// above — this adds no extra network traffic of its own) on a timer, and
// draws them as a small inline-SVG line chart. Two series: ESP32 web
// interface (rx+tx combined) and camera feed (rx only — a video pull has
// no meaningful upload). Colors are the project's validated dark-mode
// categorical slots 1 & 2 (blue/orange). KB/s is normalized by the actual
// elapsed time since the last sample (not assumed to be exactly the
// selected interval), since a rate as fast as 100ms would otherwise read
// ~10x too low from setInterval jitter alone.
const BW_SVG_NS = 'http://www.w3.org/2000/svg';
let bwIntervalMs = 1000;
let bwHistoryLen = historyLenForRate(bwIntervalMs);
let bwTimer = null;
let bwLastSampleMs = Date.now();
let bwHistory = [];
let bwPlotMeta = null; // set by drawBandwidthChart(), read by the hover handler

function svgEl(tag, attrs){
  const el = document.createElementNS(BW_SVG_NS, tag);
  for (const k in attrs) el.setAttribute(k, attrs[k]);
  return el;
}

function sampleBandwidth(){
  const now = Date.now();
  const elapsedS = Math.max(0.001, (now - bwLastSampleMs) / 1000);
  bwLastSampleMs = now;
  const espRxKBps = (espRxBytes / 1024) / elapsedS;
  const espTxKBps = (espTxBytes / 1024) / elapsedS;
  const camRxKBps = (camRxBytes / 1024) / elapsedS;
  bwHistory.push({ espRx: espRxKBps, espTx: espTxKBps, camRx: camRxKBps });
  while (bwHistory.length > bwHistoryLen) bwHistory.shift();
  espRxBytes = 0; espTxBytes = 0; camRxBytes = 0;

  document.getElementById('bwEspRx').textContent = espRxKBps.toFixed(2) + ' KB/s';
  document.getElementById('bwEspTx').textContent = espTxKBps.toFixed(2) + ' KB/s';
  document.getElementById('bwCamRx').textContent = camRxKBps.toFixed(2) + ' KB/s';
  drawBandwidthChart();
}
function startBwSampling(){
  if (bwTimer) clearInterval(bwTimer);
  bwLastSampleMs = Date.now();
  bwTimer = setInterval(sampleBandwidth, bwIntervalMs);
}

function drawBandwidthChart(){
  const svg = document.getElementById('bwChart');
  svg.textContent = ''; // full rebuild each tick; the hover overlay and its listeners are recreated below too
  const W = 600, H = 140, padL = 34, padT = 10, padB = 8, padR = 46;
  const plotW = W - padL - padR, plotH = H - padT - padB;

  const espSeries = bwHistory.map(s => s.espRx + s.espTx);
  const camSeries = bwHistory.map(s => s.camRx);
  const maxVal = Math.max(1, ...espSeries, ...camSeries);
  const niceMax = Math.max(1, Math.ceil(maxVal / 5) * 5);
  bwPlotMeta = { padL, padT, plotW, plotH, niceMax };

  for (let i = 0; i <= 2; i++){
    const frac = i / 2;
    const y = padT + plotH * (1 - frac);
    svg.appendChild(svgEl('line', { x1:padL, x2:W-padR, y1:y, y2:y, stroke:'#232a36', 'stroke-width':1 }));
    const t = svgEl('text', { x:padL-6, y:y+3, 'text-anchor':'end', fill:'#93a1b7', 'font-size':9, 'font-family':'ui-monospace,Consolas,monospace' });
    t.textContent = Math.round(niceMax * frac);
    svg.appendChild(t);
  }

  function xFor(i){ return padL + plotW * (i / (bwHistoryLen - 1)); }
  function yFor(v){ return padT + plotH * (1 - Math.min(v, niceMax) / niceMax); }

  function drawSeries(series, color){
    if (series.length === 0) return;
    let lineD = '', areaD = '';
    series.forEach((v, i) => {
      const x = xFor(i), y = yFor(v);
      lineD += (i === 0 ? 'M' : 'L') + x.toFixed(1) + ',' + y.toFixed(1) + ' ';
    });
    if (series.length > 1) {
      const firstX = xFor(0), lastX = xFor(series.length - 1), baseY = padT + plotH;
      areaD = lineD + 'L' + lastX.toFixed(1) + ',' + baseY + ' L' + firstX.toFixed(1) + ',' + baseY + ' Z';
      svg.appendChild(svgEl('path', { d: areaD, fill: color, 'fill-opacity': 0.1, stroke: 'none' }));
    }
    svg.appendChild(svgEl('path', { d: lineD, fill: 'none', stroke: color, 'stroke-width': 2, 'stroke-linejoin': 'round', 'stroke-linecap': 'round' }));
    const lastI = series.length - 1, lx = xFor(lastI), ly = yFor(series[lastI]);
    svg.appendChild(svgEl('circle', { cx: lx, cy: ly, r: 4, fill: color, stroke: '#0d1117', 'stroke-width': 2 }));
    const lbl = svgEl('text', { x: lx + 7, y: ly + 3, fill: '#93a1b7', 'font-size': 10, 'font-family': 'ui-monospace,Consolas,monospace' });
    lbl.textContent = series[lastI].toFixed(1) + 'K';
    svg.appendChild(lbl);
  }
  drawSeries(espSeries, '#3987e5');
  drawSeries(camSeries, '#d95926');

  // Hover crosshair + tooltip (built fresh each redraw, matches current data).
  const hoverGroup = svgEl('g', { style: 'display:none' });
  const crossLine = svgEl('line', { class: 'bwCrosshair', y1: padT, y2: padT + plotH });
  const tipBg = svgEl('rect', { rx: 4, fill: '#1b212b', stroke: '#3a4256', 'stroke-width': 1 });
  const tipEsp = svgEl('text', { class: 'bwTooltip', fill: '#3987e5', x: 6, y: 14 });
  const tipCam = svgEl('text', { class: 'bwTooltip', fill: '#d95926', x: 6, y: 28 });
  const tipG = svgEl('g');
  tipG.appendChild(tipBg); tipG.appendChild(tipEsp); tipG.appendChild(tipCam);
  hoverGroup.appendChild(crossLine); hoverGroup.appendChild(tipG);
  svg.appendChild(hoverGroup);
  const overlay = svgEl('rect', { x: padL, y: padT, width: plotW, height: plotH, fill: 'transparent' });
  svg.appendChild(overlay);

  overlay.onpointermove = (ev) => {
    if (!bwPlotMeta || bwHistory.length === 0) return;
    const rect = svg.getBoundingClientRect();
    const scaleX = W / rect.width;
    const svgX = (ev.clientX - rect.left) * scaleX;
    const frac = (svgX - padL) / plotW;
    const idx = Math.max(0, Math.min(bwHistory.length - 1, Math.round(frac * (bwHistoryLen - 1))));
    if (idx >= bwHistory.length) { hoverGroup.style.display = 'none'; return; }
    const s = bwHistory[idx];
    const x = xFor(idx);
    crossLine.setAttribute('x1', x); crossLine.setAttribute('x2', x);
    tipEsp.textContent = 'ESP32: ' + (s.espRx + s.espTx).toFixed(2) + ' KB/s';
    tipCam.textContent = 'Camera: ' + s.camRx.toFixed(2) + ' KB/s';
    const tipW = 120, tipH = 34;
    let tipX = x + 8;
    if (tipX + tipW > W - padR) tipX = x - tipW - 8;
    tipG.setAttribute('transform', 'translate(' + tipX + ',' + padT + ')');
    tipBg.setAttribute('width', tipW); tipBg.setAttribute('height', tipH);
    hoverGroup.style.display = 'block';
  };
  overlay.onpointerleave = () => { hoverGroup.style.display = 'none'; };
}
const bwRateSelect = document.getElementById('bwRateSelect');
populateRateSelect(bwRateSelect, bwIntervalMs);
bwRateSelect.onchange = () => {
  bwIntervalMs = parseInt(bwRateSelect.value, 10);
  bwHistoryLen = historyLenForRate(bwIntervalMs);
  while (bwHistory.length > bwHistoryLen) bwHistory.shift();
  startBwSampling();
  drawBandwidthChart();
};
startBwSampling();

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
