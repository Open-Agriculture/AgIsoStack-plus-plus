.. _PGNRequests:

PGN Requests
=============

.. toctree::
   :hidden:
   :glob:

.. contents:: Contents
   :depth: 2
   :local:

This tutorial covers the basics on how to use the library to simplify handling of PGN requests and PGN requests for repetition rate.

It is assumed you have completed the basic tutorials, as this tutorial covers application level use of the CAN stack.

.. note::

    A complete example program for handling PGN requests is `located here. <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/examples/pgn_requests/main.cpp>`_

What is a PGN request?
-----------------------

Basically if someone requests a PGN from you, they are politely asking you to either send them a message with that PGN, or perform some action. .
You have a choice to either respond with the data they want, send the Acknowledgement PGN (this is called ACK or NACK depending on if the acknowledgement is positive or negative), or do nothing.

Using the Parameter Group Number Request Protocol
--------------------------------------------------

Some of the main interactions on an ISOBUS are to request a PGN from another control function, respond to requests from other control functions, send requests for a control function to send a PGN on a certain interval, and to properly respond to those requests as well.

.. note::

    The PGN used to request another PGN is 59904 (0xEA00) and the PGN used to request a repetition rate is 52224 (0xCC00). These are destination specific PGNs, so if you do a candump and look for them on the bus, the 00's will be replaced by the destination address.

This library provides a class that makes handling these requests simpler than handling it manually for every PGN.

Configuring
^^^^^^^^^^^^

Before you can use the library to send and receive PGN requests, you'll need to have an Internal Control Function and assign the protocol to it. 
This tells the library that you want to process PGN requests on a particular CAN channel, and tells the library what control function to use when responding to requests on the bus that are sent to you.

.. code-block:: c++

    #include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"

    isobus::ParameterGroupNumberRequestProtocol::assign_pgn_request_protocol_to_internal_control_function(<your internal control function goes here>);

This may create an instance of the protocol if needed, and the library will begin handling requests on your behalf. By default, it will NACK all requests until you explicitly handle a PGN. Then, requests that match that PGN will be forwarded to your application via a callback.

Sending a PGN Request
^^^^^^^^^^^^^^^^^^^^^^

To send a PGN request, after you have assigned an internal control function, simply call :code:`isobus::ParameterGroupNumberRequestProtocol::request_parameter_group_number` and pass in the PGN you want to request, and a control function to request it from.

Passing in :code:`nullptr` will request it as a broadcast from all CFs. This is generally not a great idea to do, but is allowed.

Receiving a PGN Request
^^^^^^^^^^^^^^^^^^^^^^^^

The CAN stack can provide your application with a callback whenever it receives a PGN request destined for one of your application's internal control functions. 
This will allow you to transmit the requested data in your application. 
Or, you can tell the CAN stack from within your callback to ACK or NACK the request on your behalf. You can even take absolutely no action if you want to.

To do this, create a function that matches the type :code:`PGNRequestCallback`.

.. code-block:: c++

    bool example_proprietary_a_pgn_request_handler(std::uint32_t parameterGroupNumber,
                                               isobus::std::shared_ptr<ControlFunction> ,
                                               bool &acknowledge,
                                               isobus::AcknowledgementType &acknowledgeType,
                                               void *)
    {
    }

Then, register this function with the protocol. This tells the library that you want it call this function when the specified PGN is requested. In this case, we'll specify the ProprietaryA (PROPA) PGN.

.. code-block:: c++

    // Get a pointer to the protocol instance
    isobus::ParameterGroupNumberRequestProtocol *pgnRequestProtocol = isobus::ParameterGroupNumberRequestProtocol::get_pgn_request_protocol_by_internal_control_function(<your internal control function>);


    pgnRequestProtocol->register_pgn_request_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), example_proprietary_a_pgn_request_handler, nullptr);

Sending a Request for Repetition Rate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Sending a request for repetition rate tells another CF that they should send you the specified PGN at some rate that you desire.
You can also leave the exact rate at the discretion of the destination CF by asking for a rate of 0.

.. warning::

    Some important notes about this message:

    * Control functions are not required to monitor the bus for this message.
    * If another control function cannot or does not want to use the requested repetition rate, which is necessary for systems with fixed timing control loops, it may ignore this message.
    * If no response for repetition rate has been received, the requester shall assume that the request was not accepted. It is up to your application for how you want to deal with this.

To send a request for repetition rate, simply call :code:`isobus::ParameterGroupNumberRequestProtocol::request_repetition_rate` and specify the PGN you want to request, and which CF to make the request to.

Receiving Requests for Repetition Rate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This library can provide your application with a callback whenever it receives a request for repetition rate destined for one of your application's internal control functions. This will allow you to transmit the requested data in your application.

To do this, create a function that matches the type :code:`PGNRequestForRepetitionRateCallback`.

.. code-block:: c++

    bool example_proprietary_a_request_for_repetition_rate_handler(std::uint32_t parameterGroupNumber,
                                                               isobus::std::shared_ptr<ControlFunction> requestingControlFunction,
                                                               std::uint32_t repetitionRate,
                                                               void *)
    {
    }

Then, register your callback with the protocol, and you will begin to receive callbacks when that PGN is requested from you.

.. code-block:: c++

    // Get a pointer to the protocol instance
    isobus::ParameterGroupNumberRequestProtocol *pgnRequestProtocol = isobus::ParameterGroupNumberRequestProtocol::get_pgn_request_protocol_by_internal_control_function(<your internal control function>);

    // Now we'll set up a callback to handle requests for repetition rate for the PROPA PGN in this example
    pgnRequestProtocol->register_request_for_repetition_rate_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), example_proprietary_a_request_for_repetition_rate_handler, nullptr);

Above, you can see we're asking the CAN stack for the protocol instance we created earlier, and we're telling the stack to call our callback whenever it receives a request for repetition rate specifically for the PROPA PGN in this case.

Check out the full example here for more on what you can put inside that callback to tell the stack how to respond to the request: https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/examples/pgn_requests/main.cpp

.. note::

    Here are some additional things to keep in mind when dealing with PGN requests.

    * If the request or applicable PGN is sent to the global address, then the response is sent to the global address. This library will currently always send ACK/NACK responses to the broadcast address, but you can send any response you want from your application in response to these requests. Using the CAN stack's ACK/NACK functionality is completely optional.
    * A NACK is not desired as a response to a global request. The CAN stack will not send them in response to a global request.
    * A NACK is required if the PGN is not supported. It is highly recommended you assign an instance of the PGN request protocol to every internal control function you make if you wish to be in compliance with this rule. As long as you tell the stack to handle these requests, it will NACK any unhandled request.
    * You can pass in a callback to this protocol that handles ALL PGNs sent to that internal control function if you want by using the meta PGN called :code:`isobus::CANLibParameterGroupNumber::Any`
