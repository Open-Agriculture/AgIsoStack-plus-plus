# ISO 11783 File Server Client Example

This example shows how you can use the CAN stack to communicate with an ISOBUS file server.

The example sets up an internal control function (ICF) to transmit as, sets up a partner representing a file server, creates an instance of the File Server Client class, and performs some basic file server operations.

For this example to work properly, you must have a valid, functional ISO 11783 CAN network connected as "can0" with a file server compliant with the ISO 11783-13 standard version 3.

* Known Issue: This may not with with a John Deere file server. In the past, the Deere file servers I have interacted with do not implement their file servers in a way that is consistent with other manufacturers, and I do not have a Deere display to test with currently so I am unable to troubleshoot it. This may be fixed in the future iteration of the stack if I can acquire a Deere display or someone with a Deere display can help troubleshoot it.

## The Basics

This example will assume you have already learned the basics of the stack and ISOBUS communication, and won't go over the basic setup of your internal control function. If you need to brush up on those topics, check out the [transport layer example](https://github.com/ad3154/ISO11783-CAN-Stack/tree/main/examples/transport_layer) first, or visit our [tutorial website](https://delgrossoengineering.com/isobus-tutorial/index.html).

