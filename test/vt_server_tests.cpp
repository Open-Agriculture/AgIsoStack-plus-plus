//================================================================================================
/// @file vt_server_tests.cpp
///
/// @brief Unit tests for the VirtualTerminalServer class, focused on working-set connection
/// management (maintenance timeout, timeout-then-reconnect, and the associated NACK behavior).
/// @author Open-Agriculture
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/test_fixture.hpp"

using namespace isobus;

class DerivedTestVTServer : public VirtualTerminalServer
{
public:
	explicit DerivedTestVTServer(std::shared_ptr<InternalControlFunction> controlFunctionToUse) :
	  VirtualTerminalServer(controlFunctionToUse)
	{
	}

	bool get_is_enough_memory(std::uint32_t) const override
	{
		return true;
	}

	VTVersion get_version() const override
	{
		return VTVersion::Version3;
	}

	std::uint8_t get_number_of_navigation_soft_keys() const override
	{
		return 0;
	}

	std::uint8_t get_soft_key_descriptor_x_pixel_width() const override
	{
		return 0;
	}

	std::uint8_t get_soft_key_descriptor_y_pixel_height() const override
	{
		return 0;
	}

	std::uint8_t get_number_of_possible_virtual_soft_keys_in_soft_key_mask() const override
	{
		return 0;
	}

	std::uint8_t get_number_of_physical_soft_keys() const override
	{
		return 0;
	}

	std::uint16_t get_data_mask_area_size_x_pixels() const override
	{
		return 0;
	}

	std::uint16_t get_data_mask_area_size_y_pixels() const override
	{
		return 0;
	}

	void suspend_working_set(std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) override
	{
	}

	SupportedWideCharsErrorCode get_supported_wide_chars(std::uint8_t, std::uint16_t, std::uint16_t, std::uint8_t &numberOfRanges, std::vector<std::uint8_t> &) override
	{
		numberOfRanges = 0;
		return static_cast<SupportedWideCharsErrorCode>(0);
	}

	std::vector<std::array<std::uint8_t, 7>> get_versions(NAME) override
	{
		return {};
	}

	std::vector<std::uint8_t> get_supported_objects() const override
	{
		return {};
	}

	std::vector<std::uint8_t> load_version(const std::vector<std::uint8_t> &, NAME) override
	{
		return {};
	}

	bool save_version(const std::vector<std::uint8_t> &, const std::vector<std::uint8_t> &, NAME) override
	{
		return true;
	}

	bool delete_version(const std::vector<std::uint8_t> &, NAME) override
	{
		return true;
	}

	bool delete_all_versions(NAME) override
	{
		return true;
	}

	bool delete_object_pool(NAME) override
	{
		return true;
	}

	// ----------- Test wrappers into protected/private behavior -----------------------

	bool test_wrapper_check_if_source_is_managed(const CANMessage &message)
	{
		return check_if_source_is_managed(message);
	}

	std::size_t test_wrapper_get_number_of_managed_working_sets() const
	{
		return managedWorkingSetList.size();
	}

	std::shared_ptr<ControlFunction> test_wrapper_get_managed_working_set_control_function(std::size_t index) const
	{
		return managedWorkingSetList.at(index)->get_control_function();
	}

	std::uint32_t test_wrapper_get_managed_working_set_timestamp(std::size_t index) const
	{
		return managedWorkingSetList.at(index)->get_working_set_maintenance_message_timestamp_ms();
	}

	static constexpr std::uint32_t TIMEOUT_MS = VirtualTerminalServer::WORKING_SET_MAINTENANCE_TIMEOUT_MS;
};

/// @brief Builds a Working Set Maintenance message with the initiating flag set as requested.
static CANMessage make_working_set_maintenance_message(bool initiating, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination)
{
	const std::array<std::uint8_t, CAN_DATA_LENGTH> data = {
		0xFF, // WorkingSetMaintenanceMessage mux
		static_cast<std::uint8_t>(initiating ? 0x01 : 0x00),
		3, // VT version byte
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF
	};
	return CANMessage(CANMessage::Type::Receive,
	                  CANIdentifier(CANIdentifier::Type::Extended, static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), CANIdentifier::CANPriority::PriorityLowest7, destination->get_address(), source->get_address()),
	                  data.data(),
	                  static_cast<std::uint32_t>(data.size()),
	                  source,
	                  destination,
	                  0);
}

