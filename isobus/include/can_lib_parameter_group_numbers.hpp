#pragma once

namespace isobus
{

enum class CANLibParameterGroupNumber
{
    Any = 0x0000,
    Acknowledge = 0xE800,
    ParameterGroupNumberRequest = 0xEA00,
    TransportProtocolData = 0xEB00,
    TransportProtocolCommand = 0xEC00,
    AddressClaim = 0xEE00,
    ProprietaryA = 0xEF00
};

} // namespace isobus

