//================================================================================================
/// @file isobus_heartbeat.hpp
///
/// @brief Defines an interface for sending and receiving ISOBUS heartbeats.
/// The heartbeat message is used to determine the integrity of the communication of messages and
/// parameters being transmitted by a control function. There may be multiple instances of the
/// heartbeat message on the network, and CFs are required transmit the message on request.
/// As long as the heartbeat message is transmitted at the regular
/// time interval and the sequence number increases through the valid range, then the
/// heartbeat message indicates that the data source CF is operational and provides
/// correct data in all its messages.
///
/// @author Adrian Del Grosso
/// @author Sujan Dumaru
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_HEARTBEAT_HPP
#define ISOBUS_HEARTBEAT_HPP

#include "isobus/isobus/can_callbacks.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/utility/event_dispatcher.hpp"
#include "isobus/utility/thread_synchronization.hpp"

#include <atomic>
#include <list>
#include <vector>

namespace isobus
{
	/// @brief This class is used to send and receive ISOBUS heartbeats.
	class HeartbeatInterface
	{
	public:
		/// @brief This enum is used to define the possible errors that can occur when receiving a heartbeat.
		enum class HeartBeatError
		{
			InvalidSequenceCounter, ///< Sequence error: the counter repeated, or increased by more than 3
			TimedOut, ///< The heartbeat message has not been received within the repetition rate
			TransmissionError, ///< The sender reported an error by sending sequence counter 254
			GracefulShutdown, ///< The sender is shutting down (sequence counter 255) and is no longer tracked
			RequestNotAccepted ///< The request was not sent, or neither a Response for Repetition Rate nor a heartbeat came within 250 ms, so the control function is no longer tracked
		};

		/// @brief Constructor for a HeartbeatInterface
		/// @param[in] sendCANFrameCallback A callback used to send CAN frames
		HeartbeatInterface(const CANMessageFrameCallback &sendCANFrameCallback);

		/// @brief This can be used to disable or enable this heartbeat functionality.
		/// It's probably best to leave it enabled for most applications, but it's not
		/// strictly needed. Disabling drops all tracking and sending state, and enabling starts from scratch.
		/// @note The interface is enabled by default.
		/// @param[in] enable Set to true to enable the interface, or false to disable it.
		void set_enabled(bool enable);

		/// @brief Returns if the interface is currently enabled or not.
		/// @note The interface is enabled by default.
		/// @returns true if the interface is enabled, false if the interface is disabled
		bool is_enabled() const;

		/// @brief Requests that another control function start sending the heartbeat message.
		/// The request is queued and sent by the next update(), unless the destination is already tracked.
		/// If it cannot be sent, or neither a Response for Repetition Rate nor a heartbeat arrives within
		/// 250 ms, a RequestNotAccepted error follows and the destination is not tracked. Once tracked, a
		/// TimedOut error follows when no heartbeat arrives for more than 300 ms. The first heartbeat fires
		/// the new tracked heartbeat event.
		/// @param[in] sourceControlFunction The internal control function to use when sending the request
		/// @param[in] destinationControlFunction The destination for the request
		/// @returns true if the request was queued, otherwise false.
		bool request_heartbeat(std::shared_ptr<InternalControlFunction> sourceControlFunction,
		                       std::shared_ptr<ControlFunction> destinationControlFunction) const;

		/// @brief From the next update on, the heartbeat carries 255 (graceful shutdown) until the control function is removed.
		/// The setting is kept and applies once the heartbeat is requested.
		/// @param[in] controlFunction The internal control function that is shutting down
		void send_shutdown(std::shared_ptr<InternalControlFunction> controlFunction) const;

		/// @brief While set, the heartbeat carries 254 instead of the counter.
		/// The setting is kept and applies once the heartbeat is requested.
		/// @param[in] controlFunction The internal control function to change
		/// @param[in] error True to report a transmission error, false to clear it
		void set_transmission_error(std::shared_ptr<InternalControlFunction> controlFunction, bool error) const;

		/// @brief Called by the internal control function class when a new internal control function is added.
		/// This allows us to respond to requests for heartbeats from other control functions.
		/// @param[in] newControlFunction The new internal control function
		void on_new_internal_control_function(std::shared_ptr<InternalControlFunction> newControlFunction);

		/// @brief Called when an internal control function is deleted. Removes its PGN request registration.
		/// @param[in] destroyedControlFunction The destroyed internal control function
		void on_destroyed_internal_control_function(std::shared_ptr<InternalControlFunction> destroyedControlFunction);

		/// @brief Returns an event dispatcher for heartbeat errors.
		/// The control function that generated the error is passed to the event.
		/// @returns An event dispatcher for heartbeat errors
		EventDispatcher<HeartBeatError, std::shared_ptr<ControlFunction>> &get_heartbeat_error_event_dispatcher();

