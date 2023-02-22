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

	bool test_wrapper_send_version_request() const
	{
		return TaskControllerClient::send_version_request();
	}

	bool test_wrapper_send_request_version_response() const
	{
		return TaskControllerClient::send_request_version_response();
	}

	bool test_wrapper_send_request_structure_label() const
	{
		return TaskControllerClient::send_request_structure_label();
	}

	bool test_wrapper_send_request_localization_label() const
	{
		return TaskControllerClient::send_request_localization_label();
	}

	bool test_wrapper_send_delete_object_pool() const
	{
		return TaskControllerClient::send_delete_object_pool();
	}
};

TEST(TASK_CONTROLLER_CLIENT_TESTS, MessageEncoding)
{
	VirtualCANPlugin serverTC;
	serverTC.open();
	auto blankDDOP = std::make_shared<DeviceDescriptorObjectPool>();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
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
	while (!serverTC.get_queue_empty())
	{
		serverTC.read_frame(testFrame);
	}
	ASSERT_TRUE(serverTC.get_queue_empty());

	// Test Working Set Master Message
	ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_working_set_master());

	ASSERT_TRUE(serverTC.read_frame(testFrame));

	ASSERT_TRUE(testFrame.isExtendedFrame);
	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xFE0D);
	EXPECT_EQ(testFrame.data[0], 1); // 1 Working set member by default

	for (std::uint_fast8_t i = 1; i < 8; i++)
	{
		// Check Reserved Bytes
		ASSERT_EQ(testFrame.data[i], 0xFF);
	}

	// Test Version Request Message
	ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_version_request());

	ASSERT_TRUE(serverTC.read_frame(testFrame));

	ASSERT_TRUE(testFrame.isExtendedFrame);
	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xCB00);
	EXPECT_EQ(0x00, testFrame.data[0]);

	for (std::uint_fast8_t i = 1; i < 8; i++)
	{
		// Check Reserved Bytes
		ASSERT_EQ(testFrame.data[i], 0xFF);
	}

	// Test status message
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::SendStatusMessage);
	ASSERT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::SendStatusMessage);
	interfaceUnderTest.update();

	serverTC.read_frame(testFrame);

	ASSERT_TRUE(testFrame.isExtendedFrame);
	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xCB00);
	EXPECT_EQ(0xFF, testFrame.data[0]); // Mux
	EXPECT_EQ(0xFF, testFrame.data[1]); // Element number
	EXPECT_EQ(0xFF, testFrame.data[2]); // DDI
	EXPECT_EQ(0xFF, testFrame.data[3]); // DDI
	EXPECT_EQ(0x00, testFrame.data[4]); // Status
	EXPECT_EQ(0x00, testFrame.data[5]); // 0 Reserved
	EXPECT_EQ(0x00, testFrame.data[6]); // 0 Reserved
	EXPECT_EQ(0x00, testFrame.data[7]); // 0 Reserved

	// Test version response
	ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_request_version_response());
	serverTC.read_frame(testFrame);
	ASSERT_TRUE(testFrame.isExtendedFrame);
	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xCB00);
	EXPECT_EQ(0x10, testFrame.data[0]); // Mux
	EXPECT_EQ(0x04, testFrame.data[1]); // Version
	EXPECT_EQ(0xFF, testFrame.data[2]); // Must be 0xFF
	EXPECT_EQ(0x00, testFrame.data[3]); // Options
	EXPECT_EQ(0x00, testFrame.data[4]); // Must be zero
	EXPECT_EQ(0x00, testFrame.data[5]); // Booms
	EXPECT_EQ(0x00, testFrame.data[6]); // Sections
	EXPECT_EQ(0x00, testFrame.data[7]); // Channels

	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::Disconnected);
	interfaceUnderTest.configure(blankDDOP, 1, 2, 3, true, true, true, true, true);
	ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_request_version_response());
	serverTC.read_frame(testFrame);

	ASSERT_TRUE(testFrame.isExtendedFrame);
	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xCB00);
	EXPECT_EQ(0x10, testFrame.data[0]); // Mux
	EXPECT_EQ(0x04, testFrame.data[1]); // Version
	EXPECT_EQ(0xFF, testFrame.data[2]); // Must be 0xFF
	EXPECT_EQ(0x1F, testFrame.data[3]); // Options
	EXPECT_EQ(0x00, testFrame.data[4]); // Must be zero
	EXPECT_EQ(0x01, testFrame.data[5]); // Booms
	EXPECT_EQ(0x02, testFrame.data[6]); // Sections
	EXPECT_EQ(0x03, testFrame.data[7]); // Channels

	// Test Request structure label
	ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_request_structure_label());
	serverTC.read_frame(testFrame);
	ASSERT_TRUE(testFrame.isExtendedFrame);
	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xCB00);
	EXPECT_EQ(0x01, testFrame.data[0]);
	for (std::uint_fast8_t i = 1; i < 7; i++)
	{
		EXPECT_EQ(0xFF, testFrame.data[i]);
	}

	// Test Request localization label
	ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_request_localization_label());
	serverTC.read_frame(testFrame);
	ASSERT_TRUE(testFrame.isExtendedFrame);
	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xCB00);
	EXPECT_EQ(0x21, testFrame.data[0]);
	for (std::uint_fast8_t i = 1; i < 7; i++)
	{
		EXPECT_EQ(0xFF, testFrame.data[i]);
	}

	// Test Delete Object Pool
	ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_delete_object_pool());
	serverTC.read_frame(testFrame);
	ASSERT_TRUE(testFrame.isExtendedFrame);
	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xCB00);
	EXPECT_EQ(0xA1, testFrame.data[0]);
	for (std::uint_fast8_t i = 1; i < 7; i++)
	{
		EXPECT_EQ(0xFF, testFrame.data[i]);
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
	VirtualCANPlugin serverTC;
	serverTC.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
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
	while (!serverTC.get_queue_empty())
	{
		serverTC.read_frame(testFrame);
	}
	ASSERT_TRUE(serverTC.get_queue_empty());

	// End boilerplate

	// Check initial state
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::Disconnected);

	// Check Transition out of status message wait state
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::WaitForServerStatusMessage);
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::WaitForServerStatusMessage);

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

	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::SendWorkingSetMaster);

	// Test Request Language state
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::RequestLanguage);
	interfaceUnderTest.update();

	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::WaitForLanguageResponse);

	// Test Version Response State
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::WaitForRequestVersionResponse);
	interfaceUnderTest.update();

	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::WaitForRequestVersionResponse);

	// Send the version response to the client as the TC server
	// Send a status message and confirm we move on to the next state.
	testFrame.identifier = 0x18CB83F7;
	testFrame.data[0] = 0x10; // Mux
	testFrame.data[1] = 0x04; // Version number (Version 4)
	testFrame.data[2] = 0xFF; // Max boot time (Not available)
	testFrame.data[3] = 0b0011111; // Supports all options
	testFrame.data[4] = 0x00; // Reserved options = 0
	testFrame.data[5] = 0x01; // Number of booms for section control (1)
	testFrame.data[6] = 0x20; // Number of sections for section control (32)
	testFrame.data[7] = 0x10; // Number channels for position based control (16)
	CANNetworkManager::CANNetwork.can_lib_process_rx_message(testFrame, nullptr);

	CANNetworkManager::CANNetwork.update();

	// Test the values parsed in this state machine state
	EXPECT_EQ(TaskControllerClient::StateMachineState::WaitForRequestVersionFromServer, interfaceUnderTest.test_wrapper_get_state());
	EXPECT_EQ(TaskControllerClient::Version::SecondPublishedEdition, interfaceUnderTest.get_connected_tc_version());
	EXPECT_EQ(0xFF, interfaceUnderTest.get_connected_tc_max_boot_time());
	EXPECT_EQ(true, interfaceUnderTest.get_connected_tc_option_supported(TaskControllerClient::ServerOptions::SupportsDocumentation));
	EXPECT_EQ(true, interfaceUnderTest.get_connected_tc_option_supported(TaskControllerClient::ServerOptions::SupportsImplementSectionControlFunctionality));
	EXPECT_EQ(true, interfaceUnderTest.get_connected_tc_option_supported(TaskControllerClient::ServerOptions::SupportsPeerControlAssignment));
	EXPECT_EQ(true, interfaceUnderTest.get_connected_tc_option_supported(TaskControllerClient::ServerOptions::SupportsTCGEOWithPositionBasedControl));
	EXPECT_EQ(true, interfaceUnderTest.get_connected_tc_option_supported(TaskControllerClient::ServerOptions::SupportsTCGEOWithoutPositionBasedControl));
	EXPECT_EQ(false, interfaceUnderTest.get_connected_tc_option_supported(TaskControllerClient::ServerOptions::ReservedOption1));
	EXPECT_EQ(false, interfaceUnderTest.get_connected_tc_option_supported(TaskControllerClient::ServerOptions::ReservedOption2));
	EXPECT_EQ(false, interfaceUnderTest.get_connected_tc_option_supported(TaskControllerClient::ServerOptions::ReservedOption3));
	EXPECT_EQ(1, interfaceUnderTest.get_connected_tc_number_booms_supported());
	EXPECT_EQ(32, interfaceUnderTest.get_connected_tc_number_sections_supported());
	EXPECT_EQ(16, interfaceUnderTest.get_connected_tc_number_channels_supported());

	// Test Status Message State
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::SendStatusMessage);
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::SendStatusMessage);
	interfaceUnderTest.update();
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::RequestVersion);

	// Test transition to disconnect from NACK
	// Send a NACK
	testFrame.identifier = 0x18E883F7;
	testFrame.data[0] = 0x01; // N-ACK
	testFrame.data[1] = 0xFF;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0x83; // Address
	testFrame.data[5] = 0x00; // PGN
	testFrame.data[6] = 0xCB; // PGN
	testFrame.data[7] = 0x00; // PGN
	CANNetworkManager::CANNetwork.can_lib_process_rx_message(testFrame, nullptr);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::Disconnected);

	// Test send structure request state
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::RequestStructureLabel);
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::RequestStructureLabel);
	interfaceUnderTest.update();
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::WaitForStructureLabelResponse);

	// Test send localization request state
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::RequestLocalizationLabel);
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::RequestLocalizationLabel);
	interfaceUnderTest.update();
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::WaitForLocalizationLabelResponse);

	// Test send delete object pool states
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::SendDeleteObjectPool);
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::SendDeleteObjectPool);
	interfaceUnderTest.update();
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::WaitForDeleteObjectPoolResponse);
	// Send a response
	testFrame.identifier = 0x18CB83F7;
	testFrame.data[0] = 0xB1; // Mux
	testFrame.data[1] = 0xFF; // Ambigious
	testFrame.data[2] = 0xFF; // Ambigious
	testFrame.data[3] = 0xFF; // error details are not available
	testFrame.data[4] = 0xFF; // Reserved
	testFrame.data[5] = 0xFF; // Reserved
	testFrame.data[6] = 0xFF; // Reserved
	testFrame.data[7] = 0xFF; // Reserved
	CANNetworkManager::CANNetwork.can_lib_process_rx_message(testFrame, nullptr);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::SendRequestTransferObjectPool);

	// Test send activate object pool state
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::SendObjectPoolActivate);
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::SendObjectPoolActivate);
	interfaceUnderTest.update();
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::WaitForObjectPoolActivateResponse);

	// Test send deactivate object pool state
	interfaceUnderTest.test_wrapper_set_state(TaskControllerClient::StateMachineState::DeactivateObjectPool);
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::DeactivateObjectPool);
	interfaceUnderTest.update();
	EXPECT_EQ(interfaceUnderTest.test_wrapper_get_state(), TaskControllerClient::StateMachineState::WaitForObjectPoolDeactivateResponse);

	//! @Todo Add other states

	interfaceUnderTest.terminate();
	CANHardwareInterface::stop();
}

TEST(TASK_CONTROLLER_CLIENT_TESTS, ClientSettings)
{
	DerivedTestTCClient interfaceUnderTest(nullptr, nullptr);
	auto blankDDOP = std::make_shared<DeviceDescriptorObjectPool>();

	// Set and test the basic settings for the client
	interfaceUnderTest.configure(blankDDOP, 6, 64, 32, false, false, false, false, false);
	EXPECT_EQ(6, interfaceUnderTest.get_number_booms_supported());
	EXPECT_EQ(64, interfaceUnderTest.get_number_sections_supported());
	EXPECT_EQ(32, interfaceUnderTest.get_number_channels_supported_for_position_based_control());
	EXPECT_EQ(false, interfaceUnderTest.get_supports_documentation());
	EXPECT_EQ(false, interfaceUnderTest.get_supports_implement_section_control());
	EXPECT_EQ(false, interfaceUnderTest.get_supports_peer_control_assignment());
	EXPECT_EQ(false, interfaceUnderTest.get_supports_tcgeo_without_position_based_control());
	EXPECT_EQ(false, interfaceUnderTest.get_supports_tcgeo_with_position_based_control());
	interfaceUnderTest.configure(blankDDOP, 255, 255, 255, true, true, true, true, true);
	EXPECT_EQ(255, interfaceUnderTest.get_number_booms_supported());
	EXPECT_EQ(255, interfaceUnderTest.get_number_sections_supported());
	EXPECT_EQ(255, interfaceUnderTest.get_number_channels_supported_for_position_based_control());
	EXPECT_EQ(true, interfaceUnderTest.get_supports_documentation());
	EXPECT_EQ(true, interfaceUnderTest.get_supports_implement_section_control());
	EXPECT_EQ(true, interfaceUnderTest.get_supports_peer_control_assignment());
	EXPECT_EQ(true, interfaceUnderTest.get_supports_tcgeo_without_position_based_control());
	EXPECT_EQ(true, interfaceUnderTest.get_supports_tcgeo_with_position_based_control());
}
