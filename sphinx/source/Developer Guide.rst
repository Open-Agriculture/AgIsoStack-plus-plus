.. _DeveloperGuide:

Developer Guide
===============

This guide provides step-by-step instructions for contributors to download, build, test, and run examples in the project.

Prerequisites
-------------

Before proceeding, ensure that you have the following prerequisites installed on your system, for more details see :ref:`Environment Setup <installation-environment>`:

- Git
- CMake
- C++ Compiler

Downloading the Project
-----------------------

To download the project, follow these steps:

1. Open a terminal or command prompt.
2. Change to the directory where you want to download the project to.
3. Run the following command to clone the project repository,
   this will clone the project repository into a directory named :code:`agisostack-plus-plus`.

    .. code-block:: bash

        git clone https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git


Building the Project
--------------------

To build the project, follow these steps:

1. Change to the project directory:

    .. code-block:: bash

        cd agisostack-plus-plus

2. Create a build directory named :code:`build`:

    .. code-block:: bash

        cmake -S . -B build

3. Build the project:

    .. code-block:: bash

        cmake --build build

Selecting CAN Driver
--------------------

A default CAN driver plug-in will be selected for you based on your OS, but when compiling you can explicitly choose to use one of the natively supported CAN drivers by supplying the `CAN_DRIVER` variable.

- :code:`-DCAN_DRIVER=SocketCAN` for Socket CAN support (This is the default for Linux)
- :code:`-DCAN_DRIVER=WindowsPCANBasic` for the windows PEAK PCAN drivers (This is the default for Windows)
- :code:`-DCAN_DRIVER=MacCANPCAN` for the MacCAN PEAK PCAN driver (This is the default for Mac OS)
- :code:`-DCAN_DRIVER=TWAI` for the ESP TWAI driver (This is the preferred ESP32 driver)
- :code:`-DCAN_DRIVER=MCP2515` for the MCP2515 CAN controller
- :code:`-DCAN_DRIVER=WindowsInnoMakerUSB2CAN` for the InnoMaker USB2CAN adapter (Windows)
- :code:`-DCAN_DRIVER=TouCAN` for the Rusoku TouCAN (Windows)

Or specify multiple using a semicolon separated list: :code:`-DCAN_DRIVER="<driver1>;<driver2>"`

If your target hardware is not listed above, you can easily integrate your own hardware by `implementing a few simple functions <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/tree/main/hardware_integration#writing-a-new-can-driver-for-the-stack>`_.

Running Tests
-------------

Tests are run with GTest. They can be invoked through ctest. Once the library is compiled, navigate to the build directory to run tests:

.. code-block:: bash

    cmake -S . -B build -DBUILD_TESTING=ON
    cmake --build build
    cd build
    ctest

This will execute all the project tests and display the test results.

Running Examples
----------------

There are build-in examples in the project. By default, examples are not built.
The easiest way to build them is from the top level:

.. code-block:: bash

    cmake -S . -B build -DBUILD_EXAMPLES=ON
    cmake --build build
    cd build
    ./examples/<example_name>

Contributing
------------

We warmly welcome contributions to the project, and have a set of guidelines to help you get started: `CONTRIBUTING.md <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/CONTRIBUTING.md>`_
