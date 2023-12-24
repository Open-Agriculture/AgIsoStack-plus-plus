.. _InstallationGuide:

Installation
============

This guide will walk you through the steps to get the AgIsoStack++ library integrated into your project.

.. contents:: Contents
   :depth: 2
   :local:

Supported Platforms
-------------------

We officially support building AgIsoStack++ from source on the following platforms:
   * Ubuntu Linux (Non-WSL)
   * Raspberry Pi OS (Raspbian)
   * RHEL
   * Windows
   * MacOS
   * ESP32

.. note::

	AgIsoStack++ may also work on other platforms, and has been designed to accommodate different underlying hardware, but the ones listed above are the officially supported platforms. Additional platform support may be added if there is demand for it.

	Using a supported platform will get you the best support for any issues you may encounter.

	WSL is not supported due to the WSL kernel not supporting socket CAN by default. It may be possible to recompile the WSL kernel to support socket CAN, but we do not officially support that use case.

Currently, building from source is the only supported integration method.

.. _installation-environment:

Environment Setup
-----------------

First, lets prepare the dependencies we'll need to download and compile the stack. These are pretty basic, and you may have them already.

Linux:

.. code-block:: bash

   sudo apt install build-essential cmake git

Windows:

   * Install `Build Tools for Visual Studio <https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022>`_
   * Install `CMake <https://cmake.org/download/>`_
   * Install `Git <https://git-scm.com/download/win>`_


Integration
-----------

There are multiple ways to integrate the library into your project. 
The easiest way is to use git submodules in combination with CMake.

Git Submodules
^^^^^^^^^^^^^^

In directory of the `CMakeLists.txt` that you want to integrate the library into, run the following commands:

.. code-block:: bash

   git submodule add https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
   git submodule update --init --recursive

This will place the stack in a folder within your project called 'AgIsoStack-plus-plus'.

It is recommended to use the ALIAS targets exposed, which all follow the name `isobus::<target_name>`.

.. code-block:: cmake

   find_package(Threads)

   add_subdirectory(<path to this submodule>)

   target_link_libraries(<your executable name> PRIVATE isobus::Isobus isobus::HardwareIntegration isobus::Utility Threads::Threads)


A full example CMakeLists.txt file can be found at the end of the first tutorial: :doc:`The ISOBUS Hello World <Tutorials/The ISOBUS Hello World>`.


Finally, every time you want to update the stack to the latest version, run the following commands:

.. code-block:: bash

   git submodule update --remote --merge

This will pull the latest version of the stack into your project.

CMake FetchContent
^^^^^^^^^^^^^^^^^^

If you don't want to use git submodules, you can use CMake's FetchContent module to download the stack.

.. code-block:: cmake

   include(FetchContent)

   FetchContent_Declare(
      AgIsoStack
      GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
      GIT_TAG        main # Replace this with tag or commit hash for better stability
   )
   FetchContent_MakeAvailable(AgIsoStack)
   
   # Somewhere later in your CMakeLists.txt
   target_link_libraries(<your executable name> PRIVATE isobus::Isobus isobus::HardwareIntegration isobus::Utility Threads::Threads)

Now when you configure your CMake cache, the library will be pulled from GitHub and automatically made available for your project.

We recommend using a specific tag or commit hash instead of a branch, as this will provide better stability for your project.
Plus, it provides faster configuration times, as CMake won't need to check for updates every time you configure your project.

Precompiled
^^^^^^^^^^^

We do not officially distribute this library in binary form (DLL files, for example).
