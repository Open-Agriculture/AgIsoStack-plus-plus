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

As you know by know, a CAN frame can only contain 8 bytes of data payload.
Sometimes though, we need to send more than that. Those situations are when a *transport protocol* must be employed.

The CAN stack has been designed to make transport protocols invisible to you, the user, and easy to use.

To send a message of more than 8 bytes, simply send a message like normal, but with a larger size.

.. code-block:: c++

   longMessage = std::uint8_t[2000] = {0};

   isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, longMessage, 2000, myECU.get(), myPartner.get());

That's it!

There are some things to keep in mind though.

* You cannot send more then 1785 bytes to the broadcast address. This is because the Extended Transport Protocol (ETP) explicitly does not allow broadcasts.

* You can only send up to 117440505 as an absolute max. This is because ETP cannot send more than this.

* Sending long messages to the broadcast address is highly discouraged. The standard mandates (and the stack enforces) a mandatory delay between packets sent this way. This means those messages will be *very slow* to transmit. Although the CAN stack *is completely non-blocking* on any transmit, you can only do one BAM session at a time, so a long transmit may cause issues in your application if you need to send another BAM and the stack is still sending the last one.

Receiving More Than 8 Bytes
----------------------------

Registering for a PGN will register you for messages of *any size* for that PGN, including ones that are sent via a transport protocol.

See the :doc:`receiving messages tutorial <./Receiving Messages>` for instructions on how to do this.

That's it for this tutorial! Sending 100MB CAN messages has never been easier.
