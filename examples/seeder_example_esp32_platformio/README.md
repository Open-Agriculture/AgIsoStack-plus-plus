# ESP32 PlatformIO Seeder Example

An ESP32-S3 port of the desktop [`examples/seeder_example`](../seeder_example): a complete
ISOBUS implement that combines a Virtual Terminal (VT) client, a Task Controller (TC) client
with section control, and diagnostics, plus drivers for the board's 8 relay outputs and
8 opto-isolated digital inputs.

The section-control logic is the same as the desktop example; only the platform glue changes:
the ESP32's built-in TWAI CAN controller instead of a PC CAN adapter, an `ESP_LOG` sink, and a
FreeRTOS loop in `app_main`. The object pool is embedded directly into the firmware image, so
no filesystem is needed at runtime.

## Target hardware

**[Waveshare ESP32-S3-ETH-8DI-8RO-C](https://www.waveshare.com/wiki/ESP32-S3-ETH-8DI-8RO-C)**:
an industrial DIN-rail relay module
([product page](https://www.waveshare.com/esp32-s3-eth-8di-8ro-c.htm)) built around an
ESP32-S3-WROOM-1U-N16R8:

- ESP32-S3 dual-core Xtensa LX7 @ 240 MHz, **16 MB flash**, **8 MB PSRAM**, Wi-Fi + BLE
- **8 relay outputs** (<=10 A @ 250 VAC / 30 VDC, optocoupler-isolated), driven through an
  onboard **TCA9554 I2C GPIO expander** (address `0x20`), *not* direct GPIO
- **8 digital inputs** (5-36 V, passive/active, optocoupler-isolated), direct GPIO
- Onboard **isolated CAN transceiver** (the ISOBUS physical layer) with a jumper-selectable
  120 ohm bus-termination resistor
- 7-36 V DC screw-terminal power (12 V / 24 V tractor electrics); USB-C is 5 V for
  flashing/debug only

> **Get the "-C" (CAN) variant.** Waveshare also sells a look-alike `ESP32-S3-ETH-8DI-8RO`
> (no "C") that has **RS485 instead of CAN** and will *not* work for ISOBUS. The PoE variant
> (`ESP32-S3-POE-ETH-8DI-8RO-C`) is also CAN and works fine.

The example is not tied to this board, though; see
[Adapting to a different board](#adapting-to-a-different-board).

## Pin mapping

All board wiring lives in one header, [`src/board_config.hpp`](src/board_config.hpp):

| Function           | Pins                       | Notes                                                        |
| ------------------ | -------------------------- | ------------------------------------------------------------ |
| CAN / TWAI         | TX `GPIO17`, RX `GPIO18`   | 250 kbit/s, to the onboard isolated CAN transceiver           |
| Digital inputs 1-8 | `GPIO4`-`GPIO11`           | Direct GPIO, active-**low** at the pin, internal pull-**up**  |
| Relay I2C bus      | SDA `GPIO42`, SCL `GPIO41` | TCA9554 expander at `0x20` -> 8 relay channels                |

> The relay I2C pins are **bench-verified**: Waveshare's own block diagram labels SDA/SCL the
> other way around. `GPIO41`=SCL / `GPIO42`=SDA is what actually enumerates the expander.

## Prerequisites

- [PlatformIO](https://platformio.org/): either the **PlatformIO IDE** (VS Code extension,
  with its Upload/Monitor toolbar buttons) or **PlatformIO Core** (CLI).
- A USB-C data cable. Connect the board via its USB-C port; it enumerates as native USB
  (VID:PID `303A:1001`).

On Windows the `pio` command may not be on your `PATH`; it lives in
`%USERPROFILE%\.platformio\penv\Scripts`.

## Build and flash

Run these from this example's directory.

### Quick start (default environment)

The default `esp32-s3-devkitc-1` environment pulls AgIsoStack++ straight from GitHub, so it
needs no extra setup:

```bash
pio device list          # find your board's serial port
pio run -t upload        # build + flash (auto-detects the port if only one board is attached)
pio device monitor       # watch the boot log (115200 baud); Ctrl+] to exit
```

To target a specific port (recommended when more than one device is attached), add
`--upload-port`, e.g. `pio run -t upload --upload-port COM12` on Windows, or
`--upload-port /dev/ttyACM0` on Linux.

### Build against this repository (what CI uses)

To build against your local AgIsoStack++ checkout instead of the published GitHub version,
register the repository as a global PlatformIO library once, then use the `local_agisostack`
environment:

```bash
pio pkg install -g -l ../..    # install the repo root (it has library.json) as a library
pio run -e local_agisostack -t upload --upload-port COM12
```

(Equivalently, from the repository root: `pio pkg install -g -l ./` then
`pio run -d examples/seeder_example_esp32_platformio -e local_agisostack -t upload`.)

## Expected boot output

On the serial monitor you should see the object pool load, the digital inputs configured as
pull-up inputs, and the ISOBUS address claimed:

```text
Loaded object pool from BasePool.iop
I (851) gpio: GPIO[4]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
...
I (875) AgIsoStack: [Info][NM]: Partnered control function ... has claimed address 38 on channel 0.
```

> **No tractor terminal on the bench?** You will repeatedly see
> `[Error][VT]: Load Version Response Timeout` followed by `Resetting Failed VT Connection`.
> That is expected: the VT client is looking for a Virtual Terminal *server* (a tractor
> display) to talk to. Connect to a real ISOBUS bus with a VT, or run a VT server/simulator,
> to see the object pool rendered.

## Adapting to a different board

Edit [`src/board_config.hpp`](src/board_config.hpp):

- **Different pins**: change `CAN_TX_PIN` / `CAN_RX_PIN`, `DIGITAL_INPUT_PINS`,
  `RELAY_I2C_SDA_PIN` / `RELAY_I2C_SCL_PIN`, and `RELAY_EXPANDER_ADDRESS`.
- **No relay expander**: set `HAS_RELAY_EXPANDER = false`. Even when left `true`, the driver
  **probes the TCA9554 at boot**; if it does not acknowledge, relays are simply not driven and
  a warning is logged, so a board without the expander (or with it unpowered) still runs.
- **No digital inputs**: set `HAS_DIGITAL_INPUTS = false` to skip input init and polling.

## Troubleshooting

- **`pio run -v` crashes** with a SCons `_Null` `TypeError` while generating `firmware.bin`: a
  known issue with this platform version (`espressif32@6.10.0`). Build without `-v`.
- **`Relay expander not detected`** warning: the TCA9554 did not ACK at `0x20`. Check the I2C
  wiring (SCL `GPIO41`, SDA `GPIO42`) and that the board is powered. The application keeps
  running; relay writes are skipped.
- **Address never claims / no CAN traffic**: confirm you have the **-C** (CAN) board, the bus
  is wired to the CAN screw terminal, and (at a bus end) the 120 ohm termination jumper is
  fitted.
- **Object pool rejected as corrupt**: make sure `platformio.ini` uses `board_build.embed_files`
  (raw binary), *not* `embed_txtfiles`, which appends a NUL byte that the VT parses as a
  truncated object.
