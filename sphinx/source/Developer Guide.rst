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

See :ref:`Choosing a CAN Driver<choosing-a-can-driver>` in the API section for more details.

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

.. _doxygen:

Doxygen
-------

This project uses Doxygen to automatically generate up-to-date internal API docs.

Precompiled
^^^^^^^^^^^

You can view the precompiled, latest Doxygen here: https://delgrossoengineering.com/isobus-docs/index.html

Compiling locally
^^^^^^^^^^^^^^^^^

You can also generate the Doxygen yourself and browse it locally.

From your project, change directory into the 'AgIsoStack-plus-plus' folder.

.. code-block:: bash

   cd AgIsoStack-plus-plus

Make sure you have Doxygen installed.

Ubuntu:

.. code-block:: bash

   sudo apt install doxygen graphviz

RHEL:

.. code-block:: bash

   sudo subscription-manager repos --enable codeready-builder-for-rhel-9-$(arch)-rpms

   sudo dnf install doxygen graphviz

Windows:

Make sure you have doxygen installed: https://www.doxygen.nl/download.html


Then, generate the docs.

.. code-block:: bash

   doxygen doxyfile

The documentation will appear in the docs/html folder. Open index.html in a web browser to start browsing the docs!


Contributing
------------

We warmly welcome contributions to the project, and have a set of guidelines to help you get started: `CONTRIBUTING.md <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/CONTRIBUTING.md>`_
