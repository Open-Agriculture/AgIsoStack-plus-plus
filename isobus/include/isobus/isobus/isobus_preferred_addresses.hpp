//================================================================================================
/// @file isobus_preferred_addresses.hpp
///
/// @brief This is a reference for control function's preferred addresses as defined by
/// ISO 11783-11 and/or SAE. Preferred addresses are industry group specific.
/// You should use these when your are creating a control function that is a well known
/// function type, but if your control function doesn't arbitrate for that address, the
/// stack will claim for you in the dynamic address range.
///
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_PREFERRED_ADDRESSES_HPP
#define ISOBUS_PREFERRED_ADDRESSES_HPP

#include <cstdint>

namespace isobus
{
	/// @brief This namespace contains all defined preferred addresses for control functions, which
	/// should be used when creating a control function that is a well known function type.
	namespace preferred_addresses
	{
		/// @brief Industry Group 1 applies to on-highway equipment.
		namespace IndustryGroup1
		{
			/// This enumerates all preferred addresses for industry group 1.
			enum PreferredAddress : std::uint8_t
			{
				// 128-158 are reserved for future assignment by SAE but available for use by self configurable ECUs
				AutomatedDrivingController2 = 156,
				ElectricPropulsionControlUnit3 = 157,
				AutomatedDrivingController1 = 158,
				RoadwayInformationSystem = 159,
				AdvancedEmergencyBrakingSystem = 160,
				FifthWheelSmartSystems = 161,
				SlopeSensor = 162,
				CatalystFluidSensor = 163,
				OnBoardDiagnosticUnit2 = 164,
				RearSteeringAxleController2 = 165,
				RearSteeringAxleController3 = 166,
				InstrumentCluster2 = 167,
				Trailer5Bridge = 168,
				Trailer5LightingElectrical = 169,
				Trailer5BrakesABS_EBS = 170,
				Trailer5Reefer = 171,
				Trailer5Cargo = 172,
				Trailer5ChassisSuspension = 173,
				OtherTrailer5Devices = 174,
				OtherTrailer5Devices2 = 175,
				Trailer4Bridge = 176,
				Trailer4LightingElectrical = 177,
				Trailer4BrakesABS_EBS = 178,
				Trailer4Reefer = 179,
				Trailer4Cargo = 180,
				Trailer4ChassisSuspension = 181,
				OtherTrailer4Devices = 182,
				OtherTrailer4Devices2 = 183,
				Trailer3Bridge = 184,
				Trailer3LightingElectrical = 185,
				Trailer3BrakesABS_EBS = 186,
				Trailer3Reefer = 187,
				Trailer3Cargo = 188,
				Trailer3ChassisSuspension = 189,
				OtherTrailer3Devices = 190,
				OtherTrailer3Devices2 = 191,
				Trailer2Bridge = 192,
				Trailer2LightingElectrical = 193,
				Trailer2BrakesABS_EBS = 194,
				Trailer2Reefer = 195,
				Trailer2Cargo = 196,
				Trailer2ChassisSuspension = 197,
				OtherTrailer2Devices = 198,
				OtherTrailer2Devices2 = 199,
				Trailer1Bridge = 200,
				Trailer1LightingElectrical = 201,
				Trailer1BrakesABS_EBS = 202,
				Trailer1Reefer = 203,
				Trailer1Cargo = 204,
				Trailer1ChassisSuspension = 205,
				OtherTrailer1Devices = 206,
				OtherTrailer1Devices2 = 207,
				SteeringBodyUnit = 228,
				BodyController2 = 229,
				BodyToVehicleInterfaceControl = 230,
				ArticulationTurntableControl = 231,
				ForwardRoadImageProcessor = 232,
				DoorController3 = 233,
				DoorController4 = 234,
				TractorTrailerBridge2 = 235,
				DoorController1 = 236,
				DoorController2 = 237,
				Tachograph = 238,
				ElectricPropulsionControlUnit1 = 239,
				ElectricPropulsionControlUnit2 = 240,
				WWH_OBDTester = 241,
				ElectricPropulsionControlUnit4 = 242,
				BatteryPackMonitor1 = 243,
				BatteryPackMonitor2_APU4 = 244,
				BatteryPackMonitor3_APU3 = 245,
				BatteryPackMonitor4_APU2 = 246,
				AuxiliaryPowerUnit_APU1 = 247
			};
		} // namespace IndustryGroup1

