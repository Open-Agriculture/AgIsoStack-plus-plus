#include "can_general_parameter_group_numbers.hpp"
#include "can_network_manager.hpp"
#include "isobus_diagnostic_protocol.hpp"
#include "socket_can_interface.hpp"

#include <csignal>
#include <iterator>
#include <memory>

static std::shared_ptr<isobus::InternalControlFunction> TestInternalECU = nullptr;

using namespace std;

void signal_handler(int signum)
{
	isobus::DiagnosticProtocol::deassign_diagnostic_protocol_to_internal_control_function(TestInternalECU);
	CANHardwareInterface::stop();
	exit(signum);
}

void update_CAN_network()
{
	isobus::CANNetworkManager::CANNetwork.update();
}

void raw_can_glue(isobus::HardwareInterfaceCANFrame &rawFrame, void *parentPointer)
{
	isobus::CANNetworkManager::CANNetwork.can_lib_process_rx_message(rawFrame, parentPointer);
}

void setup()
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, "can0");
	CANHardwareInterface::start();

	CANHardwareInterface::add_can_lib_update_callback(update_CAN_network, nullptr);
	CANHardwareInterface::add_raw_can_message_rx_callback(raw_can_glue, nullptr);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);

	// Make sure you change these for your device!!!!
	// This is an example device that is using a manufacturer code that is currently unused at time of writing
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(0);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	TestInternalECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x1C, 0);

	isobus::DiagnosticProtocol::assign_diagnostic_protocol_to_internal_control_function(TestInternalECU);

	std::signal(SIGINT, signal_handler);
}

int main()
{
	isobus::DiagnosticProtocol *diagnosticProtocol;

	setup();

	diagnosticProtocol = isobus::DiagnosticProtocol::get_diagnostic_protocol_by_internal_control_function(TestInternalECU);

	// Make a few test DTCs
	isobus::DiagnosticProtocol::DiagnosticTroubleCode testDTC1(1234, isobus::DiagnosticProtocol::FailureModeIdentifier::ConditionExists, isobus::DiagnosticProtocol::LampStatus::None);
	isobus::DiagnosticProtocol::DiagnosticTroubleCode testDTC2(567, isobus::DiagnosticProtocol::FailureModeIdentifier::DataErratic, isobus::DiagnosticProtocol::LampStatus::AmberWarningLampSlowFlash);
	isobus::DiagnosticProtocol::DiagnosticTroubleCode testDTC3(8910, isobus::DiagnosticProtocol::FailureModeIdentifier::BadIntellegentDevice, isobus::DiagnosticProtocol::LampStatus::RedStopLampSolid);

	// Set a product identification string (in case someone requests it)
	diagnosticProtocol->set_product_identification_code("1234567890ABC");
	diagnosticProtocol->set_product_identification_brand("Del Grosso Engineering");
	diagnosticProtocol->set_product_identification_model("ISO 11783 CAN Stack DP Example");

	// Set a software ID string (This is what tells other ECUs what version your software is)
	diagnosticProtocol->set_software_id_field(0, "Diagnostic Protocol Example 1.0.0");
	diagnosticProtocol->set_software_id_field(1, "Another version string x.x.x.x");

	// Set an ECU ID (This is what tells other ECUs more details about your specific physical ECU)
	diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::HardwareID, "Hardware ID");
	diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::Location, "The Aether");
	diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::ManufacturerName, "None");
	diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::PartNumber, "1234");
	diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::SerialNumber, "1");

	if (nullptr != diagnosticProtocol)
	{
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

	while (true)
	{
		// CAN stack runs in other threads. Do nothing forever.
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	CANHardwareInterface::stop();
	return 0;
}
