//================================================================================================
/// @file main.cpp
///
/// @brief A minimal AgIsoStack++ program for the Arduino framework, built with PlatformIO.
/// @details The same file builds for a Teensy 4.x (FlexCAN) and an ESP32 (TWAI). It claims an
/// address, answers requests for the Proprietary A PGN, and sends one such request to the bus.
///
/// Unlike the desktop examples, nothing here runs in a thread: when ARDUINO is defined the stack
/// starts no threads of its own, so loop() has to call CANHardwareInterface::update().
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include <Arduino.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#if defined(ESP_PLATFORM)
#include "isobus/hardware_integration/twai_plugin.hpp"
#else
#include "isobus/hardware_integration/flex_can_t4_plugin.hpp"
#endif

/// @brief Writes the stack's log messages to the serial monitor
class SerialLogger : public isobus::CANStackLogger
{
public:
	void sink_CAN_stack_log(CANStackLogger::LoggingLevel level, const std::string &text) override
	{
		static const char *const LEVEL_NAMES[] = { "Debug", "Info", "Warning", "Error", "Critical" };
		const std::size_t index = static_cast<std::size_t>(level);
		Serial.print('[');
		Serial.print((index < (sizeof(LEVEL_NAMES) / sizeof(LEVEL_NAMES[0]))) ? LEVEL_NAMES[index] : "?");
		Serial.print("] ");
		Serial.println(text.c_str());
	}
};

static SerialLogger serialLogger;
static std::shared_ptr<isobus::InternalControlFunction> internalECU = nullptr;

static bool propa_request_handler(std::uint32_t parameterGroupNumber,
                                  std::shared_ptr<isobus::ControlFunction>,
                                  bool &acknowledge,
                                  isobus::AcknowledgementType &acknowledgeType,
                                  void *)
{
	if (static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA) != parameterGroupNumber)
	{
		return false; // Let another callback handle this PGN
	}
	acknowledge = true;
	acknowledgeType = isobus::AcknowledgementType::Positive;
	return true;
}

void setup()
{
	Serial.begin(115200);
	isobus::CANStackLogger::set_can_stack_logger_sink(&serialLogger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Info);

#if defined(ESP_PLATFORM)
	// TX on GPIO 21, RX on GPIO 22, through a CAN transceiver. Change them for your board.
	static twai_general_config_t twaiConfig = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL);
	static twai_timing_config_t twaiTiming = TWAI_TIMING_CONFIG_250KBITS();
	static twai_filter_config_t twaiFilter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
	twaiConfig.rx_queue_len = 32; // loop() empties the queue, so give bursts room
	// Without threads, update() reads frames until the receive queue is empty. Use 0 ms timeouts:
	// the 100 ms defaults would make that last, empty read block loop() for 100 ms every time.
	auto canDriver = std::make_shared<isobus::TWAIPlugin>(&twaiConfig, &twaiTiming, &twaiFilter, 0, 0);
#else
	// Channel 0 is the Teensy's CAN1, on pins 22 (TX) and 23 (RX) of a Teensy 4.1, through a CAN transceiver.
	auto canDriver = std::make_shared<isobus::FlexCANT4Plugin>(0);
#endif

	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);
	if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		Serial.println("Failed to start the CAN hardware interface");
		return;
	}

	isobus::NAME deviceNAME(0);
	//! Make sure you change these for your device!
	deviceNAME.set_arbitrary_address_capable(true);
	deviceNAME.set_industry_group(1);
	deviceNAME.set_device_class(0);
	deviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	deviceNAME.set_identity_number(2);
	deviceNAME.set_ecu_instance(0);
	deviceNAME.set_function_instance(0);
	deviceNAME.set_device_class_instance(0);
	deviceNAME.set_manufacturer_code(1407);
	internalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(deviceNAME, 0);

	// Address claiming takes at least 250 ms. Keep the stack running while waiting for it.
	const unsigned long claimStart = millis();
	while ((!internalECU->get_address_valid()) && ((millis() - claimStart) < 5000))
	{
		isobus::CANHardwareInterface::update();
	}
	if (!internalECU->get_address_valid())
	{
		Serial.println("Address claiming failed");
		return;
	}

	internalECU->get_pgn_request_protocol().lock()->register_pgn_request_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), propa_request_handler, nullptr);

	// Ask every device on the bus for Proprietary A
	isobus::ParameterGroupNumberRequestProtocol::request_parameter_group_number(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), internalECU, nullptr);
}

void loop()
{
	// Without threads this is what runs the stack: receiving, address claiming, transport
	// protocols and the callbacks above all happen inside update().
	isobus::CANHardwareInterface::update();
}
