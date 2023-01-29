# Transport Layer Example

This example shows some very basic usage of the CAN stack main transport layer. Specifically, it shows how to transmit CAN messages of various lengths.

The example sets up an internal control function to transmit as, and a partner control function to receive messages (A virtual terminal in this case).

For this example to work properly, you must have a valid, functional ISO 11783 CAN network connected as "can0", with a virtual terminal of any version online.

If you do not have a virtual terminal, you will need to change the example partners NAME filters to match a device that you do have on the bus, is online, and conforms to ISO 11783.

## The Basics (Defining your control functions)

### Who are you? 

Every device that communicates on a J1939 or ISO 11783 network is called a _control function_. 

Every control function on the bus must have two things in order to communicate. You must have a NAME, and an address.

A NAME describes what your control function does, and what its _function_ is, who _manufactured_ it, and other identity information.

When you provide a NAME to the CAN stack, it will use that information to claim an _address_.

The way that you do this is by creating an `InternalControlFunction`

This is how the example creates an internal control function:
```
static std::shared_ptr<isobus::InternalControlFunction> TestInternalECU = nullptr;

isobus::NAME TestDeviceNAME(0);

TestDeviceNAME.set_arbitrary_address_capable(true);
TestDeviceNAME.set_industry_group(1);
TestDeviceNAME.set_device_class(0);
TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
TestDeviceNAME.set_identity_number(2);
TestDeviceNAME.set_ecu_instance(0);
TestDeviceNAME.set_function_instance(0);
TestDeviceNAME.set_device_class_instance(0);
TestDeviceNAME.set_manufacturer_code(64);
TestInternalECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x1C, 0);
```

As a note, a shared pointer is not required. A raw pointer, or even a concrete object would also work fine.

### Who are you talking to?

There are lots of devices you might want to communicate with on a CAN bus. They are all identified by the components that make up their NAME.

You tell the CAN stack what device you want to talk to by creating a `PartnerControlFunction` and by passing into it a `NAMEFilter`, which tells the stack what kind of NAME to match with that control function.

The example sets up a partner, and filters based on function code. Specifically, the example tells the CAN stack that the partner it's looking for is a `Virtual Terminal`.

Here is how the example sets up the NAME Filter:

```
std::vector<isobus::NAMEFilter> vtNameFilters;

const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));

vtNameFilters.push_back(testFilter);

```

Then, it uses that filter to create a partner:

```
static isobus::PartneredControlFunction* TestPartner = nullptr;
TestPartner = new isobus::PartneredControlFunction(0, vtNameFilters);
```

## Getting connected

The CAN stack comes with a hardware abstraction that allows the stack to be unaware of the underlying hardware.

Right now, it only works with Socket CAN.

To get the CAN stack talking to Socket CAN, the example provides some boilerplate code. You will need this code any time you want to use a Socket CAN device.

Implementing your own hardware layer will be covered by a different example.

Here is how the example sets up Socket CAN:

```
void update_CAN_network()
{
	isobus::CANNetworkManager::CANNetwork.update();
}

void raw_can_glue(isobus::HardwareInterfaceCANFrame &rawFrame, void *parentPointer)
{
	isobus::CANNetworkManager::CANNetwork.can_lib_process_rx_message(rawFrame, parentPointer);
}

std::shared_ptr<SocketCANInterface> canDriver = std::make_shared<SocketCANInterface>("can0");
CANHardwareInterface::set_number_of_can_channels(1);
CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);
CANHardwareInterface::start();

CANHardwareInterface::add_can_lib_update_callback(update_CAN_network, nullptr);
CANHardwareInterface::add_raw_can_message_rx_callback(raw_can_glue, nullptr);
```

When you want to disconnect from the socket (such as when you exit the program), you must tell the hardware abstraction to do so:

```
CANHardwareInterface::stop();
```

## Sending a basic CAN message

To send a basic CAN message, you need to first create a payload. This is most commonly an array. Then, just call the generic `send_can_message` function located on the public interface of the network manager. `isobus::CANNetworkManager::CANNetwork.send_can_message`

Like this:
```
	std::uint8_t buffer[CAN_DATA_LENGTH] = { 0 };
	CANNetworkManager::CANNetwork.send_can_message(PGN, buffer, CAN_DATA_LENGTH, source, partner);
```

Calling `send_can_message` with no partner (`nullptr`) will send it to the global address, `0xFF`.

The `send_can_message` also has other optional parameters, like priority. See the full function signature or the doxygen for more information.

## Transport Protocol (Connection Mode)

If you attempt to send a CAN message to a specific partner, and the length is between 9 and 1785 bytes, the CAN stack will select the TP.CM protocol to send it. This is automatic.

The example attempts to send many of these messages, one for each possible data length 9 - 1785.

```
// CM Tx Example
// This loop sends all possible TP CM message sizes.
// This will take a long time
for (std::uint32_t i = 9; i <= MAX_TP_SIZE_BYTES; i++)
{
    // Send message
    if (isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, TPTestBuffer, i, TestInternalECU.get(), TestPartner))
    {
        cout << "Started TP CM Session with length " << i << endl;
    }
    else
    {
        cout << "Failed starting TP CM Session with length " << i << endl;
    }
    // Wait for this session to complete before starting the next
    // This sleep value is arbitrary
    std::this_thread::sleep_for(std::chrono::milliseconds(i * 2));
}
```

## Transport Protocol (Broadcast/BAM)

If you attempt to send a CAN message to nobody (`nullptr`, or if you just omit the destination), the CAN stack will select the TP.BAM protocol to send your message. This is automatic.

BAM should be avoided where possible, as it has manditory delays between sending messages, and can only have 1 session active at a time per internal control function.

The example attempts to send many of these messages, one for each possible data length 9 - 1785.

```
// BAM Tx Exmaple
// This loop sends all possible BAM message sizes
// This will take a very long time
for (std::uint32_t i = 9; i <= MAX_TP_SIZE_BYTES; i++)
{
    // Send message
    if (isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, TPTestBuffer, i, TestInternalECU.get()))
    {
        cout << "Started BAM Session with length " << i << endl;
    }
    else
    {
        cout << "Failed starting BAM Session with length " << i << endl;
    }
    // Wait for this session to complete before starting the next, or it will fail as only 1 BAM session is possible at a time
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * (isobus::CANNetworkConfiguration::get_minimum_time_between_transport_protocol_bam_frames() * ((i + 1) / 7))));
}
```

## Extended Transport Protocol (ETP)

If you attempt to send a CAN message to a specific partner, and the length is greater than 1785 bytes, and less than 117440505 bytes, the CAN stack will select the ETP protocol to send your message. This is automatic. ETP does not support sending to the global address.

The example attempts to send one ETP message:

```
// ETP Example
// Send one ETP message
if (isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, ETPTestBuffer, ETP_TEST_SIZE, TestInternalECU.get(), TestPartner))
{
    cout << "Started ETP Session with length " << ETP_TEST_SIZE << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}
```
