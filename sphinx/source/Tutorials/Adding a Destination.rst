.. _AddingADestination:

Adding A Destination
=====================

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

This tutorial assumes you've already completed the :doc:`ISOBUS Hello World Tutorial<./The ISOBUS Hello World>` and picks up where that tutorial left off. If you haven't done that tutorial yet, consider going back and finishing that one first.

In The ISOBUS Hello World, we learned how to set up the CAN stack and send a simple message to the broadcast address. But, what if you don't want to send to the global address? Let's say we want to talk specifically to a virtual terminal (VT).

This is where the concept of a `"PartneredControlFunction" <https://delgrossoengineering.com/isobus-docs/classisobus_1_1PartneredControlFunction.html>`_ comes in.

Let's add a VT partner to our example program.

NAME Filters
^^^^^^^^^^^^^

The first thing we need to do is construct a filter that will tell the CAN stack we only care about a VT. This is called a `"NAME filter" <https://delgrossoengineering.com/isobus-docs/classisobus_1_1NAMEFilter.html>`_.

When we create a `"PartneredControlFunction" <https://delgrossoengineering.com/isobus-docs/classisobus_1_1PartneredControlFunction.html>`_, we must supply a `"NAME filter" <https://delgrossoengineering.com/isobus-docs/classisobus_1_1NAMEFilter.html>`_ along with it so that the stack knows what kind of device you want to talk to. 
You can be as specific, or a general as you want. Adding multiple filter values with the same key, like "function" or "manufacturer code" will cause the stack to match against *either* filter.

Let's make our filter:

.. code-block:: c++

   std::vector<isobus::NAMEFilter> myPartnerFilter;

   const isobus::NAMEFilter virtualTerminalFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));

   myPartnerFilter.push_back(virtualTerminalFilter);

Here we've created a vector of filters, and one filter itself. In this case, we want to filter for an ECU whose function code matches a virtual terminal's function code.
Then, we added the filter to the list of filters.

Now, let's create our partner.

Creating a Partner
^^^^^^^^^^^^^^^^^^^

