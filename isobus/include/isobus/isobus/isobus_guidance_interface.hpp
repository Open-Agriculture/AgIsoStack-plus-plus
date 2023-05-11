//================================================================================================
/// @file isobus_guidance_interface.hpp
///
/// @brief Defines an interface for sending and receiving ISOBUS guidance messages.
/// These messages are used to steer ISOBUS compliant machines, steering valves, and
/// implements in general.
///
/// @attention Please use extreme care if you try to steer a machine with this interface!
/// Remember that this library is licensed under The MIT License, and that by obtaining a
/// copy of this library and of course by attempting to steer a machine with it, you are agreeing
/// to our license.
///
/// @note These messages are expected to be deprecated or at least made redundant in favor
/// of Tractor Implement Management (TIM) at some point by the AEF, though the timeline on that
/// is not known at the time of writing this, and it's likely that many machines will
/// continue to support this interface going forward due to its simplicity over TIM.
///
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_GUIDANCE_INTERFACE_HPP
#define ISOBUS_GUIDANCE_INTERFACE_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/utility/event_dispatcher.hpp"
#include "isobus/utility/processing_flags.hpp"

#include <memory>
#include <vector>

namespace isobus
{
	/// @brief An interface for sending and receiving ISOBUS guidance messages
	class GuidanceInterface
	{
	public:
		/// @brief Constructor for a GuidanceInterface
		/// @param[in] source The internal control function to use when sending messages, or nullptr for listen only
		/// @param[in] destination The destination control function for transmitted messages, or nullptr for broadcasts
		GuidanceInterface(std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief Destructor for the GuidanceInterface
		~GuidanceInterface();

		/// @brief An interface for sending the agricultural
		/// guidance system command message.
		///
		/// @details This message is sent by an automatic guidance control system to the
		/// machine steering system. It provides steering commands
		/// and serves as heartbeat between guidance system and steering control system.
		class GuidanceSystemCommand
		{
		public:
			/// @brief This parameter indicates whether the guidance system is
			/// attempting to control steering with this command.
			enum class CurvatureCommandStatus : std::uint8_t
			{
				NotIntendedToSteer = 0, ///< Steering Disengaged
				IntendedToSteer = 1, ///< Steering Engaged
				Error = 2,
				NotAvailable = 3
			};

			GuidanceSystemCommand() = default;

			/// @brief Sets the curvature command status that will be encoded into
			/// the CAN message. This parameter indicates whether the guidance system is
			/// attempting to control steering with this command
			/// @param[in] newStatus The status to encode into the message
			void set_status(CurvatureCommandStatus newStatus);

			/// @brief Returns the curvature command status that was set with set_status
			/// @returns The curvature command status that was set with set_status
			CurvatureCommandStatus get_status() const;

			/// @brief Desired course curvature over ground that a machine's
			/// steering system is required to achieve.
			/// @details The value you set here will be encoded into the
			/// guidance curvature command message.
			///
			/// The desired path is determined by the automatic guidance system expressed
			/// as the inverse of the instantaneous radius of curvature of the turn.
			/// Curvature is positive when the vehicle is moving forward and turning to the driver's right
			///
			/// @param[in] curvature Commanded curvature in km^-1 (inverse kilometers). Range is -8032 to 8031.75 km-1
			void set_curvature(float curvature);

			/// @brief Returns the curvature command that was set with set_curvature
			/// @returns Commanded curvature in km^-1 (inverse kilometers). Range is -8032 to 8031.75 km-1
			float get_curvature() const;

			/// @brief Stores a pointer to the source control function for this message data
			/// @param[in] sender The control function sending this information
			void set_sender_control_function(ControlFunction *sender);

			/// @brief Returns a pointer to the sender of the message. If an ICF is the sender, returns the ICF being used to transmit from.
			/// @attention The only way you could get an invalid pointer here is if you register a partner, it sends this message, then you delete the partner and
			/// call this function, as that is the only time the stack deletes a control function. That would be abnormal program flow, but at some point
			/// the stack will be updated to return a shared or weak pointer instead, but for now please be aware of that limitation.
			/// Eventually though the message will time-out normally and you can get a new pointer for
			/// the external CF that replaces the deleted partner.
			/// @returns The control function sending this instance of the guidance system command message
			ControlFunction *get_sender_control_function() const;

		private:
			ControlFunction *controlFunction = nullptr; ///< The CF that is sending the message
			float commandedCurvature = 0.0f; ///< The commanded curvature in km^-1 (inverse kilometers)
			CurvatureCommandStatus commandedStatus = CurvatureCommandStatus::NotAvailable; ///< The current status for the command
		};

		/// @brief An interface for sending and receiving the ISOBUS agricultural guidance machine message
		class AgriculturalGuidanceMachineInfo
		{
		public:
			/// @brief State of a lockout switch that allows operators to
			/// disable automatic steering system functions.
			/// @details https://www.isobus.net/isobus/pGNAndSPN/1221?type=SPN
			enum class MechanicalSystemLockout : std::uint8_t
			{
				NotActive = 0,
				Active = 1,
				Error = 2,
				NotAvailable = 3
			};

			/// @brief Machine steering system request to the automatic guidance system to
			/// change Curvature Command Status state from "Intended to steer" to "Not intended to steer".
			enum class RequestResetCommandStatus : std::uint8_t
			{
				ResetNotRequired = 0,
				ResetRequired = 1,
				Error = 2,
				NotAvailable = 3
			};

			/// @brief A typical, generic 2 bit value in J1939 with no supersceding definition in ISO 11783
			enum class GenericSAEbs02SlotValue : std::uint8_t
			{
				DisabledOffPassive = 0,
				EnabledOnActive = 1,
				ErrorIndication = 2,
				NotAvailableTakeNoAction = 3
			};

			/// @brief This parameter is used to report the steering system's present limit status
			/// associated with guidance commands that are persistent (i.e. not transient/temporary/one-shot).
			enum class GuidanceLimitStatus : std::uint8_t
			{
				NotLimited = 0, ///< Not limited
				OperatorLimitedControlled = 1, ///< Request cannot be implemented
				LimitedHigh = 2, ///< Only lower command values result in a change
				LimitedLow = 3, ///< Only higher command values result in a change
				Reserved_1 = 4, ///< Reserved
				Reserved_2 = 5, ///< Reserved
				NonRecoverableFault = 6, ///< Non-recoverable fault
				NotAvailable = 7 ///< Parameter not supported
			};

			/// @brief This parameter is used to indicate why the guidance system cannot currently accept remote
			/// commands or has most recently stopped accepting remote commands.
			enum class GuidanceSystemCommandExitReasonCode
			{
				NoReasonAllClear = 0,
				RequiredLevelOfOperatorPresenceAwarenessNotDetected = 1,
				ImplementReleasedControlOfFunction = 2,
				OperatorOverrideOfFunction = 3,
				OperatorControlNotInValidPosition = 4,
				RemoteCommandTimeout = 5,
				RemoteCommandOutOfRangeInvalid = 6,
				FunctionNotCalibrated = 7,
				OperatorControlFault = 8,
				FunctionFault = 9,
				HydraulicOilLevelTooLow = 20,
				HydraulicOilTemperatureTooLow = 21,
				VehicleTransmissionGearDoesNotAllowRemoteCommands = 22, ///< park, etc.
				VehicleSpeedTooLow = 23,
				VehicleSpeedTooHigh = 24,
				AlternateGuidanceSystemActive = 25,
				ControlUnitInDiagnosticMode = 26,
				Error = 62,
				NotAvailable = 63 ///< Parameter not supported
			};

			/// @brief Constructor for a AgriculturalGuidanceMachineInfo
			AgriculturalGuidanceMachineInfo() = default;

			/// @brief Sets the estimated course curvature over ground for the machine.
			/// @param[in] curvature The curvature in km^-1 (inverse kilometers). Range is -8032 to 8031.75 km-1
			void set_estimated_curvature(float curvature);

			/// @brief Returns the estimated curvature that was previously set with set_estimated_curvature
			/// @returns The estimated curvature in km^-1 (inverse kilometers). Range is -8032 to 8031.75 km-1
			float get_estimated_curvature() const;

			/// @brief Sets the mechanical system lockout state
			/// @param[in] state The mechanical system lockout state to report
			void set_mechanical_system_lockout_state(MechanicalSystemLockout state);

			/// @brief Returns the mechanical system lockout state
			/// @returns The mechanical system lockout state being reported
			MechanicalSystemLockout get_mechanical_system_lockout() const;

			/// @brief Sets the guidance system's readiness state to report
			/// @param[in] state The state to report. See definition of GenericSAEbs02SlotValue
			void set_guidance_steering_system_readiness_state(GenericSAEbs02SlotValue state);

			/// @brief Returns the guidance system's readiness state for steering
			/// @returns The guidance system's readiness state for steering
			GenericSAEbs02SlotValue get_guidance_steering_system_readiness_state() const;

			/// @brief Sets the guidance steering input position state
			/// @param[in] state The state to set for the guidance steering input position
			void set_guidance_steering_input_position_status(GenericSAEbs02SlotValue state);

			/// @brief Returns the guidance steering input position state
			/// @returns Guidance steering input position state
			GenericSAEbs02SlotValue get_guidance_steering_input_position_status() const;

			/// @brief Sets the request reset command to report
			/// @details Machine steering system request to the automatic guidance system to
			/// change Curvature Command Status state from "Intended to steer" to "Not intended to steer".
			/// @param[in] state The request reset command state to report
			void set_request_reset_command_status(RequestResetCommandStatus state);

			/// @brief Returns the reported request reset command
			/// @details Machine steering system request to the automatic guidance system to
			/// change Curvature Command Status state from "Intended to steer" to "Not intended to steer".
			/// @returns The reported request reset command
			RequestResetCommandStatus get_request_reset_command_status() const;

			/// @brief Sets the reported guidance limit status
			/// @details This parameter is used to report the steering system's present
			/// limit status associated with guidance commands that are persistent
			/// (i.e. not transient/temporary/one-shot).
			/// @param[in] status The limit status to report
			void set_guidance_limit_status(GuidanceLimitStatus status);

			/// @brief Returns the reported guidance limit status
			/// @details This parameter is used to report the steering system's present
			/// limit status associated with guidance commands that are persistent
			/// (i.e. not transient/temporary/one-shot).
			/// @returns The reported guidance limit status
			GuidanceLimitStatus get_guidance_limit_status() const;

			/// @brief Sets the exit code for the guidance system
			/// @details This parameter is used to indicate why the guidance system cannot currently accept
			/// remote commands or has most recently stopped accepting remote commands.
			/// @returns The exit code for the guidance system
			void set_guidance_system_command_exit_reason_code(std::uint8_t exitCode);

			/// @brief Returns the exit code for the guidance system
			/// @details This parameter is used to indicate why the guidance system cannot currently accept
			/// remote commands or has most recently stopped accepting remote commands.
			/// @returns The exit code for the guidance system
			std::uint8_t get_guidance_system_command_exit_reason_code() const;

			/// @brief Sets the state for the steering engage switch
			/// @param[in] switchStatus The engage switch state to report
			void set_guidance_system_remote_engage_switch_status(GenericSAEbs02SlotValue switchStatus);

			/// @brief Returns the state for the steering engage switch
			/// @returns The state for the steering engage switch
			GenericSAEbs02SlotValue get_guidance_system_remote_engage_switch_status() const;

			/// @brief Stores a pointer to the source control function for this message data
			/// @param[in] sender The control function sending this information
			void set_sender_control_function(ControlFunction *sender);

			/// @brief Returns a pointer to the sender of the message. If an ICF is the sender, returns the ICF being used to transmit from.
			/// @attention The only way you could get an invalid pointer here is if you register a partner, it sends this message, then you delete the partner and
			/// call this function, as that is the only time the stack deletes a control function. That would be abnormal program flow, but at some point
			/// the stack will be updated to return a shared or weak pointer instead, but for now please be aware of that limitation.
			/// Eventually though the message will time-out normally and you can get a new pointer for
			/// the external CF that replaces the deleted partner.
			/// @returns The control function sending this instance of the guidance system command message
			ControlFunction *get_sender_control_function() const;

		private:
			ControlFunction *controlFunction = nullptr; ///< The CF that is sending the message
			float estimatedCurvature = 0.0f; ///< Curvature in km^-1 (inverse kilometers). Range is -8032 to 8031.75 km-1 (SPN 5238)
			MechanicalSystemLockout mechanicalSystemLockoutState = MechanicalSystemLockout::NotAvailable; ///< The reported state of the mechanical system lockout switch (SPN 5243)
			GenericSAEbs02SlotValue guidanceSteeringSystemReadinessState = GenericSAEbs02SlotValue::NotAvailableTakeNoAction; ///< The reported state of the steering system's readiness to steer (SPN 5242)
			GenericSAEbs02SlotValue guidanceSteeringInputPositionStatus = GenericSAEbs02SlotValue::NotAvailableTakeNoAction; ///< The reported state of the steering input position. (SPN 5241)
			GenericSAEbs02SlotValue guidanceSystemRemoteEngageSwitchStatus = GenericSAEbs02SlotValue::NotAvailableTakeNoAction; ///< The reported state of the remote engage switch (SPN 9726)
			RequestResetCommandStatus requestResetCommandStatus = RequestResetCommandStatus::NotAvailable; ///< The reported state of the request reset command (SPN 5240)
			GuidanceLimitStatus guidanceLimitStatus = GuidanceLimitStatus::NotAvailable; ///< The steering system's present limit status associated with guidance commands that are persistent (SPN 5726)
			std::uint8_t guidanceSystemCommandExitReasonCode = static_cast<std::uint8_t>(GuidanceSystemCommandExitReasonCode::NotAvailable); ///< The exit code for guidance, stored as a u8 to preserve manufacturer specific values (SPN 5725)
		};

		/// @brief Sets up the class and registers it to recieve callbacks from the network manager for processing
		/// guidance messages. The class will not recieve messages if this is not called.
		void initialize();

		/// @brief Returns if the interface has been initialized
		/// @returns true if initialize has been called for this interface, otherwise false
		bool get_initialized() const;

		/// @brief Use this to configure the transmission of the guidance machine info message from your application. If you pass in an internal control function
		/// to the constructor of this class, then this message is available to be sent.
		AgriculturalGuidanceMachineInfo agriculturalGuidanceMachineInfoTransmitData;

		/// @brief Use this to configure transmission the guidance system command message from your application. If you pass in an internal control function
		/// to the constructor of this class, then this message is available to be sent.
		GuidanceSystemCommand guidanceSystemCommandTransmitData;

		/// @brief Returns the number of received, unique guidance system command sources
		/// @returns The number of CFs sending the guidance system command either as a broadcast, or to our internal control function
		std::size_t get_number_received_guidance_system_command_sources() const;

		/// @brief Returns the number of received, unique guidance machine info message sources
		/// @returns The number of CFs sending the guidance machine info message either as a broadcast, or to our internal control function
		std::size_t get_number_received_agricultural_guidance_machine_info_message_sources() const;

		/// @brief Returns the content of the agricultural guidance machine info message
		/// based on the index of the sender. Use this to read the recieved messages' content.
		/// @param[in] index An index of senders of the agricultural guidance machine info message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these commands. So generally using only index 0 will be acceptable.
		std::shared_ptr<AgriculturalGuidanceMachineInfo> get_received_agricultural_guidance_machine_info(std::size_t index);

		/// @brief Returns the content of the agricultural guidance curvature command message
		/// based on the index of the sender. Use this to read the recieved messages' content.
		/// @param[in] index An index of senders of the agricultural guidance curvature command message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these commands. So generally using only index 0 will be acceptable.
		std::shared_ptr<GuidanceSystemCommand> get_received_guidance_system_command(std::size_t index);

		/// @brief Call this cyclically to update the interface. Transmits messages if needed and processes
		/// timeouts for received messages.
		void update();

	protected:
		/// @brief Enumerates a set of flags to manage transmitting messages owned by this interface
		enum class TransmitFlags : std::uint32_t
		{
			SendGuidanceSystemCommand = 0, ///< A flag to manage sending the guidance system command message
			SendGuidanceMachineInfo, ///< A flag to manage sending the guidance machine info message

			NumberOfFlags ///< The number of flags in this enumeration
		};

		/// @brief Processes one flag (which sends the associated message)
		/// @param[in] flag The flag to process
		/// @param[in] parentPointer A pointer to the interface instance
		static void process_flags(std::uint32_t flag, void *parentPointer);

		/// @brief Processes a CAN message
		/// @param[in] message The CAN message being received
		/// @param[in] parentPointer A context variable to find the relevant instance of this class
		static void process_rx_message(CANMessage *message, void *parentPointer);

		static constexpr std::uint32_t GUIDANCE_MESSAGE_TX_INTERVAL_MS = 100; ///< How often guidance messages are sent, defined in ISO 11783-7
		static constexpr std::uint32_t GUIDANCE_MESSAGE_TIMEOUT_MS = 150; ///< Amount of time before a guidance message is stale. We currently tolerate 50ms of delay.
		static constexpr float CURVATURE_COMMAND_OFFSET_INVERSE_KM = 8032.0f; ///< Constant offset for curvature being sent on the bus in km-1
		static constexpr float CURVATURE_COMMAND_MAX_INVERSE_KM = 8031.75f; ///< The maximum curvature that can be encoded once scaling is applied
		static constexpr float CURVATURE_COMMAND_RESOLUTION_PER_BIT = 0.25f; ///< The resolution of the message in km-1 per bit
		static constexpr std::uint16_t ZERO_CURVATURE_INVERSE_KM = 32128; ///< This is the value for zero km-1 for 0.25 km-1 per bit

		/// @brief Sends the agricultural guidance machine info message based on the configured content of agriculturalGuidanceMachineInfoTransmitData
		/// @returns true if the message was sent, otherwise false
		bool send_agricultural_guidance_machine_info() const;

		/// @brief Sends the agricultural guidance system command message based on the configured content of guidanceSystemCommandTransmitData
		/// @returns true if the message was sent, otherwise false
		bool send_guidance_system_command() const;

		/// @brief Parses a message into a AgriculturalGuidanceMachineInfo class
		/// @param[in] message The message to parse
		/// @param[in] machineInfo The class into which the decoded message components will be stored
		static void parse_agricultural_guidance_machine_info_message(CANMessage *message, std::shared_ptr<AgriculturalGuidanceMachineInfo> machineInfo);

		/// @brief Parses a message into a GuidanceSystemCommand class
		/// @param[in] message The message to parse
		/// @param[in] guidanceCommand The class into which the decoded message components will be stored
		static void parse_agricultural_guidance_system_command_message(CANMessage *message, std::shared_ptr<GuidanceSystemCommand> guidanceCommand);

		ProcessingFlags txFlags; ///< Tx flag for sending messages periodically
		std::shared_ptr<InternalControlFunction> sourceControlFunction; ///< The control function to use when sending messages
		std::shared_ptr<ControlFunction> destinationControlFunction; ///< The optional destination to which messages will be sent. If nullptr it will be broadcast instead.
		std::vector<std::shared_ptr<AgriculturalGuidanceMachineInfo>> receivedAgriculturalGuidanceMachineInfoMessages; ///< A list of all received estimated curvatures
		std::vector<std::shared_ptr<GuidanceSystemCommand>> receivedGuidanceSystemCommandMessages; ///< A list of all received curvature commands and statuses
		std::uint32_t guidanceSystemCommandTransmitTimestamp_ms = 0; ///< Timestamp used to know when to transmit the guidance system command message
		std::uint32_t agriculturalGuidanceMachineInfoTransmitTimestamp_ms = 0; ///< Timestamp used to know when to transmit the agricultural guidance machine info message
		bool initialized = false; ///< Stores if the interface has been initialized
	};
} // namespace isobus

#endif // ISOBUS_GUIDANCE_HPP