#include "../../common/can_arguments_parser.hpp"
#include "../../common/create_can_driver.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/utility/iop_file_interface.hpp"

#include "../../common/console_logger.cpp"
#include "object_pool_ids.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::shared_ptr<isobus::VirtualTerminalClient> TestVirtualTerminalClient = nullptr;
static std::atomic_bool running = { true };

// In-memory storage for auxiliary function assignments for demonstration purposes
// Production applications should use nonvolatile storage to restore preferences after power cycles.
static std::mutex assignmentStorageMutex;
static std::map<std::pair<std::uint64_t, std::uint16_t>, std::vector<isobus::VirtualTerminalClient::AssignedAuxiliaryFunction>> assignmentStorage;

void signal_handler(int)
{
	running = false;
}

// This callback will provide us with event driven notifications of auxiliary input from the stack
void handle_aux_function_input(const isobus::VirtualTerminalClient::AuxiliaryFunctionEvent &event)
{
	std::cout << "Auxiliary function event received: (" << event.function.functionObjectID << ", " << event.function.inputObjectID << ", " << static_cast<int>(event.function.functionType) << "), value1: " << event.value1 << ", value2: " << event.value2 << std::endl;
}

void handle_aux_assignment_failure(const isobus::VirtualTerminalClient::AuxiliaryAssignmentFailureEvent &event)
{
	// Present this alert on the operator interface (for example, an Alarm Mask) in a production application.
	std::cerr << "Operator alert: Auxiliary function assignments are unavailable after "
	          << static_cast<int>(event.attempts) << " transmission attempt(s). Check the auxiliary controls and assignments."
	          << std::endl;
}

// Callback to load stored auxiliary function assignments
static std::vector<isobus::VirtualTerminalClient::AssignedAuxiliaryFunction> load_assignments(
  std::uint64_t deviceName,
  std::uint16_t modelIdentificationCode,
  void *)
{
	std::vector<isobus::VirtualTerminalClient::AssignedAuxiliaryFunction> assignments;
	{
		std::lock_guard<std::mutex> lock(assignmentStorageMutex);
		auto it = assignmentStorage.find(std::make_pair(deviceName, modelIdentificationCode));
		if (assignmentStorage.end() != it)
		{
			assignments = it->second;
		}
	}
	if (!assignments.empty())
	{
		std::cout << "Loading " << assignments.size() << " stored assignment(s) for device "
		          << std::hex << deviceName << std::dec
		          << " (model ID: " << modelIdentificationCode << ")" << std::endl;
		return assignments;
	}

	std::cout << "No stored assignments found for device "
	          << std::hex << deviceName << std::dec
	          << " (model ID: " << modelIdentificationCode << ")" << std::endl;
	return {}; // Return empty vector if no stored assignments
}

// Callback to store auxiliary function assignments
static void store_assignments(
  std::uint64_t deviceName,
  std::uint16_t modelIdentificationCode,
  const std::vector<isobus::VirtualTerminalClient::AssignedAuxiliaryFunction> &assignments,
  void *)
{
	{
		std::lock_guard<std::mutex> lock(assignmentStorageMutex);
		assignmentStorage[std::make_pair(deviceName, modelIdentificationCode)] = assignments;
	}

	std::cout << "Stored " << assignments.size() << " assignment(s) for device "
	          << std::hex << deviceName << std::dec
	          << " (model ID: " << modelIdentificationCode << ")" << std::endl;

	// Optionally print details of each assignment
	for (const auto &assignment : assignments)
	{
		std::cout << "  - Function ID: " << assignment.functionObjectID
		          << ", Input ID: " << assignment.inputObjectID
		          << ", Type: " << static_cast<int>(assignment.functionType) << std::endl;
	}
}

int main(int argc, char **argv)
{
	std::signal(SIGINT, signal_handler);
	const auto canParameters = CanParametersParser(argc, argv).parameters();

	auto canDriver = CANDriverFactory::create(canParameters.interface, canParameters.driver);
	if (nullptr == canDriver)
	{
		return -1;
	}

	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	// Debug logging shows preferred assignment exchanges and retries in this example.
	// Use Info for less verbose application logging.
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Debug);
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
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(1);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	std::vector<std::uint8_t> testPool = isobus::IOPFileInterface::read_iop_file("aux_functions_pooldata.iop");

	if (0 != testPool.size())
	{
		std::cout << "Loaded object pool from aux_functions_pooldata.iop" << std::endl;
	}
	else
	{
		std::cout << "Failed to load object pool from aux_functions_pooldata.iop" << std::endl;
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

	TestVirtualTerminalClient->get_auxiliary_function_event_dispatcher().add_listener(handle_aux_function_input);
	TestVirtualTerminalClient->get_auxiliary_assignment_failure_event_dispatcher().add_listener(handle_aux_assignment_failure);
	std::cout << "Registered auxiliary function input event listener." << std::endl;

	TestVirtualTerminalClient->set_auxiliary_assignment_callbacks(load_assignments, store_assignments, nullptr);
	std::cout << "Registered auxiliary assignment storage callbacks (in-memory)" << std::endl;

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
