//================================================================================================
/// @file isobus_speed_distance_messages.hpp
///
/// @brief Defines classes for processing/sending ISOBUS speed messages.
/// @attention These classes are meant to be used in the ISOBUS odometry interface, not used
/// directly by a consuming application.
/// @note The full list of standardized speeds can be found at "isobus.net"
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_SPEED_MESSAGES_HPP
#define ISOBUS_SPEED_MESSAGES_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/utility/event_dispatcher.hpp"
#include "isobus/utility/processing_flags.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace isobus
{
	/// @brief This interface manages and parses ISOBUS speed messages
	class SpeedMessagesInterface
	{
	public:
		/// @brief Constructor for a SpeedMessagesInterface
		/// @note Normally you would only configure this interface to transmit if you are
		/// serving as the tractor ECU (TECU).
		/// @param[in] source The internal control function to use when sending messages, or nullptr for listen only
		/// @param[in] enableSendingGroundBasedSpeedPeriodically If true, ground-based speed will be sent periodically. (Normally you will not want to send this unless you are sensing the speed yourself)
		/// @param[in] enableSendingWheelBasedSpeedPeriodically If true, wheel-based speed will be sent periodically. (Normally you will not want to send this unless you are sensing the speed yourself)
		/// @param[in] enableSendingMachineSelectedSpeedPeriodically If true, machine-selected speed will be sent periodically. (Normally you will not want to send this unless you are selecting the speed yourself)
		SpeedMessagesInterface(std::shared_ptr<InternalControlFunction> source,
		                       bool enableSendingGroundBasedSpeedPeriodically = false,
		                       bool enableSendingWheelBasedSpeedPeriodically = false,
		                       bool enableSendingMachineSelectedSpeedPeriodically = false);

		/// @brief Destructor for SpeedMessagesInterface. Cleans up PGN registrations if needed.
		~SpeedMessagesInterface();

		/// @brief Enumerates the values of the direction of travel for the machine.
		enum class MachineDirection : std::uint8_t
		{
			Forward = 0,
			Reverse = 1,
			Error = 2,
			NotAvailable = 3
		};

		static constexpr std::uint16_t SAEvl01_MAX_VALUE = 64255; ///< The maximum valid value for a SAEvl01 slot (see J1939)
		static constexpr std::uint32_t SAEds05_MAX_VALUE = 4211081215; ///< The maximum valid value for a SAEds05 slot (see J1939)

		/// @brief Groups the data encoded in an ISO "Wheel-based Speed and Distance" message
		class WheelBasedMachineSpeedData
		{
		public:
			/// @brief Enumerates the key switch states of the tractor or power unit.
			enum class KeySwitchState : std::uint8_t
			{
				Off = 0, ///< Key is off
				NotOff = 1, ///< Key is not off (does not always mean that it's on!)
				Error = 2,
				NotAvailable = 3
			};

			/// @brief Enumerates the states of a switch or operator input to start or enable implement operations
			enum class ImplementStartStopOperations : std::uint8_t
			{
				StopDisableImplementOperations = 0,
				StartEnableImplementOperations = 1,
				Error = 2,
				NotAvailable = 3
			};

			/// @brief This parameter indicates whether the reported direction is reversed from the perspective of the operator
			enum class OperatorDirectionReversed : std::uint8_t
			{
				NotReversed = 0,
				Reversed = 1,
				Error = 2,
				NotAvailable = 3
			};

			/// @brief Constructor for a WheelBasedMachineSpeedData
			/// @param[in] sender The control function that is sending this message
			explicit WheelBasedMachineSpeedData(ControlFunction *sender);

			std::uint32_t get_machine_distance() const;
			bool set_machine_distance(std::uint32_t distance);

			std::uint16_t get_machine_speed() const;
			bool set_machine_speed(std::uint16_t speed);

			std::uint8_t get_maximum_time_of_tractor_power() const;
			bool set_maximum_time_of_tractor_power(std::uint8_t maxTime);

			MachineDirection get_machine_direction_of_travel() const;
			bool set_machine_direction_of_travel(MachineDirection direction);

			KeySwitchState get_key_switch_state() const;
			bool set_key_switch_state(KeySwitchState state);

			ImplementStartStopOperations get_implement_start_stop_operations_state() const;
			bool set_implement_start_stop_operations_state(ImplementStartStopOperations state);

			OperatorDirectionReversed get_operator_direction_reversed_state() const;
			bool set_operator_direction_reversed_state(OperatorDirectionReversed reverseState);

			/// @brief Returns a pointer to the sender of the message. If an ICF is the sender, returns the ICF being used to transmit from.
			/// @attention The only way you could get an invalid pointer here is if you register a partner, it sends this message, then you delete the partner and
			/// call this function, as that is the only time the stack deletes a control function. That would be abnormal program flow, but at some point
			/// the stack will be updated to return a shared or weak pointer instead, but for now please be aware of that limitation.
			/// Eventually though the message will time-out normally and you can get a new pointer for
			/// the external CF that replaces the deleted partner.
			/// @returns The control function sending this instance of the guidance system command message
			ControlFunction *get_sender_control_function() const;

			/// @brief Sets the timestamp for when the message was received or sent
			/// @param[in] timestamp The timestamp, in milliseconds, when the message was sent or received
			void set_timestamp_ms(std::uint32_t timestamp);

			/// @brief Returns the timestamp for when the message was received, in milliseconds
			/// @returns The timestamp for when the message was received, in milliseconds
			std::uint32_t get_timestamp_ms() const;

		private:
			ControlFunction *const controlFunction; ///< The CF that is sending the message
			std::uint32_t timestamp_ms = 0; ///< A timestamp for when the message was released in milliseconds
			std::uint32_t wheelBasedMachineDistance_mm = 0; ///< Stores the decoded machine wheel-based distance in millimeters
			std::uint16_t wheelBasedMachineSpeed_mm_per_sec = 0; ///< Stores the decoded wheel-based machine speed in mm/s
			std::uint8_t maximumTimeOfTractorPower_min = 0; ///< Stores the maximum time of remaining tractor or power-unit-supplied electrical power at the current load
			MachineDirection machineDirectionState = MachineDirection::NotAvailable; ///< Stores direction of travel.
			KeySwitchState keySwitchState = KeySwitchState::NotAvailable; ///< Stores the key switch state of the tractor or power unit.
			ImplementStartStopOperations implementStartStopOperationsState = ImplementStartStopOperations::NotAvailable; ///< Stores the state of a switch or other operator input to start or enable implement operations.
			OperatorDirectionReversed operatorDirectionReversedState = OperatorDirectionReversed::NotAvailable; ///< Stores whether the reported direction is reversed from the perspective of the operator
		};

		/// @brief Message that provides the current machine selected speed, direction and source parameters.
		/// @details This is usually the best/authoritative source of speed information as chosen by the machine.
		class MachineSelectedSpeedData
		{
		public:
			/// @brief This parameter is used to indicate why the vehicle speed control unit cannot currently accept
			/// remote commands or has most recently stopped accepting remote commands.
			/// @note Some values are reserved or manufacturer specific. See the SPN definition.
			enum class ExitReasonCode : std::uint8_t
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
				VehicleTransmissionGearDoesNotAllowRemoteCommands = 22,
				Error = 62,
				NotAvailable = 63
			};

			/// @brief An indication of the speed source that is currently being reported in the machine selected speed parameter.
			/// @details This parameter is an indication of the speed source that is currently being reported in the machine selected speed parameter.
			/// Simulated speed is a system-generated speed message to permit implement operations when the machine is not actually moving.
			/// Blended speed is a speed message that uses a combination of the actual speed sources based on the operator's or the manufacturer's selected logic,
			/// i.e. when a ground-based speed source is less than 0.5 m/s, the speed message will then send the wheel speed source.
			enum class SpeedSource : std::uint8_t
			{
				WheelBasedSpeed = 0, ///< Wheel encoder usually
				GroundBasedSpeed = 1, ///< Radar usually
				NavigationBasedSpeed = 2, ///< GNSS Usually
				Blended = 3, ///< Some combination of source fusion
				Simulated = 4, ///< A test speed
				Reserved_1 = 5, ///< Reserved
				Reserved_2 = 6, ///< Reserved
				NotAvailable = 7 ///< N/A
			};

			/// @brief This parameter is used to report the Tractor ECU's present limit
			/// status associated with a parameter whose commands are persistent.
			/// Similar to other SAEbs03 limit statuses.
			enum class LimitStatus : std::uint8_t
			{
				NotLimited = 0,
				OperatorLimitedControlled = 1, ///< Request cannot be implemented
				LimitedHigh = 2, ///< Only lower command values result in a change
				LimitedLow = 3, ///< Only higher command values result in a change
				Reserved_1 = 4, ///< Reserved
				Reserved_2 = 5, ///< Reserved
				NonRecoverableFault = 6,
				NotAvailable ///< Parameter not supported
			};

			/// @brief Constructor for a MachineSelectedSpeedData
			/// @param[in] sender The control function that is sending this message
			explicit MachineSelectedSpeedData(ControlFunction *sender);

			std::uint32_t get_machine_distance() const;
			bool set_machine_distance(std::uint32_t distance);

			std::uint16_t get_machine_speed() const;
			bool set_machine_speed(std::uint16_t speed);

			std::uint8_t get_exit_reason_code() const;
			bool set_exit_reason_code(std::uint8_t exitCode);

			SpeedSource get_speed_source() const;
			bool set_speed_source(SpeedSource selectedSource);

			LimitStatus get_limit_status() const;
			bool set_limit_status(LimitStatus statusToSet);

			MachineDirection get_machine_direction_of_travel() const;
			bool set_machine_direction_of_travel(MachineDirection directionOfTravel);

			/// @brief Returns a pointer to the sender of the message. If an ICF is the sender, returns the ICF being used to transmit from.
			/// @attention The only way you could get an invalid pointer here is if you register a partner, it sends this message, then you delete the partner and
			/// call this function, as that is the only time the stack deletes a control function. That would be abnormal program flow, but at some point
			/// the stack will be updated to return a shared or weak pointer instead, but for now please be aware of that limitation.
			/// Eventually though the message will time-out normally and you can get a new pointer for
			/// the external CF that replaces the deleted partner.
			/// @returns The control function sending this instance of the guidance system command message
			ControlFunction *get_sender_control_function() const;

			/// @brief Sets the timestamp for when the message was received or sent
			/// @param[in] timestamp The timestamp, in milliseconds, when the message was sent or received
			void set_timestamp_ms(std::uint32_t timestamp);

			/// @brief Returns the timestamp for when the message was received, in milliseconds
			/// @returns The timestamp for when the message was received, in milliseconds
			std::uint32_t get_timestamp_ms() const;

		private:
			ControlFunction *const controlFunction; ///< The CF that is sending the message
			std::uint32_t timestamp_ms = 0; ///< A timestamp for when the message was released in milliseconds
			std::uint32_t machineSelectedSpeedDistance_mm = 0; ///< Stores the machine selected speed distance in millimeters
			std::uint16_t machineSelectedSpeed_mm_per_sec = 0; ///< Stores the machine selected speed in mm/s
			std::uint8_t exitReasonCode = static_cast<std::uint8_t>(ExitReasonCode::NotAvailable); ///< Stores why the machine has most recently stopped accepting remote commands.
			SpeedSource source = SpeedSource::NotAvailable; ///< Stores the speed source that is currently being reported
			LimitStatus limitStatus = LimitStatus::NotAvailable; ///< Stores the tractor ECU limit status
			MachineDirection machineDirectionState = MachineDirection::NotAvailable; ///< Stores direction of travel.
		};

		/// @brief Message normally sent by the Tractor ECU on the implement bus on construction and
		/// agricultural implements providing to connected systems the current measured ground speed
		/// (also includes a free-running distance counter and an indication of the direction of travel).
		///
		/// @note Accuracies of both wheel-based and ground-based sources can be speed-dependent and degrade at low speeds.
		/// Wheel-based information might not be updated at the 100ms rate at low speeds.
		class GroundBasedSpeedData
		{
		public:
			/// @brief Constructor for a GroundBasedSpeedData
			/// @param[in] sender The control function that is sending this message
			explicit GroundBasedSpeedData(ControlFunction *sender);

			std::uint32_t get_machine_distance() const;
			bool set_machine_distance(std::uint32_t distance);

			std::uint16_t get_machine_speed() const;
			bool set_machine_speed(std::uint16_t speed);

			MachineDirection get_machine_direction_of_travel() const;
			bool set_machine_direction_of_travel(MachineDirection directionOfTravel);

			/// @brief Returns a pointer to the sender of the message. If an ICF is the sender, returns the ICF being used to transmit from.
			/// @attention The only way you could get an invalid pointer here is if you register a partner, it sends this message, then you delete the partner and
			/// call this function, as that is the only time the stack deletes a control function. That would be abnormal program flow, but at some point
			/// the stack will be updated to return a shared or weak pointer instead, but for now please be aware of that limitation.
			/// Eventually though the message will time-out normally and you can get a new pointer for
			/// the external CF that replaces the deleted partner.
			/// @returns The control function sending this instance of the guidance system command message
			ControlFunction *get_sender_control_function() const;

			/// @brief Sets the timestamp for when the message was received or sent
			/// @param[in] timestamp The timestamp, in milliseconds, when the message was sent or received
			void set_timestamp_ms(std::uint32_t timestamp);

			/// @brief Returns the timestamp for when the message was received, in milliseconds
			/// @returns The timestamp for when the message was received, in milliseconds
			std::uint32_t get_timestamp_ms() const;

		private:
			ControlFunction *const controlFunction; ///< The CF that is sending the message
			std::uint32_t timestamp_ms = 0; ///< A timestamp for when the message was released in milliseconds
			std::uint32_t groundBasedMachineDistance_mm = 0; ///< Stores the ground-based speed's distance in millimeters
			std::uint16_t groundBasedMachineSpeed_mm_per_sec = 0; ///< Stores the ground-based speed in mm/s
			MachineDirection machineDirectionState = MachineDirection::NotAvailable; ///< Stores direction of travel.
		};

		/// @brief Sets up the class and registers it to receive callbacks from the network manager for processing
		/// guidance messages. The class will not receive messages if this is not called.
		void initialize();

		/// @brief Returns if the interface has been initialized
		/// @returns true if initialize has been called for this interface, otherwise false
		bool get_initialized() const;

		/// @brief Use this to configure transmission of the machine selected speed message.
		/// If you pass in an internal control function to the constructor of this class, then this message is available to be sent.
		MachineSelectedSpeedData machineSelectedSpeedTransmitData;

		/// @brief Use this to configure transmission of the wheel-based speed message.
		/// If you pass in an internal control function to the constructor of this class, then this message is available to be sent.
		WheelBasedMachineSpeedData wheelBasedSpeedTransmitData;

		/// @brief Use this to configure transmission of the ground-based speed message.
		/// If you pass in an internal control function to the constructor of this class, then this message is available to be sent.
		GroundBasedSpeedData groundBasedSpeedTransmitData;

		/// @brief Returns the number of received, unique wheel-based speed message sources
		/// @returns The number of CFs sending the wheel-based speed message
		std::size_t get_number_received_wheel_based_speed_sources() const;

		/// @brief Returns the number of received, unique ground-based speed message sources
		/// @returns The number of CFs sending the ground-based speed message
		std::size_t get_number_received_ground_based_speed_sources() const;

		/// @brief Returns the number of received, unique machine selected speed message sources
		/// @returns The number of CFs sending the machine selected speed message
		std::size_t get_number_received_machine_selected_speed_sources() const;

		/// @brief Returns the content of the machine selected speed message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the machine selected speed message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these commands. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		std::shared_ptr<MachineSelectedSpeedData> get_received_machine_selected_speed(std::size_t index);

		/// @brief Returns the content of the wheel-based speed message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the wheel-based speed message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these commands. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		std::shared_ptr<WheelBasedMachineSpeedData> get_received_wheel_based_speed(std::size_t index);

		/// @brief Returns the content of the ground-based speed message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the ground-based speed message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these commands. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		std::shared_ptr<GroundBasedSpeedData> get_received_ground_based_speed(std::size_t index);

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated wheel-based speed messages are received.
		/// @returns The event publisher for wheel-based speed messages
		EventDispatcher<const std::shared_ptr<WheelBasedMachineSpeedData>, bool> &get_wheel_based_machine_speed_data_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated machine selected speed messages are received.
		/// @returns The event publisher for machine selected speed messages
		EventDispatcher<const std::shared_ptr<MachineSelectedSpeedData>, bool> &get_machine_selected_speed_data_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated ground-based speed messages are received.
		/// @returns The event publisher for ground-based speed messages
		EventDispatcher<const std::shared_ptr<GroundBasedSpeedData>, bool> &get_ground_based_machine_speed_data_event_publisher();

		/// @brief Call this cyclically to update the interface. Transmits messages if needed and processes
		/// timeouts for received messages.
		void update();

	protected:
		/// @brief Enumerates a set of flags to manage transmitting messages owned by this interface
		enum class TransmitFlags : std::uint32_t
		{
			SendWheelBasedSpeed = 0, ///< A flag to manage sending wheel-based speed
			SendMachineSelectedSpeed, ///< A flag to manage sending machine selected speed
			SendGroundBasedSpeed, ///< A flag to manage sending ground-based speed

			NumberOfFlags ///< The number of flags in this enumeration
		};

		static constexpr std::uint32_t SPEED_DISTANCE_MESSAGE_TX_INTERVAL_MS = 100; ///< The interval (in milliseconds) defined in ISO11783-7 for sending this class's messages
		static constexpr std::uint32_t SPEED_DISTANCE_MESSAGE_RX_TIMEOUT_MS = 3 * SPEED_DISTANCE_MESSAGE_TX_INTERVAL_MS; ///< A (somewhat arbitrary) timeout for detecting stale messages.

		/// @brief Processes one flag (which sends the associated message)
		/// @param[in] flag The flag to process
		/// @param[in] parentPointer A pointer to the interface instance
		static void process_flags(std::uint32_t flag, void *parentPointer);

		/// @brief Processes a CAN message
		/// @param[in] message The CAN message being received
		/// @param[in] parentPointer A context variable to find the relevant instance of this class
		static void process_rx_message(const CANMessage &message, void *parentPointer);

		/// @brief Sends the machine selected speed message
		/// @returns true if the message was sent, otherwise false
		bool send_machine_selected_speed() const;

		/// @brief Sends the wheel-based speed message
		/// @returns true if the message was sent, otherwise false
		bool send_wheel_based_speed() const;

		/// @brief Sends the ground-based speed message
		/// @returns true if the message was sent, otherwise false
		bool send_ground_based_speed() const;

		ProcessingFlags txFlags; ///< Tx flag for sending messages periodically
		EventDispatcher<const std::shared_ptr<WheelBasedMachineSpeedData>, bool> wheelBasedMachineSpeedDataEventPublisher; ///< An event publisher for notifying when new wheel-based speed messages are received
		EventDispatcher<const std::shared_ptr<MachineSelectedSpeedData>, bool> machineSelectedSpeedDataEventPublisher; ///< An event publisher for notifying when new machine selected speed messages are received
		EventDispatcher<const std::shared_ptr<GroundBasedSpeedData>, bool> groundBasedSpeedDataEventPublisher; ///< An event publisher for notifying when new ground-based speed messages are received
		std::vector<std::shared_ptr<WheelBasedMachineSpeedData>> receivedWheelBasedSpeedMessages; ///< A list of all received wheel-based speed messages
		std::vector<std::shared_ptr<MachineSelectedSpeedData>> receivedMachineSelectedSpeedMessages; ///< A list of all received machine selected speed messages
		std::vector<std::shared_ptr<GroundBasedSpeedData>> receivedGroundBasedSpeedMessages; ///< A list of all received ground-based speed messages
		std::uint32_t wheelBasedSpeedTransmitTimestamp_ms = 0; ///< Timestamp used to know when to transmit the wheel-based speed message in milliseconds
		std::uint32_t machineSelectedSpeedTransmitTimestamp_ms = 0; ///< Timestamp used to know when to transmit the machine selected speed message in milliseconds
		std::uint32_t groundBasedSpeedTransmitTimestamp_ms = 0; ///< Timestamp used to know when to transmit the ground-based speed message in milliseconds
		bool initialized = false; ///< Stores if the interface has been initialized
	};
} // namespace isobus

#endif // ISOBUS_SPEED_MESSAGES_HPP
