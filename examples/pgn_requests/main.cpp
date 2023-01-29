#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/socket_can_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_configuration.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"

#include <csignal>
#include <iostream>
#include <memory>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::uint32_t propARepetitionRate_ms = 0xFFFFFFFF;
static isobus::ControlFunction *repetitionRateRequestor = nullptr;

using namespace std;

void signal_handler(int)
{
	CANHardwareInterface::stop();
	_exit(EXIT_FAILURE);
}

void update_CAN_network()
{
	isobus::CANNetworkManager::CANNetwork.update();
}

void raw_can_glue(isobus::HardwareInterfaceCANFrame &rawFrame, void *parentPointer)
{
	isobus::CANNetworkManager::CANNetwork.can_lib_process_rx_message(rawFrame, parentPointer);
}

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
		acknowledge = true;
		acknowledgeType = isobus::AcknowledgementType::Positive;
		retVal = true;
	}
	else
	{
		// If any other PGN is requested, since this callback doesn't handle it, return false.
		// Returning false will tell the stack to keep looking for another callback (if any exist) to handle this PGN
		retVal = false;
	}
	return retVal;
}

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

int main()
{
	std::signal(SIGINT, signal_handler);

	std::shared_ptr<CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	canDriver = std::make_shared<SocketCANInterface>("can0");
#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
	canDriver = std::make_shared<PCANBasicWindowsPlugin>(PCAN_USBBUS1);
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
	canDriver = std::make_shared<InnoMakerUSB2CANWindowsPlugin>(0); // CAN0
#endif
	if (nullptr == canDriver)
	{
		std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
		std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
		return -1;
	}

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid" << std::endl;
		return -2;
	}

	CANHardwareInterface::add_can_lib_update_callback(update_CAN_network, nullptr);
	CANHardwareInterface::add_raw_can_message_rx_callback(raw_can_glue, nullptr);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);

	//! Make sure you change these for your device!!!!
	//! This is an example device that is using a manufacturer code that is currently unused at time of writing
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	std::shared_ptr<isobus::InternalControlFunction> TestInternalECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x1C, 0);
	std::signal(SIGINT, signal_handler);

	// Wait to make sure address claiming is done. The time is arbitrary.
	//! @todo Check this instead of asuming it is done
	std::this_thread::sleep_for(std::chrono::milliseconds(1250));

	// Tell the CAN stack that we want to respond to PGN requests that are sent to our internal control function
	isobus::ParameterGroupNumberRequestProtocol::assign_pgn_request_protocol_to_internal_control_function(TestInternalECU);

	// Get a pointer to the protocol instance we just assigned
	isobus::ParameterGroupNumberRequestProtocol *pgnRequestProtocol = isobus::ParameterGroupNumberRequestProtocol::get_pgn_request_protocol_by_internal_control_function(TestInternalECU);

	// Register a callback to handle PROPA PGN Requests
	if (nullptr != pgnRequestProtocol)
	{
		pgnRequestProtocol->register_pgn_request_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), example_proprietary_a_pgn_request_handler, nullptr);

		// Now, if you send a PGN request for EF00 to our internal control function, the stack will acknowledge it. Other requests will be NACK'ed (negative acknowledged)
		// NOTE the device you send from MUST have address claimed.

		// Now we'll set up a callback to handle requests for repetition rate for the PROPA PGN
		pgnRequestProtocol->register_request_for_repetition_rate_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), example_proprietary_a_request_for_repetition_rate_handler, nullptr);

		// Now we'll get a callback when someone requests a repetition rate for PROPA.
		// The application (not the stack) must handle these requests, as the CAN stack does not know what data to send when responding.
		// It's entirely application defined!
		// So we'll handle that below in the `while(true)` loop as an example.
		// You do not need to handle every PGN. Only ones you care about. ISOBUS allows you to ignore any and all requests for repetition rate if you want with no reponse needed.

		// This is how you would request a PGN from someone else. In this example, we request it from the broadcast address.
		// Generally you'd want to replace nullptr with your partner control function as its a little nicer than just asking everyone on the bus for a PGN
		isobus::ParameterGroupNumberRequestProtocol::request_parameter_group_number(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), TestInternalECU.get(), nullptr);
	}

	while (true)
	{
		if (0xFFFFFFFF != propARepetitionRate_ms)
		{
			// If someone has requested a repetition rate for PROPA, service it here (in the application layer)
			std::uint8_t buffer[isobus::CAN_DATA_LENGTH] = { 0 };
			isobus::CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), buffer, isobus::CAN_DATA_LENGTH, TestInternalECU.get(), repetitionRateRequestor);
			std::this_thread::sleep_for(std::chrono::milliseconds(propARepetitionRate_ms));
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Do nothing. Wait time is arbitrary.
		}
	}

	CANHardwareInterface::stop();

	return 0;
}
