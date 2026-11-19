.. _API Heartbeat:

ISOBUS Heartbeat API
====================

The heartbeat message (PGN 61668/0xF0E4) is used to determine the integrity of the communication of messages and parameters being transmitted by a control function. 
There may be multiple instances of the heartbeat message on the network, and CFs are required to transmit the message on request.
As long as the heartbeat message is transmitted at the regular time interval and the sequence number increases through the valid range, then the heartbeat message indicates that the data source CF is operational and provides correct data in all its messages.

Each of your internal control functions starts sending its heartbeat every 100 ms once another control function requests it with a repetition rate of 0, 100 or 65535 ms.
The stack only supports 100 ms for now, so a request for any other rate is ignored with a warning, even though the standard allows a source to honour other rates.

To monitor another control function, call ``request_heartbeat()`` and listen to the event dispatchers:

- ``get_new_tracked_heartbeat_event_dispatcher()`` fires on the first heartbeat received from a control function, requested or not.
- ``get_heartbeat_error_event_dispatcher()`` reports ``RequestNotAccepted``, ``TimedOut``, ``InvalidSequenceCounter``, ``TransmissionError`` and ``GracefulShutdown``.
  After ``RequestNotAccepted``, ``TimedOut`` and ``GracefulShutdown`` the control function is no longer tracked, so request it again if you still need it.
- ``get_heartbeat_recovered_event_dispatcher()`` fires when a control function leaves an error state after 8 correct heartbeats in a row.

Call ``set_transmission_error()`` to report a fault in your own control function.
Call ``send_shutdown()`` at least 100 ms before deactivating it, so that receivers see a graceful shutdown instead of a timeout.

.. note::
   This interface is enabled by default, but can be disabled if you want to stop your heartbeat(s) or don't care about the safety-critical path of the machine.

.. doxygenclass:: isobus::HeartbeatInterface
   :members:
