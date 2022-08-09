//================================================================================================
/// @file can_network_configuration.hpp
///
/// @brief This is a class for changing stack settings.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_NETWORK_CONFIGURATION_HPP
#define CAN_NETWORK_CONFIGURAION_HPP

#include <cstdint>

namespace isobus
{
    class CANNetworkConfiguration
    {
    public:
        CANNetworkConfiguration();
        ~CANNetworkConfiguration();

        static void set_max_number_transport_protcol_sessions(std::uint32_t value);
        static std::uint32_t get_max_number_transport_protcol_sessions();

    private:
        static std::uint32_t maxNumberTransportProtocolSessions;
    };
} // namespace isobus

#endif // CAN_NETWORK_CONFIGURATION_HPP
