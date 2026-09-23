/*
 * WaveShare ESP32-ETH-POE-8DI-8DO — I/O web console + serial CLI
 *
 * Board (ESP32-S3-POE-ETH-8DI-8DO family):
 *   - MCU: ESP32-S3 (native USB CDC)
 *   - Ethernet: WIZnet W5500 over SPI, PoE powered
 *   - 8x digital inputs (DI1-DI8): direct GPIO, optocoupler isolated
 *   - 8x digital outputs (DO1-DO8): TCA9554 I2C GPIO expander -> Darlington sink outputs
 *
 * *** PIN MAP PROVENANCE ***
 * Waveshare's own wiki page could not be *fetched* by URL from the
 * environment that wrote this file (waveshare.com, files.waveshare.com and
 * every mirror tried were blocked by network egress rules), but the DI and
 * I2C/output-expander pins below ARE confirmed straight from Waveshare's
 * own official block diagram (pasted in by the user from the
 * ESP32-S3-POE-ETH-8DI-8DO wiki page):
 *   - DI1-DI8  -> GPIO4,5,6,7,8,9,10,11 ("Digital Input Detect Pin 1-8")
 *   - I2C bus  -> SDA=GPIO42, SCL=GPIO41, driving a TCA9554PWR expander
 *                 whose EXIO1-EXIO8 -> optocoupler isolation -> DO1-DO8
 * The I2C pins, TCA9554 address/registers, and DI GPIOs are additionally
 * confirmed by a serial CLI sketch the user has actually run and verified
 * working on their own board — that sketch is also the source for
 * INPUTS_ACTIVE_LOW being true (an energized input reads GPIO LOW on this
 * board), overriding an earlier, wrong active-high guess taken from the
 * ESP-IDF reference driver's own configurable default.
 * The W5500 Ethernet SPI pins are NOT shown in that diagram crop and are
 * not exercised by the user's tested sketch either, so they remain sourced
 * from hennejg/waveshare-ESP32-S3-io, an ESP-IDF firmware repo built
 * specifically for this board family (components/board/eth.c) — real,
 * exercised driver code, not a secondhand summary, but still one step
 * short of a source the user has personally verified. If you can see the
 * Ethernet block of the official diagram, or have run an Ethernet-using
 * sketch on this board, double check ETH_PHY_CS/IRQ/RST/SCK/MISO/MOSI
 * below against it.
 * Waveshare also sells other similarly named boards (ESP32-S3-ETH-8DI-8RO,
 * plain ESP32-ETH-POE-8DI-8DO without "S3", ...) that may differ, so if
 * your own silkscreen/manual disagrees anywhere, trust your own hardware
 * and fix the "BOARD PIN MAP" block below — everything else in this sketch
 * only talks to hardware through readInput()/setOutput()-style helpers, so
 * that block is normally the only change needed.
 *
 * Arduino IDE setup:
 *   - Boards Manager: install "esp32" by Espressif Systems (core 3.x)
 *   - Board: an ESP32S3 Dev Module variant matching this board (enable PSRAM
 *     if fitted)
 *   - Tools > USB CDC On Boot: Enabled  (required for the Serial CLI over the
 *     native USB port)
 *   - No extra libraries required — only Arduino-ESP32 built-ins (ETH, WebServer,
 *     Wire, SPI) are used.
 */

#include <ETH.h>
#include <WebServer.h>
#include <SPI.h>
#include <Wire.h>

// ---------------------------------------------------------------------------
// BOARD PIN MAP — VERIFY AGAINST YOUR BOARD BEFORE FLASHING
// ---------------------------------------------------------------------------

// W5500 Ethernet (SPI)
#define ETH_PHY_TYPE_  ETH_PHY_W5500
#define ETH_PHY_ADDR_  1
#define ETH_PHY_CS     16
#define ETH_PHY_IRQ    12
#define ETH_PHY_RST    39
#define ETH_SPI_SCK    15
#define ETH_SPI_MISO   14
#define ETH_SPI_MOSI   13
#define ETH_SPI_HOST_  SPI2_HOST

// I2C bus (TCA9554PWR output expander lives here — confirmed pins from
// Waveshare's own block diagram)
#define I2C_SDA_PIN    42
#define I2C_SCL_PIN    41
#define PCA9554_ADDR   0x20 // TCA9554PWR default address (A0-A2 tied low)

