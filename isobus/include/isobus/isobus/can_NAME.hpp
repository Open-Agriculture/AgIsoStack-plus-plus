//================================================================================================
/// @file can_NAME.hpp
///
/// @brief A class that represents a control function's NAME
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_NAME_HPP
#define CAN_NAME_HPP

#include <cstdint>
#include <tuple>

namespace isobus
{
	//================================================================================================
	/// @class NAME
	///
	/// @brief A class that represents an ISO11783 control function NAME from an address claim.
	//================================================================================================
	class NAME
	{
	public:
		/// @brief The encoded components that comprise a NAME
		enum class NAMEParameters
		{
			IdentityNumber, ///< Usually the serial number of the ECU, unique for all similar control functions
			ManufacturerCode, ///< The J1939/ISO11783 manufacturer code of the ECU with this NAME
			EcuInstance, ///< The ECU instance of the ECU with this NAME. Usually increments in NAME order with similar CFs
			FunctionInstance, ///< The function instance of the ECU. Similar to Virtual Terminal number.
			FunctionCode, ///< The function of the ECU, as defined by ISO11783
			DeviceClass, ///< Also known as the vehicle system from J1939, describes general ECU type
			DeviceClassInstance, ///< The instance number of this device class
			IndustryGroup, ///< The industry group associated with this ECU, such as "agricultural"
			ArbitraryAddressCapable ///< Defines if this ECU supports address arbitration
		};

