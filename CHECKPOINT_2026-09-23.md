# Checkpoint — 2026-09-23

Working state of `waveshare_esp32_eth_poe_8di_8do.ino`, confirmed against
real hardware unless noted.

## Confirmed on real hardware
- I2C bus (SDA=GPIO42, SCL=GPIO41) and TCA9554PWR expander at address
  `0x20` — matches both Waveshare's official block diagram and the user's
  own tested serial CLI program
- Digital inputs DI1-DI8 on GPIO4-11, **active-LOW** (energized reads GPIO
  LOW)
- Digital outputs DO1-DO8 via the TCA9554, **active-LOW** (writing the
  expander pin low turns the Darlington on)
- Serial CLI: `help`, `status`, `on/off/toggle <1-8>`, `all on`, `all off`,
  `ip`
- Web UI: live DI indicators (styled like the DO buttons), DO toggle
  buttons with optimistic (instant) click feedback, status/debug log,
  connected-client panel

## Not yet confirmed on hardware
- W5500 Ethernet SPI pins (CS=GPIO16, IRQ=GPIO12, RST=GPIO39, SCK=GPIO15,
  MISO=GPIO14, MOSI=GPIO13) — sourced from `hennejg/waveshare-ESP32-S3-io`
  driver code, not from the user's own tested hardware or the official
  diagram (that pin block wasn't in the diagram crop shared so far)
- Ethernet DHCP auto-recovery on cable disconnect/reconnect — implemented
  via the Arduino-ESP32 ETH event system and logs to serial, but not yet
  observed working on the actual board

## Known-good commit
`920e06a` — "Web UI: instant output clicks and tighter, non-overlapping
polling"
