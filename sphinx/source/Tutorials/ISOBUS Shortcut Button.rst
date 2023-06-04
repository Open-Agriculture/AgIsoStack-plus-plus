.. _ISOBUS_Shorcut_Button:

ISOBUS Shortcut Button (ISB)
=============================

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

This tutorial covers the basics on how to communicate with (or as) an ISB.

This tutorial covers part of implement application level use of the CAN stack, and assumes you have read the beginner tutorials.

ISB Overview
-------------

An ISB is a button that an operator can press to simultaneously command all implements to stop what they are doing, sort of like an emergency stop button.

This library provides an interface that will both parse and send the message associated with this functionality, called the "Stop implement operations" message.

In general, this message may be sent by any control function connected to the implement bus on forestry or agriculture implements, so it is at your discretion if you want to send this message, but it is VERY highly advised that you use the provided interface to receive the message so that your application can transition to a safe state when requested.

If your application is performing implement operations, it may be required for you to accept this message for ISOBUS certification, as the ISOBUS standard states: "All implements shall start a process to stop all operations when this broadcast message is received from any CF".

It is also important to note that if you are performing implement operations, using the ISB interface, and have a VT object pool, you must also reflect the state of the ISB on your home screen in some way.

To learn more, see https://www.isobus.net/ for the full description of SPN 5140.

How to Use the ISB Interface
-----------------------------

Receiving ISB messages
^^^^^^^^^^^^^^^^^^^^^^^

To receive messages from an ISB, create an instance of the :code:`ShortcutButtonInterface`, located in :code:`isobus/isobus/isobus_shortcut_button_interface.hpp`.
You will need to pass in an internal control function so that the interface can infer the CAN channel it should listen on.
Once you create the interface, call :code:`initialize` to set up the interface.

.. code-block:: c++

	#include "isobus/isobus/isobus_shortcut_button_interface.hpp"

	ShortcutButtonInterface isbInterface(internalECU);

	isbInterface.initialize();

With the interface created, you now have two options for getting updates from the interface when ISB events happen.

Option 1: You can register for callbacks from the interface by using its EventDispatcher, and by giving it a function to call when an ISB message is received.

.. code-block:: c++

    auto isbEventHandle = isbInterface.get_stop_all_implement_operations_state_event_dispatcher().add_listener(yourCallbackFunction);

.. note::

    The format of the function you need to use with add_listener is:
	:code:`void yourCallbackFunction(ShortcutButtonInterface::StopAllImplementOperationsState receivedState)`

Option 2: You can occasionally poll the current state from the interface.

.. code-block:: c++

    isbInterface.get_state();

Lastly, make sure you call the :code:`update` routine in the interface periodically so that the interface can process message timeouts.
The suggested rate is at least every 100ms. Longer delays might cause you to miss timeouts, which might cause a safety issue if you are performing implement operations and not stopping when required.

.. code-block:: c++

    isbInterface.update();

Sending ISB messages
^^^^^^^^^^^^^^^^^^^^^^^

If you want to send the ISB message, you have to construct the ISB interface with the :code:`serverEnabled` constructor parameter set to true.

.. code-block:: c++

    ShortcutButtonInterface isbServerInterface(internalECU, true);

Now, as long as you call the update routine periodically, the interface will send the message automatically when needed.
If you want to change the state the interface is sending, simply call :code:`set_stop_all_implement_operations_state` with your desired state.

.. code-block:: c++

    isbServerInterface.set_stop_all_implement_operations_state(ShortcutButtonInterface::StopAllImplementOperationsState::StopImplementOperations);

.. warning::

    If you are using the interface as an ISB server, you should update the interface more frequently than when receiving, because CAN message transmits will only happen when :code:`update` is called.
    A rate of 50ms is a reasonable minimum rate.
