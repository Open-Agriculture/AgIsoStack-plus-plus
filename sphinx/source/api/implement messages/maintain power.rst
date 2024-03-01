.. _API MaintainPower:

Maintain Power API
==================

This interface provides a way to manage sending and receiving the "maintain power" message.
This message is sent by any control function connected to the implement bus and requests that the Tractor ECU (TECU) not switch off the power for 2 s after it has received the wheel-based speed and distance message indicating that the ignition has been switched off.
The message also includes the connected implement(s) operating state.
You can choose if the TECU maintains actuator power independently of ECU power as well, as an option.

.. note::
    If you are using the library for implement section control, you might want to maintain actuator power using this interface to ensure your section valves close when keyed off.

.. doxygenclass:: isobus::MaintainPowerInterface
   :members:
