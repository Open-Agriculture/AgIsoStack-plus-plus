.. _FAQ:

Frequently Asked Questions
===========================

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:


Does sending a large CAN message (via a transport protocol) block the current thread while it is sent?
-------------------------------------------------------------------------------------------------------

No. All transmits are asynchronous.

What do I do if I see "undefined reference to `pthread_create`"
----------------------------------------------------------------

If you are seeing something similar to this error:

.. code-block:: text

   /usr/bin/ld: AgIsoStack-plus-plus/socket_can/libSocketCANInterface.so: undefined reference to `pthread_create'
   collect2: error: ld returned 1 exit status
   make[2]: *** [CMakeFiles/isobus_hello_world.dir/build.make:86: isobus_hello_world] Error 1
   make[1]: *** [CMakeFiles/Makefile2:285: CMakeFiles/isobus_hello_world.dir/all] Error 2
   make: *** [Makefile:130: all] Error 2

Make sure your CMake links these to your executable:

.. code-block:: text

   set(THREADS_PREFER_PTHREAD_FLAG ON)
   find_package(Threads)

   target_link_libraries(<your executable name> PRIVATE isobus::Isobus isobus::HardwareIntegration Threads::Threads)

I have some other issue!
-------------------------

Please submit an issue at the project's GitHub page:
https://github.com/Open-Agriculture/AgIsoStack-plus-plus/issues
