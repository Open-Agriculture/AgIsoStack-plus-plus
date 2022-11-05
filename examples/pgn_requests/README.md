# PGN Request Handling Example

This example shows how you can use the CAN stack to simplify working with PGN requests (PGN 0xEA00) and PGN requests for repetition rate (PGN 0xCC00).

The example sets up an internal control function (ICF) to transmit as, tells the stack that it wants to accept PGN requests for that ICF, sets up handling of the PROPA PGN (0xEF00), and sends a requst of its own to the broadcast address.

For this example to work properly, you must have a valid, functional ISO 11783 CAN network connected as "can0".

## The Basics

This example will assume you have already learned the basics of the stack and ISOBUS communication, and won't go over the basic setup of your internal control function. If you need to brush up on those topics, check out the [transport layer example](https://github.com/ad3154/ISO11783-CAN-Stack/tree/main/examples/transport_layer) first, or visit our [tutorial website](https://delgrossoengineering.com/isobus-tutorial/index.html).

## PGN Requests

The Request message type, identified by the PGN 0xEA00 (59904) provides the ability to request information globally or from a specific destination.

Basically if someone requests a PGN from you using this request PGN, they are politely asking you to either send them a message with that PGN, or perform some action. You have a choice to either respond with the data they want, send the Acknowledgement PGN (this is called ACK or NACK depending on if the acknowledgement is positive or negative), or do nothing.

See ISO 11783-3 for a full description, but here are some general things to keep in mind regarding this message.

* If the request or applicable PGN is sent to the global address, then the response is sent to the global address. The stack will currently always send ACK/NACK responses to the broadcast address, but you can send any reponse you want from your application in response to these requests. Using the CAN stack's ACK/NACK functionality is completely optional.
* A NACK is not desired as a response to a global request. The CAN stack _will not send them_ in response to a global request.
* A NACK is required if the PGN is not supported. _It is highly recommended you assign an instance of the PGN request protocol to every internal control function you make if you wish to be in compliance with this rule._ As long as you tell the stack to handle these requests, it will NACK any unhandled request.

### Configuring PGN Request Handling

PGN requests and requests for repetition rate are optional features that the CAN stack can handle for you, similar to how the Diagnostic Protocol is an optional feature. If you want to enable this functionality for one of your internal control functions, all you need to do is tell the CAN stack to assign the PGN request protocol to that internal control function. Like this:

```
isobus::ParameterGroupNumberRequestProtocol::assign_pgn_request_protocol_to_internal_control_function(<your internal control function goes here>);
```

This will create an instance of the protocol, and the stack will begin handling requests. By default, it will NACK all requests until you explicitly handle a PGN. Then, requests that match that PGN will be forwarded to your application via a callback.

You can also pass in a callback that handles ALL PGNs sent to that internal control function if you want by using the meta PGN called `isobus::CANLibParameterGroupNumber::Any`.

### Receiving a PGN Request

The CAN stack can provide your application with a callback whenever it receives a PGN request destined for one of your application's internal control functions. This will allow you to transmit the requested data in your application. Or, you can tell the CAN stack from within your callback to ACK or NACK the request on your behalf. You can even take absolutely no action if you want to.

Here's an example callback that is meant to handle PROPA PGN (0xEF00) requests, and tells the CAN stack to send a positive acknowledgement back to the requestor.
```
bool example_proprietary_a_pgn_request_handler(std::uint32_t parameterGroupNumber,
                                               isobus::ControlFunction *,
                                               bool &acknowledge,
                                               isobus::AcknowledgementType &acknowledgeType,
                                               void *)
{
	bool retVal;

	// This function will be called whenever PGN EF00 is requested.
	// Add whatever logic you want execute to on reciept of a PROPA request.
	// One normal thing to do would be to send a CAN message with that PGN.

	// In this example though, we'll simply acknowledge the request.
	if (static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA) == parameterGroupNumber)
	{
        // These tell the CAN stack we want it to ACK the request on our behalf.
		acknowledge = true;
		acknowledgeType = isobus::AcknowledgementType::Positive;
		retVal = true;
	}
	else
	{
		// If any other PGN is passed-in, or we don't want to handle this callback for some reason return false.
		// Returning false will tell the stack to keep looking for another callback (if any exist) to handle this PGN.
        // Note that the CAN stack will not call this callback with a PGN that you weren't expecting unless you
        // mis-register the callback with the wrong PGN.
		retVal = false;
	}
	return retVal;
}
```

In order for this callback to be called at the appropriate times, you must register it with the CAN stack, like this:

