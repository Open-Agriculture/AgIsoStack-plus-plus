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

AUX-N Functions are disabled by default. Enable them explicitly with
``set_auxiliary_functions_enabled(true)`` before calling ``VirtualTerminalClient::initialize()``.
Registering assignment persistence callbacks does not enable Functions. AUX-N Inputs are enabled
independently by registering input Object IDs with ``add_auxiliary_input_object_id()``.
A Working Set can provide Inputs, Functions, or both.
For AUX-N Functions, configure the partnered VT with a Function Instance NAME filter of 0.
Assignment operations use this primary VT, where the Working Set's object pool is loaded.

The application must supply a correct object pool and valid function and input Object IDs.
The client does not inspect the pool for AUX-N capabilities. Configure the model identification
code and any persistence callbacks before initialization. Preferred Assignment synchronization
runs independently of normal VT initialization and does not delay the Connected state.
Inputs report Initializing until the VT confirms pool loading, then Ready. Assignment failure
is reported through the auxiliary assignment failure event dispatcher and does not disconnect the VT.
Ready devices load preferred assignments during ``update()``. If a preferred assignment command
arrives before loading completes, the client loads preferences synchronously before modifying them.
Load and store callbacks are serialized and run without the device-state mutex. They must be fast
and bounded so callback execution and contention permit assignment responses within one second
and maintenance every 100 ms. Applications using slow storage should enqueue storage work
asynchronously, and applications supplying their own update loop must call it frequently enough
to meet the protocol deadlines. Callbacks must not reenter AUX-N preference operations.

Accepted preferred assignments are persisted independently of response delivery. Store callbacks
are deferred to ``update()`` and all pending accepted changes are flushed at ``terminate()``.
Assignment responses use an eight-entry FIFO and are retried in receive order when queuing fails.
If the FIFO is full and cannot advance, the newest command is refused before application and an
error is logged. Missed response deadlines and undelivered responses at session termination are
also logged; these diagnostics do not make late responses compliant.

Preferred Assignment retransmissions use an immutable snapshot. Device loss or model changes
mark affected snapshots obsolete, without sending an overlapping replacement. A valid response,
including rejection, completes the transaction. An obsolete snapshot is discarded after its
response timeout instead of retransmitted. After three unanswered transmissions, failure is
reported and synchronization continues only when another change is pending or a new trigger
occurs. There is no permanent suspension. Automatic timeout recovery cannot distinguish a late
response to an old command from a response to its replacement, because responses have no
transaction identifier.

The lifecycle is:

.. code-block:: text

   Configure client
       ↓
   set AUX-N settings/callbacks
       ↓
   initialize()
       ↓
   runtime AUX-N operation
       ↓
   terminate()
