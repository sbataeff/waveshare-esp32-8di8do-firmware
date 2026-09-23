# WaveShare ESP32-ETH-POE-8DI-8DO — I/O web console + serial CLI

Arduino IDE sketch for the WaveShare ESP32 PoE Ethernet board with 8 digital
inputs and 8 digital outputs.

## ⚠️ Pin map provenance

Waveshare's own wiki page (`www.waveshare.com`, `files.waveshare.com`, and
every mirror tried — `manuals.plus`, `openelab.io`, `letscontrolit.com`,
`spotpear.com`, archive.org, a fetch proxy) could not be *fetched by URL*
from this environment — all blocked by network egress rules.

However, the DI and I2C/output-expander pins **are now confirmed directly
from Waveshare's own official block diagram** (the user pasted in the
relevant crop from the ESP32-S3-POE-ETH-8DI-8DO wiki page):

- Digital inputs DI1-DI8 → GPIO4, 5, 6, 7, 8, 9, 10, 11 ("Digital Input
  Detect Pin 1-8"), matches this sketch exactly
- I2C bus → SDA=GPIO42, SCL=GPIO41, driving a **TCA9554PWR** expander
  whose EXIO1-EXIO8 go through optocoupler isolation to DO1-DO8, matches
  this sketch exactly

On top of that, the user has actually run a serial CLI sketch on their own
board and confirmed it works — the strongest evidence available. That
sketch independently confirms the same I2C pins, TCA9554 address `0x20`,
and DI GPIOs, and settles one thing the diagram doesn't show: **inputs are
active-LOW** (an energized input reads GPIO LOW), not active-high as
originally guessed from the ESP-IDF reference driver's own (configurable)
default.

The W5500 Ethernet SPI pins were not visible in that diagram crop, and
aren't exercised by the user's tested sketch either, so they remain sourced
from the driver code of
[`hennejg/waveshare-ESP32-S3-io`](https://github.com/hennejg/waveshare-ESP32-S3-io),
an ESP-IDF firmware repo built and maintained specifically for the
ESP32-S3-POE-ETH-8DI-8DO / 8DI-8RO board family
(`components/board/eth.c`) — real, exercised driver code, but one step
short of the official diagram:

- W5500 Ethernet (SPI2_HOST): CS=GPIO16, IRQ=GPIO12, RST=GPIO39,
  SCK=GPIO15, MISO=GPIO14, MOSI=GPIO13, PHY addr 1
- TCA9554PWR output expander at I2C address `0x20` (config reg `0x03`,
  output reg `0x01`) driving DO1-DO8 on expander pins 0-7

If you can get the Ethernet block of the official diagram (it wasn't in the
crop shared so far), it's worth a final cross-check against those five W5500
pins.

Waveshare still sells other similarly named boards (`ESP32-S3-ETH-8DI-8RO`,
a plain `ESP32-ETH-POE-8DI-8DO` without "S3", ...) that may differ, so if
your own board's silkscreen/manual disagrees anywhere above, trust your own
hardware and fix the `BOARD PIN MAP` block. Everything else in the sketch
talks to hardware only through `readInput()`/`setOutput()`-style helpers,
so that block is normally the only change needed.

## What it does

- **Ethernet as a DHCP client** on the onboard W5500 port. No static IP is
  configured, so the board always requests a lease via DHCP.
- **Auto-reconnect handling**: link up/down transitions are handled by the
  Arduino-ESP32 network event system. On cable disconnect it logs
  `[ETH] link DOWN`; on reconnect it logs `[ETH] link UP — requesting DHCP
  lease...` followed by `[ETH] DHCP OK — IP ...` once a new lease is
  obtained — no reboot needed, and every transition is printed to the serial
  monitor.
- **Web UI** (served from the board itself, plain HTML/CSS/JS, no external
  libraries) at `http://<board-ip>/`:
  - Live ON/OFF indicators for all 8 inputs, styled the same as the output
    buttons (polls every 250ms)
  - Toggle buttons for all 8 outputs
  - A "Device / Connected client" panel showing the board's hostname,
    Ethernet link state, DHCP IP, MAC, uptime, free heap, and the
    **requesting browser's IP address and User-Agent**
  - A scrolling Status/Debug log panel (device-side ring buffer, last 30
    events: input changes, output changes, Ethernet link events, heartbeats)
- **Serial CLI over native USB CDC** (`Serial`, 115200 baud):
  - `help` — list commands
  - `status` — print all DI/DO states plus Ethernet link/IP
  - `on <1-8>` / `off <1-8>` / `toggle <1-8>` — manually drive an output
  - `all on` / `all off` — drive all 8 outputs at once
  - `ip` — print current link state / DHCP IP
  - Every DI channel auto-reports over serial the moment it goes active or
    inactive (debounced ~20ms), e.g. `[DI3] ACTIVE`
  - A heartbeat line prints every 30s with link/IP/heap so you can confirm
    the board and its Ethernet link are alive without needing an input event

## Arduino IDE setup

1. Boards Manager → install **esp32 by Espressif Systems** (core 3.x).
2. Select the ESP32-S3 Dev Module board variant matching this board (enable
   PSRAM if your board has it fitted).
3. **Tools → USB CDC On Boot: Enabled** — required for the serial CLI to work
   over the native USB port.
4. No extra libraries to install — only built-in `ETH`, `WebServer`, `Wire`,
   `SPI`.
5. Flash, open the Serial Monitor at 115200 baud, type `help`.

## Notes / assumptions

- Digital inputs are **active-LOW** (`INPUTS_ACTIVE_LOW = true`): an
  energized input reads GPIO LOW. Confirmed by a serial CLI sketch the user
  has run and verified working on their own board. Flip the flag if your
  board reads backward.
- Digital outputs are **active-LOW** at the TCA9554
  (`OUTPUTS_ACTIVE_LOW = true`): writing an expander pin low turns its
  Darlington output on. Confirmed by the user's own tested hardware — the
  earlier active-high guess (from a reference driver, not this board) had
  `on`/`off` backward. Flip the flag if your board reads inverted.
- The web page polls `/api/status` and `/api/log` rather than using
  WebSockets/SSE, to keep the sketch dependency-free and to avoid blocking
  the CLI/input polling loop.
