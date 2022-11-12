## CAN Hardware Integration

This folder is where the boundary between the CAN stack reaches the actual hardware.

The data flow for the CAN stack looks like this:

```mermaid
flowchart LR;
	layer1(CAN Stack) --> layer2(Abstraction Boundary)
	layer2(Abstraction Boundary) --> layer3(CAN Hardware Interface)
	layer3(CAN Hardware Interface) --> layer4(Abstract CAN Hardware Plugin Interface)
	layer4(Abstract CAN Hardware Plugin Interface) --> layer5(Your CAN Driver)
	layer5(Your CAN Driver) --> layer6(Physical CAN Bus)
```

Let's discuss how these components work. Then, we'll go over how you can write your own CAN driver to easily integrate with the stack.

### The Abstraction Boundary

The CAN stack relies on a couple of functions being defined externally to function. This boundary keeps the core stack completely isolated from the hardware layer. Thus, the content of the `isobus` folder can function entirely on its own if that meets your needs better than using the built in hardware interface layer.
These functions are:

* `bool send_can_message_to_hardware(HardwareInterfaceCANFrame frame);`
	- This is how the CAN stack will send a frame to the actual hardware.
	- This is already defined in `CANHardwareInterface` and wraps the actual CAN driver calls.
	- `CANHardwareInterface` Stores these frames in a queue that will be fed to your underlying CAN driver.
	- `CANHardwareInterface` Will call `bool write_frame(const isobus::HardwareInterfaceCANFrame &canFrame)` in `CANHardwarePlugin` from the internal queuing mechanism, which is when the frame officially goes to your CAN driver.
	
* `void raw_can_glue(isobus::HardwareInterfaceCANFrame &rawFrame, void *parentPointer)`
	- This function is how the CAN stack will receive frames
	- The name is arbitrary. You can name it whatever you want as long as the signature is the same and it calls `isobus::CANNetworkManager::CANNetwork.can_lib_process_rx_message(rawFrame, parentPointer);` inside it.
	- The `CANHardwareInterface` will take care of calling this. You just need to tell it what function to call, like this `CANHardwareInterface::add_raw_can_message_rx_callback(raw_can_glue, nullptr);`

* `void update_CAN_network()`
	- You need some void function like this that calls `isobus::CANNetworkManager::CANNetwork.update();` periodically.
	- The `CANHardwareInterface` provides this periodic update. You just need to add your function, like this `CANHardwareInterface::add_can_lib_update_callback(update_CAN_network, nullptr);`

### The CANHardwareInterface

The `CANHardwareInterface` class was created to provide a common queuing and thread layer for running the CAN stack and all CAN drivers to simplify integration and crucially to provide a consistent, safe order of operations for all the function calls needed to properly drive the stack.

For example, although receiving and sending CAN messages is multi-threaded, the stack avoids a lot of complexity by making the consumption of the receive message queue and update of protocols effectively single threaded so no expensive mutexing is needed in the core of the stack.

The `CANHardwareInterface` also is designed to deal with the generic base class for a CAN driver, called `CANHardwarePlugin`, so that it too is completely hardware agnostic.

### The CANHardwarePlugin

This class is a generic base class for a CAN driver. It's meant to provide a common interface for `CANHardwareInterface` to send and receive messages for the stack.

This is the class that you need to implement if you want to integrate CAN hardware the stack doesn't support (yet). 

Consider making a PR back to the stack with your CAN driver if you write one!

### The Actual CAN Driver

This is where the platform specific code should go. Then, just use CMake to compile the appropriate one!

## Writing A New CAN Driver for the Stack

If the CAN stack doesn't support your target platform, you can add support! Adding a new CAN driver helps the entire community.

A CAN driver just needs to inherit from `CANHardwarePlugin` and implement all the functions defined in it (don't worry, there's only 5).

Make sure you follow the repo's contribution standards and code of conduct, then open a PR for it!
