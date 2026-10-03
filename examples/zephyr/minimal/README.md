# Minimal Zephyr build

This application builds AgIsoStack++ as a Zephyr module with the CMSIS-RTOS2
threading backend. It installs `ZephyrTimeSource`, opens the configured Zephyr
CAN device through one CAN hardware channel, checks that it initialized, sends
standard and extended frames, checks that both return through the receive path,
then stops the stack. It is a CAN smoke test, not a full ISOBUS application.

From a Zephyr West workspace, with this checkout available at
`/path/to/AgIsoStack-plus-plus`:

```sh
west build -b native_sim/native/64 /path/to/AgIsoStack-plus-plus/examples/zephyr/minimal
west build -t run
```

The `native_sim` board uses Zephyr's CAN loopback device. Other boards need a
ready CAN device selected as `zephyr,canbus` in devicetree and a working CAN
loopback path for this test.

`CMakeLists.txt` adds this checkout through `ZEPHYR_EXTRA_MODULES`. An external
Zephyr application can instead add the repository to its West manifest or
`ZEPHYR_EXTRA_MODULES`, then link `Isobus`, `HardwareIntegration`, and `Utility`
to `app`. Set `CONFIG_CAN=y`, `CONFIG_CMSIS_RTOS_V2=y`, and
`CONFIG_AGISOSTACK_CMSIS_RTOS2_THREADING=y`. The sample's `prj.conf` also shows
the C++ standard library, CMSIS thread resource, and stack settings needed
for this startup path. Register a `ZephyrTimeSource` with
`SystemTiming::override_time_source()` before starting the stack and keep it
alive while the stack runs.