		/// @brief Industry Group 2 applies to agricultural and forestry equipment.
		namespace IndustryGroup2
		{
			/// This enumerates all preferred addresses for industry group 2.
			enum PreferredAddress : std::uint8_t
			{
				// 128-235 are reserved by ISO for the self-configurable address capability
				DataLogger = 236,
				TIMServer = 237,
				SequenceController = 238,
				PositionControl = 239,
				TractorECU = 240,
				TailingsMonitoring = 241,
				HeaderControl = 242,
				ProductLossMonitoring = 243,
				ProductMoistureSensing = 244,
				NonVirtualTerminalDisplay_ImplementBus = 245,
				OperatorControls_MachineSpecific = 246,
				TaskController_MappingComputer = 247
			};
		} // namespace IndustryGroup1

		/// @brief Industry Group 3 applies to construction equipment.
		namespace IndustryGroup3
		{
			/// This enumerates all preferred addresses for industry group 3.
			enum PreferredAddress : std::uint8_t
			{
				/// 128 thru 207 are reserved for future assignment by SAE
				/// 208 thru 223 are reserved for future assignment
				RotationSensor = 224,
				LiftArmController = 225,
				SlopeSensor = 226,
				MainController_SkidSteerLoader = 227,
				LoaderControl = 228,
				LaserTracer = 229,
				LandLevelingSystemDisplay = 230,
				SingleLandLevelingSystemSupervisor = 231,
				LandLevelingElectricMast = 232,
				SingleLandLevelingSystemOperatorInterface = 233,
				LaserReceiver = 234,
				SupplementalSensorProcessingUnit1 = 235,
				SupplementalSensorProcessingUnit2 = 236,
				SupplementalSensorProcessingUnit3 = 237,
				SupplementalSensorProcessingUnit4 = 238,
				SupplementalSensorProcessingUnit5 = 239,
				SupplementalSensorProcessingUnit6 = 240,
				EngineMonitor1 = 241,
				EngineMonitor2 = 242,
				EngineMonitor3 = 243,
				EngineMonitor4 = 244,
				EngineMonitor5 = 245,
				EngineMonitor6 = 246,
				EngineMonitor7 = 247
			};
		} // namespace IndustryGroup3

		/// @brief Industry Group 4 applies to marine equipment.
		namespace IndustryGroup4
		{
			/// This enumerates all preferred addresses for industry group 4.
			enum PreferredAddress : std::uint8_t
			{
				/// 128 thru 207 are reserved for future assignment by SAE
				/// 208 thru 227 are reserved for future assignment
				PropulsionSensorHubAndGateway1 = 228,
				PropulsionSensorHubAndGateway2 = 229,
				PropulsionSensorHubAndGateway3 = 230,
				PropulsionSensorHubAndGateway4 = 231,
				Transmission3 = 232,
				Transmission4 = 233,
				Transmission5 = 234,
				Transmission6 = 235,
				Display1forProtectionSystemforMarineEngines = 236,
				ProtectionSystemforMarineEngines = 237,
				AlarmSystemControl1forMarineEngines = 238,
				Engine3 = 239,
				Engine4 = 240,
				Engine5 = 241,
				MarineDisplay1 = 242,
				MarineDisplay2 = 243,
				MarineDisplay3 = 244,
				MarineDisplay4 = 245,
				MarineDisplay5 = 246,
				MarineDisplay6 = 247
			};
		} // namespace IndustryGroup4

		/// @brief Industry Group 5 applies to industrial process control stationary equipment (Gen-sets).
		namespace IndustryGroup5
		{
			/// This enumerates all preferred addresses for industry group 5.
			enum PreferredAddress : std::uint8_t
			{
				/// 128 thru 207 are reserved for future assignment by SAE
				/// 208 thru 229 are reserved for future assignment
				GeneratorVoltageRegulator = 230,
				Engine3 = 231,
				Engine4 = 232,
				Engine5 = 233,
				GeneratorSetController = 234,
				SupplementalSensorProcessingUnit1 = 235,
				SupplementalSensorProcessingUnit2 = 236,
				SupplementalSensorProcessingUnit3 = 237,
				SupplementalSensorProcessingUnit4 = 238,
				SupplementalSensorProcessingUnit5 = 239,
				SupplementalSensorProcessingUnit6 = 240,
				EngineMonitor1 = 241,
				EngineMonitor2 = 242,
				EngineMonitor3 = 243,
				EngineMonitor4 = 244,
				EngineMonitor5 = 245,
				EngineMonitor6 = 246,
				EngineMonitor7 = 247
			};
		} // namespace IndustryGroup5
	} // namespace preferred_addresses
} // namespace isobus
#endif // ISOBUS_PREFERRED_ADDRESSES_HPP
