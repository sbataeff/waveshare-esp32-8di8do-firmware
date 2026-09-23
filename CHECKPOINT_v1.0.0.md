# Checkpoint — v1.0.0

First stable checkpoint. This file distinguishes three different levels of
confidence — don't read "verified" as "confirmed on your board" unless it
says so explicitly.

## Confirmed on real hardware (by the user)

- I2C bus (SDA=GPIO42, SCL=GPIO41) and TCA9554PWR expander at address
  `0x20` — matches both Waveshare's official block diagram and a serial
  CLI program the user wrote and ran themselves
- Digital inputs DI1-DI8 on GPIO4-11, **active-LOW** (energized reads GPIO
  LOW)
- Digital outputs DO1-DO8 via the TCA9554, **active-LOW** (writing the
  expander pin low turns the Darlington on)
- Serial CLI compiles and runs: `help`, `status`, `on/off/toggle <1-8>`,
  `all on`, `all off`, `ip`
- Web UI loads and serves from the board: DI indicators, DO toggle buttons
  with optimistic click feedback, status/debug log, connected-client panel

## Verified only by automated smoke test (Playwright against a mock server
## standing in for the ESP32) — not yet exercised on the actual board

Everything added after the checkpoint above was built against a Node-based
mock server replicating the firmware's HTTP responses, not the real
ESP32-S3, because this development environment has no physical hardware
access. Each of these passed its own targeted Playwright test (frame
parsing, byte counts, timer restarts, network isolation, etc. — see the
corresponding commit messages for what each test actually checked), but
none of that is a substitute for running on the real board:

- Ethernet subnet/gateway reporting, `ARDUINO_EVENT_ETH_LOST_IP` handling,
  the "still waiting for DHCP" heartbeat
- GUI client connection tracking (`/api/clients`) and the page-side
  connection watchdog banner
- IP camera feed: rewritten from `<img src>` to a hand-rolled
  `multipart/x-mixed-replace` parser (needed for real byte counting),
  with 1s auto-reconnect
- Connection Speed Test (`/api/ping?size=N`, 1B-1MB) and the live latency
  ticker
- Bandwidth Trend and Latency Trend charts, each with an independent
  0.1s-2s update-rate dropdown
- The top-right master ENABLE/DISABLE switch — stops every timer, aborts
  in-flight requests via a shared `AbortController`, and refuses new
  `espFetch()` calls outright while disabled

## Not yet confirmed at all

- W5500 Ethernet SPI pins (CS=GPIO16, IRQ=GPIO12, RST=GPIO39, SCK=GPIO15,
  MISO=GPIO14, MOSI=GPIO13) — sourced from `hennejg/waveshare-ESP32-S3-io`
  driver code, not from the user's own tested hardware or the official
  Waveshare diagram (that pin block was never shared)
- Whether DHCP actually completes on the real board (link-up was observed
  in an earlier serial log; a completed lease with IP/subnet/gateway
  printed was not, as of this checkpoint) — if it's still stuck, the
  "still waiting for DHCP" heartbeat added since should show whether it's
  actively retrying or genuinely stuck, which is the next thing to check
  against the real hardware

## Known-good commit

`a6b17f6` — "Add top-right ENABLE/DISABLE master switch to stop all ESP32
traffic"
