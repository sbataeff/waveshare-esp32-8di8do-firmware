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
- Ethernet DHCP completes on the real board: serial log shows a clean
  `eth=up ip=10.228.211.113` heartbeat, no "still waiting for DHCP" stall.
  Since link came up and a full lease was obtained, this also validates
  the W5500 SPI pin block (CS=GPIO16, IRQ=GPIO12, RST=GPIO39, SCK=GPIO15,
  MISO=GPIO14, MOSI=GPIO13) by extension — wrong pins would not produce a
  working link+lease
- GUI client connection tracking (server side): serial log shows
  `noteWebClient()`/`markStaleClients()` firing correctly — "new client
  connected", "went stale — no response Ns", "reconnected" all logged as
  expected for a real browser hitting the board

## Watch items (working, but worth a longer look)

- One connected client cycled stale→reconnect every ~8-10s instead of
  staying continuously live, even though the page polls every 150ms and
  `CLIENT_STALE_MS` is 3000ms. Most likely explanation: the browser tab
  was in the background (serial monitor in focus instead) and got its JS
  timers throttled by the browser — not a firmware bug. Worth a quick
  recheck with the web UI tab kept in the foreground; if it still goes
  stale there, that would point at a real polling bug instead.
- Free heap drifted from 302292 to 296100 (~6KB) over about 5 minutes with
  one client connected. Could be normal fragmentation from per-request
  JSON/String building, could be a slow leak — not enough data yet from a
  short run. Worth leaving it running (with a client connected) for 30+
  minutes to see if it plateaus.

## Verified only by automated smoke test (Playwright against a mock server
## standing in for the ESP32) — not yet exercised on the actual board

Everything added after the checkpoint above was built against a Node-based
mock server replicating the firmware's HTTP responses, not the real
ESP32-S3, because this development environment has no physical hardware
access. Each of these passed its own targeted Playwright test (frame
parsing, byte counts, timer restarts, network isolation, etc. — see the
corresponding commit messages for what each test actually checked), but
none of that is a substitute for running on the real board:

- The page-side connection watchdog banner (server-side client tracking is
  now confirmed above, but the browser-side "⚠ NOT LIVE" banner itself
  hasn't been visually confirmed on the real board yet)
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

- IP camera feed, Connection Speed Test/latency ticker, Bandwidth/Latency
  Trend charts, and the ENABLE/DISABLE master switch — still only
  exercised against the mock server, not the real board (see the smoke
  test list above)

## Known-good commit

`72a9e01` — "Checkpoint v1.0.0: version marker, updated README,
hardware-vs-test-only status" (tagged as the `v1.0.0` GitHub release)
