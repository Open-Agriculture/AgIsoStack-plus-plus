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
	/// @brief PGNs commonly used by the CAN stack
	enum class CANLibParameterGroupNumber
	{
		Any = 0x0000, ///< A fake PGN used internally to denote the superset of all PGNs
		WorkingSetMaster = 0xFE0D, ///< Working set master PGN
		VirtualTerminalToECU = 0xE600, ///< VT to ECU PGN
		ECUtoVirtualTerminal = 0xE700, ///< ECU to VT PGN
		Acknowledge = 0xE800, ///< ACK PGN
		ParameterGroupNumberRequest = 0xEA00, ///< PGN Request
		TransportProtocolData = 0xEB00, ///< TP Data PGN
		TransportProtocolCommand = 0xEC00, ///< TP Command PGN
		AddressClaim = 0xEE00, ///< Address claim PGN
		ProprietaryA = 0xEF00 ///< Proprietary A PGN
	};

} // namespace isobus

#endif // CAN_GENERAL_PARAMETER_GROUP_NUMBERS_HPP
