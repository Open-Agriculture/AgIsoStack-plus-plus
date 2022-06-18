#pragma once

#include <cstdint>

namespace isobus
{
    class CANLibConfiguration
    {
    public:
        CANLibConfiguration();
        ~CANLibConfiguration();

        static void set_max_number_transport_protcol_sessions(std::uint32_t value);
        static std::uint32_t get_max_number_transport_protcol_sessions();

    private:
        static std::uint32_t maxNumberTransportProtocolSessions;
    };
} // namespace isobus
