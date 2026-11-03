# ISO1200PT Hardware Bring-Up Guide

Use this guide to check the controller in stages. **Do not connect solenoids or hydraulic pressure until the earlier checks pass.** The relay outputs are not a substitute for the machine's safety systems.

## 1. ESP32 and MCP2515 CAN board

Connect the MCP2515 SPI signals to these ESP32 pins:

| MCP2515 signal | ESP32 pin |
| --- | --- |
| SCK | GPIO18 |
| MOSI | GPIO23 |
| MISO | GPIO19 |
| CS | GPIO5 |
| INT | GPIO4 |

The firmware configures CAN for **250 kbit/s** with an **8 MHz MCP2515 crystal**. The old wiring note that used GPIO15 for CS was incorrect; use GPIO5. Check that the module's voltage and logic connections are suitable for the ESP32 before powering it.

## 2. I2C output boards

The firmware uses I2C on **SDA GPIO21** and **SCL GPIO22** at **100 kHz**.

For the first bring-up, power both MCP23017 chips from **3.3 V**. The MCP23017 supports 1.8–5.5 V, so a level shifter is not needed for this 3.3 V test. Connect grounds together.

| Chip | Address straps | I2C address | Use |
| --- | --- | --- | --- |
| MCP23017 #1 | A0, A1, A2 to GND | `0x20` | Solenoids 1–16 |
| MCP23017 #2 | A0 to its power pin; A1 and A2 to GND | `0x21` | Solenoids 17–18 |

Tie each MCP23017 RESET pin high to its 3.3 V supply. **Do not leave RESET floating.** The firmware probes both addresses during startup and reports whether each chip was found.

Fold, unfold and plant hydraulic actuation is inhibited only by an MCP23017 output-board fault. Fan or vacuum power state, sensor readings and faults do not gate hydraulic outputs.

## 3. Relay boards and solenoid channel map

The confirmed relay hardware is two 8-channel opto-isolated boards and one 4-channel opto-isolated board:

| Relay board input | MCP23017 output |
| --- | --- |
| 8-channel board #1, channels 1–8 | MCP23017 #1, port A, bits 0–7 |
| 8-channel board #2, channels 1–8 | MCP23017 #1, port B, bits 0–7 |
| 4-channel board, channels 1–2 | MCP23017 #2, port A, bits 0–1 |
| 4-channel board, channels 3–4 | Spare |

The complete solenoid mapping is:

| Solenoid | MCP23017 | Port | Bit | Relay board input |
| ---: | --- | --- | ---: | --- |
| 1 | #1 (`0x20`) | A | 0 | 8-channel #1, channel 1 |
| 2 | #1 (`0x20`) | A | 1 | 8-channel #1, channel 2 |
| 3 | #1 (`0x20`) | A | 2 | 8-channel #1, channel 3 |
| 4 | #1 (`0x20`) | A | 3 | 8-channel #1, channel 4 |
| 5 | #1 (`0x20`) | A | 4 | 8-channel #1, channel 5 |
| 6 | #1 (`0x20`) | A | 5 | 8-channel #1, channel 6 |
| 7 | #1 (`0x20`) | A | 6 | 8-channel #1, channel 7 |
| 8 | #1 (`0x20`) | A | 7 | 8-channel #1, channel 8 |
| 9 | #1 (`0x20`) | B | 0 | 8-channel #2, channel 1 |
| 10 | #1 (`0x20`) | B | 1 | 8-channel #2, channel 2 |
| 11 | #1 (`0x20`) | B | 2 | 8-channel #2, channel 3 |
| 12 | #1 (`0x20`) | B | 3 | 8-channel #2, channel 4 |
| 13 | #1 (`0x20`) | B | 4 | 8-channel #2, channel 5 |
| 14 | #1 (`0x20`) | B | 5 | 8-channel #2, channel 6 |
| 15 | #1 (`0x20`) | B | 6 | 8-channel #2, channel 7 |
| 16 | #1 (`0x20`) | B | 7 | 8-channel #2, channel 8 |
| 17 | #2 (`0x21`) | A | 0 | 4-channel board, channel 1 |
| 18 | #2 (`0x21`) | A | 1 | 4-channel board, channel 2 |

