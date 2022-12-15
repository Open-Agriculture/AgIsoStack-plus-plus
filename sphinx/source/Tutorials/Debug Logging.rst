.. _DebugLogging:

Debug Logging
==============

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

The CAN stack has debug logging that you can use to find issues with your application, your ISOBUS communication, or even issues with the CAN stack itself.
This logging is provided in the form of a virtual class that you can implement to sink the log entries to any destination you want.

Let's write a very simple custom logger that will consume the output from the CAN stack's log statements and emit them to :code:`stdout`.

.. code-block:: c++

	#include "isobus/isobus/can_warning_logger.hpp"
	#include <iostream>

	class CustomLogger : public isobus::CANStackLogger
	{
	public:
		void LogCANLibWarning(const std::string &text) override
		{
			std::cout << text << std::endl;
		}
	};

Here you can see that we've inherited from the CAN Stack's :code:`CANStackLogger` and implemented the function :code:`LogCANLibWarning`.
In our example the text that the stack is logging will be printed to the console.
You could also emit this text to your favorite logger instead if you like, such as `spdlog <https://github.com/gabime/spdlog>`_.

Now, we just need to create a static instance of this logger, and tell the CAN stack to use it.

.. code-block:: c++

	static CustomLogger logger;

	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);

The logger must be static or otherwise not go out of scope, as the CAN stack saves a reference to that logger object!

That's all there is to it! Now, when you run your program you should see some logging messages are written to your console.

A full example of this is included with the `VT example program <https://github.com/ad3154/ISO11783-CAN-Stack/blob/main/examples/vt_version_3_object_pool/main.cpp>`_.
