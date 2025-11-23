#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/utility/iop_file_interface.hpp"
#include "isobus/utility/system_timing.hpp"

#include "../../common/console_logger.cpp"
#include "object_pool_ids.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::shared_ptr<isobus::VirtualTerminalClient> TestVirtualTerminalClient = nullptr;
static constexpr std::uint64_t MODEL_IDENTIFICATION_CODE = 1; ///< The model identification code of 'our' input device, this should be increased when changes are made to the input(s) definitions

static constexpr std::uint64_t BUTTON_CYCLIC_DELAY = 3500; ///< 3.5 seconds between button presses
static constexpr std::uint64_t SLIDER_CYCLIC_DELAY = 1000; ///< 1 second between slider movements
static std::uint64_t lastButtonTimestamp = 0;
static std::uint64_t lastSliderTimestamp = 0;

static bool buttonPressed = false;
static std::uint16_t buttonTransitions = 0;

static constexpr std::uint16_t SLIDER_MAX_POSITION = 0xFAFF;
static bool backToZero = false;
static std::uint16_t sliderPosition = 0;
static std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
}

void simulate_button_press()
{
	buttonPressed = !buttonPressed;
	TestVirtualTerminalClient->update_auxiliary_input(AUXN_INPUT_BUTTON, buttonPressed, buttonTransitions);
	buttonTransitions++;
}

void simulate_slider_move()
{
	constexpr std::uint16_t AMOUNT_TO_MOVE = SLIDER_MAX_POSITION / 20; // We should now be able to reach the end of the slider in 20 steps
	if (backToZero)
	{
		if (sliderPosition > AMOUNT_TO_MOVE)
		{
			sliderPosition -= AMOUNT_TO_MOVE;
		}
		else
		{
			sliderPosition = 0;
			backToZero = false;
		}
	}
	else
	{
		if (sliderPosition < SLIDER_MAX_POSITION - AMOUNT_TO_MOVE)
		{
			sliderPosition += AMOUNT_TO_MOVE;
		}
		else
		{
			sliderPosition = SLIDER_MAX_POSITION;
			backToZero = true;
		}
	}
	TestVirtualTerminalClient->update_auxiliary_input(AUXN_INPUT_SLIDER, sliderPosition, 0xFFFF);
}

void on_periodic_update()
{
	if (nullptr != TestVirtualTerminalClient &&
	    !TestVirtualTerminalClient->get_auxiliary_input_learn_mode_enabled())
	{
		if (isobus::SystemTiming::time_expired_ms(lastButtonTimestamp, BUTTON_CYCLIC_DELAY))
		{
			lastButtonTimestamp = isobus::SystemTiming::get_timestamp_ms();
			simulate_button_press();
		}
		if (isobus::SystemTiming::time_expired_ms(lastSliderTimestamp, SLIDER_CYCLIC_DELAY))
		{
			lastSliderTimestamp = isobus::SystemTiming::get_timestamp_ms();
			simulate_slider_move();
		}
	}
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
#elif defined(ISOBUS_SYS_TEC_AVAILABLE)
	canDriver = std::make_shared<isobus::SysTecWindowsPlugin>();
#endif
	if (nullptr == canDriver)
	{
		std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
		std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
		return -1;
	}

	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Info); // Change this to Debug to see more information
	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
		return -2;
	}

	isobus::CANHardwareInterface::get_periodic_update_event_dispatcher().add_listener(on_periodic_update);
	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);

	//! Make sure you change these for your device!!!!
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(1);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	std::vector<std::uint8_t> testPool = isobus::IOPFileInterface::read_iop_file("aux_inputs_pooldata.iop");

	if (0 != testPool.size())
	{
		std::cout << "Loaded object pool from aux_inputs_pooldata.iop" << std::endl;
	}
	else
	{
		std::cout << "Failed to load object pool from aux_inputs_pooldata.iop" << std::endl;
		return -3;
	}

	// Generate a unique version string for this object pool (this is optional, and is entirely application specific behavior)
	std::string objectPoolHash = isobus::IOPFileInterface::hash_object_pool_to_version(testPool);

	const isobus::NAMEFilter filterVirtualTerminal(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	const std::vector<isobus::NAMEFilter> vtNameFilters = { filterVirtualTerminal };
	auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	auto TestPartnerVT = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	TestVirtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
	TestVirtualTerminalClient->set_object_pool(0, testPool.data(), testPool.size(), objectPoolHash);
	TestVirtualTerminalClient->set_auxiliary_input_model_identification_code(MODEL_IDENTIFICATION_CODE);
	TestVirtualTerminalClient->add_auxiliary_input_object_id(AUXN_INPUT_SLIDER);
	TestVirtualTerminalClient->add_auxiliary_input_object_id(AUXN_INPUT_BUTTON);
	TestVirtualTerminalClient->initialize(true);

	while (running)
	{
		// CAN stack runs in other threads. Do nothing forever.
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	TestVirtualTerminalClient->terminate();
	isobus::CANHardwareInterface::stop();
	return 0;
}