/// @brief Builds a Get Memory Message, a connection-dependent ECU->VT message that is not the maintenance message.
static CANMessage make_get_memory_message(std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination)
{
	const std::array<std::uint8_t, CAN_DATA_LENGTH> data = { 0xC0, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };
	return CANMessage(CANMessage::Type::Receive,
	                  CANIdentifier(CANIdentifier::Type::Extended, static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), CANIdentifier::CANPriority::PriorityLowest7, destination->get_address(), source->get_address()),
	                  data.data(),
	                  static_cast<std::uint32_t>(data.size()),
	                  source,
	                  destination,
	                  0);
}

class VirtualTerminalServerTest : public AgIsoStackTestFixture
{
	// Wrapper to give tests a more meaningful name - no content.
};

TEST_F(VirtualTerminalServerTest, TimeoutIsRefreshedByAnyMessageNotJustMaintenance)
{
	auto internalECU = test_helpers::create_mock_internal_control_function(0x26);
	auto client = test_helpers::create_mock_control_function(0x81);
	DerivedTestVTServer serverUnderTest(internalECU);

	ASSERT_TRUE(serverUnderTest.test_wrapper_check_if_source_is_managed(make_working_set_maintenance_message(true, client, internalECU)));
	ASSERT_EQ(1U, serverUnderTest.test_wrapper_get_number_of_managed_working_sets());

	// Advance almost to the timeout, then refresh via a non-maintenance message.
	time_source.update_for_ms(DerivedTestVTServer::TIMEOUT_MS - 500);
	EXPECT_TRUE(serverUnderTest.test_wrapper_check_if_source_is_managed(make_get_memory_message(client, internalECU)));

	// If the non-maintenance message had not refreshed the timeout, this would now be considered timed out.
	time_source.update_for_ms(DerivedTestVTServer::TIMEOUT_MS - 500);
	EXPECT_TRUE(serverUnderTest.test_wrapper_check_if_source_is_managed(make_get_memory_message(client, internalECU)));
}

TEST_F(VirtualTerminalServerTest, ClientIsNotManagedAfterTimeoutUntilItReAnnounces)
{
	auto internalECU = test_helpers::create_mock_internal_control_function(0x26);
	auto client = test_helpers::create_mock_control_function(0x81);
	DerivedTestVTServer serverUnderTest(internalECU);

	ASSERT_TRUE(serverUnderTest.test_wrapper_check_if_source_is_managed(make_working_set_maintenance_message(true, client, internalECU)));

	time_source.update_for_ms(DerivedTestVTServer::TIMEOUT_MS + 1);

	// A non-maintenance message from a timed-out client must not be treated as managed.
	EXPECT_FALSE(serverUnderTest.test_wrapper_check_if_source_is_managed(make_get_memory_message(client, internalECU)));

	// Re-announcing with a new Working Set Master message re-accepts the client.
	EXPECT_TRUE(serverUnderTest.test_wrapper_check_if_source_is_managed(make_working_set_maintenance_message(true, client, internalECU)));
	EXPECT_TRUE(serverUnderTest.test_wrapper_check_if_source_is_managed(make_get_memory_message(client, internalECU)));
}