		/// @brief Returns an event dispatcher that fires when a new control function starts being tracked.
		/// The tracked control function is passed to the event.
		/// @returns An event dispatcher for new tracked heartbeat events
		EventDispatcher<std::shared_ptr<ControlFunction>> &get_new_tracked_heartbeat_event_dispatcher();

		/// @brief Returns an event dispatcher that fires when a tracked control function leaves a
		/// sequence or transmission error after 8 correct sequential heartbeats in a row.
		/// The control function that recovered is passed to the event.
		/// @returns An event dispatcher for heartbeat recovery events
		EventDispatcher<std::shared_ptr<ControlFunction>> &get_heartbeat_recovered_event_dispatcher();

		/// @brief Processes a CAN message, called by the network manager.
		/// @param[in] message The CAN message being received
		void process_rx_message(const CANMessage &message);

		/// @brief Updates the interface. Called by the network manager,
		/// so there is no need for you to call it in your application.
		void update();

	private:
		/// @brief This enum is used to define special values for the sequence counter.
		enum class SequenceCounterSpecialValue : std::uint8_t
		{
			Initial = 251, ///< The heartbeat sequence number value shall be set to 251 once upon initialization of a CF.
			FirstReserved = 252, ///< Reserved; receivers shall ignore it
			LastReserved = 253, ///< Reserved; receivers shall ignore it
			Error = 254, ///< Sequence Number value 254 indicates an error condition.
			NotAvailable = 255 ///< The sender is shutting down; also marks a tracked control function that has not sent a heartbeat yet
		};

		/// @brief The state of a tracked heartbeat, as seen by the receiver.
		enum class ReceiveState : std::uint8_t
		{
			Operational, ///< Heartbeats are arriving with valid sequence counters
			SequenceError, ///< The counter repeated or increased by more than 3, and has not yet recovered
			TransmissionError ///< The sender reported an error with sequence counter 254, and has not yet recovered
		};

		/// @brief The kinds of request the application can make of this interface.
		enum class CommandType : std::uint8_t
		{
			RequestHeartbeat, ///< Request the heartbeat from another control function
			Shutdown, ///< Make an internal control function send the graceful shutdown value
			SetTransmissionError, ///< Make an internal control function send the transmission error value
			ClearTransmissionError, ///< Make an internal control function resume sending the counter
			Remove ///< Stop tracking an internal control function that was deactivated
		};

		static constexpr std::uint32_t SEQUENCE_TIMEOUT_MS = 300; ///< If the repetition rate exceeds 300 ms an error in the communication is detected.
		static constexpr std::uint32_t SEQUENCE_INITIAL_RESPONSE_TIMEOUT_MS = 250; ///< When requesting a heartbeat from another device, If no response for the repetition rate has been received after 250 ms, the requester shall assume that the request was not accepted
		static constexpr std::uint32_t SEQUENCE_REPETITION_RATE_MS = 100; ///< A consuming CF shall send a Request for Repetition rate for the heart beat message with a repetition rate of 100 ms
		static constexpr std::uint8_t MAX_SEQUENCE_COUNTER = 250; ///< The sequence counter runs from 0 to 250, then restarts at 0
		static constexpr std::uint8_t MAX_ALLOWED_INCREMENT = 3; ///< An increase of more than 3 is a sequence error
		static constexpr std::uint8_t RECOVERY_MESSAGE_COUNT = 8; ///< Correct sequential heartbeats in a row needed to leave an error state
		static constexpr std::uint16_t REPETITION_RATE_DEFAULT = 0; ///< A requested rate of 0 asks for the default rate, which for the heartbeat is 100 ms
		static constexpr std::uint16_t REPETITION_RATE_NO_CHANGE = 0xFFFF; ///< A requested rate of 0xFFFF means the rate shall not change

		/// @brief A request from the application, applied by update() so that only the update thread touches trackedHeartbeats.
		class PendingCommand
		{
		public:
			CommandType type; ///< What the application asked for
			std::shared_ptr<InternalControlFunction> source; ///< The control function to send the request from (only for RequestHeartbeat)
			std::shared_ptr<ControlFunction> controlFunction; ///< The control function the command applies to
		};

		/// @brief This class is used to store information about a tracked heartbeat.
		class Heartbeat
		{
		public:
			/// @brief Constructor for a Heartbeat
			/// @param[in] sendingControlFunction The control function that is sending the heartbeat
			explicit Heartbeat(std::shared_ptr<ControlFunction> sendingControlFunction);

			/// @brief Transmits a heartbeat message (for internal control functions only).
			/// Updates the sequence counter and timestamp if needed.
			/// @param[in] parent The parent interface
			/// @returns True if the message is sent, otherwise false.
			bool send(const HeartbeatInterface &parent);

