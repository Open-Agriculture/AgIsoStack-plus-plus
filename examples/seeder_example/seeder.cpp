//================================================================================================
/// @file seeder.cpp
///
/// @brief This is the implementation of an example seeder application
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "seeder.hpp"

#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/isobus_diagnostic_protocol.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"
#include "isobus/isobus/isobus_task_controller_client.hpp"

#include "../common/console_logger.cpp"

#include <iostream>

bool Seeder::initialize(const std::string &interfaceName)
{
	bool retVal = true;

	// Automatically load the desired CAN driver based on the available drivers
	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	std::string interfaceNameToOpen = interfaceName.empty() ? "can0" : interfaceName;
	canDriver = std::make_shared<isobus::SocketCANInterface>(interfaceNameToOpen);
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
	int channel = interfaceName.empty() ? 0 : std::stoi(interfaceName);
	canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0); // CAN0
#elif (defined(ISOBUS_MACCANPCAN_AVAILABLE) || defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE))
	int channel = interfaceName.empty() ? PCAN_USBBUS1 : (std::stoi(interfaceName) - 1 + PCAN_USBBUS1);
#if defined(ISOBUS_MACCANPCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::MacCANPCANPlugin>(channel);
#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
	canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(channel);
#endif
#endif
	if (nullptr == canDriver)
	{
		std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
		std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
		return false;
	}

	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Debug); // Change this to Debug to see more information
	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
		return false;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);

	//! This is an example device that is using a manufacturer code that is currently unused at time of writing
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(2);
	TestDeviceNAME.set_device_class(4);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::RateControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	const isobus::NAMEFilter filterVirtualTerminal(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	const isobus::NAMEFilter filterTaskController(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::TaskController));
	const isobus::NAMEFilter filterTaskControllerInstance(isobus::NAME::NAMEParameters::FunctionInstance, 0);
	const isobus::NAMEFilter filterTaskControllerIndustryGroup(isobus::NAME::NAMEParameters::IndustryGroup, static_cast<std::uint8_t>(isobus::NAME::IndustryGroup::AgriculturalAndForestryEquipment));
	const isobus::NAMEFilter filterTaskControllerDeviceClass(isobus::NAME::NAMEParameters::DeviceClass, static_cast<std::uint8_t>(isobus::NAME::DeviceClass::NonSpecific));
	const std::vector<isobus::NAMEFilter> tcNameFilters = { filterTaskController,
		                                                      filterTaskControllerInstance,
		                                                      filterTaskControllerIndustryGroup,
		                                                      filterTaskControllerDeviceClass };
	const std::vector<isobus::NAMEFilter> vtNameFilters = { filterVirtualTerminal };
	auto InternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	auto PartnerVT = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);
	auto PartnerTC = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, tcNameFilters);

	diagnosticProtocol = std::make_unique<isobus::DiagnosticProtocol>(InternalECU);
	diagnosticProtocol->initialize();

	diagnosticProtocol->set_product_identification_code("1234567890ABC");
	diagnosticProtocol->set_product_identification_brand("AgIsoStack++");
	diagnosticProtocol->set_product_identification_model("AgIsoStack++ Seeder Example");
	diagnosticProtocol->set_software_id_field(0, "Example 1.0.0");
	diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::HardwareID, "1234");
	diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::Location, "N/A");
	diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::ManufacturerName, "Open-Agriculture");
	diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::PartNumber, "1234");
	diagnosticProtocol->set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::SerialNumber, "2");
	diagnosticProtocol->ControlFunctionFunctionalitiesMessageInterface.set_task_controller_geo_client_option(255);
	diagnosticProtocol->ControlFunctionFunctionalitiesMessageInterface.set_task_controller_section_control_client_option_state(1, 255);
	diagnosticProtocol->ControlFunctionFunctionalitiesMessageInterface.set_functionality_is_supported(isobus::ControlFunctionFunctionalities::Functionalities::MinimumControlFunction, 1, true);
	diagnosticProtocol->ControlFunctionFunctionalitiesMessageInterface.set_functionality_is_supported(isobus::ControlFunctionFunctionalities::Functionalities::UniversalTerminalWorkingSet, 1, true);
	diagnosticProtocol->ControlFunctionFunctionalitiesMessageInterface.set_functionality_is_supported(isobus::ControlFunctionFunctionalities::Functionalities::TaskControllerBasicClient, 1, true);
	diagnosticProtocol->ControlFunctionFunctionalitiesMessageInterface.set_functionality_is_supported(isobus::ControlFunctionFunctionalities::Functionalities::TaskControllerGeoClient, 1, true);
	diagnosticProtocol->ControlFunctionFunctionalitiesMessageInterface.set_functionality_is_supported(isobus::ControlFunctionFunctionalities::Functionalities::TaskControllerSectionControlClient, 1, true);

	VTApplication = std::make_unique<SeederVtApplication>(PartnerVT, PartnerTC, InternalECU);
	VTApplication->initialize();

	return retVal;
}

void Seeder::terminate()
{
	if (nullptr != VTApplication)
	{
		VTApplication->VTClientInterface->terminate();
		VTApplication->TCClientInterface.terminate();
	}
	if (nullptr != diagnosticProtocol)
	{
		diagnosticProtocol->terminate();
	}
	isobus::CANHardwareInterface::stop();
}

void Seeder::update()
{
	if (nullptr != VTApplication)
	{
		VTApplication->update();
		diagnosticProtocol->update();
	}
}
