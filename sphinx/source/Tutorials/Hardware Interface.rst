.. _HardwareInterface:

Using the Hardware Interface
==============================

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

This tutorial covers how to connect the library to your hardware in more depth than previously covered in the other tutorials.

How the Hardware Interface Works
---------------------------------

The :code:`CANHardwareInterface` class was created to provide a common queuing and thread layer for running the CAN stack and all CAN drivers to simplify integration and crucially to provide a consistent, safe order of operations for all the function calls needed to properly drive the stack.

For example, although receiving and sending CAN messages is multi-threaded, the stack avoids a lot of complexity by making the consumption of the receive message queue and update of protocols effectively single threaded so no expensive mutexing is needed in the core of the stack.

The CANHardwareInterface also is designed to deal with the generic base class for a CAN driver, called :code:`CANHardwarePlugin`, so that it too is completely hardware agnostic.

The hardware interface works as follows at a high level:

* The hardware interface runs a thread that fills a receive queue. This ensures that messages are taken from the hardware or socket in a timely manor, and are saved for processing.
* The hardware interface runs the network manager's main, periodic thread.
	This thread operates in three steps.

	1. The periodic thread drains the receive queue. Messages are processed on this thread by the network manager. Many callbacks to the application happen on this thread.
	2. The main update routine of the network manager is run. This executes all main protocol logic.
	3. The periodic thread tries to send messages out of the transmit queue until a transmit fails.

	The thread then runs a few milliseconds later and repeats the same steps.


Choosing a CAN Driver
--------------------------

The library contains built-in hardware integrations for popular CAN interfaces, such as SocketCAN and PEAK.

When compiling with CMake, a default CAN driver plug-in will be selected for you based on your OS, but when compiling you can explicitly choose to use one of the natively supported CAN drivers by supplying the CAN_DRIVER variable.


* :code:`-DCAN_DRIVER=SocketCAN` Will compile with Socket CAN support (This is the default for Linux)
* :code:`-DCAN_DRIVER=WindowsPCANBasic` Will compile with windows support for the PEAK PCAN drivers (This is the default for Windows)
* :code:`-DCAN_DRIVER=MacCANPCAN` Will compile with support for the MacCAN PEAK PCAN driver (This is the default for Mac OS)
* :code:`-DCAN_DRIVER=TWAI` Will compile with support for the ESP TWAI driver
* :code:`-DCAN_DRIVER=MCP2515` Will compile with support for the MCP2515 CAN controller

You can include the header file :code:`isobus/hardware_integration/available_can_drivers.hpp` to get access to the CAN drivers that have been included via your CMake configuration.

Using the Hardware Interface
-----------------------------

To use the :code:`CANHardwareInterface`, simply create at least one CAN plugin, set the number of CAN channels to be at least 1, assign your driver(s) it to a CAN channel index, and call :code:`start`.

.. code-block:: c++

    #if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	    std::shared_ptr<isobus::CANHardwarePlugin> canDriver = std::make_shared<isobus::SocketCANInterface>("can0");
	#endif

	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	isobus::CANHardwareInterface::start();

You may also want to check the return value of :code:`start` and check the return value of :code:`canDriver->get_is_valid()` to know if everything is working.

You can see this within the examples `located here. <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/tree/main/examples>`_

Writing Your Own CAN Driver
----------------------------

If the library does not support your specific CAN hardware, you can easily add support for your hardware by implementing a few functions.

A new CAN driver just needs to inherit from :code:`CANHardwarePlugin` and implement all the functions defined in it (don't worry, there's only 5).

`Here is the class you'll want to inherit from. <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/hardware_integration/include/isobus/hardware_integration/can_hardware_plugin.hpp>`_

Then, you can add it to your application and use it just like the built-in plugins.