			std::shared_ptr<ControlFunction> controlFunction; ///< The CF that is sending the message
			std::uint32_t timestamp_ms; ///< The last time the message was sent by the associated control function
			std::uint8_t sequenceCounter = static_cast<std::uint8_t>(SequenceCounterSpecialValue::Initial); ///< The sequence counter used to validate the heartbeat. Counts from 0-250 normally.
			ReceiveState state = ReceiveState::Operational; ///< For received heartbeats, whether the sender is in an error state
			std::uint8_t correctMessagesInARow = 0; ///< For received heartbeats, the correct sequential messages counted towards recovery
			bool shutdown = false; ///< For internal control functions, true once the graceful shutdown value is being sent
			bool transmissionError = false; ///< For internal control functions, true while the transmission error value is being sent
			bool requestPending = false; ///< For requested heartbeats, true until a Response for Repetition Rate or a heartbeat arrives from the control function
			bool sending = false; ///< For internal control functions, true once the heartbeat has been requested, so it is being sent
			bool timedOut = false; ///< For received heartbeats in an error state that stopped arriving; kept so the 8 correct heartbeats are still needed when it resumes
		};

		/// @brief Processes a PGN request for a heartbeat.
		/// @param[in] parameterGroupNumber The PGN being requested
		/// @param[in] requestingControlFunction The control function that is requesting the heartbeat
		/// @param[in] targetControlFunction The control function that is being requested to send the heartbeat
		/// @param[in] repetitionRate The repetition rate for the heartbeat
		/// @param[in] parentPointer A context variable to find the relevant instance of this class
		/// @returns True if the request was accepted, otherwise false.
		static bool process_request_for_heartbeat(std::uint32_t parameterGroupNumber,
		                                          std::shared_ptr<ControlFunction> requestingControlFunction,
		                                          std::shared_ptr<ControlFunction> targetControlFunction,
		                                          std::uint32_t repetitionRate,
		                                          void *parentPointer);

		/// @brief Queues a command for the next update() unless the interface is disabled.
		/// @param[in] command The command to queue
		/// @returns True if the command was queued.
		bool queue_command(const PendingCommand &command) const;

		/// @brief Applies a request from the application. Called from update().
		/// @param[in] command The request to apply
		void apply_command(const PendingCommand &command);

		/// @brief Sends a due local heartbeat or checks a remote heartbeat for timeout.
		/// @param[in,out] heartbeat The tracked heartbeat to update
		/// @returns True if the heartbeat should be removed from tracking.
		bool update_tracked_heartbeat(Heartbeat &heartbeat);

		/// @brief Sends a local heartbeat when due and advances its counter on success.
		/// @param[in,out] heartbeat The local heartbeat to send
		void send_heartbeat_if_due(Heartbeat &heartbeat) const;

		/// @brief Checks the sequence counter of a received heartbeat against the previous one and updates the state.
		/// @param[in,out] heartbeat The tracked heartbeat the message belongs to
		/// @param[in] sequenceCounter The sequence counter that was received
		void process_sequence_counter(Heartbeat &heartbeat, std::uint8_t sequenceCounter);

		/// @brief Puts a tracked heartbeat in an error state and raises the error once on entering it.
		/// @param[in,out] heartbeat The tracked heartbeat in error
		/// @param[in] errorState The error state to enter
		void enter_error_state(Heartbeat &heartbeat, ReceiveState errorState);

		/// @brief Finds the tracked heartbeat of a control function.
		/// @param[in] controlFunction The control function to look for
		/// @returns The tracked heartbeat, or trackedHeartbeats.end() if it is not tracked
		std::list<Heartbeat>::iterator find_tracked_heartbeat(const std::shared_ptr<ControlFunction> &controlFunction);

		const CANMessageFrameCallback sendCANFrameCallback; ///< A callback for sending a CAN frame
		EventDispatcher<HeartBeatError, std::shared_ptr<ControlFunction>> heartbeatErrorEventDispatcher; ///< Event dispatcher for heartbeat errors
		EventDispatcher<std::shared_ptr<ControlFunction>> newTrackedHeartbeatEventDispatcher; ///< Event dispatcher for when a heartbeat message from another control function becomes tracked by this interface
		EventDispatcher<std::shared_ptr<ControlFunction>> heartbeatRecoveredEventDispatcher; ///< Event dispatcher for when a heartbeat recovers from an error state
		mutable std::vector<PendingCommand> pendingCommands; ///< Requests from the application, waiting for the next update
		mutable Mutex pendingCommandsMutex; ///< Protects pendingCommands
		std::list<Heartbeat> trackedHeartbeats; ///< Store tracked heartbeat data, per CF
		std::atomic_bool enabled{ true }; ///< Attribute that specifies if this interface is enabled. When false, the interface does nothing.
	};
} // namespace isobus

#endif
