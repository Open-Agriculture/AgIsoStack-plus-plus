.. _AddingADestination:

Adding A Destination
=====================

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

This tutorial assumes you've already completed the :doc:`ISOBUS Hello World Tutorial<./The ISOBUS Hello World>` and picks up where that tutorial left off. If you haven't done that tutorial yet, consider going back and finishing that one first.

In The ISOBUS Hello World, we learned how to set up the CAN stack and send a simple message to the broadcast address. But, what if you don't want to send to the global address? Let's say we want to talk specifically to a virtual terminal (VT).

This is where the concept of a `PartneredControlFunction <https://delgrossoengineering.com/isobus-docs/classisobus_1_1PartneredControlFunction.html>`_ comes in.

Let's add a VT partner to our example program.

NAME Filters
^^^^^^^^^^^^^

The first thing we need to do is construct a filter that will tell the CAN stack we only care about a VT. This is called a `NAME filter <https://delgrossoengineering.com/isobus-docs/classisobus_1_1NAMEFilter.html>`_.

When we create a `PartneredControlFunction <https://delgrossoengineering.com/isobus-docs/classisobus_1_1PartneredControlFunction.html>`_, we must supply a `NAME filter <https://delgrossoengineering.com/isobus-docs/classisobus_1_1NAMEFilter.html>`_ along with it so that the stack knows what kind of device you want to talk to. 
You can be as specific, or a general as you want. Adding multiple filter values with the same key, like "function" or "manufacturer code" will cause the stack to match against *either* filter.

Let's make our filter:

.. code-block:: c++

   std::vector<isobus::NAMEFilter> myPartnerFilter;

   const isobus::NAMEFilter virtualTerminalFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));

   myPartnerFilter.push_back(virtualTerminalFilter);

Here we've created a vector of filters, and one filter itself. In this case, we want to filter for an ECU whose function code matches a virtual terminal's function code.
Then, we added the filter to the list of filters.

Now, let's create our partner.

Creating a Partner
^^^^^^^^^^^^^^^^^^^

First, we create a pointer to store our partner. I'm using a shared_ptr in the example, but you can use a regular concrete object, or a raw pointer, or whatever you want.

The main reason I use shared_ptr is because that is what the interface for a VirtualTerminalClient expects.

.. code-block:: c++

   std::shared_ptr<isobus::PartneredControlFunction> myPartner = nullptr;

   myPartner = std::make_shared<isobus::PartneredControlFunction>(0, myPartnerFilter);

Above, we've just instantiated a partner *on CAN channel 0* using the filter we made in the previous step.

Whenever a device comes onto the bus matching that filter, the CAN stack will associate it with our `PartneredControlFunction <https://delgrossoengineering.com/isobus-docs/classisobus_1_1PartneredControlFunction.html>`_.

Think of this as the "destination" for your messages. When you send a destination specific message, you must provide a :code:`PartneredControlFunction` to the :code:`send_can_message` function.

Now let's send the message!

Sending a Destination Specific Message
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this step, we will construct and send a proprietary A message to our partner.

.. code-block:: c++

   std::array<std::uint8_t, isobus::CAN_DATA_LENGTH> messageData = {0}; // Data is just all zeros

   isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, messageData.data(), isobus::CAN_DATA_LENGTH, myECU.get(), myPartner.get());

As you can see, the call to the network manager to send the message is nearly identical to the one to send it to the broadcast address, but with the addition of our partner :code:`myPartner`.

It is highly recommended that you review all possible parameters for :code:`send_can_message` so you know what options are available.

Check out the documentation for it `here <https://delgrossoengineering.com/isobus-docs/classisobus_1_1CANNetworkManager.html>`_.

You may also want to add some delay if your program would otherwise exit immediately, to ensure the message is sent before your program exits.

.. code-block c++

   std::this_thread::sleep_for(std::chrono::milliseconds(10));

Putting It All Together
^^^^^^^^^^^^^^^^^^^^^^^^

The final program for this tutorial (including the code from the previous Hello World tutorial) looks like this:

.. code-block:: c++

   #include "isobus/isobus/can_network_manager.hpp"
   #include "isobus/hardware_integration/socket_can_interface.hpp"
   #include "isobus/hardware_integration/can_hardware_interface.hpp"
   #include "isobus/isobus/can_partnered_control_function.hpp"

   #include <memory>
   #include <csignal>

   void signal_handler(int)
   {
      CANHardwareInterface::stop(); // Clean up the threads
		_exit(EXIT_FAILURE);
   }

   void update_CAN_network()
   {
      isobus::CANNetworkManager::CANNetwork.update();
   }

   void raw_can_glue(isobus::HardwareInterfaceCANFrame &rawFrame, void *parentPointer)
   {
      isobus::CANNetworkManager::CANNetwork.can_lib_process_rx_message(rawFrame, parentPointer);
   }

   int main()
   {
      isobus::NAME myNAME(0); // Create an empty NAME
      std::shared_ptr<isobus::InternalControlFunction> myECU = nullptr; // A pointer to hold our InternalControlFunction
      std::shared_ptr<isobus::PartneredControlFunction> myPartner = nullptr; // A pointer to hold a partner

      // Set up the hardware layer to use SocketCAN interface on channel "can0"
      std::shared_ptr<SocketCANInterface> canDriver = std::make_shared<SocketCANInterface>("can0");
      CANHardwareInterface::set_number_of_can_channels(1);
      CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

      if ((!CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
      {
         std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
         return -1;
      }

      // Handle control+c
      std::signal(SIGINT, signal_handler);

      CANHardwareInterface::add_can_lib_update_callback(update_CAN_network, nullptr);
      CANHardwareInterface::add_raw_can_message_rx_callback(raw_can_glue, nullptr);

      //! Make sure you change these for your device!!!!
      //! This is an example device that is using a manufacturer code that is currently unused at time of writing
      myNAME.set_arbitrary_address_capable(true);
      myNAME.set_industry_group(1);
      myNAME.set_device_class(0);
      myNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
      myNAME.set_identity_number(2);
      myNAME.set_ecu_instance(0);
      myNAME.set_function_instance(0);
      myNAME.set_device_class_instance(0);
      myNAME.set_manufacturer_code(64);

      // Define a NAME filter for our partner
      std::vector<isobus::NAMEFilter> myPartnerFilter;
      const isobus::NAMEFilter virtualTerminalFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
      myPartnerFilter.push_back(virtualTerminalFilter);

      // Create our InternalControlFunction
      myECU = std::make_shared<isobus::InternalControlFunction>(myNAME, 0x1C, 0);

      // Create our PartneredControlFunction
      myPartner = std::make_shared<isobus::PartneredControlFunction>(0, myPartnerFilter);

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      std::array<std::uint8_t, isobus::CAN_DATA_LENGTH> messageData = {0}; // Data is just all zeros

      // Send a message to the broadcast address
      isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, messageData.data(), isobus::CAN_DATA_LENGTH, myECU.get());

      // Send a message to our partner (if it is present)
      isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, messageData.data(), isobus::CAN_DATA_LENGTH, myECU.get(), myPartner.get());

      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      // Clean up the threads
      CANHardwareInterface::stop();

      return 0;
   }
  
Like before, you can compile it with :code:`cmake --build build` and run it!

In our next tutorial, we'll cover receiving messages.
