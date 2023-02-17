#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_task_controller_client.hpp"
#include "isobus/utility/system_timing.hpp"

using namespace isobus;

class DerivedTestTCClient : public TaskControllerClient
{
public:
	DerivedTestTCClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  TaskControllerClient(partner, clientSource){};

	bool test_wrapper_send_working_set_master() const
	{
		return TaskControllerClient::send_working_set_master();
	}

	void test_wrapper_set_state(TaskControllerClient::StateMachineState newState)
	{
		TaskControllerClient::set_state(newState);
	}

	TaskControllerClient::StateMachineState test_wrapper_get_state() const
	{
		return TaskControllerClient::get_state();
	}
};

TEST(TASK_CONTROLLER_CLIENT_TESTS, MessageEncoding)
{
	auto testPlugin = std::make_shared<VirtualCANPlugin>("", true);

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, testPlugin);
	CANHardwareInterface::add_can_lib_update_callback(
	  [] {
		  CANNetworkManager::CANNetwork.update();
	  },
	  nullptr);
	CANHardwareInterface::start();

	NAME clientNAME(0);
	clientNAME.set_industry_group(2);
	clientNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::RateControl));
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x81, 0);

	HardwareInterfaceCANFrame testFrame;

	std::uint32_t waitingTimestamp_ms = SystemTiming::get_timestamp_ms();

	while ((!internalECU->get_address_valid()) &&
	       (!SystemTiming::time_expired_ms(waitingTimestamp_ms, 2000)))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ASSERT_TRUE(internalECU->get_address_valid());

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::TaskController));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = std::make_shared<PartneredControlFunction>(0, vtNameFilters);

	// Force claim a partner
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EEFFF7;
	testFrame.data[0] = 0x03;
	testFrame.data[1] = 0x04;
	testFrame.data[2] = 0x00;
	testFrame.data[3] = 0x12;
	testFrame.data[4] = 0x00;
	testFrame.data[5] = 0x82;
	testFrame.data[6] = 0x00;
	testFrame.data[7] = 0xA0;
	CANNetworkManager::CANNetwork.can_lib_process_rx_message(testFrame, nullptr);

	DerivedTestTCClient interfaceUnderTest(vtPartner, internalECU);

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	// Get the virtual CAN plugin back to a known state
	while (!testPlugin->get_queue_empty())
	{
		testPlugin->read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin->get_queue_empty());

	// Test Working Set Master Message
	ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_working_set_master());

	testPlugin->read_frame(testFrame);

	ASSERT_EQ(testFrame.dataLength, 8);
	ASSERT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xFE0D);
	ASSERT_EQ(testFrame.data[0], 1); // 1 Working set member by default

	for (std::uint_fast8_t i = 1; i < 8; i++)
	{
		// Check Reserved Bytes
		ASSERT_EQ(testFrame.data[i], 0xFF);
	}

	CANHardwareInterface::stop();
	CANHardwareInterface::set_number_of_can_channels(0);
}

TEST(TASK_CONTROLLER_CLIENT_TESTS, BadPartnerDeathTest)
{
	NAME clientNAME(0);
	clientNAME.set_industry_group(2);
	clientNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::RateControl));
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x81, 0);
	DerivedTestTCClient interfaceUnderTest(nullptr, internalECU);
	ASSERT_FALSE(interfaceUnderTest.get_is_initialized());
	EXPECT_DEATH(interfaceUnderTest.initialize(false), "");
}

TEST(TASK_CONTROLLER_CLIENT_TESTS, BadICFDeathTest)
{
	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::TaskController));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = std::make_shared<PartneredControlFunction>(0, vtNameFilters);
	DerivedTestTCClient interfaceUnderTest(vtPartner, nullptr);
	ASSERT_FALSE(interfaceUnderTest.get_is_initialized());
	EXPECT_DEATH(interfaceUnderTest.initialize(false), "");
}

TEST(TASK_CONTROLLER_CLIENT_TESTS, StateMachineTests)
{
	// Boilerplate...
	auto testPlugin = std::make_shared<VirtualCANPlugin>("", true);

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, testPlugin);
	CANHardwareInterface::add_can_lib_update_callback(
	  [] {
		  CANNetworkManager::CANNetwork.update();
	  },
	  nullptr);
	CANHardwareInterface::start();

	NAME clientNAME(0);
	clientNAME.set_industry_group(2);
	clientNAME.set_ecu_instance(1);
	clientNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::RateControl));
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x83, 0);

	HardwareInterfaceCANFrame testFrame;

	std::uint32_t waitingTimestamp_ms = SystemTiming::get_timestamp_ms();

	while ((!internalECU->get_address_valid()) &&
	       (!SystemTiming::time_expired_ms(waitingTimestamp_ms, 2000)))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ASSERT_TRUE(internalECU->get_address_valid());

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::TaskController));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = std::make_shared<PartneredControlFunction>(0, vtNameFilters);

	// Force claim a partner
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EEFFF7;
	testFrame.data[0] = 0x03;
	testFrame.data[1] = 0x04;
	testFrame.data[2] = 0x00;
	testFrame.data[3] = 0x12;
	testFrame.data[4] = 0x00;
	testFrame.data[5] = 0x82;
	testFrame.data[6] = 0x00;
	testFrame.data[7] = 0xA0;
	CANNetworkManager::CANNetwork.can_lib_process_rx_message(testFrame, nullptr);

	DerivedTestTCClient interfaceUnderTest(vtPartner, internalECU);
	interfaceUnderTest.initialize(false);

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	// Get the virtual CAN plugin back to a known state
	while (!testPlugin->get_queue_empty())
	{
		testPlugin->read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin->get_queue_empty());

	// End boilerplate

	// Check initial state
	ASSERT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::Disconnected);

	// Check Transition out of status message wait state
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::WaitForServerStatusMessage);
	ASSERT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::WaitForServerStatusMessage);

	// Send a status message and confirm we move on to the next state.
	testFrame.identifier = 0x18CBFFF7;
	testFrame.data[0] = 0xFE; // Status mux
	testFrame.data[1] = 0xFF; // Element number, set to not available
	testFrame.data[2] = 0xFF; // DDI (N/A)
	testFrame.data[3] = 0xFF; // DDI (N/A)
	testFrame.data[4] = 0x00; // Status
	testFrame.data[5] = 0x00; // Command address
	testFrame.data[6] = 0x00; // Command
	testFrame.data[7] = 0xFF; // Reserved
	CANNetworkManager::CANNetwork.can_lib_process_rx_message(testFrame, nullptr);

	std::this_thread::sleep_for(std::chrono::milliseconds(15));

	ASSERT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::BeginSendingWorkingSetMaster);

	// Test Request Language state
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::RequestLanguage);
	interfaceUnderTest.update();

	ASSERT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::WaitForLanguageResponse);

	//! @Todo Add other states

	interfaceUnderTest.terminate();
	CANHardwareInterface::stop();
}
