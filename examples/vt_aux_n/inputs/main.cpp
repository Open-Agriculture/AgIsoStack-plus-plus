#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_warning_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/utility/iop_file_interface.hpp"
#include "isobus/utility/system_timing.hpp"
#include "object_pool_ids.h"

#ifdef WIN32
#include "isobus/hardware_integration/pcan_basic_windows_plugin.hpp"
static PCANBasicWindowsPlugin canDriver(PCAN_USBBUS1);
#else
#include "isobus/hardware_integration/socket_can_interface.hpp"
static SocketCANInterface canDriver("can0");
#endif

#include <csignal>
#include <iostream>
#include <memory>

static std::shared_ptr<isobus::InternalControlFunction> TestInternalECU = nullptr;
static std::shared_ptr<isobus::PartneredControlFunction> TestPartnerVT = nullptr;
static std::shared_ptr<isobus::VirtualTerminalClient> TestVirtualTerminalClient = nullptr;
std::vector<isobus::NAMEFilter> vtNameFilters;
const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
static std::vector<std::uint8_t> testPool;

static constexpr std::uint64_t MODEL_IDENTIFICATION_CODE = 1; ///< The model identification code of 'our' input device, this should be increased when changes are made to the input(s) definitions

static constexpr std::uint64_t BUTTON_CYCLIC_DELAY = 3500; ///< 1 second between button presses
static constexpr std::uint64_t B2_CYCLIC_DELAY = 1000; ///< 3.5 seconds between slider movements
static std::uint64_t lastButtonTimestamp = 0;
static std::uint64_t lastSliderTimestamp = 0;

static bool buttonPressed = false;
static std::uint16_t buttonTransitions = 0;

static constexpr std::uint16_t SLIDER_MAX_POSITION = 0xFAFF;
static bool backToZero = false;
static std::uint16_t sliderPosition = 0;

using namespace std;

// A log sink for the CAN stack
class CustomLogger : public isobus::CANStackLogger
{
public:
	void LogCANLibWarning(const std::string &text) override
	{
		std::cout << text << std::endl; // Write the text to stdout
	}
};

static CustomLogger logger;

void signal_handler(int signum)
{
	CANHardwareInterface::stop();
	if (nullptr != TestVirtualTerminalClient)
	{
		TestVirtualTerminalClient->terminate();
	}
	exit(signum);
}

void simulate_button_press()
{
	buttonPressed = !buttonPressed;
	TestVirtualTerminalClient->update_auxiliary_input(AUXN_INPUT_BUTTON, buttonPressed, buttonTransitions);
	// std::cout << "Auxiliary Button input value change send (pressed: " << buttonPressed << ", transitions: " << buttonTransitions << ")" << std::endl;
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
	// std::cout << "Auxiliary slider input value change send (value1: " << sliderPosition << ")" << std::endl;
}

void update_CAN_network()
{
	if (isobus::SystemTiming::time_expired_ms(lastButtonTimestamp, BUTTON_CYCLIC_DELAY))
	{
		lastButtonTimestamp = isobus::SystemTiming::get_timestamp_ms();
		simulate_button_press();
	}
	if (isobus::SystemTiming::time_expired_ms(lastSliderTimestamp, B2_CYCLIC_DELAY))
	{
		lastSliderTimestamp = isobus::SystemTiming::get_timestamp_ms();
		simulate_slider_move();
	}

	isobus::CANNetworkManager::CANNetwork.update();
}

void raw_can_glue(isobus::HardwareInterfaceCANFrame &rawFrame, void *parentPointer)
{
	isobus::CANNetworkManager::CANNetwork.can_lib_process_rx_message(rawFrame, parentPointer);
}

void setup()
{
	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, &canDriver);

	if ((!CANHardwareInterface::start()) || (!canDriver.get_is_valid()))
	{
		std::cout << "Failed to connect to the socket. The interface might be down." << std::endl;
	}

	CANHardwareInterface::add_can_lib_update_callback(update_CAN_network, nullptr);
	CANHardwareInterface::add_raw_can_message_rx_callback(raw_can_glue, nullptr);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);

	// Make sure you change these for your device!!!!
	// This is an example device that is using a manufacturer code that is currently unused at time of writing
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(1);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);
	vtNameFilters.push_back(testFilter);

	testPool = isobus::IOPFileInterface::read_iop_file("vtpooldata.iop");

	if (0 != testPool.size())
	{
		std::cout << "Loaded object pool from vtpooldata.iop" << std::endl;
	}
	else
	{
		std::cout << "Failed to load object pool from vtpooldata.iop" << std::endl;
	}

	// Generate a unique version string for this object pool (this is optional, and is entirely application specific behavior)
	std::string objectPoolHash = isobus::IOPFileInterface::hash_object_pool_to_version(testPool);

	TestInternalECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x1D, 0);
	TestPartnerVT = std::make_shared<isobus ::PartneredControlFunction>(0, vtNameFilters);
	TestVirtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
	TestVirtualTerminalClient->set_object_pool(0, isobus::VirtualTerminalClient::VTVersion::Version3, testPool.data(), testPool.size(), objectPoolHash);
	TestVirtualTerminalClient->set_auxiliary_input_model_identification_code(MODEL_IDENTIFICATION_CODE);

	/// @todo Remove this once the VT client is able to know which objects are uploaded to the VT (#65)
	/// This is a temporary workaround to make sure the VT client knows which inputs are available
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));

	TestVirtualTerminalClient->initialize(true);
	std::signal(SIGINT, signal_handler);
}

int main()
{
	setup();

	while (true)
	{
		// CAN stack runs in other threads. Do nothing forever.
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	CANHardwareInterface::stop();
	return 0;
}
