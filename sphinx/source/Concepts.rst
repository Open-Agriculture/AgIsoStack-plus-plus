.. _Concepts:

Concepts
============

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

The Basics
-----------

If you've found this project, you probably already know a few things about ISO 11783 (ISOBUS) or J1939.
It's OK if you haven't heard of those before, but this concepts overview assumes you know a bit about CAN already.

Lets discuss some of the basics you'll need to know to use this library.

An ISOBUS network is a 250000 baud CAN bus that follows the AEF's ISO 11783 standard of communication, which itself is based on SAE's J1939. The ISOBUS standards are designed for agricultural vehicles and forestry.

As far as this library is concerned, CAN frames contain at most 8 bytes of data, along with a 32 bit identifer.
This identifier contains a number of important things.

The identifer contains:

* The Parameter group number (PGN)

   * This is what identifies what's in the data paylod, like the subject line of an email
   * You will need to know some of these (or create some) to communicate with most devices
   * Some standard ones can be found here: https://www.isobus.net/isobus/pGNAndSPN/?type=PGN

* The source address

   * These describes who is sending the message, like the return address on a peice of mail

* The Destination address

   * Some messages allow you to specify a Destination
   * Some messages are meant to be received by everyone (a broadcast)
   * The Parameter group number determines if a message is a broadcast or destined for a specific address.

* A Priority

   * This determines, when messages collide on the bus, which one "wins" and which transmitter has to re-transmit their message later
   * This is handled by hardware, not software generally, so we usually only care about this when transmitting messages, not receiveing them

In order to send messages on a J1939 or ISO bus, you need a 1 byte address, which identifes you on the bus.
Likewise, you need to know the address of who you're sending a mesage to (unless you are sending a broadcast).
Broadcasts are always sent to the broadcast address, which is 255 (0xFF).
There's also something called the NULL address, which is the address you use when you have no address (or are in the process of getting one - we'll talk about that later).

Control Functions
--------------------

In reality, addresses are useless for most things other than filling in or decoding a CAN frame.
They tell you nothing about the identity of that device, what it does, who made it, or if it even cares that you are talking to it.
It's not even reliable for sending messages, as J1939 and ISOBUS devices can change address at any time.

Thus, J1939 and ISO 11783 defined the basic component of a network as a *control function*.

A control function has an address, but is first and foremost identified by its NAME.

A control function's address could change at *any time* via a process called address claiming and arbitration. We'll talk more about this later.

The NAME
^^^^^^^^^

Conceptually, a NAME is a 64 bit value that uniquely identifies a control function on the network.

A NAME has the following components in it:

* Identity Number

   * This is usually the serial number of the control function.
   * It's supposed to be unique amongst control functions with identical other NAME values

* Manufacturer Code

   * Identifies who made the control function - based on a list https://www.isobus.net/isobus/manufacturerCode

* ECU Instance

   * Usually increments in NAME order with similar control functions
   * Basically if you have multiple devices with similar names, this can be used to establish a heierarchy within the ISO NAME amongst them

* Function

   * ISO 11783 Defines a list of functions that describe what your control function does
   * This could be a lot of different things: https://www.isobus.net/isobus/nameFunction

* Function Instance

   * The function instance of the ECU. Similar to Virtual Terminal number.

* Device Class 

   * This is also known as the "vehicle system" from J1939
   * This describes general ECU type, such as "Sprayers" or "Backhoe"

* Device Class Instance

   * The instance number of this device class, similar to ECU instance
   * Used to establish a heierarchy between similar NAMEs if needed.

* Industry Group

   * The industry group associated with this ECU, such as "agricultural"
   * See more here: https://www.isobus.net/isobus/nameFunction

* Arbitrary Address Capability

   * Defines if this ECU supports address arbitration

Address Claiming
^^^^^^^^^^^^^^^^^

J1939 and ISOBUS define a process called *address claiming* which very deterministically assigns a control function an address based on their *NAMEs* 

The library will handle this for you completely, but it's important to know it exists, as this is what ties a NAME to an address.

Review
^^^^^^^
So, to review, a control function is permentantly identified by its NAME.
This NAME is used during *address claiming* to assign the control function an *address*.
This address is not permenant, and may change at any time if address claming occurs again for some reason.

Now that you understand these concepts, you can create your first control function and send some basic messges!
Check out the Basic Tutorial now if you'd like to practice this.

Transport Layer
----------------

As we discussed before, a CAN frame hold only 8 bytes of data. But sometimes, you want to send more than that!

This is what the transport layer is for. It should be transparent to you most of the time when using the library, and allows you to send messages of nearly any size.

Here are the different transport layers this library provides, along with a short explanation of what they are:

* Transport Protocol (Broadcast Announce Message)

   * This protocol is called BAM for short
   * It handles sending more than 8 bytes, but less than or equal to 1785 bytes to the broadcast address
   * There are very severe implications for sending a transport message to the broadcast address! Because sending messages to the broadcast address cannot be stopped by a receiver, and because they are often unsolicited, there is a *required delay* between each 8 byte frame that comprises a BAM packet. This makes delivery of the message very, very slow. A control function can also only have 1 BAM transmit in-progress at any time, because the PGN of the message is actually embedded in the payload of the reassembled packet, not the individual frames. This means that if you have a BAM that is on-going, you will be unable to send another until the first one is done. Thus, BAM should be your last choice. If you need to send more than 8 bytes, try to do it without using a broadcast.

* Transport Protocol (Connection Mode)

   * This protocol handles sending more than 8 bytes, but less than or equal to 1785 bytes to a specific destination address
   * This protocol can be very fast, as there are no delays needed between packets.
   * Either side can abort the the session if there's an issue.
   * Multiple sessions can be on-going at once, as long as they are to different destinations.

* Extended Transport Protocol (ETP)

   * Used for sending more than 1785 bytes, but less than or equal to 117440505 bytes.
   * Does not support broadcasts of any length - must be destination specific.
   * This protocol supports the most data of any ISOBUS transport protocols.
   * Can be very, very, very slow (many minutes) depending on the amount of data transferred, but there are no delays between messages, so the delay mostly comes from the number of bytes you need to send overall.

The CAN stack library will take care of choosing which protocol to use automatically!

Now that you understand these concepts, check out the Transport Layer Tutorial and example.

Diagnostic Protocol
--------------------
