# WaveShare ESP32-S3-POE-ETH-8DI-8DO Firmware

Arduino IDE firmware for the WaveShare ESP32-S3-POE-ETH-8DI-8DO industrial
I/O board (8 digital inputs, 8 digital outputs, PoE Ethernet, native USB).

Sketch and full documentation: [`waveshare_esp32_eth_poe_8di_8do/`](waveshare_esp32_eth_poe_8di_8do/).

At a glance, the firmware provides:

- Ethernet as a DHCP client on the onboard W5500 port, with auto-recovery
  (re-DHCP + serial logging) on cable disconnect/reconnect
- A self-hosted web UI showing live input status, output toggle controls,
  a status/debug log, and the connected browser's IP/User-Agent
- A USB CDC serial CLI (`help`, `status`, `on/off/toggle <1-8>`, `ip`) with
  automatic per-channel serial reporting when an input changes state

See the sketch folder's README for the full pin map, its provenance, and
Arduino IDE setup instructions.
