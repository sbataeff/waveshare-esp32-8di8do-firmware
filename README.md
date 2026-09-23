# WaveShare ESP32-S3-POE-ETH-8DI-8DO Firmware

**v1.0.0** — see [`CHECKPOINT_v1.0.0.md`](CHECKPOINT_v1.0.0.md) for what's
confirmed on real hardware vs. still unverified.

Arduino IDE firmware for the WaveShare ESP32-S3-POE-ETH-8DI-8DO industrial
I/O board (8 digital inputs, 8 digital outputs, PoE Ethernet, native USB).

Sketch and full documentation: [`waveshare_esp32_eth_poe_8di_8do/`](waveshare_esp32_eth_poe_8di_8do/).

At a glance, the firmware provides:

- Ethernet as a DHCP client on the onboard W5500 port, with auto-recovery
  (re-DHCP + serial logging) on cable disconnect/reconnect, subnet/gateway
  reporting, and a "still waiting for DHCP" heartbeat so a stuck lease is
  visibly distinguishable from a firmware bug
- A self-hosted web UI: live input/output status and control, a device info
  panel, connected-GUI-client tracking with a stale/live watchdog, an IP
  camera (MJPEG) feed with auto-reconnect, a connection speed test with
  adjustable payload size, live bandwidth and latency trend charts (each
  with its own adjustable update rate), and a top-right master
  ENABLE/DISABLE switch that stops all communication with the board to save
  hotspot data when the page is just left open
- A USB CDC serial CLI (`help`, `status`, `on/off/toggle <1-8>`, `all
  on/off`, `ip`) with automatic per-channel serial reporting when an input
  changes state

See the sketch folder's README for the full pin map, its provenance, and
Arduino IDE setup instructions.

## Revision tracking

Every change lands as its own descriptive commit on `main` — that commit
history is the primary changelog. Meaningful milestones also get a
`CHECKPOINT_*.md` file at the repo root summarizing what's confirmed on
real hardware vs. still unverified at that point, so you don't have to
reconstruct that from the commit log.
