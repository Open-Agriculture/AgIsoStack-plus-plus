# Minimal Zephyr build

This application builds AgIsoStack++ as a Zephyr module with the CMSIS-RTOS2
threading backend. It installs `ZephyrTimeSource`, starts the stack's CAN
hardware interface with no configured CAN channels, then stops it. It is a
build and startup check, not a CAN hardware example.

From a Zephyr West workspace, with this checkout available at
`/path/to/AgIsoStack-plus-plus`:

```sh
west build -b native_sim/native/64 /path/to/AgIsoStack-plus-plus/examples/zephyr/minimal
west build -t run
```

`CMakeLists.txt` adds this checkout through `ZEPHYR_EXTRA_MODULES`. An external
Zephyr application can instead add the repository to its West manifest or
`ZEPHYR_EXTRA_MODULES`, then link `Isobus`, `HardwareIntegration`, and `Utility`
to `app`. Set `CONFIG_CMSIS_RTOS_V2=y` and
`CONFIG_AGISOSTACK_CMSIS_RTOS2_THREADING=y`. The sample's `prj.conf` also shows
the C++ standard library, CMSIS thread resource, and stack settings needed
for this startup path. Register a `ZephyrTimeSource` with
`SystemTiming::override_time_source()` before starting the stack and keep it
alive while the stack runs.

The module deliberately builds with no CAN driver. A Zephyr CAN plugin and
board specific CAN configuration are the next step toward [#319](https://github.com/Open-Agriculture/AgIsoStack-plus-plus/issues/319).