// Digital inputs DI1..DI8 (direct GPIO, internal pull-up enabled).
static const uint8_t DI_PINS[8] = {4, 5, 6, 7, 8, 9, 10, 11};
// Confirmed active-LOW (an energized input reads GPIO LOW) by a serial CLI
// sketch the user has actually run and verified working on this exact
// board — the strongest evidence available for this field, overriding the
// ESP-IDF reference driver's own (differently-defaulted, invert-configurable)
// assumption used earlier. Flip to false only if your own board reads
// backward.
static const bool INPUTS_ACTIVE_LOW = true;

// Digital outputs DO1..DO8 (TCA9554 expander pins 0..7 drive Darlington
// sink outputs). Confirmed active-LOW (writing the expander pin LOW turns
// the Darlington ON) by the user's own tested hardware — "on" was
// physically turning outputs off with the earlier active-high default,
// so that guess (taken from a reference driver, not this board) was wrong.
static const bool OUTPUTS_ACTIVE_LOW = true;

// ---------------------------------------------------------------------------
// Device identity / behavior
// ---------------------------------------------------------------------------

static const char *HOSTNAME = "buck-timmy-io-8di8do";
static const unsigned long DEBOUNCE_MS = 20;
static const unsigned long HEARTBEAT_MS = 30000; // periodic serial status ping

WebServer server(80);

// ---------------------------------------------------------------------------
// Event log (ring buffer) — backs the web UI's Status / Debug panel
// ---------------------------------------------------------------------------

#define LOG_CAPACITY 30
static String logBuffer[LOG_CAPACITY];
static uint8_t logHead = 0;
static uint8_t logCount = 0;

static void logEvent(const String &msg) {
  char stamped[96];
  snprintf(stamped, sizeof(stamped), "[%10lu] %s", millis(), msg.c_str());
  Serial.println(stamped);
  logBuffer[logHead] = String(stamped);
  logHead = (logHead + 1) % LOG_CAPACITY;
  if (logCount < LOG_CAPACITY) logCount++;
}

// ---------------------------------------------------------------------------
// Ethernet: DHCP client with link up/down auto-recovery reporting
// ---------------------------------------------------------------------------

static bool ethLinkUp = false;
static bool ethHasIp = false;
static IPAddress ethIp;
static IPAddress ethSubnet;
static IPAddress ethGateway;
static unsigned long ethLinkUpSinceMs = 0;

static void onEthEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname(HOSTNAME);
      logEvent("[ETH] driver started, waiting for link...");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      ethLinkUp = true;
      ethLinkUpSinceMs = millis();
      logEvent("[ETH] link UP — requesting DHCP lease...");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP: {
      ethHasIp = true;
      ethIp = ETH.localIP();
      ethSubnet = ETH.subnetMask();
      ethGateway = ETH.gatewayIP();
      char msg[128];
      snprintf(msg, sizeof(msg), "[ETH] DHCP OK — IP %s  subnet %s  gateway %s  MAC %s  %s",
               ethIp.toString().c_str(), ethSubnet.toString().c_str(),
               ethGateway.toString().c_str(), ETH.macAddress().c_str(),
               ETH.fullDuplex() ? "full-duplex" : "half-duplex");
      logEvent(msg);
      break;
    }
    case ARDUINO_EVENT_ETH_LOST_IP:
      ethHasIp = false;
      ethIp = IPAddress(0, 0, 0, 0);
      ethSubnet = IPAddress(0, 0, 0, 0);
      ethGateway = IPAddress(0, 0, 0, 0);
      logEvent("[ETH] DHCP lease lost — link still up, will re-DHCP");
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      ethLinkUp = false;
      ethHasIp = false;
      ethIp = IPAddress(0, 0, 0, 0);
      ethSubnet = IPAddress(0, 0, 0, 0);
      ethGateway = IPAddress(0, 0, 0, 0);
      logEvent("[ETH] link DOWN — cable unplugged, will re-DHCP on reconnect");
      break;
    case ARDUINO_EVENT_ETH_STOP:
      ethLinkUp = false;
      ethHasIp = false;
      logEvent("[ETH] driver stopped");
      break;
    default:
      break;
  }
}

static void ethInit() {
  Network.onEvent(onEthEvent);
  ETH.begin(ETH_PHY_TYPE_, ETH_PHY_ADDR_, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST,
            ETH_SPI_HOST_, ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI);
  // No static IP is configured, so ETH stays in DHCP client mode; the
  // Arduino-ESP32 network stack automatically re-runs DHCP whenever the
  // link comes back up (ARDUINO_EVENT_ETH_CONNECTED -> ...GOT_IP above),
  // which is what reports reconnects to the serial monitor.
}

// ---------------------------------------------------------------------------
// Digital outputs — PCA9554 I2C GPIO expander (DO1..DO8 -> expander bits 0..7)
// ---------------------------------------------------------------------------

#define PCA9554_REG_INPUT   0x00
#define PCA9554_REG_OUTPUT  0x01
#define PCA9554_REG_POLARITY 0x02
#define PCA9554_REG_CONFIG  0x03

static uint8_t outputState = 0x00; // bit i = DO(i+1), 1 = commanded ON

static bool pca9554Write(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(PCA9554_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static void outputsInit() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  pca9554Write(PCA9554_REG_CONFIG, 0x00);   // all 8 expander pins as outputs
  pca9554Write(PCA9554_REG_OUTPUT, OUTPUTS_ACTIVE_LOW ? 0xFF : 0x00); // all off
}

static void applyOutputs() {
  uint8_t wire = OUTPUTS_ACTIVE_LOW ? (uint8_t)~outputState : outputState;
  if (!pca9554Write(PCA9554_REG_OUTPUT, wire)) {
    logEvent("[DO] I2C write to PCA9554 failed — check wiring/address");
  }
}

static bool setOutput(uint8_t channel1to8, bool on) {
  if (channel1to8 < 1 || channel1to8 > 8) return false;
  uint8_t bit = channel1to8 - 1;
  bool wasOn = outputState & (1 << bit);
  if (on) outputState |= (1 << bit); else outputState &= ~(1 << bit);
  applyOutputs();
  if (wasOn != on) {
    char msg[32];
    snprintf(msg, sizeof(msg), "[DO%u] -> %s", channel1to8, on ? "ON" : "OFF");
    logEvent(msg);
  }
  return true;
}

static bool getOutput(uint8_t channel1to8) {
  if (channel1to8 < 1 || channel1to8 > 8) return false;
  return outputState & (1 << (channel1to8 - 1));
}

static void setAllOutputs(bool on) {
  outputState = on ? 0xFF : 0x00;
  applyOutputs();
  logEvent(on ? "[DO] all -> ON" : "[DO] all -> OFF");
}

// ---------------------------------------------------------------------------
// Digital inputs — direct GPIO, debounced, auto-report on change
// ---------------------------------------------------------------------------

static bool diRaw[8] = {false};
static bool diStable[8] = {false};
static unsigned long diLastChangeMs[8] = {0};

static void inputsInit() {
  for (uint8_t i = 0; i < 8; i++) pinMode(DI_PINS[i], INPUT_PULLUP);
}

static void pollInputs() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < 8; i++) {
    bool level = digitalRead(DI_PINS[i]) == LOW;
    bool active = INPUTS_ACTIVE_LOW ? level : !level;
    if (active != diRaw[i]) {
      diRaw[i] = active;
      diLastChangeMs[i] = now;
    }
    if (diStable[i] != diRaw[i] && (now - diLastChangeMs[i]) >= DEBOUNCE_MS) {
      diStable[i] = diRaw[i];
      char msg[24];
      snprintf(msg, sizeof(msg), "[DI%u] %s", i + 1, diStable[i] ? "ACTIVE" : "inactive");
      logEvent(msg);
    }
  }
}

// ---------------------------------------------------------------------------
// Web GUI client tracking — who's actually polling us, and are they still
// live or just a stale/cached browser tab that stopped talking to us
// ---------------------------------------------------------------------------

#define MAX_TRACKED_CLIENTS 6
#define CLIENT_STALE_MS 3000 // no request from this IP in this long = stale

struct WebClient {
  IPAddress ip;
  String userAgent;
  unsigned long lastSeenMs;
  bool used;
  bool stale; // last-logged staleness state, so transitions log once
};
static WebClient webClients[MAX_TRACKED_CLIENTS];

static void noteWebClient(IPAddress ip, const String &ua) {
  unsigned long now = millis();
  int freeSlot = -1;
  for (int i = 0; i < MAX_TRACKED_CLIENTS; i++) {
    if (webClients[i].used && webClients[i].ip == ip) {
      bool wasStale = webClients[i].stale;
      webClients[i].lastSeenMs = now;
      webClients[i].stale = false;
      if (ua.length()) webClients[i].userAgent = ua;
      if (wasStale) {
        char msg[64];
        snprintf(msg, sizeof(msg), "[WEB] client %s reconnected", ip.toString().c_str());
        logEvent(msg);
      }
      return;
    }
    if (!webClients[i].used && freeSlot == -1) freeSlot = i;
  }
  // New client. If the table is full, evict whichever tracked client was
  // seen longest ago rather than refusing to track a new one.
  int slot = freeSlot;
  if (slot == -1) {
    unsigned long oldest = now + 1;
    slot = 0;
    for (int i = 0; i < MAX_TRACKED_CLIENTS; i++) {
      if (webClients[i].lastSeenMs < oldest) {
        oldest = webClients[i].lastSeenMs;
        slot = i;
      }
    }
  }
  webClients[slot].ip = ip;
  webClients[slot].userAgent = ua;
  webClients[slot].lastSeenMs = now;
  webClients[slot].used = true;
  webClients[slot].stale = false;
  char msg[80];
  snprintf(msg, sizeof(msg), "[WEB] new client connected: %s", ip.toString().c_str());
  logEvent(msg);
}

static void scanClientStaleness() {
  unsigned long now = millis();
  for (int i = 0; i < MAX_TRACKED_CLIENTS; i++) {
    if (!webClients[i].used) continue;
    unsigned long agoMs = now - webClients[i].lastSeenMs;
    if (agoMs > CLIENT_STALE_MS && !webClients[i].stale) {
      webClients[i].stale = true;
      char msg[80];
      snprintf(msg, sizeof(msg), "[WEB] client %s went stale — no response %lus",
               webClients[i].ip.toString().c_str(), agoMs / 1000);
      logEvent(msg);
    }
  }
}

// ---------------------------------------------------------------------------
// Web UI — single page, polls /api/status every 150ms for near-real-time view
// ---------------------------------------------------------------------------

static IPAddress lastClientIp(0, 0, 0, 0);
static String lastClientUserAgent = "(none yet)";

#include "web_page.h"

static void captureClientInfo() {
  lastClientIp = server.client().remoteIP();
  if (server.hasHeader("User-Agent")) lastClientUserAgent = server.header("User-Agent");
  noteWebClient(lastClientIp, lastClientUserAgent);
}

static void handleRoot() {
  captureClientInfo();
  server.send_P(200, "text/html", INDEX_HTML);
}

static String jsonEscape(const String &in) {
  String out;
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '"' || c == '\\') out += '\\';
    out += c;
  }
  return out;
}

static void handleApiStatus() {
  captureClientInfo();
  String json = "{";
  json += "\"hostname\":\"" + String(HOSTNAME) + "\",";
  json += "\"eth_link\":" + String(ethLinkUp ? "true" : "false") + ",";
  json += "\"eth_ip\":\"" + (ethHasIp ? ethIp.toString() : String("0.0.0.0")) + "\",";
  json += "\"eth_subnet\":\"" + (ethHasIp ? ethSubnet.toString() : String("0.0.0.0")) + "\",";
  json += "\"eth_gateway\":\"" + (ethHasIp ? ethGateway.toString() : String("0.0.0.0")) + "\",";
  json += "\"mac\":\"" + ETH.macAddress() + "\",";
  json += "\"uptime_s\":" + String(millis() / 1000) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"client_ip\":\"" + lastClientIp.toString() + "\",";
  json += "\"client_ua\":\"" + jsonEscape(lastClientUserAgent) + "\",";
  json += "\"di\":[";
  for (uint8_t i = 0; i < 8; i++) { json += diStable[i] ? "true" : "false"; if (i < 7) json += ","; }
  json += "],\"do\":[";
  for (uint8_t i = 0; i < 8; i++) { json += getOutput(i + 1) ? "true" : "false"; if (i < 7) json += ","; }
  json += "]}";
  server.send(200, "application/json", json);
}

