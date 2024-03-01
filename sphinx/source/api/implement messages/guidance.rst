.. _API Guidance:

ISOBUS Guidance API
===================

The guidance API is an interface for sending and receiving ISOBUS guidance messages.
These messages are used to steer ISOBUS compliant machines, steering valves, and implements in general.


.. warning::

    Please use extreme care if you try to steer a machine with this interface!
    Remember that this library is licensed under The MIT License, and that by obtaining a
    copy of this library and of course by attempting to steer a machine with it, you are agreeing
    to our license.

.. note::
    These messages are expected to be deprecated or at least made redundant in favor
    of Tractor Implement Management (TIM) at some point by the AEF, though the timeline on that
    is not known at the time of writing this, and it's likely that many machines will
    continue to support this interface going forward due to its simplicity over TIM.
    This project is not affiliated with the AEF, and the AEF has not endorsed this project.

.. doxygenclass:: isobus::AgriculturalGuidanceInterface
   :members:
