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
		AgriculturalGuidanceMachineInfo = 0xAC00,
		AgriculturalGuidanceSystemCommand = 0xAD00,
		DiagnosticMessage22 = 0xC300,
		ExtendedTransportProtocolDataTransfer = 0xC700,
		ExtendedTransportProtocolConnectionManagement = 0xC800,
		ProcessData = 0xCB00,
		RequestForRepetitionRate = 0xCC00,
		DiagnosticMessage13 = 0xDF00,
		VirtualTerminalToECU = 0xE600,
		ECUtoVirtualTerminal = 0xE700,
		Acknowledge = 0xE800,
		ParameterGroupNumberRequest = 0xEA00,
		TransportProtocolDataTransfer = 0xEB00,
		TransportProtocolConnectionManagement = 0xEC00,
		AddressClaim = 0xEE00,
		ProprietaryA = 0xEF00,
		MachineSelectedSpeed = 0xF022,
		ProductIdentification = 0xFC8D,
		ControlFunctionFunctionalities = 0xFC8E,
		DiagnosticProtocolIdentification = 0xFD32,
		MachineSelectedSpeedCommand = 0xFD43,
		WorkingSetMaster = 0xFE0D,
		LanguageCommand = 0xFE0F,
		MaintainPower = 0xFE47,
		WheelBasedSpeedAndDistance = 0xFE48,
		GroundBasedSpeedAndDistance = 0xFE49,
		ECUIdentificationInformation = 0xFDC5,
		DiagnosticMessage1 = 0xFECA,
		DiagnosticMessage2 = 0xFECB,
		DiagnosticMessage3 = 0xFECC,
		DiagnosticMessage11 = 0xFED3,
		CommandedAddress = 0xFED8,
		SoftwareIdentification = 0xFEDA,
		AllImplementsStopOperationsSwitchState = 0xFD02,
		VesselHeading = 0x1F112,
		RateOfTurn = 0x1F113,
		PositionRapidUpdate = 0x1F801,
		CourseOverGroundSpeedOverGroundRapidUpdate = 0x1F802,
		PositionDeltaHighPrecisionRapidUpdate = 0x1F803,
		GNSSPositionData = 0x1F805,
		Datum = 0x1F814
	};

} // namespace isobus

#endif // CAN_GENERAL_PARAMETER_GROUP_NUMBERS_HPP
