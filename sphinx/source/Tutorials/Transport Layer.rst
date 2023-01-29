.. _TransportLayer:

Using the Transport Layer
==========================

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

This tutorial covers how to send and receive multi-frame messages. It is assumed you have completed all the other tutorials, as this tutorial picks up where :doc:`receiving messages <./Receiving Messages>` left off.

Sending More Than 8 Bytes
--------------------------

As you know by now, a CAN frame can only contain 8 bytes of data payload.
Sometimes though, we need to send more than that. Those situations are when a *transport protocol* must be employed.

The CAN stack has been designed to make most transport protocols invisible to you, the user, and easy to use.

To send a message of more than 8 bytes, simply send a message like normal, but with a larger size.

.. code-block:: c++

   std::uint8_t longMessage[2000] = {0};

   isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, longMessage, 2000, myECU.get(), myPartner.get());

That's it! Your message will be sent with either TP or ETP depending on the size of the payload.

There are some things to keep in mind though.

* You cannot send more than 1785 bytes to the broadcast address. This is because the Extended Transport Protocol (ETP) explicitly does not allow broadcasts and Fast Packet only allows messages up to 223 bytes.

* You can only send up to 117440505 bytes as an absolute max. This is because ETP is defined such that it cannot send more than this.

.. warning::

	Sending long messages to the broadcast address is highly discouraged. The standard mandates (and the stack enforces) a mandatory delay between packets sent this way. This means those messages will be *very slow* to transmit. Although the CAN stack *is completely non-blocking* on any transmit, you can only do one BAM session at a time, so a long transmit may cause issues in your application if you need to send another BAM and the stack is still sending the last one.

.. note::

	Unlike TP and ETP, NMEA 2000 Fast Packet messages must be sent and registered for explicitly. This will be covered below in the section "Sending and Reveiving a Fast Packet Message".

Receiving More Than 8 Bytes
----------------------------

Registering for a PGN will register you for messages of *any size* for that PGN, including ones that are sent via a TP and ETP.

See the :doc:`receiving messages tutorial <./Receiving Messages>` for instructions on how to do this.

Sending and Receiving a Fast Packet Message
--------------------------------------------

To send a NMEA 2000 Fast Packet message, you'll need to call the function :code:`isobus::FastPacketProtocol::Protocol.send_multipacket_message`. You can view the API docs for this function `here <https://delgrossoengineering.com/isobus-docs/classisobus_1_1FastPacketProtocol.html#a5ba5d9ca1467b87aee566c6346431707>`_.

Here's an example:

.. code-block:: c++

	std::uint8_t testMessageData[100] = {0};

	isobus::FastPacketProtocol::Protocol.send_multipacket_message(0x1F001, testMessageData, 100, someInternalControlFunction, nullptr, isobus::CANIdentifier::PriorityLowest7, nullptr);

This example would send a 100 byte message from `someInternalControlFunction` to the broadcast address with the PGN 0x1F001.

To receive messages sent via Fast Packet, you have to tell the CAN stack that it should interpret a certain PGN using that protocol rather than treating it as regular 8 byte frames with the same PGN.

You can do this by calling :code:`isobus::FastPacketProtocol::Protocol.register_multipacket_message_callback`. You can vew the API docs for this function `in the doxygen <https://delgrossoengineering.com/isobus-docs/classisobus_1_1FastPacketProtocol.html#a97f39f3272dfa9133abaab26592b1f50>`_.

.. note::

	A full example of both sending and receiving can be found in the `examples folder <https://github.com/ad3154/ISO11783-CAN-Stack/tree/main/examples>`_ of this project.

That's it for this tutorial! Sending 100MB CAN messages has never been easier.
