.. _API Heartbeat:

ISOBUS Heartbeat API
====================

The heartbeat message (PGN 61668/0xF0E4) is used to determine the integrity of the communication of messages and parameters being transmitted by a control function. 
There may be multiple instances of the heartbeat message on the network, and CFs are required transmit the message on request. 
As long as the heartbeat message is transmitted at the regular time interval and the sequence number increases through the valid range, then the heartbeat message indicates that the data source CF is operational and provides correct data in all its messages.

.. note::
   This interface is enabled by default, but can be disabled if you want to stop your heartbeat(s) or don't care about the safety-critical path of the machine.

.. doxygenclass:: isobus::HeartbeatInterface
   :members:
