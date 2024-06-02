//================================================================================================
/// @file isobus_maintain_power_interface.hpp
///
/// @brief Defines an interface for sending and receiving the maintain power message (PGN 65095).
/// @details This interface is for managing the maintain power message, which is a
/// message sent by any CF connected to the implement bus requesting that the Tractor
/// ECU (TECU) not switch off the power for 2s after it has received the wheel-based speed and
/// distance message indicating that the ignition has been switched off.
/// The message also includes the connected implement(s) operating state.
/// You can choose if the TECU maintains actuator power independently of ECU power as well.
/// You might want to maintain actuator power to ensure your section valves close when keyed off.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_MAINTAIN_POWER_INTERFACE_HPP
#define ISOBUS_MAINTAIN_POWER_INTERFACE_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/utility/event_dispatcher.hpp"
#include "isobus/utility/processing_flags.hpp"

#include <memory>

namespace isobus
{
	/// @brief Manages sending and receiving the maintain power message (PGN 65095)
	class MaintainPowerInterface
	{
	public:
		/// @brief Stores information sent/received in a maintain power message.
		class MaintainPowerData
		{
		public:
			/// @brief Signal that indicates that an implement is connected to a tractor or power unit and is in work state.
			/// @details SPN 7447
			enum class ImplementInWorkState : std::uint8_t
			{
				ImplementNotInWorkState = 0,
				ImplementInWorkState = 1,
				ErrorIndication = 2,
				NotAvailable = 3
			};

			/// @brief Signal that indicates that an implement is connected to a tractor or power unit and is ready for work.
			/// @details SPN 1871
			enum class ImplementReadyToWorkState
			{
				ImplementNotReadyForFieldWork = 0,
				ImplementReadyForFieldWork = 1,
				ErrorIndication = 2,
				NotAvailable = 3
			};

			/// @brief Indicates the state of an implement where it may be disconnected from a tractor or power unit.
			/// @details SPN 1870
			enum class ImplementParkState
			{
				ImplementMayNotBeDisconnected = 0,
				ImplementMayBeDisconnected = 1,
				ErrorIndication = 2,
				NotAvailable = 3
			};

			/// @brief Indicates the transport state of an implement connected to a tractor or power unit.
			/// @details SPN 1869
			enum class ImplementTransportState
			{
				ImplementMayNotBeTransported = 0,
				ImplementMayBeTransported = 1,
				ErrorIndication = 2,
				NotAvailable = 3
			};

			/// @brief Enumerates the different states that can be requested in the "Maintain Actuator Power" SPN
			/// @details SPN 1868
			enum class MaintainActuatorPower
			{
				NoFurtherRequirementForPWR = 0,
				RequirementFor2SecondsMoreForPWR = 1,
				Reserved = 2,
				DontCare = 3
			};

			/// @brief Enumerates the different states that can be requested in the "Maintain ECU Power" SPN
			/// @details SPN 1867
			enum class MaintainECUPower
			{
				NoFurtherRequirementForECU_PWR = 0,
				RequirementFor2SecondsMoreForECU_PWR = 1,
				Reserved = 2,
				DontCare = 3
			};

			/// @brief Constructor for a MaintainPowerData object, which stores information sent/received in a maintain power message.
			/// @param[in] sendingControlFunction The control function to use if sending the message
			explicit MaintainPowerData(std::shared_ptr<ControlFunction> sendingControlFunction);

			/// @brief Sets the reported implement in-work state
			/// @param[in] inWorkState The reported implement in-work state to set
			/// @returns True if the set value was different from the stored value, otherwise false.
			bool set_implement_in_work_state(ImplementInWorkState inWorkState);

			/// @brief Returns the reported implement in-work state
			/// @return The reported implement in-work state
			ImplementInWorkState get_implement_in_work_state() const;

			/// @brief Sets the reported implement ready to work state
			/// @param[in] readyToWorkState The reported implement ready to work state
			/// @returns True if the set value was different from the stored value, otherwise false.
			bool set_implement_ready_to_work_state(ImplementReadyToWorkState readyToWorkState);

			/// @brief Returns the reported implement ready to work state
			/// @return The reported implement ready to work state
			ImplementReadyToWorkState get_implement_ready_to_work_state() const;

			/// @brief Sets the reported implement park state
			/// @param[in] parkState The reported implement park state to set
			/// @returns True if the set value was different from the stored value, otherwise false.
			bool set_implement_park_state(ImplementParkState parkState);

			/// @brief Returns the reported implement park state
			/// @returns The reported implement park state
			ImplementParkState get_implement_park_state() const;

			/// @brief Sets the reported implement transport state
			/// @param[in] transportState The reported implement transport state to set
			/// @returns True if the set value was different from the stored value, otherwise false.
			bool set_implement_transport_state(ImplementTransportState transportState);

			/// @brief Returns the reported implement transport state
			/// @returns The reported implement transport state
			ImplementTransportState get_implement_transport_state() const;

			/// @brief Sets the reported maintain actuator power state
			/// @param[in] maintainState The reported maintain actuator power state
			/// @returns True if the set value was different from the stored value, otherwise false.
			bool set_maintain_actuator_power(MaintainActuatorPower maintainState);

			/// @brief Returns the reported maintain actuator power state
			/// @returns The reported maintain actuator power state
			MaintainActuatorPower get_maintain_actuator_power() const;

			/// @brief Sets the reported maintain ECU power state
			/// @param[in] maintainState The reported maintain ECU power state
			/// @returns True if the set value was different from the stored value, otherwise false.
			bool set_maintain_ecu_power(MaintainECUPower maintainState);

			/// @brief Returns the reported maintain ECU power state
			/// @returns The reported maintain ECU power state
			MaintainECUPower get_maintain_ecu_power() const;

			/// @brief Returns a pointer to the sender of the message. If an ICF is the sender, returns the ICF being used to transmit from.
			/// @attention The only way you could get an invalid pointer here is if you register a partner, it sends this message, then you delete the partner and
			/// call this function, as that is the only time the stack deletes a control function. That would be abnormal program flow, but at some point
			/// the stack will be updated to return a shared or weak pointer instead, but for now please be aware of that limitation.
			/// Eventually though the message will time-out normally and you can get a new pointer for
			/// the external CF that replaces the deleted partner.
			/// @returns The control function sending this instance of the guidance system command message
			std::shared_ptr<ControlFunction> get_sender_control_function() const;

			/// @brief Sets the timestamp for when the message was received or sent
			/// @param[in] timestamp The timestamp, in milliseconds, when the message was sent or received
			void set_timestamp_ms(std::uint32_t timestamp);

			/// @brief Returns the timestamp for when the message was received, in milliseconds
			/// @returns The timestamp for when the message was received, in milliseconds
			std::uint32_t get_timestamp_ms() const;

		private:
			MaintainPowerData() = delete;
			std::shared_ptr<ControlFunction> sendingControlFunction = nullptr; ///< The control function that is sending the message.
			std::uint32_t timestamp_ms = 0; ///< A timestamp for when the message was released in milliseconds
			ImplementInWorkState currentImplementInWorkState = ImplementInWorkState::NotAvailable; ///< The reported implement in-work state
			ImplementReadyToWorkState currentImplementReadyToWorkState = ImplementReadyToWorkState::NotAvailable; ///< The reported implement ready to work state
			ImplementParkState currentImplementParkState = ImplementParkState::NotAvailable; ///< The reported implement park state
			ImplementTransportState currentImplementTransportState = ImplementTransportState::NotAvailable; ///< The reported transport state of the implement
			MaintainActuatorPower currentMaintainActuatorPowerState = MaintainActuatorPower::DontCare; ///< The reported state for maintaining actuator power for 2 more seconds
			MaintainECUPower currentMaintainECUPowerState = MaintainECUPower::DontCare; ///< The reported state for maintaining ECU power for 2 more seconds
		};

		/// @brief Constructor for a MaintainPowerInterface
		/// @param[in] sourceControlFunction The control function to send the message from, or nullptr to listen only
		explicit MaintainPowerInterface(std::shared_ptr<InternalControlFunction> sourceControlFunction);

		/// @brief Destructor for a MaintainPowerInterface
		~MaintainPowerInterface();

		/// @brief Sets up the class and registers it to receive callbacks from the network manager for processing
		/// messages. The class will not receive messages if this is not called.
		void initialize();

		/// @brief Returns if the interface has been initialized
		/// @returns true if initialize has been called for this interface, otherwise false
		bool get_initialized() const;

		/// @brief Use this to tell the interface how long it should transmit the maintain power message
		/// after it detects a key state transition to off. The interface will use whatever you have set in
		/// maintainPowerTransmitData when performing automatic transmission of the message.
		/// @attention The interface will always send the message at least once with what you have configured
		/// in maintainPowerTransmitData if it was set up with an internal control function, but you should
		/// take care to configure maintainPowerTransmitData with the parameters that will ensure you have enough time
		/// to safely stop your section control and shutdown your application, because when we stop sending this message
		/// the TECU may kill power to your device or the actuators without warning.
		/// @param[in] timeToMaintainPower The amount of time in milliseconds that the interface will send the maintain
		/// power message when the key transitions to off.
		void set_maintain_power_time(std::uint32_t timeToMaintainPower);

