#include "isobus/isobus/can_NAME.hpp"
#include "isobus/isobus/can_network_manager.hpp"

#include <cstring>
#include <future>
#include <thread>

#include "control_function_helpers.hpp"
#include "gtest/gtest.h"

namespace test_helpers
{
	using namespace isobus;

	NAME find_available_name(std::uint8_t canPort)
	{
		// We use a reserved function for testing purposes: https://www.isobus.net/isobus/nameFunction/95
		NAME name(0);
		name.set_arbitrary_address_capable(true);
		name.set_industry_group(0); // Global
		name.set_device_class(0); // Non-specific System
		name.set_device_class_instance(0);
		name.set_manufacturer_code(1407); // Open-Agriculture
		name.set_function_code(128); // Reserved
		name.set_function_instance(0);
		name.set_identity_number(0);
		name.set_ecu_instance(canPort);

		// By design, this loop will never end if there are no unused names
		bool unusedNAMEfound = false;
		while (!unusedNAMEfound)
		{
			unusedNAMEfound = true;
			for (auto function : CANNetworkManager::CANNetwork.get_control_functions(true))
			{
				if (function->get_NAME() == name)
				{
					name.set_identity_number(name.get_identity_number() + 1); // We increment the identity number until we find an unused NAME
					unusedNAMEfound = false;
					break;
				}
			}
		}

		return name;
	}

	bool is_address_occupied(std::uint8_t address, std::uint8_t canPort)
	{
		bool addressOccupied = false;
		for (auto function : CANNetworkManager::CANNetwork.get_control_functions(false))
		{
			if ((function->get_can_port() == canPort) && (function->get_address() == address))
			{
				addressOccupied = true;
				break;
			}
		}
		return addressOccupied;
	}

	std::shared_ptr<InternalControlFunction> claim_internal_control_function(std::uint8_t address, std::uint8_t canPort)
	{
		EXPECT_FALSE(is_address_occupied(address, canPort));

		NAME name = find_available_name(canPort);
		auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(name, canPort, address);

		// Make sure address claiming is done before we return
		auto addressClaimedFuture = std::async(std::launch::async, [&internalECU]() {
			while (!internalECU->get_address_valid())
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
		});

		// If this fails, probably the update thread is not started
		EXPECT_TRUE(addressClaimedFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);
		EXPECT_TRUE(internalECU->get_address_valid());

		// Fail if we didn't get the address we wanted, since we expect this when testing with identifiers
		EXPECT_EQ(internalECU->get_address(), address);

		return internalECU;
	}

	std::shared_ptr<PartneredControlFunction> force_claim_partnered_control_function(std::uint8_t address, std::uint8_t canPort)
	{
		NAME name = find_available_name(canPort);

		std::vector<NAMEFilter> NAMEFilters{
			NAMEFilter(NAME::NAMEParameters::IdentityNumber, name.get_identity_number()),
			NAMEFilter(NAME::NAMEParameters::ManufacturerCode, name.get_manufacturer_code()),
			NAMEFilter(NAME::NAMEParameters::EcuInstance, name.get_ecu_instance()),
			NAMEFilter(NAME::NAMEParameters::FunctionInstance, name.get_function_instance()),
			NAMEFilter(NAME::NAMEParameters::FunctionCode, name.get_function_code()),
			NAMEFilter(NAME::NAMEParameters::DeviceClass, name.get_device_class()),
			NAMEFilter(NAME::NAMEParameters::DeviceClassInstance, name.get_device_class_instance()),
			NAMEFilter(NAME::NAMEParameters::IndustryGroup, name.get_industry_group()),
			NAMEFilter(NAME::NAMEParameters::ArbitraryAddressCapable, name.get_arbitrary_address_capable())
		};
		auto partnerECU = CANNetworkManager::CANNetwork.create_partnered_control_function(canPort, NAMEFilters);

		// Force claim message
		CANMessageFrame testFrame = {};
		testFrame.channel = canPort;
		testFrame.identifier = 0x18EEFF00 | address;
		testFrame.isExtendedFrame = true;
		testFrame.dataLength = 8;
		testFrame.data[0] = static_cast<std::uint8_t>(name.get_full_name());
		testFrame.data[1] = static_cast<std::uint8_t>(name.get_full_name() >> 8);
		testFrame.data[2] = static_cast<std::uint8_t>(name.get_full_name() >> 16);
		testFrame.data[3] = static_cast<std::uint8_t>(name.get_full_name() >> 24);
		testFrame.data[4] = static_cast<std::uint8_t>(name.get_full_name() >> 32);
		testFrame.data[5] = static_cast<std::uint8_t>(name.get_full_name() >> 40);
		testFrame.data[6] = static_cast<std::uint8_t>(name.get_full_name() >> 48);
		testFrame.data[7] = static_cast<std::uint8_t>(name.get_full_name() >> 56);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);

		EXPECT_TRUE(partnerECU->get_address_valid());

		// Fail if we didn't get the address we wanted, since we expect this when testing with identifiers
		EXPECT_EQ(partnerECU->get_address(), address);
		return partnerECU;
	}

	class WrappedControlFunction : public ControlFunction
	{
	public:
		WrappedControlFunction(NAME name, std::uint8_t address, std::uint8_t canPort) :
		  ControlFunction(name, address, canPort) {}
	};

	std::shared_ptr<isobus::ControlFunction> create_mock_control_function(std::uint8_t address)
	{
		return std::make_shared<WrappedControlFunction>(NAME(0), address, 0);
	}

	class WrappedInternalControlFunction : public InternalControlFunction
	{
	public:
		WrappedInternalControlFunction(NAME name, std::uint8_t address, std::uint8_t canPort) :
		  InternalControlFunction(name, address, canPort)
		{
			// We need to set the address manually, since there won't be an address claim state machine running
			ControlFunction::address = address;
		}
	};

	std::shared_ptr<isobus::InternalControlFunction> create_mock_internal_control_function(std::uint8_t address)
	{
		return std::make_shared<WrappedInternalControlFunction>(NAME(0), address, 0);
	}

} // namespace test_helpers
