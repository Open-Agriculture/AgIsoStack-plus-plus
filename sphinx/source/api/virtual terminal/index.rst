.. _API VirtualTerminal:

Virtual Terminal API
====================

The Virtual Terminal (VT) allows the operator to control different implements via a single terminal.
The VT consists of two parts: the client and the server.

.. note::
   
   The VT is sometimes referred to as an Universal Terminal (UT), these two terms are interchangeable.

The client is the application that determines what content is displayed. The server is the application that display the client's content.
Because the client and server are separate applications, they are almost always run on separate devices. 
The client usually runs on the implement, and the server runs on the tractor.

.. toctree:: 
   :maxdepth: 1

   client
   server
   objects

Auxiliary Control API
^^^^^^^^^^^^^^^^^^^^^

Auxiliary Control (AUX) allows the operator to control an implement through physical inputs.

.. note::

   There exists both a new (AUX-N) and an old (AUX-O) version of the auxiliary control protocol.
   These two versions are **not** cross-compatible.

There are two parts to the Auxiliary Control:

* Inputs: The individual buttons, switches, and knobs that the operator can use to control the implement.
* Functions: The functions that the implement can perform, like raising or lowering.

These two parts are connected by a mapping. The mapping determines which inputs control which functions.
This mapping can be done by the operator on the Virtual Terminal, and is stored in the implement.

Both the Inputs and Functions are given by a :ref:`Virtual Terminal Client <API VirtualTerminalClient>` with AuxiliaryInputObjects and AuxiliaryOutputObjects.

