.. _ReceivingMessages:

Receiving Messages
===================

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

So far we've covered setting up the CAN stack, sending broadcast messages, and sending destination specific messages.

Now we'll learn how to receive messages.

Receiving Broadcast Messages
-----------------------------

Receiving any message from the CAN stack will always come in the form of a callback.

Broadcast messages are the easiest to receive. Simply register a callback for the PGN you want to receive.

In this example, we'll define a function that will process all received proprietary A messages sent to the broadcast address.

.. code-block:: c++

   void propa_callback(isobus::CANMessage *CANMessage, void *parent)
   {
      if (nullptr != CANMessage)
      {
         std::cout << CANMessage->get_data_length() << std::endl;
      }
   }

This callback isn't particularly useful, but it does illustrate how to use the callback system.

Basically, whenever a PROPA message is received that was sent to the broadcast address, it will print out the length of that message to the console.

Now, we just need to tell the CAN stack to call that callback when an appropriate message is received.

.. code-block:: c++

   isobus::CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(0xEF00, propa_callback, nullptr);

So in the above code, we're telling the stack that for any broadcasts with PGN 0xEF00 to call that function. The :code:`nullptr` variable is a generic context variable. Whatever you pass in for that variable will be passed into broadcast_propa_callback when it is called later. 
This can be useful for figuring out what object wanted the callback. For example, if we were registering this callback for a particular class, that class could pass in `this` as that argument to have its own pointer passed back to it in the callback.

It's OK if you don't understand that last variable's usage for now. In our example, we're just ignoring it.

Feel free to review the API docs for this function `here <https://delgrossoengineering.com/isobus-docs/classisobus_1_1CANNetworkManager.html>`_.

How Do We Test It?
^^^^^^^^^^^^^^^^^^^^

Our example program has to stay running for us to receive messages, so if we want to test our latest changes, we'll want to add an infinite sleep into the program, so that it runs until we press control+c.

.. code-block:: c++

   while (true)
   {
      // CAN stack runs in other threads. Do nothing forever.
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   }

So, our updated tutorial program now should look like this:

.. code-block:: c++

   #include "isobus/isobus/can_network_manager.hpp"
   #include "isobus/hardware_integration/socket_can_interface.hpp"
   #include "isobus/hardware_integration/can_hardware_interface.hpp"
   #include "isobus/isobus/can_partnered_control_function.hpp"

   #include <memory>
   #include <csignal>
   #include <iostream>

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

   void propa_callback(isobus::CANMessage *CANMessage, void *parent)
   {
      if (nullptr != CANMessage)
      {
         std::cout << CANMessage->get_data_length() << std::endl;
      }
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

      // Create our InternalControlFunction
      myECU = std::make_shared<isobus::InternalControlFunction>(myNAME, 0x1C, 0);

      // Define a NAME filter for our partner
      std::vector<isobus::NAMEFilter> myPartnerFilter;
      const isobus::NAMEFilter virtualTerminalFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
      myPartnerFilter.push_back(virtualTerminalFilter);

      // Register to receive broadcast PROPA messages
      isobus::CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(0xEF00, propa_callback, nullptr);

      // Create our PartneredControlFunction
      myPartner = std::make_shared<isobus::PartneredControlFunction>(0, myPartnerFilter);

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      std::array<std::uint8_t, isobus::CAN_DATA_LENGTH> messageData = {0}; // Data is just all zeros

      // Send a message to the broadcast address
      isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, messageData.data(), isobus::CAN_DATA_LENGTH, myECU.get());

      // Send a message to our partner (if it is present)
      isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, messageData.data(), isobus::CAN_DATA_LENGTH, myECU.get(), myPartner.get());

      while (true)
      {
         // CAN stack runs in other threads. Do nothing forever.
         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }

      // Clean up the threads
      CANHardwareInterface::stop();

      return 0;
   }

This will be tricky to test, as you would need another valid ISO 11783 device on the bus *that has properly address claimed* and is also sending this message in order for the stack to receive it and pass it to your callback.

The point is, you will want to adapt this to meet your own needs, and receive messages you care about!

Receiving Destination Specific Messages
----------------------------------------

Like with sending messages to specific destinations, you also need a `PartneredControlFunction <https://delgrossoengineering.com/isobus-docs/classisobus_1_1PartneredControlFunction.html>`_ to receive messages from specific destinations.

Limiting what you receive to specific, known partners (or at least desirable function codes) will help keep your application efficient! Nothing kills performance more quickly than receiving all messages from everyone all the time.

Using our previous code, if we want to get a callback for PROPA messages from our virtual terminal partner, we just add it, like this:

.. code-block:: c++

   myPartner->add_parameter_group_number_callback(0xEF00, propa_callback, nullptr);

That's it! Now our callback will get called whenever that partner sends us specifically a PROPA message.
Of course, we also registered that same callback function for broadcasts, so now that function will be called for either a broadcast PROPA, or one from our partner to us, specifically.

A Note About Receiving TP and ETP Messages
--------------------------------------------

We'll talk a bit more about multi-frame messages in our next tutorial, but wanted to point out that this method is also how you receive multi-frame TP or ETP messages!

For example, if someone sends a PROPA message via BAM that is 1785 bytes long, it will also be sent to our callback!

You now know how to receive messages of any size and any PGN!

Check out our next tutorial about the transport layer!
