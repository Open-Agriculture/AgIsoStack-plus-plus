//================================================================================================
/// @file can_network_configuration.cpp
///
/// @brief This is a class for changing stack settings.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "can_network_configuration.hpp"

namespace isobus
{
    std::uint32_t CANNetworkConfiguration::maxNumberTransportProtocolSessions = 4;

	CANNetworkConfiguration::CANNetworkConfiguration()
	{
	}

	CANNetworkConfiguration::~CANNetworkConfiguration()
	{
	}

	void CANNetworkConfiguration::set_max_number_transport_protcol_sessions(std::uint32_t value)
	{
		maxNumberTransportProtocolSessions = value;
	}

	std::uint32_t CANNetworkConfiguration::get_max_number_transport_protcol_sessions()
	{
		return maxNumberTransportProtocolSessions;
	}
}