		/// @brief See ISO11783-1 For complete descriptions of the ISO NAME function codes
		enum class Function
		{
			Engine = 0, ///< The typical mechanical power source of the machine
			AuxiliaryPowerUnit = 1, ///< Power source for operating systems without the use of the prime drive engine
			ElectricPropulsionControl = 2, ///< Control system which operates the drive mechanism when it is electrically powered
			Transmission = 3, ///< Mechanical system for altering the speed vs. torque output of the engine
			BatteryPackMonitor = 4, ///< Monitors the condition of a battery pack
			ShiftControl = 5, ///< Control Unit that determines and transmits onto the network the gear desired by the operator
			PowerTakeOffRearOrPrimary = 6, ///< System that controls the mechanical power derived from a prime engine and used to operate auxiliary items
			SteeringAxle = 7, ///< Adjusts attack angle of steering axle
			DrivingAxle = 8, ///< Adjusts attack angle of driving axle
			SystemControlBrakes = 9, ///< Controls service braking system electronically
			SteerAxleControlBrakes = 10, ///< Control for actuating the service brakes on a steered axle
			DriveAxleControlBrakes = 11, ///< Control for actuating the service brakes on a drive axle
			EnginerRetarder = 12, ///< Controller for the retarder capabilities of the engine
			DrivelineRetarder = 13, ///< Controller for the retarder capabilities of the driveline
			CruiseControl = 14, ///< Control system for maintaining the vehicle's speed at a fixed operator selectable value
			FuelSystem = 15, ///< Controls fuel flow from the tank to the filter to the water removal/separator to the engine and then back to the tank
			SteeringControl = 16, ///< Controls steering in steer-by-wire
			SteerAxleSuspensionControl = 17, ///< Control system for the suspension of a steered axle
			DriveAxleSuspensionControl = 18, ///< Control system for the suspension of a driven axle
			InstrumentCluster = 19, ///< Gauge display for a vehicle, usually in the cab
			TripRecorder = 20, ///< System for accumulating data versus travel of the vehicle
			CabClimateControl = 21, ///< System for controlling the climate within the cab of the vehicle
			AerodynamicControl = 22, ///< Modifies drag by altering body panels
			VehicleNavigation = 23, ///< System associated with the vehicles physical location
			VehicleSecurity = 24, ///< System for comparing operator-provided data sequences against reference
			NetworkInterconnectUnit = 25, ///< ECU for connecting different network segments together
			BodyControl = 26, ///< Can handle suspension control for the body sections independent from the axle sections
			PowerTakeOffFrontOrSecondary = 27, ///< System that controls the mechanical power derived from a prime engine and used to operate auxiliary items
			OffVehicleGateway = 28, ///< ECU for connecting between vehicle network(s) and an off-vehicle system or network
			VirtualTerminal = 29, ///< General-purpose intelligent display with a specific message set defined in ISO 11783-6
			ManagementComputerOne = 30, ///< Manages vehicle systems, i.e. powertrain
			PropulsionBatteryCharger = 31, ///< Unit used to charge propulsion batteries in an electric vehicle
			HeadwayControl = 32, ///< Forward-looking collision avoidance, collision warning, speed controller, or speed control
			SystemMonitor = 33, ///< Generic system monitor
			HydraulicPumpControl = 34, ///< Pump controller that provides hydraulic power
			SystemControlSuspension = 35, ///< Controller responsible for coordinating the over-all suspension of a vehicle
			SystemControlPneumatic = 36, ///< Controller responsible for coordinating the pneumatics of a vehicle
			CabController = 37, ///< Controller located in/near vehicle cab used to perform functions that are grouped together for convenience
			TirePressureControl = 38, ///< Unit that provides control of centralized tire inflation
			IgnitionControl = 39, ///< Unit for altering the ignition of an engine
			SeatControl = 40, ///< System for controlling the seats (operator and passenger) within the cab
			OperatorControlsLighting = 41, ///< Controller for sending the operator lighting controls messages
			WaterPumpControl = 42, ///< Controller for a water pump mounted on the vehicle/machine
			TransmissionDisplay = 43, ///< Display designed specifically to display transmission information
			ExhaustEmissionControl = 44, ///< Emissions controller
			VehicleDynamicStabilityControl = 45, ///< Stability controller
			OilSystemMonitor = 46, ///< Monitors oil level, life, temperature
			InformationSystemControl = 47, ///< Information management for a vehicle's application, such as cargo management
			RampControl = 48, ///< Loading unloading chairlift, ramps, lifts or tailgates
			ClutchConverterControl = 49, ///< When transmission is distributed this handles torque converter lock-up or engine-transmission connection
			AuxiliaryHeater = 50, ///< Primary heat typically being taken from the engine coolant
			ForwardLookingCollisionWarningSystem = 51, ///< System which detects and warns of impending collision
			ChassisControl = 52, ///< Controls the chassis (not body or cab) components
			AlternatorElectricalChargingSystem = 53, ///< Vehicle's primary on-board charging controller
			CommunicationsCellular = 54, ///< Radio communications unit designed to communicate via the cellular telephone system
			CommunicationsSatellite = 55, ///< Radio communications unit designed specifically to communicate via some satellite system
			CommunicationsRadio = 56, ///< Radio unit designed specifically to communicate via a terrestrial p2p system
			OperatorControlsSteeringColumn = 57, ///< Unit that gathers the operator inputs from switches/levers/etc and transmits associated messages
			FanDriveControl = 58, ///< Primary control system affecting the operation of the main cooling fan
			Starter = 59, ///< Mechanical system for initiating rotation in an engine
			CabDisplayCab = 60, ///< Used for a fairly elaborate in cab display, non VT and non instrument cluster
			FileServerOrPrinter = 61, ///< Printing or file storage unit on the network
			OnboardDiagnosticUnit = 62, ///< Tool that can be permanently mounted on the vehicle and which may not support all of the ISO 11783-12 messages
			EngineValveController = 63, ///< Control system used to manipulate the actuation of engine intake or exhaust
			EnduranceBraking = 64, ///< Sum of all units in a vehicle which enable the driver to brake with virtually no friction
			GasFlowMeasurement = 65, ///< Provides measurement of gas flow rates and associated parameters
			IOController = 66, ///< Reporting and/or control unit for external input and output channels
			ElectricalSystemController = 67, ///< Can include load centres, fuse boxes and power distribution boards
			Reserved = 68, ///< Reserved range beginning
			MaxFunctionCode = 127 ///< Max allocated function code
		};