		/// @brief Returns the amount of time that the interface will continue to send the maintain
		/// power message after it detects a key transition to off.
		/// @returns The amount of time in milliseconds that the interface will send the maintain power message for.
		std::uint32_t get_maintain_power_time() const;

		/// @brief Returns the number of unique senders of the maintain power message
		/// @returns The number of unique senders of the maintain power message
		std::size_t get_number_received_maintain_power_sources() const;

		/// @brief Returns the content of a received maintain power message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the maintain power message
		/// @returns A pointer to the maintain power message data, or nullptr if the index is out of range
		std::shared_ptr<MaintainPowerData> get_received_maintain_power(std::size_t index);

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated maintain power messages are received.
		/// @returns The event publisher for maintain power messages
		EventDispatcher<const std::shared_ptr<MaintainPowerData>, bool> &get_maintain_power_data_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when the key switch transitions from
		/// the not-off state to the off state. When you get this callback, you can then shutdown your application safely.
		/// @note You can get more comprehensive key switch events by using the wheel-selected speed events in the
		/// SpeedMessagesInterface file.
		/// @returns The event publisher for key switch off transitions
		EventDispatcher<> &get_key_switch_transition_off_event_publisher();

		/// @brief Use this to configure the transmission of the maintain power message.
		MaintainPowerData maintainPowerTransmitData;

		/// @brief Call this cyclically to update the interface. Transmits messages if needed and processes
		/// timeouts for received messages.
		void update();

	protected:
		/// @brief Transmits the maintain power message
		/// @returns True if the message was sent, otherwise false
		bool send_maintain_power() const;

		/// @brief Processes one flag (which sends the associated message)
		/// @param[in] flag The flag to process
		/// @param[in] parentPointer A pointer to the interface instance
		static void process_flags(std::uint32_t flag, void *parentPointer);

		/// @brief Processes a CAN message
		/// @param[in] message The CAN message being received
		/// @param[in] parentPointer A context variable to find the relevant instance of this class
		static void process_rx_message(const CANMessage &message, void *parentPointer);

		ProcessingFlags txFlags; ///< Tx flag for sending the maintain power message. Handles retries automatically.

	private:
		/// @brief Enumerates a set of flags to manage transmitting messages owned by this interface
		enum class TransmitFlags : std::uint32_t
		{
			SendMaintainPower = 0, ///< A flag to manage sending the maintain power message

			NumberOfFlags ///< The number of flags in this enumeration
		};

		/// @brief Enumerates the key switch states of the tractor or power unit.
		enum class KeySwitchState : std::uint8_t
		{
			Off = 0, ///< Key is off
			NotOff = 1, ///< Key is not off (does not always mean that it's on!)
			Error = 2,
			NotAvailable = 3
		};

		static constexpr std::uint32_t MAINTAIN_POWER_TIMEOUT_MS = 2000; ///< The amount of time that power can be maintained per message, used as the timeout as well
		std::vector<std::shared_ptr<MaintainPowerData>> receivedMaintainPowerMessages; ///< A list of all received maintain power messages
		EventDispatcher<const std::shared_ptr<MaintainPowerData>, bool> maintainPowerDataEventPublisher; ///< An event publisher for notifying when new maintain power messages are received
		EventDispatcher<> keySwitchOffEventPublisher; ///< An event publisher for notifying when the key switch transitions to the off state
		std::uint32_t keyNotOffTimestamp = 0; ///< A timestamp to track when the key was detected as ON, used to detect transitions to "Not On".
		std::uint32_t keyOffTimestamp = 0; ///< A timestamp to track when the key is off, used to calculate how many messages to send and when to send them.
		std::uint32_t maintainPowerTransmitTimestamp_ms = 0; ///< Timestamp used to know when to transmit the maintain power message in milliseconds
		std::uint32_t maintainPowerTime_ms = 0; ///< The amount of time to ask the TECU to maintain actuator/section power. Will be rounded up to the next 2s mark when sent.
		bool initialized = false; ///< Stores if the interface has been initialized
	};
} // namespace isobus
#endif // ISOBUS_MAINTAIN_POWER_INTERFACE_HPP