```
// Get a pointer to the protocol instance
isobus::ParameterGroupNumberRequestProtocol *pgnRequestProtocol = isobus::ParameterGroupNumberRequestProtocol::get_pgn_request_protocol_by_internal_control_function(<your internal control function>);


pgnRequestProtocol->register_pgn_request_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), example_proprietary_a_pgn_request_handler, nullptr);
```

Here you can see we're asking the CAN stack for the protocol instance we created earlier, and we're telling the stack to call our callback whenever it receives a PGN request for PROPA.

### Sending a PGN Request

Sending a PGN request is extremely simple. The CAN stack will take care of message encoding for you. All you need to do is call the function `isobus::ParameterGroupNumberRequestProtocol::request_parameter_group_number`.

Here's what that looks like in the example:

```

isobus::ParameterGroupNumberRequestProtocol::request_parameter_group_number(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), TestInternalECU.get(), nullptr);

```

The three parameters are:

* The PGN to request
* The internal control function to send from
* The destination control function to send to, or `nullptr` if you want to send it to the broadcast address

This function will return `true` if the request was sent.

## Requests for Repetition Rate

This message, identified by PGN 0xCC00 (52224) allows the system to adapt the bus bandwidth to the needs of the user of the message. Essentially, it's a way for a control function to ask another control function politely to send it a specific PGN at a desired rate. This includes a default rate that can be requested with a value of 0x0000. If it is possible for the source of the message with the requested PGN to deliver the message with the desired repetition rate, it should honour the request.

Some important notes about this message:

* Control functions are not required to monitor the bus for this message.
* If another control function cannot or does not want to use the requested repetition rate, which is necessary for systems with fixed timing control loops, it may ignore this message. As such _the CAN stack will not NACK unhandled requests for this PGN, as it is not required._
* If no response for repetition rate has been received, the requester shall assume that the request was not accepted. It is up to your application for how you want to deal with this.

### Configuring Requests for Repetition Rate Handling

Configuring the handling of this message is the same as configuring handling of PGN requests. If you've assigned the PGN request protocol an internal control function, requests for repetition rate will also be handled.

### Receiving Requests for Repetition Rate

The CAN stack can provide your application with a callback whenever it receives a request for repetition rate destined for one of your application's internal control functions. This will allow you to transmit the requested data in your application.

Here's an example callback that is meant to handle PROPA PGN (0xEF00) requests for repetition rate:

```
bool example_proprietary_a_request_for_repetition_rate_handler(std::uint32_t parameterGroupNumber,
                                                               isobus::ControlFunction *requestingControlFunction,
                                                               std::uint32_t repetitionRate,
                                                               void *)
{
	bool retVal;

	if (static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA) == parameterGroupNumber)
	{
		retVal = true;

		// Put whatever logic you want to in here so that you can begin to handle the request.
		// The CAN stack provides this easy way to receive requests for repetition rate, but
		// your application must handle the actual processing and sending of those messages at the requested rate
		// since the stack has no idea what your application actually does with most PGNs.

		// In this example, I'll handle it by saving the repetition rate in a global variable and have 
		// main() service it at the desired rate.
		repetitionRateRequestor = requestingControlFunction;
		propARepetitionRate_ms = repetitionRate;
	}
	else
	{
		// If any other PGN is requested, since this callback doesn't handle it, return false.
		// Returning false will tell the stack to keep looking for another callback (if any exist) to handle this PGN
		retVal = false;
	}
	return retVal;
}
```

In order for this callback to be called at the appropriate times, you must register it with the CAN stack, like this:

```
// Get a pointer to the protocol instance
isobus::ParameterGroupNumberRequestProtocol *pgnRequestProtocol = isobus::ParameterGroupNumberRequestProtocol::get_pgn_request_protocol_by_internal_control_function(<your internal control function>);

// Now we'll set up a callback to handle requests for repetition rate for the PROPA PGN
pgnRequestProtocol->register_request_for_repetition_rate_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), example_proprietary_a_request_for_repetition_rate_handler, nullptr);
```

Here you can see we're asking the CAN stack for the protocol instance we created earlier, and we're telling the stack to call our callback whenever it receives a request for repetition rate specifically for the PROPA PGN.

### Sending a Request for Repetition Rate

Sending a request for repetition rate is as simple as it was for a PGN request. The CAN stack will take care of message encoding for you. All you need to do is call the function `isobus::ParameterGroupNumberRequestProtocol::request_repetition_rate`.

The use of this was omitted in the example, as it does not make sense to request a repetition rate from the broadcast address, and I do not know what devices you might have on your bus, so I suggest you check out the doxygen for this function so you are familliar with its parameters and usage.

That's all for this example! You should now be able to easily deal with PGN requests and requests for repetition rate in your application.
