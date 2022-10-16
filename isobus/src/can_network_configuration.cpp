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
	std::uint32_t CANNetworkConfiguration::minimumTimeBetweenTransportProtocolBAMFrames = 50;

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

	void CANNetworkConfiguration::set_minimum_time_between_transport_protocol_bam_frames(std::uint32_t value)
	{
		constexpr std::uint32_t MAX_BAM_FRAME_DELAY_MS = 200;
		constexpr std::uint32_t MIN_BAM_FRAME_DELAY_MS = 10;

		if ((value <= MAX_BAM_FRAME_DELAY_MS) &&
		    (value >= MIN_BAM_FRAME_DELAY_MS))
		{
			minimumTimeBetweenTransportProtocolBAMFrames = value;
		}
	}

	std::uint32_t CANNetworkConfiguration::get_minimum_time_between_transport_protocol_bam_frames()
	{
		return minimumTimeBetweenTransportProtocolBAMFrames;
	}
}