		/// @brief A useful way to compare sesson objects to each other for equality
		/// @param[in] obj The rhs of the operator
		/// @returns `true` if the objects are "equal"
		bool operator==(const NAME &obj) const;

		/// @brief A structure that tracks the pair of a NAME parameter and associated value
		typedef std::pair<const NAMEParameters, const std::uint32_t> NameParameterFilter;

		/// @brief Constructor for a NAME
		/// @param[in] rawNAMEData The raw 64 bit NAME of an ECU
		explicit NAME(std::uint64_t rawNAMEData = 0);

		/// @brief Returns if the ECU is capable of address arbitration
		/// @returns true if the ECU can arbitrate addresses
		bool get_arbitrary_address_capable() const;

		/// @brief Sets the data in the NAME that corresponds to the arbitration capability of the ECU
		/// @param[in] value True if the ECU supports arbitration, false if not
		void set_arbitrary_address_capable(bool value);

		/// @brief Returns the industry group encoded in the NAME
		/// @returns The industry group encoded in the NAME
		std::uint8_t get_industry_group() const;

		/// @brief Sets the industry group encoded in the NAME
		/// @param[in] value The industry group to encode in the NAME
		void set_industry_group(std::uint8_t value);

		/// @brief Returns the device class (vehicle system) encoded in the NAME
		/// @returns The device class (vehicle system) encoded in the NAME
		std::uint8_t get_device_class_instance() const;

		/// @brief Sets the device class instance (vehicle system) to be encoded in the NAME
		/// @param[in] value The device class instance (vehicle system) to be encoded in the NAME
		void set_device_class_instance(std::uint8_t value);

		/// @brief Returns the device class (vehicle system) encoded in the NAME
		/// @returns The device class (vehicle system) encoded in the NAME
		std::uint8_t get_device_class() const;

		/// @brief Sets the device class (vehicle system) to be encoded in the NAME
		/// @param[in] value The device class (vehicle system) to be encoded in the NAME
		void set_device_class(std::uint8_t value);

		/// @brief Gets the function code encoded in the NAME
		/// @returns The function code encoded in the NAME
		std::uint8_t get_function_code() const;

		/// @brief Sets the function code encoded in the NAME
		/// @param[in] value The function code to be encoded in the NAME
		void set_function_code(std::uint8_t value);

		/// @brief Gets the function instance encoded in the NAME
		/// @returns The function instance encoded in the NAME
		std::uint8_t get_function_instance() const;

		/// @brief Sets the function instance encoded in the NAME
		/// @param[in] value The function instance to be encoded in the NAME
		void set_function_instance(std::uint8_t value);

		/// @brief Gets the ecu instance encoded in the NAME
		/// @returns The ecu instance encoded in the NAME
		std::uint8_t get_ecu_instance() const;

		/// @brief Sets the ecu instance encoded in the NAME
		/// @param[in] value The ecu instance to be encoded in the NAME
		void set_ecu_instance(std::uint8_t value);

		/// @brief Gets the manufacturer code encoded in the NAME
		/// @returns The manufacturer code encoded in the NAME
		std::uint16_t get_manufacturer_code() const;

		/// @brief Sets the manufacturer code encoded in the NAME
		/// @param[in] value The manufacturer code to be encoded in the NAME
		void set_manufacturer_code(std::uint16_t value);

		/// @brief Gets the identity number encoded in the NAME
		/// @returns The identity number encoded in the NAME
		std::uint32_t get_identity_number() const;

		/// @brief Sets the identity number encoded in the NAME
		/// @param[in] value The identity number to be encoded in the NAME
		void set_identity_number(std::uint32_t value);

		/// @brief Gets the raw 64 bit NAME
		/// @returns The raw 64 bit NAME
		std::uint64_t get_full_name() const;

		/// @brief Sets the raw, encoded 64 bit NAME
		/// @param[in] value The raw, encoded 64 bit NAME
		void set_full_name(std::uint64_t value);

	private:
		std::uint64_t rawName; ///< A raw, 64 bit NAME encoded with all NAMEParameters
	};
} // namespace isobus

#endif // CAN_NAME_HPP