TEST_F(VirtualTerminalServerTest, TimedOutClientIsReacceptedWhenSameNameClaimsItsAddressAgain)
{
	auto internalECU = test_helpers::create_mock_internal_control_function(0x26);
	auto originalClientNAME = test_helpers::find_available_name(0);
	auto client = std::make_shared<isobus::ControlFunction>(originalClientNAME, 0x81, 0);
	DerivedTestVTServer serverUnderTest(internalECU);

	ASSERT_TRUE(serverUnderTest.test_wrapper_check_if_source_is_managed(make_working_set_maintenance_message(true, client, internalECU)));

	time_source.update_for_ms(DerivedTestVTServer::TIMEOUT_MS + 1);

	// Simulate the network manager replacing the control function instance for the same NAME,
	// such as after an address-claim roll call evicts and restores it (a different object, same identity).
	auto reclaimedClient = std::make_shared<isobus::ControlFunction>(originalClientNAME, 0x81, 0);
	ASSERT_NE(reclaimedClient, client);

	// Even though this is not a Working Set Master message, the matching NAME is evidence that
	// this working set master is still on the bus, so it should be re-accepted.
	EXPECT_TRUE(serverUnderTest.test_wrapper_check_if_source_is_managed(make_get_memory_message(reclaimedClient, internalECU)));
	ASSERT_EQ(1U, serverUnderTest.test_wrapper_get_number_of_managed_working_sets());
	EXPECT_EQ(reclaimedClient, serverUnderTest.test_wrapper_get_managed_working_set_control_function(0));
}

TEST_F(VirtualTerminalServerTest, NackForTimedOutWorkingSetIsSentToTheWorkingSetMasterNotGlobally)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x26, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x81, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();
	testPlugin.clear_queue();

	// An End of Object Pool message from a client that never registered with the server should be
	// NACKed (it is connection-dependent, unlike the stateless Get Memory/Hardware/etc. messages),
	// and per ISO 11783-6:2014 4.6.9 that NACK must be sent to the working set master, not broadcast.
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(7,
	                                                                                                    static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
	                                                                                                    internalECU,
	                                                                                                    client,
	                                                                                                    { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));
	CANNetworkManager::CANNetwork.update();

	CANMessageFrame nackFrame = {};
	time_source.update_for_ms(5);
	ASSERT_TRUE(testPlugin.read_frame(nackFrame));

	const std::uint32_t expectedUnicastIdentifier = test_helpers::create_ext_can_id(static_cast<std::uint8_t>(CANIdentifier::CANPriority::PriorityLowest7),
	                                                                                static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge),
	                                                                                client,
	                                                                                internalECU);
	EXPECT_EQ(expectedUnicastIdentifier, nackFrame.identifier);
	EXPECT_EQ(0x01, nackFrame.data[0]); // Negative Acknowledgement
	EXPECT_EQ(client->get_address(), nackFrame.data[4]); // Address of the CF the NACK is about

	CANHardwareInterface::stop();
}

TEST_F(VirtualTerminalServerTest, ObjectPoolTransferFromUnregisteredClientIsNotProcessed)
{
	auto internalECU = test_helpers::create_mock_internal_control_function(0x26);
	auto client = test_helpers::create_mock_control_function(0x81);
	DerivedTestVTServer serverUnderTest(internalECU);

	ASSERT_TRUE(serverUnderTest.test_wrapper_check_if_source_is_managed(make_working_set_maintenance_message(true, client, internalECU)));

	// Let the working set time out without ever tearing it down.
	time_source.update_for_ms(DerivedTestVTServer::TIMEOUT_MS + 1);

	const std::array<std::uint8_t, CAN_DATA_LENGTH> objectPoolTransferData = { 0x11, 0, 0, 0, 0, 0, 0, 0 };
	CANMessage objectPoolTransfer(CANMessage::Type::Receive,
	                              CANIdentifier(CANIdentifier::Type::Extended, static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), CANIdentifier::CANPriority::PriorityLowest7, internalECU->get_address(), client->get_address()),
	                              objectPoolTransferData.data(),
	                              static_cast<std::uint32_t>(objectPoolTransferData.size()),
	                              client,
	                              internalECU,
	                              0);

	// A timed-out client is not managed, so its Object Pool Transfer must be rejected rather than
	// handed to process_connection_dependent_messages() to be reassembled and stored.
	EXPECT_FALSE(serverUnderTest.test_wrapper_check_if_source_is_managed(objectPoolTransfer));
}
