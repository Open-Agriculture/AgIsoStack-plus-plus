#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/isobus_file_server_client.hpp"
#include "isobus/utility/system_timing.hpp"

using namespace isobus;

class DerivedTestFileServerClient : public FileServerClient
{
public:
	DerivedTestFileServerClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  FileServerClient(partner, clientSource){};

	bool test_wrapper_send_change_current_directory_request(std::string path)
	{
		return FileServerClient::send_change_current_directory_request(path);
	}

	bool test_wrapper_send_client_connection_maintenance() const
	{
		return send_client_connection_maintenance();
	}

	bool test_wrapper_send_close_file(std::shared_ptr<FileServerClient::FileInfo> fileMetadata) const
	{
		return FileServerClient::send_close_file(fileMetadata);
	}

	bool test_wrapper_send_get_file_server_properties() const
	{
		return send_get_file_server_properties();
	}

	bool test_wrapper_send_open_file(std::shared_ptr<FileServerClient::FileInfo> fileMetadata) const
	{
		return send_open_file(fileMetadata);
	}

	void test_wrapper_set_state(FileServerClient::StateMachineState newState)
	{
		FileServerClient::set_state(newState);
	}

	void test_wrapper_set_state(FileServerClient::StateMachineState newState, std::uint32_t timestamp_ms)
	{
		FileServerClient::set_state(newState, timestamp_ms);
	}

	bool test_wrapper_request_current_volume_status(std::string volumeName) const
	{
		return FileServerClient::request_current_volume_status(volumeName);
	}
};

TEST(FILE_SERVER_CLIENT_TESTS, StateMachineTests)
{
	VirtualCANPlugin serverFS;
	serverFS.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::add_can_lib_update_callback(
	  [](void *) {
		  CANNetworkManager::CANNetwork.update();
	  },
	  nullptr);
	CANHardwareInterface::start();

	NAME clientNAME(0);
	clientNAME.set_industry_group(2);
	clientNAME.set_ecu_instance(4);
	clientNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::DriveAxleControlBrakes));
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x93, 0);

	HardwareInterfaceCANFrame testFrame;

	std::uint32_t waitingTimestamp_ms = SystemTiming::get_timestamp_ms();

	while ((!internalECU->get_address_valid()) &&
	       (!SystemTiming::time_expired_ms(waitingTimestamp_ms, 2000)))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ASSERT_TRUE(internalECU->get_address_valid());

	std::vector<isobus::NAMEFilter> fsNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::FileServer));
	fsNameFilters.push_back(testFilter);

	auto tcPartner = std::make_shared<PartneredControlFunction>(0, fsNameFilters);

	// Force claim a partner
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EEFF22;
	testFrame.data[0] = 0x03;
	testFrame.data[1] = 0x04;
	testFrame.data[2] = 0x00;
	testFrame.data[3] = 0x12;
	testFrame.data[4] = 0x00;
	testFrame.data[5] = 0x52;
	testFrame.data[6] = 0x00;
	testFrame.data[7] = 0xA0;
	CANNetworkManager::CANNetwork.can_lib_process_rx_message(testFrame, nullptr);

	DerivedTestFileServerClient interfaceUnderTest(tcPartner, internalECU);

	// Test initial state
	EXPECT_EQ(FileServerClient::StateMachineState::Disconnected, interfaceUnderTest.get_state());
}

TEST(FILE_SERVER_CLIENT_TESTS, MessageEncoding)
{
	VirtualCANPlugin serverFS;
	serverFS.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::add_can_lib_update_callback(
	  [](void *) {
		  CANNetworkManager::CANNetwork.update();
	  },
	  nullptr);
	CANHardwareInterface::start();

	NAME clientNAME(0);
	clientNAME.set_industry_group(2);
	clientNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::AlarmDevice));
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x90, 0);

	HardwareInterfaceCANFrame testFrame;

	std::uint32_t waitingTimestamp_ms = SystemTiming::get_timestamp_ms();

	while ((!internalECU->get_address_valid()) &&
	       (!SystemTiming::time_expired_ms(waitingTimestamp_ms, 2000)))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ASSERT_TRUE(internalECU->get_address_valid());

	std::vector<isobus::NAMEFilter> fsNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::FileServer));
	fsNameFilters.push_back(testFilter);

	auto fileServerPartner = std::make_shared<PartneredControlFunction>(0, fsNameFilters);

	// Force claim a partner
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EEFF23;
	testFrame.data[0] = 0x03;
	testFrame.data[1] = 0x04;
	testFrame.data[2] = 0x00;
	testFrame.data[3] = 0x12;
	testFrame.data[4] = 0x00;
	testFrame.data[5] = 0x52;
	testFrame.data[6] = 0x00;
	testFrame.data[7] = 0xA0;
	CANNetworkManager::CANNetwork.can_lib_process_rx_message(testFrame, nullptr);

	DerivedTestFileServerClient interfaceUnderTest(fileServerPartner, internalECU);

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	// Get the virtual CAN plugin back to a known state
	while (!serverFS.get_queue_empty())
	{
		serverFS.read_frame(testFrame);
	}
	ASSERT_TRUE(serverFS.get_queue_empty());

	EXPECT_EQ(true, interfaceUnderTest.test_wrapper_send_client_connection_maintenance());

	ASSERT_TRUE(serverFS.read_frame(testFrame));

	ASSERT_TRUE(testFrame.isExtendedFrame);
	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xAA00);
	EXPECT_EQ(0x00, testFrame.data[0]); // Mux
	EXPECT_EQ(0x03, testFrame.data[1]); // Version

	for (std::uint_fast8_t i = 2; i < 8; i++)
	{
		// Check Reserved Bytes
		EXPECT_EQ(testFrame.data[i], 0xFF);
	}

	EXPECT_EQ(true, interfaceUnderTest.test_wrapper_send_get_file_server_properties());
	ASSERT_TRUE(serverFS.read_frame(testFrame));

	ASSERT_TRUE(testFrame.isExtendedFrame);
	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xAA00);
	EXPECT_EQ(0x01, testFrame.data[0]); // Mux
	for (std::uint_fast8_t i = 1; i < 8; i++)
	{
		// Check Reserved Bytes
		EXPECT_EQ(testFrame.data[i], 0xFF);
	}

	EXPECT_EQ(true, interfaceUnderTest.test_wrapper_request_current_volume_status("~/"));
	ASSERT_TRUE(serverFS.read_frame(testFrame));

	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xAA00);
	EXPECT_EQ(0x02, testFrame.data[0]); // Mux
	EXPECT_EQ(0x00, testFrame.data[1]); // Mode
	EXPECT_EQ(0x02, testFrame.data[2]); // Length LSB
	EXPECT_EQ(0x00, testFrame.data[3]); // Length MSB
	EXPECT_EQ('~', testFrame.data[4]); // Path
	EXPECT_EQ('/', testFrame.data[5]); // Path
	EXPECT_EQ(0xFF, testFrame.data[6]); // Reserved (due to length of 2)
	EXPECT_EQ(0xFF, testFrame.data[7]); // Reserved (due to length of 2)
}