static void handleApiLog() {
  String json = "{\"lines\":[";
  for (uint8_t n = 0; n < logCount; n++) {
    uint8_t idx = (logHead + LOG_CAPACITY - logCount + n) % LOG_CAPACITY;
    json += "\"" + jsonEscape(logBuffer[idx]) + "\"";
    if (n < logCount - 1) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

static void handleApiClients() {
  captureClientInfo();
  unsigned long now = millis();
  String json = "{\"clients\":[";
  bool first = true;
  for (int i = 0; i < MAX_TRACKED_CLIENTS; i++) {
    if (!webClients[i].used) continue;
    if (!first) json += ",";
    first = false;
    unsigned long agoMs = now - webClients[i].lastSeenMs;
    json += "{\"ip\":\"" + webClients[i].ip.toString() + "\",";
    json += "\"ua\":\"" + jsonEscape(webClients[i].userAgent) + "\",";
    json += "\"ago_ms\":" + String(agoMs) + ",";
    json += "\"active\":" + String(agoMs <= CLIENT_STALE_MS ? "true" : "false") + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

static void handleApiOutput() {
  captureClientInfo();
  if (!server.hasArg("ch") || !server.hasArg("state")) {
    server.send(400, "text/plain", "missing ch/state");
    return;
  }
  int ch = server.arg("ch").toInt();
  bool on = server.arg("state") == "on";
  if (!setOutput((uint8_t)ch, on)) {
    server.send(400, "text/plain", "invalid channel");
    return;
  }
  server.send(200, "text/plain", "ok");
}

static void webInit() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/log", HTTP_GET, handleApiLog);
  server.on("/api/clients", HTTP_GET, handleApiClients);
  server.on("/api/output", HTTP_POST, handleApiOutput);
  server.begin();
}

// ---------------------------------------------------------------------------
// Serial CLI (USB CDC) — help / status / on / off / toggle
// ---------------------------------------------------------------------------

static String cliBuffer;

static void printHelp() {
  Serial.println(F("Commands:"));
  Serial.println(F("  help              - show this help"));
  Serial.println(F("  status            - show DI/DO states and Ethernet status"));
  Serial.println(F("  on <1-8>          - turn output DOn ON"));
  Serial.println(F("  off <1-8>         - turn output DOn OFF"));
  Serial.println(F("  toggle <1-8>      - toggle output DOn"));
  Serial.println(F("  all on            - turn all 8 outputs ON"));
  Serial.println(F("  all off           - turn all 8 outputs OFF"));
  Serial.println(F("  ip                - show current DHCP IP / link state"));
}

static void printStatus() {
  Serial.println(F("--- status ---"));
  Serial.print(F("ETH link:   ")); Serial.println(ethLinkUp ? F("UP") : F("DOWN"));
  Serial.print(F("ETH IP:     ")); Serial.println(ethHasIp ? ethIp.toString() : String("0.0.0.0"));
  Serial.print(F("Subnet:     ")); Serial.println(ethHasIp ? ethSubnet.toString() : String("0.0.0.0"));
  Serial.print(F("Gateway:    ")); Serial.println(ethHasIp ? ethGateway.toString() : String("0.0.0.0"));
  Serial.print(F("MAC:        ")); Serial.println(ETH.macAddress());
  Serial.print(F("Uptime:     ")); Serial.print(millis() / 1000); Serial.println(F("s"));
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print(F("DI")); Serial.print(i + 1); Serial.print(F(": "));
    Serial.println(diStable[i] ? F("ACTIVE") : F("inactive"));
  }
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print(F("DO")); Serial.print(i + 1); Serial.print(F(": "));
    Serial.println(getOutput(i + 1) ? F("ON") : F("OFF"));
  }
  Serial.println(F("--------------"));
}

static void processCliCommand(String line) {
  line.trim();
  if (line.length() == 0) return;
  int sp = line.indexOf(' ');
  String cmd = (sp == -1) ? line : line.substring(0, sp);
  String arg = (sp == -1) ? "" : line.substring(sp + 1);
  cmd.toLowerCase();
  arg.trim();

  if (cmd == "help") {
    printHelp();
  } else if (cmd == "status") {
    printStatus();
  } else if (cmd == "ip") {
    Serial.print(F("link=")); Serial.print(ethLinkUp ? F("up") : F("down"));
    Serial.print(F(" ip=")); Serial.print(ethHasIp ? ethIp.toString() : String("0.0.0.0"));
    Serial.print(F(" subnet=")); Serial.print(ethHasIp ? ethSubnet.toString() : String("0.0.0.0"));
    Serial.print(F(" gateway=")); Serial.println(ethHasIp ? ethGateway.toString() : String("0.0.0.0"));
  } else if (cmd == "all") {
    String a = arg;
    a.toLowerCase();
    if (a == "on") {
      setAllOutputs(true);
      Serial.println(F("All outputs ON"));
    } else if (a == "off") {
      setAllOutputs(false);
      Serial.println(F("All outputs OFF"));
    } else {
      Serial.println(F("error: expected 'all on' or 'all off'"));
    }
  } else if (cmd == "on" || cmd == "off" || cmd == "toggle") {
    int ch = arg.toInt();
    if (ch < 1 || ch > 8) {
      Serial.println(F("error: channel must be 1-8, e.g. 'on 3'"));
      return;
    }
    bool target = (cmd == "on") ? true : (cmd == "off") ? false : !getOutput(ch);
    setOutput((uint8_t)ch, target);
  } else {
    Serial.print(F("Unknown command: "));
    Serial.println(cmd);
    Serial.println(F("Type 'help' for a list of commands."));
  }
}

static void handleSerialCli() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      processCliCommand(cliBuffer);
      cliBuffer = "";
    } else {
      cliBuffer += c;
      if (cliBuffer.length() > 64) cliBuffer = ""; // guard against runaway input
    }
  }
}

