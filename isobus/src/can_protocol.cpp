//================================================================================================
/// @file can_protocol.cpp
///
/// @brief A base class for all protocol classes. Allows the network manager to update them
/// in a generic, dynamic way.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_protocol.hpp"

#include "isobus/isobus/can_network_manager.hpp"

#include <algorithm>

namespace isobus
{
	CANLibProtocol::CANLibProtocol() :
	  initialized(false)
	{
		CANNetworkManager::CANNetwork.protocolList.push_back(this);
	}

	CANLibProtocol::~CANLibProtocol()
	{
		auto protocolLocation = find(CANNetworkManager::CANNetwork.protocolList.begin(), CANNetworkManager::CANNetwork.protocolList.end(), this);

		if (CANNetworkManager::CANNetwork.protocolList.end() != protocolLocation)
		{
			CANNetworkManager::CANNetwork.protocolList.erase(protocolLocation);
		}
	}

	bool CANLibProtocol::get_is_initialized() const
	{
		return initialized;
	}

	bool CANLibProtocol::get_protocol(std::uint32_t index, CANLibProtocol *&returnedProtocol)
	{
		returnedProtocol = nullptr;

		if (index < CANNetworkManager::CANNetwork.protocolList.size())
		{
			returnedProtocol = CANNetworkManager::CANNetwork.protocolList[index];
		}
		return (nullptr != returnedProtocol);
	}

	std::size_t CANLibProtocol::get_number_protocols()
	{
		return CANNetworkManager::CANNetwork.protocolList.size();
	}

	void CANLibProtocol::initialize(CANLibBadge<CANNetworkManager>)
	{
		initialized = true;
	}

} // namespace isobus
