#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/socket_can_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_diagnostic_protocol.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <iterator>
#include <memory>

static std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
}

int main()
{
	std::signal(SIGINT, signal_handler);

	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::SocketCANInterface>("can0");
#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
	canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(PCAN_USBBUS1);
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
	canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0); // CAN0
#elif defined(ISOBUS_MACCANPCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::MacCANPCANPlugin>(PCAN_USBBUS1);
#endif
	if (nullptr == canDriver)
	{
		std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
		std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
		return -1;
	}

	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
		return -2;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);

	//! Make sure you change these for your device!!!!
	//! This is an example device that is using a manufacturer code that is currently unused at time of writing
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(0);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	auto TestInternalECU = isobus::InternalControlFunction::create(TestDeviceNAME, 0x1C, 0);

	// Wait to make sure address claiming is done. The time is arbitrary.
	//! @todo Check this instead of asuming it is done
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	isobus::DiagnosticProtocol::assign_diagnostic_protocol_to_internal_control_function(TestInternalECU);

	isobus::DiagnosticProtocol *diagnosticProtocol = isobus::DiagnosticProtocol::get_diagnostic_protocol_by_internal_control_function(TestInternalECU);

	// Make a few test DTCs
	isobus::DiagnosticProtocol::DiagnosticTroubleCode testDTC1(1234, isobus::DiagnosticProtocol::FailureModeIdentifier::ConditionExists, isobus::DiagnosticProtocol::LampStatus::None);
	isobus::DiagnosticProtocol::DiagnosticTroubleCode testDTC2(567, isobus::DiagnosticProtocol::FailureModeIdentifier::DataErratic, isobus::DiagnosticProtocol::LampStatus::AmberWarningLampSlowFlash);
	isobus::DiagnosticProtocol::DiagnosticTroubleCode testDTC3(8910, isobus::DiagnosticProtocol::FailureModeIdentifier::BadIntellegentDevice, isobus::DiagnosticProtocol::LampStatus::RedStopLampSolid);

	if (nullptr != diagnosticProtocol)
	{
		// Set a product identification string (in case someone requests it)
		diagnosticProtocol->set_product_identification_code("1234567890ABC");
		diagnosticProtocol->set_product_identification_brand("Open-Agriculture");
		diagnosticProtocol->set_product_identification_model("AgIsoStack++ CAN Stack DP Example");

		// Set a software ID string (This is what tells other ECUs what version your software is)
		diagnosticProtocol->set_software_id_field(0, "Diagnostic Protocol Example 1.0.0");
		diagnosticProtocol->set_software_id_field(1, "Another version string x.x.x.x");

		// Set an ECU ID (This is what tells other ECUs more details about your specific physical ECU)
		diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::HardwareID, "Hardware ID");
		diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::Location, "The Aether");
		diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::ManufacturerName, "None");
		diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::PartNumber, "1234");
		diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::SerialNumber, "1");

		// Let's say that our ECU has the capability of a universal terminal working set (as an example) and
		// contains weak internal bus termination.
		// This info gets reported to any ECU on the bus that requests our capabilities through the
		// control function functionalities message.
		diagnosticProtocol->ControlFunctionFunctionalitiesMessageInterface.set_functionality_is_supported(isobus::ControlFunctionFunctionalities::Functionalities::MinimumControlFunction, 1, true);
		diagnosticProtocol->ControlFunctionFunctionalitiesMessageInterface.set_minimum_control_function_option_state(isobus::ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type1ECUInternalWeakTermination, true);
		diagnosticProtocol->ControlFunctionFunctionalitiesMessageInterface.set_functionality_is_supported(isobus::ControlFunctionFunctionalities::Functionalities::UniversalTerminalWorkingSet, 1, true);

		// Set the DTCs active. This should put them in the DM1 message
		diagnosticProtocol->set_diagnostic_trouble_code_active(testDTC1, true);
		diagnosticProtocol->set_diagnostic_trouble_code_active(testDTC2, true);
		diagnosticProtocol->set_diagnostic_trouble_code_active(testDTC3, true);

		std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // Send the DM1 for a while

		// Set the DTCs inactive. This should put them in the DM2 message
		diagnosticProtocol->set_diagnostic_trouble_code_active(testDTC1, false);
		diagnosticProtocol->set_diagnostic_trouble_code_active(testDTC2, false);
		diagnosticProtocol->set_diagnostic_trouble_code_active(testDTC3, false);

		std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // Send the DM2 for a while

		diagnosticProtocol->clear_inactive_diagnostic_trouble_codes(); // All messages should now be clear!
	}

	while (running)
	{
		// CAN stack runs in other threads. Do nothing forever.
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	isobus::CANHardwareInterface::stop();
	isobus::DiagnosticProtocol::deassign_all_diagnostic_protocol_to_internal_control_functions();
	return 0;
}