// ---------------------------------------------------------------------------
// Setup / loop
// ---------------------------------------------------------------------------

static unsigned long lastHeartbeat = 0;
static unsigned long lastDhcpWaitMsg = 0;
static const unsigned long DHCP_WAIT_MSG_MS = 5000; // remind every 5s while link is up but no lease yet
static unsigned long lastClientStalenessScan = 0;
static const unsigned long CLIENT_STALENESS_SCAN_MS = 1000;

void setup() {
  Serial.begin(115200);
  delay(300); // let native USB CDC enumerate before first prints
  Serial.println();
  Serial.println(F("=== WaveShare ESP32 8DI/8DO — boot ==="));
  Serial.println(F("Type 'help' for available commands."));

  inputsInit();
  outputsInit();
  ethInit();
  webInit();

  logEvent("[BOOT] setup complete, web console on port 80");
}

void loop() {
  server.handleClient();
  pollInputs();
  handleSerialCli();

  unsigned long now = millis();
  if (now - lastHeartbeat >= HEARTBEAT_MS) {
    lastHeartbeat = now;
    char msg[80];
    snprintf(msg, sizeof(msg), "[HEARTBEAT] eth=%s ip=%s heap=%u",
             ethLinkUp ? "up" : "down",
             ethHasIp ? ethIp.toString().c_str() : "0.0.0.0",
             (unsigned)ESP.getFreeHeap());
    logEvent(msg);
  }

  // Visible proof the board is actively retrying DHCP, not silently stuck —
  // if this keeps printing for more than ~30-60s, the board itself is fine
  // and the problem is network-side (no DHCP server reachable on this link,
  // wrong switch port, etc.), not a firmware issue.
  if (ethLinkUp && !ethHasIp && (now - lastDhcpWaitMsg >= DHCP_WAIT_MSG_MS)) {
    lastDhcpWaitMsg = now;
    char msg[64];
    snprintf(msg, sizeof(msg), "[ETH] still waiting for DHCP lease... (%lus since link up)",
             (now - ethLinkUpSinceMs) / 1000);
    logEvent(msg);
  }

  if (now - lastClientStalenessScan >= CLIENT_STALENESS_SCAN_MS) {
    lastClientStalenessScan = now;
    scanClientStaleness();
  }
}
