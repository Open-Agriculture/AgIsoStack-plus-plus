.. _UsingTeensyAndArduinoWithPlatformIO:

Using Teensy and Arduino with PlatformIO
=========================================

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

This tutorial covers how to use the library with the Arduino framework in PlatformIO, on a Teensy 4.0 or 4.1, or on an ESP32.
If you want to use the ESP-IDF framework on an ESP32 instead, see :ref:`UsingESP32WithPlatformIO`.

.. note::

    A complete example program that builds for all of these boards from one file is `located here. <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/tree/main/examples/arduino_platformio>`_

Installation
-------------

Install Visual Studio Code and the PlatformIO extension, as described in the installation section of :ref:`UsingESP32WithPlatformIO`.
You can also use the PlatformIO Core command line tool on its own.

Then create a project for your board, select :code:`Arduino` as the framework, and add the library to your :code:`platformio.ini` file:

.. code-block:: ini

    [env:teensy41]
    platform = teensy
    board = teensy41
    framework = arduino
    lib_deps = https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git

No other settings are needed. The library's :code:`library.json` tells PlatformIO which source files to build, and only builds the CAN driver for your board.

.. warning::

    PlatformIO will not automatically update AgIsoStack. You can run :code:`pio pkg update` to update it manually from the PlatformIO core CLI.
    For a reproducible build, pin the library to a commit instead: :code:`lib_deps = https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git#<commit>`.

Supported boards
-----------------

* **Teensy 4.0 and 4.1** use the :code:`FlexCANT4Plugin`. The FlexCAN_T4 library it needs is included with AgIsoStack, so you do not need to install it.
* **ESP32** boards use the :code:`TWAIPlugin`, for the ESP32's built-in CAN controller.
* Teensy 3.x boards are not supported.

You will still need a CAN transceiver between the board and the bus. You cannot connect the bus to the controller's pins directly.

No threads: call update() from loop()
--------------------------------------

On a desktop, the library runs the CAN stack in threads of its own.
When :code:`ARDUINO` is defined, it starts no threads at all, and the Virtual Terminal and Task Controller clients do not start any either, even when you ask them to with :code:`initialize(true)`.

This means your program has to keep the stack running, by calling :code:`update()` from :code:`loop()`:

.. code-block:: c++

    void loop()
    {
        isobus::CANHardwareInterface::update();
    }

Receiving frames from the driver, address claiming, the transport protocols and all of your callbacks happen inside this call.
Keep the rest of :code:`loop()` short, and don't use :code:`delay()`. If the stack is not updated for a while, frames can be lost when the driver's receive buffer fills up.

If you use the Virtual Terminal or Task Controller client, call its :code:`update()` from :code:`loop()` as well.

Setting up the CAN driver
--------------------------

On a Teensy 4.x, the plugin takes the number of the CAN controller, counted from 0:

.. code-block:: c++

    #include "isobus/hardware_integration/flex_can_t4_plugin.hpp"

    auto canDriver = std::make_shared<isobus::FlexCANT4Plugin>(0);

Channel 0 is :code:`CAN1`, channel 1 is :code:`CAN2`, and channel 2 is :code:`CAN3`.
That is pins 22 and 23 for :code:`CAN1`, 0 and 1 for :code:`CAN2`, and 30 and 31 for :code:`CAN3`.
On a Teensy 4.0, pins 30 and 31 are pads on the bottom of the board that you have to solder to, so :code:`CAN1` or :code:`CAN2` is the easier choice there.
The plugin runs the bus at 250 kbit/s, the ISO 11783 bit rate.

On an ESP32, configure the TWAI as described in :ref:`UsingESP32WithPlatformIO`, but pass zero timeouts to the plugin:

.. code-block:: c++

    #include "isobus/hardware_integration/twai_plugin.hpp"

    static twai_general_config_t twaiConfig = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL);
    static twai_timing_config_t twaiTiming = TWAI_TIMING_CONFIG_250KBITS();
    static twai_filter_config_t twaiFilter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    twaiConfig.rx_queue_len = 32;
    auto canDriver = std::make_shared<isobus::TWAIPlugin>(&twaiConfig, &twaiTiming, &twaiFilter, 0, 0);

Without threads, :code:`update()` reads frames until the driver's receive queue is empty.
With the default timeouts of 100 ms, that last read waits 100 ms for a frame that does not come, so every call to :code:`update()` would block :code:`loop()` for 100 ms.
The default receive queue of 5 frames is also short for a busy bus, which is why the example raises it.

Starting the stack
-------------------

From here on, the code is the same as on any other platform:

.. code-block:: c++

    void setup()
    {
        // ... create canDriver as shown above ...
        isobus::CANHardwareInterface::set_number_of_can_channels(1);
        isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);
        isobus::CANHardwareInterface::start();

        isobus::NAME deviceNAME(0);
        // ... set up your NAME, see The ISOBUS Hello World ...
        internalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(deviceNAME, 0);

        // Address claiming takes at least 250 ms. Keep the stack running while waiting for it.
        while (!internalECU->get_address_valid())
        {
            isobus::CANHardwareInterface::update();
        }
    }

The wait loop calls :code:`update()` itself, because :code:`loop()` does not run until :code:`setup()` returns.
In a real device you will want a timeout on it, as the example has.

Logging
--------

To see the stack's log messages in the serial monitor, give it a logger that writes to :code:`Serial`.
The example contains one, and :ref:`DebugLogging` explains how logging works.
If you need to save memory, you can leave logging out entirely by defining :code:`DISABLE_CAN_STACK_LOGGER`, for example with :code:`build_flags = -D DISABLE_CAN_STACK_LOGGER`.

Building and uploading the example
-----------------------------------

From the :code:`examples/arduino_platformio` folder:

.. code-block:: bash

    pio run -e teensy41              # build for one board
    pio run -e teensy41 -t upload    # build and upload
    pio device monitor               # watch the log messages

The available environments are :code:`teensy41`, :code:`teensy40` and :code:`esp32`.
