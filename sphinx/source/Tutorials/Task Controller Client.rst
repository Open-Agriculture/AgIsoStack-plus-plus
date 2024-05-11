.. _TaskControllerClient:

Task Controller Client
=======================

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

This tutorial will walk you through basic use of the Task Controller client in AgIsoStack. The Task Controller client is used to connect an implement to a Task Controller.

It's suggested that you read the Task Controller Basics tutorial before starting this one, as it will give you an understanding of what a Task Controller is, and what a DDOP is.

Creating the Task Controller Client
------------------------------------

In order to communicate with a task controller, first you should identify the capabilities of your implement. How many sections can it control? How many simultaneous VRA rates can it handle? Can it function correctly without TC provided position based (GNSS) control?

Next, you'll have to create a DDOP that describes your implement to the TC. This DDOP will be uploaded to the TC when AgIsoStack connects to it, and will tell the TC what your implement is capable of doing.

Check out `our seeder example <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/examples/seeder_example/section_control_implement_sim.cpp#L98>`_ to see how a DDOP can be created, or review the :doc:`task controller basics tutorial <./Task Controller Basics>`.

Once you know the answer to these questions and you have a DDOP, you can create a :code:`TaskControllerClient`, and configure it to match your implement's capabilities.

Here's an example of how you can create a :code:`TaskControllerClient` in AgIsoStack:

.. code-block:: c++

	#include "isobus/isobus/isobus_task_controller_client.hpp"

	// Create a TaskControllerClient
	// The parameters are, in order:
	// The control function for the TC
	// The control function used to send messages to the TC
	// The control function for a virtual terminal you're connected to with the VT client (optional - helps synchronize language and units in some cases)
	isobus::TaskControllerClient OurTaskControllerClient(PartnerTC, InternalECU, nullptr);

	// Configure the TaskControllerClient with our DDOP.
    // The parameters are, in order:
    // Includes support for 1 boom, 10 sections, 1 rate
    // Supports documentation, not supporting TC-GEO without position based control
    // Supports TC-GEO with position based control, not supporting peer control
    // Supports TC-SC section control
	OurTaskControllerClient.configure(myDDOP, 1, 10, 1, true, false, true, false, true);

Now that we have our TC client, we need to define some functions to handle what we do when the TC requests information from us, and when it commands us to do something.

First, we define a function to handle requests:

.. code-block:: c++

    bool request_value_command_callback(std::uint16_t elementNumber,
                                        std::uint16_t DDI,
                                        std::int32_t &value,
                                        void *parentPointer)
    {

    }

This function will be called by the TC client when the TC requests a value from the implement. You should fill in the function to return the requested value. Generally this means that you'll want to `switch` on the DDI and/or element number, and return the appropriate value based on what your implement is doing.

You'll want to return true if the TC provided a valid DDI and element number, and you gave it a value in return.
Returning false will cause the TC client to send an error message to the TC, indicating that the requested value was not available. A TC expects all DPD values in your DDOP to be available, so you should basically never return false.

See the `seeder example <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/examples/seeder_example/section_control_implement_sim.cpp#L219>`_ or the `TC client example <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/examples/task_controller_client/section_control_implement_sim.cpp>`_ for an example of how this function can be implemented.

Next, we define a function to handle commands:

.. code-block:: c++

	bool command_value_command_callback(std::uint16_t elementNumber,
										std::uint16_t DDI,
										std::int32_t processVariableValue,
										void *parentPointer)
	{

	}

This function will be called by the TC client when the TC sends a command to the implement. You should fill in the function to handle the command. Generally this means that you'll want to `switch` on the DDI and/or element number, and set the appropriate value based on what your implement is doing.

You'll want to return true if the TC provided a valid DDI and element number, and you executed the command successfully. If you return false, the TC client will send an error message to the TC, indicating that the command was not executed. A TC expects all "settable" DPD values in your DDOP to be writable, so you should basically never return false.

You may also then need to trigger sending a value back to the TC, to confirm that the command was received and executed. For example, if the TC sends a command to turn on a section, you probably set the section to on. 
Then, if your section's "Actual Condensed Work State" has an on-change trigger, you need to send the new value of that information to the TC.

To accomplish this, and in fact, to send any value to the TC, you can call the function :code:`on_value_changed_trigger` on the :code:`TaskControllerClient` object. Which will cause the interface to call your previously defined :code:`request_value_command_callback` function with the appropriate DDI and element number and send a message to the TC.

Lastly, we need to tell the TC client to start running. We do this by calling :code:`initialize`.

.. code-block:: c++

	OurTaskControllerClient.initialize(true); // The "true" parameter tells the TC client to start running in a separate thread. If you pass "false", the TC client will run in the same thread as your main program and you'll have to call `update` on it periodically.

This starts the TC client running. It will now handle messages from the TC, and call your :code:`request_value_command_callback` and :code:`command_value_command_callback` functions as needed. All CAN messaging is handled for you, so you don't need to worry about manually sending any process data messages.

Once you are done with your TC client, you should call :code:`terminate` on it to stop it from running.

.. code-block:: c++

	OurTaskControllerClient.terminate();

Other Useful Features
---------------------

There are many other functions on the `TaskControllerClient` object that you can use to interact with the TC. You should review the `TaskControllerClient` class in the `AgIsoStack documentation <https://delgrossoengineering.com/isobus-docs/isobus__task__controller__client_8hpp_source>`_ to see what functions are available.

Some highlights include:

- :code:`reupload_device_descriptor_object_pool` Which will re-upload the DDOP to the TC. This can be useful if you need to change the DDOP after the TC client has already started running.
- :code:`request_task_controller_identification` Which will request the TC to display its TC "number" on its screen, if applicable. This can be useful if you need to know visually what TC you're connected to.
- :code:`get_is_connected` Which will return true if the TC client is connected to a TC, and false otherwise. Useful to know if you're connected to a TC or not.
- :code:`get_is_task_active` Which will return true if the TC indicates it is currently running a task, and false otherwise. Not all TCs use this value properly, so it may not be useful in all cases.

Be sure to check out our examples for more information on how to use the `TaskControllerClient` object.