The current FOLD and UNFOLD state tables energize solenoids **1, 3, 7, 12, 16, 17, and 18 in every action**. Verify those repeated outputs against the planter schematic before connecting any coil. The source tables also give electrically identical actions for **UNFOLD 2 and 6**, **FOLD 4 and 5**, and **Plant Limited Lift and Full Lift**. Confirm those matches against the schematic; this guide does not change them.

### Relay input polarity

Set each relay board's High/Low trigger jumper to **HIGH**, as confirmed for this hardware. The firmware's shared `RELAY_ACTIVE_LOW` setting defaults to `0`, so logical OFF drives the output low and logical ON drives it high. If a no-load test proves the board is active-LOW instead, change `RELAY_ACTIVE_LOW` in `examples/1200PT/include/mcp23017_output_safety.h` to `1` before using it.

Before connecting solenoids, test one relay input at a time with a multimeter or the board's input LED and **no load connected**. Use the separate 5 V supply for the relay board and a 3.3 V jumper from the ESP32/controller side to test its input; connect the grounds together. Observe whether the relay activates at the HIGH or LOW input level, then set the firmware polarity to match. Avoid shorts and do not connect the 12 V coil supply during this check.

For the bench relay test, use a separate **5 V supply rated for at least 3 A**, common ground, and power the MCP23017s at 3.3 V. Connect unused relay inputs to GND as confirmed for these boards. Test only one relay at a time, with no solenoids or other loads connected.

For later coil tests, install a flyback diode across every 12 V coil, fuse the 12 V supply, and use a common ground. Add a 10 kΩ bias resistor to every relay input so it stays off during boot or a crash: use pull-downs for active-HIGH inputs, or pull-ups for active-LOW inputs.

## 4. Fan, vacuum and sensor pins

| Function | ESP32 pin | Firmware configuration |
| --- | --- | --- |
| Fan proportional-valve PWM | GPIO25 | 250 Hz placeholder; duty limited to 20–80% while enabled |
| Vacuum proportional-valve PWM | GPIO26 | 250 Hz placeholder; duty limited to 20–80% while enabled |
| Fan RPM pulse input | GPIO34 | Input with rising-edge interrupt |
| Vacuum pressure input | GPIO35 | ADC1 channel 7; 12-bit read |
| S-bin sensor input | GPIO36 | Digital input |

GPIO34, GPIO35 and GPIO36 are input-only ESP32 pins and have no usable internal pull-up or pull-down. The fan RPM input needs an **external 10 kΩ pull-up to 3.3 V**; its firmware configuration does not enable an internal pull-up. Keep the vacuum pressure signal at or below **3.3 V** at GPIO35; use a suitable divider if the sensor can output 5 V. GPIO36 is configured as an input with no internal pulls. The required external bias for the S-bin signal depends on the sensor output; the firmware does not specify a pull resistor.

## 5. Power and safety checks

- Keep ESP32, MCP23017 and relay-board grounds common for signal tests.
- For the initial relay test, use the separate 5 V, at-least-3 A supply and leave all solenoid loads disconnected.
- For a later 12 V coil test, use a fused supply and a flyback diode at every coil.
- Keep the 10 kΩ relay-input bias resistors in place so inputs do not float during reset or loss of control power.
- Do not test hydraulic pressure until the wiring, no-load relay checks, coil checks, PWM checks and sensor checks are complete.

## 6. Bring-up checklist

1. **ESP32 and CAN only:** Check the startup CAN result and pin report. Use GPIO5 for MCP2515 CS, not GPIO15.
2. **Add the MCP23017 boards:** Power them at 3.3 V, check address straps and RESET, then confirm that the startup report finds both `0x20` and `0x21`.
3. **Add relay boards without solenoids:** Set HIGH trigger, use the separate 5 V supply with common ground, and test one input at a time with a 3.3 V jumper. Leave unused inputs tied to GND.
4. **Test solenoids on a fused bench supply:** Add the required flyback diodes and connect one coil at a time only after relay polarity is confirmed.
5. **Check PWM outputs:** Verify GPIO25 and GPIO26 frequency and duty on the bench before connecting valve drivers.
6. **Connect and verify sensors:** Keep GPIO35 within 3.3 V; provide the GPIO34 external 10 kΩ pull-up and confirm the S-bin sensor's output and biasing.
7. **Hydraulic pressure last:** Do not apply pressure until the electrical checks and mechanical safety review are complete.
