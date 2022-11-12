.. _InstallationGuide:

Installation
============

.. contents:: Contents
   :depth: 2
   :local:

Supported Platforms
--------------------

We support building the ISO 11783 CAN stack from source on the following platforms:
   * Ubuntu Linux (Non-WSL)
   * RHEL

Make sure you are using a supported platform in order to receive the best support for any issues you may encounter.

WSL is not supported due to the WSL kernel not supporting socket CAN by default.

Currently, building from source is the only supported integration method.

Environment Setup
--------------------

First, lets prepare the dependencies we'll need to compile the CAN stack. These are pretty basic, and you may have them already.

.. code-block:: bash

   sudo apt install build-essential cmake git

Downloading the CAN Stack
--------------------------

In your project that you want to add the CAN stack to, add the CAN stack as a submodule.

.. code-block:: bash

   git submodule add https://github.com/ad3154/ISO11783-CAN-Stack.git
   git submodule update --init --recursive

This will place the CAN stack in a folder within your project called 'ISO11783-CAN-Stack'.

Building the CAN Stack
-----------------------

There are a couple options for the next step.

CMake:
^^^^^^

If your project is already using CMake to build your project, or this is a new project, the suggested way to get the library compiling is to add the 'ISO11783-CAN-Stack' folder we just created to your CMake as a subdirectory.

.. code-block:: text

   set(THREADS_PREFER_PTHREAD_FLAG ON)
   find_package(Threads REQUIRED)

   add_subdirectory("ISO11783-CAN-Stack")

   ...

   target_link_libraries(<your target> Isobus HardwareIntegration ${CMAKE_THREAD_LIBS_INIT})

Using CMake has a lot of advantages, such as if the library is updated with additional files, or the file names change, it will not break your compilation.
   
Non-CMake:
^^^^^^^^^^

If you are not using CMake, just make sure to add all the files from the 'ISO11783-CAN-Stack/isobus' folder to your project so they all get compiled. You'll want to make sure the 'ISO11783-CAN-Stack/isobus/include' folder is part of your include path.

If you're using socket CAN, make sure 'socket_can/include' is also in your include path.

