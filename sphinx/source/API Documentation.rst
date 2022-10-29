.. _API Documentation:

API Documentation
==================

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

This project uses Doxygen to automatically generate up-to-date API docs.

Precompiled Doxygen
-------------------

You can view the precompiled, latest Doxygen here: https://delgrossoengineering.com/isobus-docs/index.html

Compiling the Doxygen locally
-----------------------------

You can also generate the Doxygen yourself and browse it locally.

From your project, change directory into the 'ISO11783-CAN-Stack' folder.

.. code-block:: bash

   cd ISO11783-CAN-Stack

Make sure you have Doxygen installed.

Ubuntu:

.. code-block:: bash

   sudo apt install doxygen graphviz

RHEL:

.. code-block:: bash

   sudo subscription-manager repos --enable codeready-builder-for-rhel-9-$(arch)-rpms

   sudo dnf install doxygen graphviz


Then, generate the docs.

.. code-block:: bash

   doxygen

The documentation will appear in the docs/html folder. Open index.html in a web browser to start browsing the docs!
