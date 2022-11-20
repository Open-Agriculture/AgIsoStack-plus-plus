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
		Any = 0x0000,
		DiagnosticMessage22 = 0xC300,
		ExtendedTransportProtocolDataTransfer = 0xC700,
		ExtendedTransportProtocolConnectionManagement = 0xC800,
		RequestForRepetitionRate = 0xCC00,
		DiagnosticMessage13 = 0xDF00,
		VirtualTerminalToECU = 0xE600,
		ECUtoVirtualTerminal = 0xE700,
		Acknowledge = 0xE800,
		ParameterGroupNumberRequest = 0xEA00,
		TransportProtocolData = 0xEB00,
		TransportProtocolCommand = 0xEC00,
		AddressClaim = 0xEE00,
		ProprietaryA = 0xEF00,
		ProductIdentification = 0xFC8D,
		DiagnosticProtocolIdentification = 0xFD32,
		WorkingSetMaster = 0xFE0D,
		ECUIdentificationInformation = 0xFDC5,
		DiagnosticMessage1 = 0xFECA,
		DiagnosticMessage2 = 0xFECB,
		DiagnosticMessage3 = 0xFECC,
		DiagnosticMessage11 = 0xFED3,
		SoftwareIdentification = 0xFEDA
	};

} // namespace isobus

#endif // CAN_GENERAL_PARAMETER_GROUP_NUMBERS_HPP
