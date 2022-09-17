//================================================================================================
/// @file can_general_parameter_group_numbers.hpp
///
/// @brief Defines some PGNs that are used in the library or are very common
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_GENERAL_PARAMETER_GROUP_NUMBERS_HPP
#define CAN_GENERAL_PARAMETER_GROUP_NUMBERS_HPP

namespace isobus
{

	enum class CANLibParameterGroupNumber
	{
		Any = 0x0000,
		WorkingSetMaster = 0xFE0D,
		VirtualTerminalToECU = 0xE600,
		ECUtoVirtualTerminal = 0xE700,
		Acknowledge = 0xE800,
		ParameterGroupNumberRequest = 0xEA00,
		TransportProtocolData = 0xEB00,
		TransportProtocolCommand = 0xEC00,
		AddressClaim = 0xEE00,
		ProprietaryA = 0xEF00
	};

} // namespace isobus

#endif // CAN_GENERAL_PARAMETER_GROUP_NUMBERS_HPP
