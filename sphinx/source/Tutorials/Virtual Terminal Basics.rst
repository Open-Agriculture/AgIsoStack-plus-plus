.. _VirtualTerminalBasics:

Virtual Terminal
===================

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

This tutorial covers the basics on how to communicate with a Virtual Terminal (VT) Server using the CAN stack.

It is assumed you have completed all the other tutorials, as this tutorial covers application level use of the CAN stack.

The Virtual Terminal Client
----------------------------

The first step in communicating with a VT is creating an object called :code:`VirtualTerminalClient`. 
This object will act as your interface for all VT communication. 
The client requires two things to instantiate: a :code:`PartneredControlFunction` and an :code:`InternalControlFunction`. 
This is so that it can send messages on your behalf needed to maintain the connection and simplify the API over sending raw CAN messages to the VT.

Let's start our program fresh, with a folder containing only the CAN stack.

.. code-block:: bash

	mkdir vt_example
	cd vt_example
	git clone https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git

Create the file `main.cpp` as shown below inside that folder with the requisite control functions.

.. code-block:: c++

	#include "isobus/hardware_integration/available_can_drivers.hpp"
	#include "isobus/hardware_integration/can_hardware_interface.hpp"
	#include "isobus/isobus/can_network_manager.hpp"
	#include "isobus/isobus/can_partnered_control_function.hpp"

	#include <atomic>
	#include <csignal>
	#include <iostream>

	//! It is discouraged to use global variables, but it is done here for simplicity.
	static std::atomic_bool running = { true };

	void signal_handler(int)
	{
		running = false;
	}

	int main()
	{
		std::signal(SIGINT, signal_handler);

		// Automatically load the desired CAN driver based on the available drivers
		std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
	#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
		canDriver = std::make_shared<isobus::SocketCANInterface>("can0");
	#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
		canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(PCAN_USBBUS1);
	#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
		canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0); // CAN0
	#elif defined(ISOBUS_MACCANPCAN_AVAILABLE)
		canDriver = std::make_shared<isobus::MacCANPCANPlugin>(PCAN_USBBUS1);
	#elif defined(ISOBUS_SYS_TEC_AVAILABLE)
		canDriver = std::make_shared<isobus::SysTecWindowsPlugin>();
	#endif
		if (nullptr == canDriver)
		{
			std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
			std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
			return -1;
		}
		isobus::CANHardwareInterface::set_number_of_can_channels(1);
		isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

		if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
		{
			std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
			return -2;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(250));

		isobus::NAME TestDeviceNAME(0);

		//! Consider customizing some of these fields, like the function code, to be representative of your device
		TestDeviceNAME.set_arbitrary_address_capable(true);
		TestDeviceNAME.set_industry_group(1);
		TestDeviceNAME.set_device_class(0);
		TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
		TestDeviceNAME.set_identity_number(2);
		TestDeviceNAME.set_ecu_instance(0);
		TestDeviceNAME.set_function_instance(0);
		TestDeviceNAME.set_device_class_instance(0);
		TestDeviceNAME.set_manufacturer_code(1407);

		const isobus::NAMEFilter filterVirtualTerminal(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
		const std::vector<isobus::NAMEFilter> vtNameFilters = { filterVirtualTerminal };
		auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
		auto TestPartnerVT = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

		while (running)
		{
			// CAN stack runs in other threads. Do nothing forever.
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		isobus::CANHardwareInterface::stop();
		return 0;
	}


It's the same boilerplate we've done before, but note the following key things:

* We're using shared pointers (`std::shared_ptr`) to make our control functions. The VT client class requires shared pointers to your control functions, so we want to make sure we start with those.

* Our partner has been defined to be any device on the bus with a function code that identifies it as a VT (see `isobus::NAME::NAMEParameters::FunctionCode <https://delgrossoengineering.com/isobus-docs/classisobus_1_1NAME.html#a5f22513106207fd1a1e0c72b17a77f77>`_ if you want to see some other function code that exist.)

With those notes in mind, let's create our VT client. We also introduce a helper class here to make updating the VT easier, but we'll go over that in a bit.

.. code-block:: c++

	#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
	#include "isobus/isobus/isobus_virtual_terminal_client_update_helper.hpp"

	//! It is discouraged to use global variables, but we have done it here for simplicity.
	static std::shared_ptr<isobus::VirtualTerminalClient> TestVirtualTerminalClient = nullptr;
	static std::shared_ptr<isobus::VirtualTerminalClientUpdateHelper> virtualTerminalUpdateHelper = nullptr;

	int main()
	{
		...

		TestVirtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
		virtualTerminalUpdateHelper = std::make_shared<isobus::VirtualTerminalClientUpdateHelper>(virtualTerminalClient);

		...
	}

Now, we've got our client created, and we need to configure it. More specifically, we need to give it at least one object pool to manage.

Object Pools
-------------

Imagine an ISOBUS object pool as a container filled with different visual components (objects). Each object has a specific role, defined by the ISO 11783-6 standard.
Combining these objects creates the user interface you see on a VT.
In more technical terms, it's a way for a "headless" device to explain to a VT how to draw its UI in a very consistent way that will be portable across machines and platforms.

This data is essentially a long list of objects that represent things to a VT, such as:

* Font attributes
* Bitmaps
* Strings
* Numbers
* Pointers
* Lines
* Rectangles
* Polygons
* Ellipses
* A few more complex objects, like bar graphs and graphics contexts

An ECU uploads this blob of objects to the VT along with their semi-hierarchical relationships, the VT checks to make sure the format is valid, then starts displaying it on its screen.
Then, the ECU can use a set of CAN messages to update what's shown on the screen, and likewise, the VT can use CAN messages to tell the ECU when things like button presses happen.

This back-and-forth communication is the foundation for ISOBUS implement applications, and the CAN stack aims to make this communication as easy as possible.

The `utility` folder in the CAN stack contains a helper function to read standard ISOBUS .iop files. These files are the industry standard way of storing the binary blob that is the object pool.

You can make one for yourself if you have access to an ISOBUS object pool designer tool of some kind, but for our purposes, one has been included in the examples folder for you called `VT3TestPool.iop`.

For this example, let's `download <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/examples/virtual_terminal/version3_object_pool/VT3TestPool.iop>`_ that object pool (or grab it from the examples folder within the CAN stack), and place it in the same directory as your main.cpp file.

Let's also grab `this header file <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/examples/virtual_terminal/version3_object_pool/objectPoolObjects.h>`_ and place it in the same folder.
This file is for convenience, and tells us what objects are inside the IOP file along with their object ID. Files like these are often created by VT object pool designer programs to give nice, human-readable names to your objects so that instead of
referencing object 5001, we can instead reference it by the nicer name `acknowledgeAlarm_SoftKey` for example.

Now, let's add some code to our example to read in this IOP file, and give it to our VT client as our only object pool.

.. code-block:: c++

	#include "isobus/utility/iop_file_interface.hpp"


	int main() 
	{
		...

		std::vector<std::uint8_t> testPool = isobus::IOPFileInterface::read_iop_file("VT3TestPool.iop");
		if (testPool.empty())
		{
			std::cout << "Failed to load object pool from VT3TestPool.iop" << std::endl;
			return -3;
		}
		std::cout << "Loaded object pool from VT3TestPool.iop" << std::endl;

		// Generate a unique version string for this object pool (this is optional, and is entirely application specific behavior)
		std::string objectPoolHash = isobus::IOPFileInterface::hash_object_pool_to_version(testPool);

		...

		TestVirtualTerminalClient->set_object_pool(0, testPool.data(), testPool.size());

		...
	}


Note how :code:`testPool` is not static here. It is required that whatever pointer you pass into the VT client via :code:`set_object_pool` MUST remain valid (i.e., not deleted or out of scope) during the object pool upload, or your application may crash.

Furthermore, a hash is generated for the object pool. This is a unique string that represents the object pool. It is used to tell the VT if the object pool has changed since the last time it was uploaded. If it has, the VT will request the new object pool from the ECU. If it hasn't, the VT will assume the object pool is the same as the last time it was uploaded and will not request it again but instead load it from their cache.

If your object pool is too large to store in memory, or you are on an embedded platform with limited resources, you may instead want to use the :code:`register_object_pool_data_chunk_callback` method instead which will get smaller chunks of data from you as the upload proceeds.
This can be used to read from some external device if needed in segments or just to save RAM.

You can also have the CAN stack automatically scale your object pool to match the dimensions of whatever VT it ends up loading to. 
This can be helpful if you designed your object pool for a certain data mask size, but need the pool to load on VTs with different resolutions or VTs that support different fonts than you designed your pool with.
To do this, just tell the client what sizes you used when creating your object pool with the :code:`set_object_pool_scaling` function. The documentation for that function can be found `in our API docs <https://delgrossoengineering.com/isobus-docs/classisobus_1_1VirtualTerminalClient.html#a677ff706f3ebe65e1ea9972e0a7304da>`_.


.. note::

    Since we are now using a function in the "isobus/utility" folder to load this IOP file, we will also need to link to the CAN stack's utility library in our CMakeLists.txt file. You can do this by adding :code:`isobus::Utility` to your :code:`target_link_libraries` statement. We'll also need to add some CMake to move the IOP file to the binary location, so that when the program is compiled, the IOP will end up in a location accessible to your program.

	We'll go over the full CMake closer to the end of this tutorial.

Now, we've added all the code needed to upload the pool to the VT! But we haven't added any actual application logic yet - we've just set up the communication.
In the next section we'll actually make the on-screen objects functional.

VT Application Layer
---------------------

Now that we've got our VT client configured, let's go over how to use the VT client in your application.

Button and Softkey Events
^^^^^^^^^^^^^^^^^^^^^^^^^^^

One of the main ways you'll get feedback from the VT is button events. These occur because the VT sends a message whenever a button is pressed, released, held, or aborted (pressed but not released).
You will want to set up processing of these events in your program so that you can take some action based on these events.

To do this, let's create a callback that accepts :code:`VirtualTerminalClient::VTKeyEvent`. This is a struct that contains information about the key event that occurred.

.. code-block:: c++

	// This callback will provide us with event driven notifications of button presses from the stack
	void handle_button_event(const isobus::VirtualTerminalClient::VTKeyEvent &event)
	{
		switch (event.keyEvent)
		{
			case isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased:
			case isobus::VirtualTerminalClient::KeyActivationCode::ButtonStillHeld:
			{
				switch (event.objectID)
				{
					case Plus_Button:
					{
						virtualTerminalUpdateHelper->increase_numeric_value(ButtonExampleNumber_VarNum);
					}
					break;

					case Minus_Button:
					{
						virtualTerminalUpdateHelper->decrease_numeric_value(ButtonExampleNumber_VarNum);
					}
					break;

					default:
						break;
				}
			}
			break;

			default:
				break;
		}
	}

We'll use this function as our callback. This function will first look at the `key event <https://delgrossoengineering.com/isobus-docs/classisobus_1_1VirtualTerminalClient.html#a5d002c96e4343f2aac9abecb03cc7873>`_ the VT is reporting to decide what to do. 
In this example, we're doing actions based on the :code:`ButtonUnlatchedOrReleased` and :code:`ButtonStillHeld` events. Meaning we're doing something when the button is released, and we're doing something when the button is held down. 
The :code:`ButtonStillHeld` event is useful for things like incrementing a value when a button is held down, or for scrolling through a list when a button is held down, as it will be called every so often while the button is held down.
Next, we're looking at the :code:`objectID` to decide what to do. In this example, we're incrementing and decrementing the counter on the screen.

We'll also want to add a listener for softkey events, as they are a common way to interact with the VT. Let's add a callback for that too.

.. code-block:: c++

	// This callback will provide us with event driven notifications of softkey presses from the stack
	void handle_softkey_event(const isobus::VirtualTerminalClient::VTKeyEvent &event)
	{
		if (event.keyNumber == 0)
		{
			// We have the alarm ACK code, so if we have an active alarm, acknowledge it by going back to the main runscreen
			virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(example_WorkingSet, mainRunscreen_DataMask);
		}

		switch (event.keyEvent)
		{
			case isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased:
			{
				switch (event.objectID)
				{
					case alarm_SoftKey:
					{
						virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(example_WorkingSet, example_AlarmMask);
					}
					break;

					case acknowledgeAlarm_SoftKey:
					{
						virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(example_WorkingSet, mainRunscreen_DataMask);
					}
					break;

					default:
						break;
				}
			}
			break;

			default:
				break;
		}
	}

Here we're doing something similar to the button event callback, but we're also checking the :code:`keyNumber` to see if it's 0. That is the special softkey event that is sent when the user presses a proprietary button to acknowledge an alarm on the VT.
Furthermore, the actions we're taking is to change the active masks, more specifically, switching between the main runscreen data mask :code:`mainRunscreen_DataMask` and an alarm mask called :code:`example_AlarmMask`. This will cause a pop-up on the VT (and possibly some beeping if your VT has a speaker).

Other Events
^^^^^^^^^^^^^

In this example, we're only using the button and softkey events, but be sure to check out all the other events you can use! They include events such as:

	* Change Numeric Value Events
	* Pointing Events
	* Change String Value Events
	* Select Input Object Events
	* Change Active Mask Events
	* User Layout Hide/Show Events
	* Audio Signal Termination Events
	* ESC Messages

With all those different events, you can get all kinds of context from the VT about what the user is doing.

Configuring the VT client
^^^^^^^^^^^^^^^^^^^^^^^^^

Now that we have our callback, we have to tell the VT client about it so that it knows to call it when appropriate.

.. code-block:: c++
	
	virtualTerminalClient->get_vt_soft_key_event_dispatcher().add_listener(handle_softkey_event);
	virtualTerminalClient->get_vt_button_event_dispatcher().add_listener(handle_button_event);

This is how you add a listener to the VT client. You can add as many listeners as you want, and they will all be called when the appropriate event occurs.

Furthermore, we need to initialize the VT client. This is done by calling the :code:`initialize` function on the VT client. This will start the process of uploading the object pool to the VT, and will also start the process of the VT sending us the current active mask and other information about the VT.

And lastly, we need to configure our VT update helper. It's not strictly necessary to use it, but it makes it easier to update the VT, and it also provides some nice features like tracking numeric values and soft key masks.
We need to tell it which objects to track, and then call the :code:`initialize` function on it.

.. code-block:: c++

	virtualTerminalUpdateHelper->add_tracked_numeric_value(ButtonExampleNumber_VarNum, 214748364); // In the object pool the output number has an offset of -214748364 so we use this to represent 0.
	virtualTerminalUpdateHelper->initialize();

.. note::
	when we added handling for the :code:`Minus_Button` and :code:`Plus_Button` in the object pool, we are incrementing and decrementing a variable in the program so that we can keep track of the displayed value on the screen.
	This variable starts out with a value of 214748364 because the output number in the object pool has an offset applied to it of -214748364. This essentially means that if we send the VT a value of 214748364, it will be shown as 0. If we subtract 1, it will be shown as -1, or if we add 1 it will be shown as 1.
	This is how a VT handles negative numbers.

Other Actions
^^^^^^^^^^^^^^

Of course, you have the ability to do a lot more than just react to events. The VT client exposes many functions that you can call to make the VT directly do things with your object pool.

Check out the `API documentation <https://delgrossoengineering.com/isobus-docs/classisobus_1_1VirtualTerminalClient.html>`_ (or the header file :code:`isobus_virtual_terminal_client.hpp`) for the full list of functionality available, but the most commonly used ones are listed below along with the name of the function to use on the API.

* Changing the active mask
	* :code:`virtualTerminalClient->send_change_active_mask`
	* :code:`virtualTerminalUpdateHelper->set_active_data_or_alarm_mask`
* Changing a numeric value
	* :code:`virtualTerminalClient->send_change_numeric_value`
	* :code:`virtualTerminalUpdateHelper->set_numeric_value`
	* :code:`virtualTerminalUpdateHelper->increase_numeric_value`
	* :code:`virtualTerminalUpdateHelper->decrease_numeric_value`
* Changing a string value
	* :code:`virtualTerminalClient->send_change_string_value`
* Changing a soft key mask
	* :code:`virtualTerminalClient->send_change_softkey_mask`
	* :code:`virtualTerminalUpdateHelper->set_active_soft_key_mask`
* Changing a list item
	* :code:`virtualTerminalClient->send_change_list_item`
* Changing an attribute, such as hiding a container
	* :code:`virtualTerminalClient->send_change_attribute`
	* :code:`virtualTerminalUpdateHelper->set_attribute`

Final Result
--------------

Here's the final code for this example:

.. code-block:: c++

	#include "isobus/hardware_integration/available_can_drivers.hpp"
	#include "isobus/hardware_integration/can_hardware_interface.hpp"
	#include "isobus/isobus/can_network_manager.hpp"
	#include "isobus/isobus/can_partnered_control_function.hpp"
	#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
	#include "isobus/isobus/isobus_virtual_terminal_client_update_helper.hpp"
	#include "isobus/utility/iop_file_interface.hpp"

	#include "objectPoolObjects.h"

	#include <atomic>
	#include <csignal>
	#include <iostream>

	//! It is discouraged to use global variables, but it is done here for simplicity.
	static std::shared_ptr<isobus::VirtualTerminalClient> virtualTerminalClient = nullptr;
	static std::shared_ptr<isobus::VirtualTerminalClientUpdateHelper> virtualTerminalUpdateHelper = nullptr;
	static std::atomic_bool running = { true };

	void signal_handler(int)
	{
		running = false;
	}

	// This callback will provide us with event driven notifications of softkey presses from the stack
	void handle_softkey_event(const isobus::VirtualTerminalClient::VTKeyEvent &event)
	{
		if (event.keyNumber == 0)
		{
			// We have the alarm ACK code, so if we have an active alarm, acknowledge it by going back to the main runscreen
			virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(example_WorkingSet, mainRunscreen_DataMask);
		}

		switch (event.keyEvent)
		{
			case isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased:
			{
				switch (event.objectID)
				{
					case alarm_SoftKey:
					{
						virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(example_WorkingSet, example_AlarmMask);
					}
					break;

					case acknowledgeAlarm_SoftKey:
					{
						virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(example_WorkingSet, mainRunscreen_DataMask);
					}
					break;

					default:
						break;
				}
			}
			break;

			default:
				break;
		}
	}

	// This callback will provide us with event driven notifications of button presses from the stack
	void handle_button_event(const isobus::VirtualTerminalClient::VTKeyEvent &event)
	{
		switch (event.keyEvent)
		{
			case isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased:
			case isobus::VirtualTerminalClient::KeyActivationCode::ButtonStillHeld:
			{
				switch (event.objectID)
				{
					case Plus_Button:
					{
						virtualTerminalUpdateHelper->increase_numeric_value(ButtonExampleNumber_VarNum);
					}
					break;

					case Minus_Button:
					{
						virtualTerminalUpdateHelper->decrease_numeric_value(ButtonExampleNumber_VarNum);
					}
					break;

					default:
						break;
				}
			}
			break;

			default:
				break;
		}
	}

	int main()
	{
		std::signal(SIGINT, signal_handler);

		// Automatically load the desired CAN driver based on the available drivers
		std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
	#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
		canDriver = std::make_shared<isobus::SocketCANInterface>("can0");
	#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
		canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(PCAN_USBBUS1);
	#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
		canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0); // CAN0
	#elif defined(ISOBUS_MACCANPCAN_AVAILABLE)
		canDriver = std::make_shared<isobus::MacCANPCANPlugin>(PCAN_USBBUS1);
	#elif defined(ISOBUS_SYS_TEC_AVAILABLE)
		canDriver = std::make_shared<isobus::SysTecWindowsPlugin>();
	#endif
		if (nullptr == canDriver)
		{
			std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
			std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
			return -1;
		}

		isobus::CANHardwareInterface::set_number_of_can_channels(1);
		isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

		if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
		{
			std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
			return -2;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(250));

		isobus::NAME TestDeviceNAME(0);

		//! Consider customizing some of these fields, like the function code, to be representative of your device
		TestDeviceNAME.set_arbitrary_address_capable(true);
		TestDeviceNAME.set_industry_group(1);
		TestDeviceNAME.set_device_class(0);
		TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
		TestDeviceNAME.set_identity_number(2);
		TestDeviceNAME.set_ecu_instance(0);
		TestDeviceNAME.set_function_instance(0);
		TestDeviceNAME.set_device_class_instance(0);
		TestDeviceNAME.set_manufacturer_code(1407);

		std::vector<std::uint8_t> testPool = isobus::IOPFileInterface::read_iop_file("VT3TestPool.iop");

		if (testPool.empty())
		{
			std::cout << "Failed to load object pool from VT3TestPool.iop" << std::endl;
			return -3;
		}
		std::cout << "Loaded object pool from VT3TestPool.iop" << std::endl;

		// Generate a unique version string for this object pool (this is optional, and is entirely application specific behavior)
		std::string objectPoolHash = isobus::IOPFileInterface::hash_object_pool_to_version(testPool);

		const isobus::NAMEFilter filterVirtualTerminal(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
		const std::vector<isobus::NAMEFilter> vtNameFilters = { filterVirtualTerminal };
		auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
		auto TestPartnerVT = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

		virtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
		virtualTerminalClient->set_object_pool(0, testPool.data(), testPool.size(), objectPoolHash);
		virtualTerminalClient->get_vt_soft_key_event_dispatcher().add_listener(handle_softkey_event);
		virtualTerminalClient->get_vt_button_event_dispatcher().add_listener(handle_button_event);
		virtualTerminalClient->initialize(true);

		virtualTerminalUpdateHelper = std::make_shared<isobus::VirtualTerminalClientUpdateHelper>(virtualTerminalClient);
		virtualTerminalUpdateHelper->add_tracked_numeric_value(ButtonExampleNumber_VarNum, 214748364); // In the object pool the output number has an offset of -214748364 so we use this to represent 0.
		virtualTerminalUpdateHelper->initialize();

		while (running)
		{
			// CAN stack runs in other threads. Do nothing forever.
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		virtualTerminalClient->terminate();
		isobus::CANHardwareInterface::stop();
		return 0;
	}


Writing up the CMake
^^^^^^^^^^^^^^^^^^^^^

The CMake for this program is a bit more complex than the other examples.

We'll start off like we did in "ISOBUS Hello World".

.. code-block:: cmake

	cmake_minimum_required(VERSION 3.16)

	project(
	  isobus_vt_tutorial
	  VERSION 1.0
	  LANGUAGES CXX
	  DESCRIPTION "An example VT client program"
	)

	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)

	add_subdirectory("AgIsoStack-plus-plus")

	add_executable(vt_example main.cpp)

Looking at "ISOBUS Hello World", we had this next:

.. code-block:: cmake

	target_link_libraries(isobus_hello_world PRIVATE isobus::Isobus isobus::HardwareIntegration Threads::Threads)

But like we mentioned earlier, we're now using a function (the IOP file reader) in the isobus utility library called "Utility", so we need to link that too:

.. code-block:: cmake

	target_link_libraries(isobus_hello_world PRIVATE isobus::Isobus isobus::HardwareIntegration isobus::Utility Threads::Threads)

We also want to move our IOP file to be in the same folder as the executable after it's built, so that it can locate it.
We can do that with this little handy bit of CMake:

.. code-block:: cmake

	add_custom_command(
		TARGET vt_example
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/VT3TestPool.iop
		$<TARGET_FILE_DIR:vt_example>/VT3TestPool.iop)

Now you should be able to build and run the program!

.. code-block:: bash

	cmake -S . -B build
	cmake --build build
	cd build
	./vt_example

That's it for this Tutorial. You should be able to run it as long as you have a VT and a supported CAN driver, see the test pool be uploaded, and be able to interact with all buttons on screen!

If you would like to see more advanced VT tutorials or have other feedback, please visit our `GitHub page <https://github.com/Open-Agriculture/AgIsoStack-plus-plus>`_ and feel free to open a discussion! We're friendly, we promise.
