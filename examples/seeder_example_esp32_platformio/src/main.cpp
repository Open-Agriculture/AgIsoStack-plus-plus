//================================================================================================
/// @file main.cpp
///
/// @brief Defines `app_main` for the ESP32 port of the seeder example.
/// @details This is the same "complete" application as the desktop
///          examples/seeder_example (Virtual Terminal client + Task Controller
///          client + section control + diagnostics), but with the platform
///          glue swapped for ESP32: the built-in TWAI CAN controller instead of
///          a PC CAN adapter, an ESP_LOG sink instead of std::cout, and a
///          FreeRTOS loop in `app_main` instead of a std::thread + signal
///          handler.
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include "board_config.hpp"
#include "input_driver.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/twai_plugin.hpp"
#include "isobus/isobus/can_NAME.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/isobus_diagnostic_protocol.hpp"
#include "relay_driver.hpp"
#include "vt_application.hpp"

#include "console_logger.cpp"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <memory>

static constexpr const char *TAG = "AgIsoStack";

extern "C" void app_main()
{
	// Set up the ESP32's built-in TWAI (classic CAN 2.0) controller as the
	// CAN driver, at the ISO 11783 / J1939 baud rate of 250 kbit/s, accepting
	// all messages (nothing filtered out in hardware).
	twai_general_config_t twaiConfig = TWAI_GENERAL_CONFIG_DEFAULT(board_config::CAN_TX_PIN, board_config::CAN_RX_PIN, TWAI_MODE_NORMAL);
	twai_timing_config_t twaiTiming = TWAI_TIMING_CONFIG_250KBITS();
	twai_filter_config_t twaiFilter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
	auto canDriver = std::make_shared<isobus::TWAIPlugin>(&twaiConfig, &twaiTiming, &twaiFilter);

	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Info); // Change this to Debug to see more information
	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		ESP_LOGE(TAG, "Failed to start hardware interface. The CAN driver might be invalid.");
		return;
	}

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
	auto internalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	auto partnerVT = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);
	auto partnerTC = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, tcNameFilters);

	auto diagnosticProtocol = std::make_unique<isobus::DiagnosticProtocol>(internalECU);
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

	auto vtApplication = std::make_unique<SeederVtApplication>(partnerVT, partnerTC, internalECU);
	if (!vtApplication->initialize())
	{
		ESP_LOGE(TAG, "Failed to initialize the seeder VT application.");
		return;
	}

	// Bring up the optional board hardware described in board_config.hpp. Each is gated on its
	// feature flag, so the example also runs on an ESP32-S3 board that lacks the relay expander or
	// the opto-isolated inputs. The relay expander is additionally probed at runtime: if it does not
	// acknowledge, relaysActive stays false and we never drive it, so a missing or unpowered IC
	// degrades to "no physical relays" instead of faulting the application.
	const std::uint8_t sectionCount = vtApplication->get_number_of_sections();
	if (board_config::HAS_DIGITAL_INPUTS)
	{
		input_driver::init();
	}
	bool relaysActive = false;
	if (board_config::HAS_RELAY_EXPANDER)
	{
		relaysActive = relay_driver::init();
		if (!relaysActive)
		{
			ESP_LOGW(TAG, "Relay expander not detected; section commands will not drive physical relays.");
		}
	}

	while (true)
	{
		// Sample the digital inputs (debounced) and flag any row whose fault input is active. DI 1..6
		// map to rows 1..6. A fault is machine feedback (e.g. motor disconnected or no seed): the row
		// keeps its command (relay stays energized) but reports "not working" and is flagged on the VT.
		// Skipped on a board without digital inputs (board_config::HAS_DIGITAL_INPUTS false).
		if (board_config::HAS_DIGITAL_INPUTS)
		{
			input_driver::update();
		}
		for (std::uint8_t section = 0; board_config::HAS_DIGITAL_INPUTS && (section < sectionCount); ++section)
		{
			vtApplication->set_section_fault(section, input_driver::read_debounced(static_cast<std::uint8_t>(section + 1)));
		}

		vtApplication->update();
		diagnosticProtocol->update();

		// Mirror each section's COMMANDED on/off state onto its relay (section 0 -> relay 1, ...).
		// We drive the output from the command, not the fault-affected actual state, so a row that
		// reports a fault (e.g. no seed) stays energized -- the fault is only reported back to the TC
		// and shown on the VT. Gated on relaysActive so a board without the expander (or one where it
		// was not detected at init) does no I2C traffic here. set_relay() also skips the write when a
		// channel is unchanged, so this stays cheap at 20 Hz.
		for (std::uint8_t section = 0; relaysActive && (section < sectionCount); ++section)
		{
			relay_driver::set_relay(static_cast<std::uint8_t>(section + 1), vtApplication->get_section_commanded_state(section));
		}

		vTaskDelay(pdMS_TO_TICKS(50));
	}

	// Never reached (the loop above runs forever), but shown for completeness.
	vtApplication->VTClientInterface->terminate();
	vtApplication->TCClientInterface.terminate();
	diagnosticProtocol->terminate();
	isobus::CANHardwareInterface::stop();
}
