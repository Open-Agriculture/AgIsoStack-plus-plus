//================================================================================================
/// @file can_NAME.hpp
///
/// @brief A class that represents a control function's NAME
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
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

		/// @brief The industry group is part of the ISO NAME.
		/// It allocates devices and their functions by a specific industry.
		/// Function codes' meanings are defined in relation to industry group and device class.
		enum class IndustryGroup : std::uint8_t
		{
			Global = 0, ///< Global applies to all industries
			OnHighwayEquipment = 1,
			AgriculturalAndForestryEquipment = 2,
			ConstructionEquipment = 3,
			Marine = 4,
			IndustrialOrProcessControl = 5,
			Reserved1 = 6, ///< Reserved for future assignment by SAE. Should not be used.
			Reserved2 = 7 ///< Reserved for future assignment by SAE. Should not be used.
		};

		/// @brief See ISO11783-1 and www.isobus.net For complete descriptions of the ISO NAME function codes
		/// @note Functions are defined in relation to industry group and device class. See www.isobus.net for more info.
		enum class Function : std::uint8_t
		{
			// Common Functions
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
			EngineRetarder = 12, ///< Controller for the retarder capabilities of the engine
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
			AftertreatmentSystemGasMeasurement = 68, ///< Sensor for measuring gas properties before and after an aftertreatment system
			EngineEmissionAftertreatmentSystem = 69, ///< Engine Emission Aftertreatment System
			AuxiliaryRegenerationDevice = 70, ///< Auxiliary Regeneration Device used as part of an after treatment system
			TransferCaseControl = 71, ///< The device which controls the selection of the number of drive wheels (for example 2 or 4 wheel drive)
			CoolantValveController = 72, ///< Device used to control the flow of coolant (water, oil, air, etc�) for any thermal management system
			RolloverDetectionControl = 73, ///< Device designed for detection of vehicle rollover
			LubricationSystem = 74, ///< The Lubrication System pumps quantities of lubricant to each machine/vehicle joint that need to be lubricated
			SupplementalFan = 75, ///< This is an auxillary fan used for additional cooling. It is in addition to the primary cooling fan.
			TemperatureSensor = 76, ///< Device which measures temperature.
			FuelPropertiesSensor = 77, ///< Device which measures fuel properties
			FireSuppressionSystem = 78, ///< Fire Suppression System
			PowerSystemsManager = 79, ///< Controller application that manages the power output of one or more power systems. See also IG 5 Function 129 - Generator Set Controller
			ElectricPowertrain = 80, ///< Controller application in charge of controlling and coordinating the operation of an electric drive system
			HydraulicPowertrain = 81, ///< Controller application in charge of controlling and coordinating the operation of a hydraulic drive system
			FileServer = 82, ///< A file storage unit on the network - A permanent connection may exist and the unit is expected to store data (as in magnetic or eeprom devices). See Function 61 for a combination File Server/Printer unit
			Printer = 83, ///< A printing unit on the network - A permanent connection may exist and the unit is expected to be able to print (paper type output). See Function 61 for a combination File Server/Printer unit
			StartAidDevice = 84, ///< Device that controls hardware and/or conveys information related to assisting an engine in starting, such as a glow plug, grid heater, etc.
			EngineInjectionControlModule = 85, ///< A device for direct or port injection of fuel for engine combustion and with which an engine controller may communicate
			EVCommunicationController = 86, ///< A controller or application that manages the connection to an external power source, i.e. the Electric Vehicle Supply Equipment
			DriverImpairmentDevice = 87, ///< Device which prevents the starting of a vehicle motor due to driver impairment. Example is an alcohol interlock device
			ElectricPowerConverter = 88, ///< An inverter or converter used to transform AC or DC power to or from an AC or DC source
			SupplyEquipmentCommunicationController = 89, ///< Typically part of an Electrical Vehicle Supply Equipment (EVSE) in an electric vehicle charging station
			VehicleAdapterCommunicationController = 90, ///< A controller inside of the adapter placed between an Electric Vehicle Supply Equipment (EVSE) vehicle connector and the vehicle inlet
			RateControl = 128, ///< Control of the rate of product placed on or in the soil
			SectionOnOffControl = 129, ///< On/Off control of individual sections
			PositionControl = 131, ///< Multiple axis position control of a device's application boom
			MachineControl = 132, ///< Control of outputs including adjustment of any ancillary functions like position and/or rotation speed.
			ProductFlow = 133, ///< Measuring function to monitor the current product flow.
			ProductLevel = 134, ///< Measuring function to monitor the current product level in the bin/tank
			DepthOrHeightControl = 135, ///< Control of the depth of the tool in the soil, or control of the height of the boom above the surface of the soil or above the height of the standing crop
			FrameControl = 136, ///< Control of the folding and unfolding of the frame of the device. Control used to change between transport and field operation position. Not to be used for field operations

			// Non-specific system (Device class 0) industry group and vehicle system
			OffBoardDiagnosticServiceTool = 129, ///< Off-board diagnostic-service tool
			OnBoardDiagnosticDataLogger = 130, ///< On-board data logger
			PCKeyboard = 131, ///< A user interface similar to a PC keyboard
			SafetyRestraintSystem = 132, ///< The safety restraint system can be for controlling activation of airbags, belt tensioners, roll over protection systems, etc
			Turbocharger = 133, ///< Turbocharger used on the engine
			GroundBasedSpeedSensor = 134, ///< Measures actual ground speed of a vehicle with a device such as radar or other such devices
			Keypad = 135, ///< An operator input device used to control machine functions or provide data
			HumiditySensor = 136, ///< Device which measures air humidity
			ThermalManagementSystemController = 137, ///< This device controls all devices that may be used in a thermal management system including Jacket Water Cooling, Charged Air Cooling, Transmission Cooling, Electronics Cooling, Aux Oil Cooling, etc
			BrakeStrokeAlert = 138, ///< The device that evaluates air brake stroke for normal stroke, over stroke, dragging brake, or a non-functioning brake actuator and is permanently mounted on the vehicle
			OnBoardAxleGroupScale = 139, ///< The device that determines axle group weights and is permanently mounted on the vehicle.
			OnBoardAxleGroupDisplay = 140, ///< The device that displays axle group weights and may be permanently mounted on the vehicle
			BatteryCharger = 141, ///< A device used to charge batteries in a vehicle from an off-board source of electrical energy.
			TurbochargerCompressorBypass = 142, ///< Device used to control the flow across the Compressor Bypass
			TurbochargerWastegate = 143, ///< Device used to control the position of the Wastegate to adjust the exhaust flow
			Throttle = 144, ///< Device used to control the air/fuel mixture into the cylinders for combustion
			InertialSensor = 145, ///< Detects a change in geographic position, a change in velocity, and/or a change in orientation. This may include but is not limited to an accelerometer, gyroscopes, etc
			FuelActuator = 146, ///< Device used to control the flow of fuel (or fuel rack) on a engine
			EngineExhaustGasRecirculation = 147, ///< Device that controls the engine exhaust gas recirculation system
			EngineExhaustBackpressure = 148, ///< Device that controls the engine exhaust backpressure
			OnBoardBinWeightingScale = 149, ///< Device that determines bin weights and is permanently mounted on the vehicle
			OnBoardBinWeighingScaleDisplay = 150, ///< Device that displays bin weights and may be permanently mounted on the vehicle
			EngineCylinderPressureMonitoringSystem = 151, ///< System designed to monitor engine cylinder pressures and provide combustion information
			ObjectDetection = 152, ///< System for detection of undesirable objects in the product flow
			ObjectDetectionDisplay = 153, ///< Display designed specifically for displaying and managing object detection information
			ObjectDetectionSensor = 154, ///< Detects the presence of objects within a region.
			PersonnelDetectionDevice = 155, /// < Device for the detection of personnel in proximity to a vehicle.

			// ******** On-Highway (Industry Group 1) ********
			// Non-specific system (Device class 0) industry group 1
			Tachograph = 128, ///< Records engine speed over a period of time
			DoorController = 129, ///< Door controller
			ArticulationTurntableControl = 130, ///< Control of the articulation turntable for joined body buses
			BodyToVehicleInterfaceControl = 131, ///< Manages interaction of vehicle functions and body functions. May be combination of body signals and gateway functionalities
			SlopeSensor = 132, ///< Sensor for measuring a slope along an axis
			RetarderDisplay = 134, ///< Display module that shows information pertaining to the retarder (driveline or exhaust or engine)
			DifferentialLockController = 135, ///< Differential Lock Controller
			LowVoltageDisconnect = 136, ///< Monitors the voltage of the starting battery bank and disconnects predetermined auxiliary loads to assure enough power is left in the batteries for starting the vehicle
			RoadwayInformation = 137, ///< Devices that use this function will provide information relevant to the roadway in which the vehicle is traveling. This includes attributes such as intersections, grade, speed limit, number of lanes, etc
			AutomatedDriving = 138, ///< Automated Driving System. See SAE J3016

			// Non-specific system (Device class 0) Tractor industry group 1
			ForwardRoadImageProcessing = 128, ///< Determine vehicle position from lane markings. Performance, Advisory & Warning only
			FifthWheelSmartSystem = 129, ///< Any systems relative to the operation & status/safety monitoring of the fifth wheel coupler system
			CatalystFluidSensor = 130, ///< The Catalyst Fluid Sensor can measure the catalyst fluid temperature, the catalyst fluid level and the catalyst fluid quality
			AdaptiveFrontLightingSystem = 131, ///< System used to adjust the vehicle front lighting for the current operating conditions (city, highway, country,etc.)
			IdleControlSystem = 132, ///< The device automatically starts and stops the engine when the vehicle is stationary for the purpose of reducing excess idle time.
			UserInterfaceSystem = 133, ///< The User Interface System is a two way interface system. Uses of this may include, but are not limited to, setting climate conditions, setting system parameters, and/or logging operating conditions

			// ******** Agriculture (Industry Group 2) ********
			// Non-specific system (Device class 0) industry group 2
			NonVirtualTerminalDisplay = 128, ///< An operator display connected to the 11783 network that cannot perform as a Virtual Terminal and is not allowed to send a VT status message
			OperatorControlsMachineSpecific = 129, ///< Operator interface controls, either auxiliary control inputs or a proprietary means, provided by a control function
			TaskController = 130, ///< A control function on the 11783 network that is responsible for the sending, receiving and logging of process data as defined in ISO11783-10.
			ForeignObjectDetection = 133, ///< 	Detection of undesirable objects in the product flow
			TractorECU = 134, ///< (TECU) An interface unit between the tractor and the implement bus representing the tractor and its messages on the 11783 network
			SequenceControlMaster = 135, ///< The master controller in the Sequence Control System as defined in ISO11783-14
			ProductDosing = 136, ///< Control function that adds an active ingredient to a liquid carrier for application to fields (direct injection systems)
			ProductTreatment = 137, ///< Control function that mixes a treatment to a dry product applied to or harvested from fields
			DataLogger = 139, ///< Data logger as defined in ISO11783-10 for non-task related data logging
			DecisionSupport = 140, ///< A control function which is used by the operator or by another control function to configure an operation to perform optimally under the current circumstances
			LightingController = 141, ///< Control function that controls electrical power to the lights and reports the status of the lights. This control function can be used on trailers or implements
			TIMServer = 142, ///< Control function that represents a Tractor Implement Management (TIM) Server

			// Tractor (Device class 1) Industry Group 2
			AuxiliaryValveControl = 129, ///< Control of addressed tractor mounted auxiliary valves
			RearHitchControl = 130, ///< Control of the rear hitch of an agricultural tractor
			FrontHitchControl = 131, ///< Control of the front hitch of an agricultural tractor
			CenterHitchControl = 134, ///< Control of center hitch of an agricultural tractor

			// Planters/Seeders (Device Class 4) Industry Group 2
			DownPressure = 137, ///< Control of the ground contact pressure on the product delivery unit for optimal operation e.g. pressure on openers for penetrating the ground

			// Fertilizers (Device Class 5) Industry Group 2
			ProductPressure = 130, ///< 	Monitoring of the pressure of the product in the delivery booms

			// Harvesters (Device Class 7) Industry Group 2
			TailingMonitor = 128, ///< Measuring system to monitor the quantity of unthreshed material returned to threshing machine
			HeaderControl = 129, ///< Control of the headers reel height and rotation and material delivery rate
			ProductLossMonitor = 130, ///< Measuring system to monitor the amount of grain being delivered back onto the soil
			HarvesterProductMoisture = 131, ///< Measuring system to monitor the moisture content of the grain

			// Forage (Device Class 9) Industry Group 2
			TwineWrapperControl = 128, ///< Control of the wrapping of twine around a bale before discharge from the baler
			ProductPackagingControl = 129, ///< Control of packaging process for the forage material.
			ForageProductMoisture = 131, ///< Measuring system to monitor the moisture of the forage content.

			// Transport/Trailer (Device Class 11) Industry Group 2
			UnloadControl = 136, ///< Control of trailer unloading process

			// Sensor Systems (Device Class 17) Industry Group 2
			GuidanceFeeler = 128, ///< Mechanical function for determining row position in the field
			CameraSystem = 129, ///< Provides images or processed data for control operations.
			CropScouting = 130, ///< Measures vegetation parameters in a standing crop
			MaterialPropertiesSensing = 131, ///< Sensing system to detect material properties like density, particle size, color or constituents
			InertialMeasurementUnit = 132, /// < A sensor unit providing inertial measurements
			ProductMass = 135, ///< Measuring function to monitor the mass of the product
			VibrationKnock = 136, ///< Measuring function to determine the vibration or knock behaviour of a system
			WeatherInstruments = 137, ///< The "Weather Instruments" function code shall be used by ISO11783 compliant Weather Instruments
			SoilScouting = 138, ///< Soil Sensor to measure different soil physical parameters. One example of a soil sensor is a system that measures the apparent conductivity

			// ******** Construction (Industry Group 3) ********
			// Non-specific system (Device class 0) Industry Group 3
			ConstructionSupplementalEngineControlSensing = 128, ///< Supplemental Engine Control Sensing
			LaserReceiver = 129, ///< Laser Receiver
			LandLevelingSystemOperatorInterface = 130, ///< A component that allows the user to control the Land Leveling System and display information about the operation of the system
			LandLevelingElectricMast = 131, ///< Land Leveling Electric Mast
			SingleLandLevelingSystemSupervisor = 132, ///< Single Land Leveling System Supervisor
			LandLevelingSystemDisplay = 133, ///< Land Leveling System Display
			LaserTracer = 134, ///< Laser Tracer
			LoaderControl = 135, ///< Loader control unit
			ConstructionEquipmentSlopeSensor = 136, ///< Measures the slope along a axis.
			LiftArmControl = 137, ///< Controller whose primary purpose is to control the lift arms and tilt functions on a construction loader, skid steer loader, or similar machine.
			SupplementalSensorProcessingUnits = 138, ///< An ECU functioning as an I/O module connected to the bus with the designed purpose of data collection (input or output)
			HydraulicSystemPlanner = 139, ///< Coordinates the functions of a number of valve controllers
			HydraulicValveController = 140, ///< The valve controller will typically control the flow of oil to a specific cylinder.
			JoystickControl = 141, ///< Joystick Control
			RotationSensor = 142, ///< A device that measures the rotational angle around an axis
			SonicSensor = 143, ///< A device that measures distance via ultrasonic pulse/echo range techniques.
			SurveyTotalStationTarget = 144, ///< A survey total station target shall be located on a construction machine and shall be connected to the CAN network. It is targeted by the total station device.
			HeadingSensor = 145, ///< A device that measures vehicle azimuth.
			AlarmDevice = 146, ///< Device that provides an audible and/or visual alarm

			// Skid Steer Loader (Device Class 1) Industry Group 3
			SkidSteerMainController = 128, ///< Main controller for a skid steer machine

			// Crawler (Device Class 4) Industry Group 3
			BladeController = 128, ///< Controller for blade height.

			// Grader (Device Class 8) Industry Group 3
			HFWDController = 128, ///< Hydraulic front wheel drive controller

			// ******** Marine (Industry Group 4) ********
			Alarm1SystemControlForMarineEngines = 128, ///< The ECU that controls the Alarm functions on an engine of a Marine System.
			ProtectionSystemForMarineEngines = 129, ///< The first ECU that controls the Protection functions on the first engine of a Marine System.
			DisplayForProtectionSystemForMarineEngines = 130, ///< The ECU that provides the display of information and/or indicators associated specifically with the protection system on an engine of a Marine System.

			// Power Management And Lighting System (Device Class 30)
			Switch = 130, ///< A CAN switch
			Load = 140, ///< Load

			// Steering systems (Device class 40)
			FollowUpController = 130, ///< Follow-up controller
			ModeController = 140, ///< Mode Controller
			AutomaticSteeringController = 150, ///< Automatic Steering Controller
			HeadingSensors = 160, ///< Heading Sensors

			// Propulsion Systems
			EngineRoomMonitoring = 130, ///< Marine engine room monitoring system
			EngineInterface = 140, ///< Marine Engine interface
			EngineController = 150, ///< Marine Engine Controller
			EngineGateway = 160, ///< Marine engine gateway
			ControlHead = 170, ///< Marine electronic control head
			Actuator = 180, ///< Marine actuator
			GaugeInterface = 190, ///< Marine Gauge Interface
			GaugeLarge = 200, ///< Large marine gauge
			GaugeSmall = 210, ///< Small marine gauge
			PropulsionSensorsAndGateway = 220, ///< Propulsion sensors and gateway

			// Navigation Systems
			SounderDepth = 130, ///< Sounder.
			GlobalNavigationSatelliteSystem = 145, ///< Marine GNSS
			LoranC = 150, ///< Marine Loran C
			SpeedSensors = 155, ///< Marine speed sensors
			TurnRateIndicator = 160, ///< Marine turn rate indicator
			IntegratedNavigation = 170, ///< Marine integrated navigation
			RadarOrRadarPlotting = 200, ///< Radar and/or Radar Plotting
			ElectronicChartDisplayAndInformationSystem = 205, ///< ECDIS
			ElectronicChartSystem = 210, ///< ECS
			DirectionFinder = 220, ///< Direction Finder

			// Communications Systems
			EmergencyPositionIndicatingBeacon = 130, ///< EPIRB
			AutomaticIdentificationSystem = 140, ///< Marine automatic identification system
			DigitalSelectiveCalling = 150, ///< DSC
			DataReceiver = 160, ///< Marine data receiver
			Satellite = 170, ///< A satellite ?
			RadioTelephoneMF_HF = 180, ///< Radio - Telephone(MF / HF)
			RadioTelephoneVHF = 190, ///< Radio - Telephone(VHF)
			TimeDateSystems = 130, ///< Marine time date system
			VoyageDataRecorder = 140, ///< Marine Voyage Data Recorder
			IntegratedInstrumentation = 150, ///< Marine Integrated Instrumentation
			GeneralPurposeDisplays = 160, ///< 	Marine General Purpose Displays
			GeneralSensorBox = 170, ///< Marine General Sensor Box
			MarineWeatherInstruments = 180, ///< Marine Weather Instruments
			TransducerGeneral = 190, ///< Marine Transducer/general
			NMEA0183Converter = 200, ///< NMEA 0183 Converter

			// ******** Industrial / Process Control (Industry Group 5) ********
			GeneratorSupplementalEngineControlSensing = 128, ///< Supplemental Engine Control Sensing
			GeneratorSetController = 129, ///< Generator set controller used to collect data and control.
			GeneratorVoltageRegulator = 130, ///< Generator Voltage Regulator
			ChokeActuator = 131, ///< Device used to Control the flow of air on a Gas Engine.
			WellStimulationPump = 132, ///< Device which communicates operating parameters of a well stimulation pump used in oil and gas drilling applications.

			MaxFunctionCode = 255 ///< Max allocated function code
		};

		/// @brief The device class is part of the ISO NAME and is known in J1939 as the "vehicle system".
		/// This is a 7-bit field defined and assigned by SAE.
		/// Device class provides a common name for a group of functions
		/// within a connected network.
		enum class DeviceClass
		{
			NonSpecific = 0,
			Tractor = 1, ///< Industry Group 1 and 2
			SkidSteerLoader = 1, ///< Industry Group 3
			Trailer = 2, ///< Industry group 1 and 2
			ArticulatedDumpTruck = 2, ///< Industry group 3
			SecondaryTillage = 3, ///< Industry group 2
			Backhoe = 3, ///< Industry group 3
			PlanterSeeder = 4, ///< Industry group 2
			Crawler = 4, ///< Industry group 3
			Fertilizer = 5, ///< Industry group 2
			Excavator = 5, ///< Industry group 3
			Sprayer = 6, ///< Industry group 2
			Forklift = 6, ///< Industry group 3
			Harvester = 7, ///< Industry group 2
			FourWheelDriveLoader = 7, ///< Industry group 3
			RootHarvester = 8, ///< Industry group 2
			Grader = 8, ///< Industry group 3
			Forage = 9, ///< Industry group 2
			MillingMachine = 9, ///< Industry group 3
			Irrigation = 10, ///< Industry group 2
			RecyclerAndSoilStabilizer = 10, ///< Industry group 3
			SystemTools = 10, ///< Industry group 4
			TransportTrailer = 11, ///< Industry group 2
			BindingAgentSpreader = 11, ///< Industry group 3
			FarmYardOperations = 12, ///< Industry group 2
			Paver = 12, ///< Industry group 3
			PoweredAuxiliaryDevices = 13, ///< Industry group 2
			Feeder = 13, ///< Industry group 3
			SpecialCrops = 14, ///< Industry group 2
			ScreeningPlant = 14, ///< Industry group 3
			Earthwork = 15, ///< Industry group 2
			Stacker = 15, ///< Industry group 3
			Skidder = 16, ///< Industry group 2
			Roller = 16, ///< Industry group 3
			SensorSystems = 17, ///< Industry group 2
			Crusher = 17, ///< Industry group 3
			TimberHarvester = 19, ///< Industry group 2
			Forwarder = 20, ///< Industry group 2
			SafetySystems = 20, ///< Industry group 4
			TimberLoader = 21, ///< Industry group 2
			TimberProcessor = 22, ///< Industry group 2
			Mulcher = 23, ///< Industry group 2
			UtilityVehicle = 24, ///< Industry group 2
			SlurryManureApplicator = 25, ///< Industry group 2
			Gateway = 25, ///< Industry group 4
			FeederMixer = 26, ///< Industry group 2
			WeederNonChemical = 27, ///< Industry group 2
			TurfOrLawnCareMower = 28, ///< Industry group 2
			ProductMaterialHandling = 29, ///< Industry group 2
			PowerManagementAndLightingSystem = 30, ///< Industry group 4
			SteeringSystems = 40, ///< Industry group 4
			PropulsionSystems = 50, ///< Industry group 4
			NavigationSystems = 60, ///< Industry group 4
			CommunicationsSystems = 70, ///< Industry group 4
			InstrumentationOrGeneral = 80, ///< Industry group 4
			EnvironmentalHVACSystem = 90, ///< Industry group 4
			DeckCargoOrFishingEquipment = 100, ///< Industry group 4
			NotAvailable = 127 //< Applicable to all IGs
		};

		/// @brief A useful way to compare session objects to each other for equality
		/// @param[in] obj The rhs of the operator
		/// @returns `true` if the objects are "equal"
		bool operator==(const NAME &obj) const;

		/// @brief A structure that tracks the pair of a NAME parameter and associated value
		using NameParameterFilter = std::pair<const NAMEParameters, const std::uint32_t>;

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
