Task Controller API
===================

The Task Controller (TC) is responsible for recording and planning data. It interfaces with operators
to schedule tasks for the implement to execute, offering automated precision 
that is crucial for tasks such as precision farming.

.. note:: 
   
   A :ref:`Control Function <API ControlFunction>` registered as a Task Controller, but specifically defined 
   to perform data logging functionality, is often referred to as a Data Logger (DL).

Like the :ref:`Virtual Terminal <API VirtualTerminal>`, the Task Controller consists of both a client and a server. 
The client is the one that executes the tasks that are scheduled by the server.

.. toctree::
   :maxdepth: 1

   client
   server