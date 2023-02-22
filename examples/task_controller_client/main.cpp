#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"
#include "isobus/isobus/isobus_task_controller_client.hpp"
#include "isobus/utility/to_string.hpp"

#include "console_logger.cpp"
#include "ddop_definitions.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::shared_ptr<isobus::TaskControllerClient> TestTCClient = nullptr;
static std::atomic_bool running = { true };

using namespace std;

void signal_handler(int)
{
	running = false;
}

void update_CAN_network()
{
	isobus::CANNetworkManager::CANNetwork.update();
}

void raw_can_glue(isobus::HardwareInterfaceCANFrame &rawFrame, void *parentPointer)
{
	isobus::CANNetworkManager::CANNetwork.can_lib_process_rx_message(rawFrame, parentPointer);
}

bool create_example_sprayer_ddop(std::shared_ptr<isobus::DeviceDescriptorObjectPool> poolToPopulate)
{
	bool retVal = true;
	poolToPopulate->clear();

	// Make a test pool as a 120ft sprayer with 16 sections, 1 liquid product
	// Set up device
	retVal &= poolToPopulate->add_device("Isobus++ UnitTest", "1.0.0", "123", "I++1.0", TestTCClient->languageCommandInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0);
	retVal &= poolToPopulate->add_device_element("Sprayer", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, isobus::task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement));
	retVal &= poolToPopulate->add_device_process_data("Actual Work State", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualWorkState), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState));
	retVal &= poolToPopulate->add_device_value_presentation("minutes", 0, 1.0f, 1, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TimePresentation));
	retVal &= poolToPopulate->add_device_element("Connector", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), isobus::task_controller_object::DeviceElementObject::Type::Connector, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector));
	retVal &= poolToPopulate->add_device_process_data("Connector X", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorXOffset));
	retVal &= poolToPopulate->add_device_process_data("Connector Y", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorYOffset));
	retVal &= poolToPopulate->add_device_property("Type", 6, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ConnectorType), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType));

	// Set up Boom
	retVal &= poolToPopulate->add_device_element("Boom", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), 0, isobus::task_controller_object::DeviceElementObject::Type::Function, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom));
	retVal &= poolToPopulate->add_device_property("Offset X", 0, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::BoomXOffset));
	retVal &= poolToPopulate->add_device_property("Offset Y", 0, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::BoomYOffset));
	retVal &= poolToPopulate->add_device_property("Offset Z", 0, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetZ), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::BoomZOffset));
	retVal &= poolToPopulate->add_device_process_data("Actual Working Width", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualWorkingWidth), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ActualWorkingWidth));
	retVal &= poolToPopulate->add_device_process_data("Setpoint Work State", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointWorkState), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SetpointWorkState));
	retVal &= poolToPopulate->add_device_process_data("Area Total", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::TotalArea), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::AreaPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::Total), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::AreaTotal));

	// Set up sections for section control
	// Using 7 ft sections
	for (std::size_t i = 0; i < NUMBER_SECTIONS_TO_CREATE; i++)
	{
		retVal &= poolToPopulate->add_device_element("Section " + isobus::to_string(static_cast<int>(i)), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Section1) + i, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), isobus::task_controller_object::DeviceElementObject::Type::Section, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Section1) + i);
		retVal &= poolToPopulate->add_device_property("Offset X", -20, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Section1XOffset) + i);
		retVal &= poolToPopulate->add_device_property("Offset Y", (1067 * i) - 18288, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Section1YOffset) + i);
		retVal &= poolToPopulate->add_device_property("Width", 2 * 1067, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualWorkingWidth), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Section1Width) + i);
	}
	retVal &= poolToPopulate->add_device_process_data("Actual Work State 1-16", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState1_16), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ActualCondensedWorkingState));
	retVal &= poolToPopulate->add_device_process_data("Setpoint Work State", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState1_16), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SetpointCondensedWorkingState));

	// Set up bin/tank
	retVal &= poolToPopulate->add_device_element("Product", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), isobus::task_controller_object::DeviceElementObject::Type::Bin, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct));
	retVal &= poolToPopulate->add_device_process_data("Tank Capacity", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::MaximumVolumeContent), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::VolumePresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TankCapacity));
	retVal &= poolToPopulate->add_device_process_data("Tank Volume", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualVolumeContent), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::VolumePresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::TimeInterval), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TankVolume));

	// Set up presentations
	retVal &= poolToPopulate->add_device_value_presentation("mm", 0, 1.0f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation));
	retVal &= poolToPopulate->add_device_value_presentation("m", 0, 0.001f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation));
	retVal &= poolToPopulate->add_device_value_presentation("m^2", 0, 1.0f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::AreaPresentation));
	retVal &= poolToPopulate->add_device_value_presentation("L", 0, 0.001f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::VolumePresentation));

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

	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Debug); // Change this to Debug to see more information
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
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
	TestDeviceNAME.set_device_class(2);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::RateControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	const isobus::NAMEFilter filterTaskController(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::TaskController));
	const std::vector<isobus::NAMEFilter> tcNameFilters = { filterTaskController };
	std::shared_ptr<isobus::InternalControlFunction> TestInternalECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x1C, 0);
	std::shared_ptr<isobus::PartneredControlFunction> TestPartnerTC = std::make_shared<isobus::PartneredControlFunction>(0, tcNameFilters);

	TestTCClient = std::make_shared<isobus::TaskControllerClient>(TestPartnerTC, TestInternalECU, nullptr);

	// Set up some TC specific variables
	auto myDDOP = std::make_shared<isobus::DeviceDescriptorObjectPool>();
	bool tcClientStarted = false;

	while (running)
	{
		if (!tcClientStarted)
		{
			if (create_example_sprayer_ddop(myDDOP))
			{
				TestTCClient->configure(myDDOP, 1, NUMBER_SECTIONS_TO_CREATE, 16, true, false, true, false, true);
				TestTCClient->initialize(true);
				tcClientStarted = true;
			}
			else
			{
				std::cout << "Failed to create DDOP" << std::endl;
				break;
			}
		}

		// The CAN stack runs in other threads. Not much to do here.
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	TestTCClient->terminate();
	CANHardwareInterface::stop();
	return (!tcClientStarted);
}
