#include "can_lib_configuration.hpp"

namespace isobus
{
    std::uint32_t CANLibConfiguration::maxNumberTransportProtocolSessions = 4;

	CANLibConfiguration::CANLibConfiguration()
	{
	}

	CANLibConfiguration::~CANLibConfiguration()
	{
	}

	void CANLibConfiguration::set_max_number_transport_protcol_sessions(std::uint32_t value)
	{
		maxNumberTransportProtocolSessions = value;
	}

	std::uint32_t CANLibConfiguration::get_max_number_transport_protcol_sessions()
	{
		return maxNumberTransportProtocolSessions;
	}
}
