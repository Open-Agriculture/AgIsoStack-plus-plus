.. _InstallationGuide:

Installation
============

.. contents:: Contents
   :depth: 2
   :local:

Supported Platforms
--------------------

We officially support building ISOBUS++ from source on the following platforms:
   * Ubuntu Linux (Non-WSL)
   * Raspberry Pi OS (Raspbian)
   * RHEL
   * Windows
   * MacOS
   * ESP32

.. note::

	ISOBUS++ may also work on other platforms, and has been designed to accomodate different underlying hardware, but the ones listed above are the officially supported platforms. Additional platform support may be added if there is demand for it.

	Using a supported platform will get you the best support for any issues you may encounter.

	WSL is not supported due to the WSL kernel not supporting socket CAN by default. It may be possible to recompile the WSL kernel to support socket CAN, but we do not officially support that use case.

	Currently, building from source is the only supported integration method.

Environment Setup (Linux)
--------------------------

First, lets prepare the dependencies we'll need to compile the CAN stack. These are pretty basic, and you may have them already.

.. code-block:: bash

   sudo apt install build-essential cmake git

Downloading ISOBUS++
----------------------

In your project that you want to add the CAN stack to, add the CAN stack as a submodule.

.. code-block:: bash

   git submodule add https://github.com/ad3154/Isobus-plus-plus.git
   git submodule update --init --recursive

This will place the CAN stack in a folder within your project called 'Isobus-plus-plus'.

Building the CAN Stack
-----------------------

There are a couple options for the next step.

CMake:
^^^^^^

If your project is already using CMake to build your project, or this is a new project, the suggested way to get the library compiling is to add the 'Isobus-plus-plus' folder we just created to your CMake as a subdirectory.

.. code-block:: text

   set(THREADS_PREFER_PTHREAD_FLAG ON)
   find_package(Threads REQUIRED)

   add_subdirectory("Isobus-plus-plus")

   ...

   target_link_libraries(<your target> PRIVATE isobus::Isobus isobus::HardwareIntegration Threads::Threads)

Using CMake has a lot of advantages, such as if the library is updated with additional files, or the file names change, it will not break your compilation.
   
Non-CMake:
^^^^^^^^^^

If you are not using CMake, just make sure to add all the files from the 'Isobus-plus-plus/isobus' folder, the 'Isobus-plus-plus/hardware_integration' folder, and the 'Isobus-plus-plus/utility' folder to your project so they all get compiled. 

You'll want to make sure the 'Isobus-plus-plus/isobus/include/isobus/isobus' folder is part of your include path as well as 'Isobus-plus-plus/utility/include/isobus/utility' and 'Isobus-plus-plus/hardware_integration/include/isobus/hardware_integration'.
