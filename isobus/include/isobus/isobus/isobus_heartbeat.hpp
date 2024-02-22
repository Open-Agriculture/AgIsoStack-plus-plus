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
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_HEARTBEAT_HPP
#define ISOBUS_HEARTBEAT_HPP

#include "isobus/isobus/can_callbacks.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/utility/event_dispatcher.hpp"

#include <list>

namespace isobus
{
	/// @brief This class is used to send and receive ISOBUS heartbeats.
	class HeartbeatInterface
	{
	public:
		/// @brief This enum is used to define the possible errors that can occur when receiving a heartbeat.
		enum class HeartBeatError
		{
			InvalidSequenceCounter, ///< The sequence counter is not valid
			TimedOut ///< The heartbeat message has not been received within the repetition rate
		};

		/// @brief Constructor for a HeartbeatInterface
		/// @param[in] sendCANFrameCallback A callback used to send CAN frames
		HeartbeatInterface(const CANMessageFrameCallback &sendCANFrameCallback);

		/// @brief This can be used to disable or enable this heartbeat functionality.
		/// It's probably best to leave it enabled for most applications, but it's not
		/// strictly needed.
		/// @note The interface is enabled by default.
		/// @param[in] enable Set to true to enable the interface, or false to disable it.
		void set_enabled(bool enable);

		/// @brief Returns if the interface is currently enabled or not.
		/// @note The interface is enabled by default.
		/// @returns true if the interface is enabled, false if the interface is disabled
		bool is_enabled() const;

		/// @brief This method can be used to request that another control function on the bus
		/// start sending the heartbeat message. This does not mean the request will be honored.
		/// In order to know if your request was accepted, you will need to either
		/// register for timeout events, register for heartbeat events, or check to see if your
		/// destination control function ever responded at some later time using the various methods
		/// available to you on this class' public interface.
		/// @note CFs may take up to 250ms to begin sending the heartbeat.
		/// @param[in] sourceControlFunction The internal control function to use when sending the request
		/// @param[in] destinationControlFunction The destination for the request
		/// @returns true if the request was transmitted, otherwise false.
		bool request_heartbeat(std::shared_ptr<InternalControlFunction> sourceControlFunction,
		                       std::shared_ptr<ControlFunction> destinationControlFunction) const;

		/// @brief Called by the internal control function class when a new internal control function is added.
		/// This allows us to respond to requests for heartbeats from other control functions.
		/// @param[in] newControlFunction The new internal control function
		void on_new_internal_control_function(std::shared_ptr<InternalControlFunction> newControlFunction);

		/// @brief Called when an internal control function is deleted. Cleans up stale registrations with
		/// PGN request protocol.
		/// @param[in] destroyedControlFunction The destroyed internal control function
		void on_destroyed_internal_control_function(std::shared_ptr<InternalControlFunction> destroyedControlFunction);

		/// @brief Returns an event dispatcher which can be used to register for heartbeat errors.
		/// Heartbeat errors are generated when a heartbeat message is not received within the
		/// repetition rate, or when the sequence counter is not valid.
		/// The control function that generated the error is passed as an argument to the event.
		/// @returns An event dispatcher for heartbeat errors
		EventDispatcher<HeartBeatError, std::shared_ptr<ControlFunction>> &get_heartbeat_error_event_dispatcher();

		/// @brief Returns an event dispatcher which can be used to register for new tracked heartbeat events.
		/// An event will be generated when a new control function is added to the list of CFs sending heartbeats.
		/// The control function that generated the error is passed as an argument to the event.
		/// @returns An event dispatcher for new tracked heartbeat events
		EventDispatcher<std::shared_ptr<ControlFunction>> &get_new_tracked_heartbeat_event_dispatcher();

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
			Error = 254, ///< Sequence Number value 254 indicates an error condition.
			NotAvailable = 255 ///< This value shall be used when the transmitted CF is in a shutdown status and is gracefully disconnecting from the network.
		};

		static constexpr std::uint32_t SEQUENCE_TIMEOUT_MS = 300; ///< If the repetition rate exceeds 300 ms an error in the communication is detected.
		static constexpr std::uint32_t SEQUENCE_INITIAL_RESPONSE_TIMEOUT_MS = 250; ///< When requesting a heartbeat from another device, If no response for the repetition rate has been received after 250 ms, the requester shall assume that the request was not accepted
		static constexpr std::uint32_t SEQUENCE_REPETITION_RATE_MS = 100; ///< A consuming CF shall send a Request for Repetition rate for the heart beat message with a repetition rate of 100 ms

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
			std::uint32_t repetitionRate_ms = SEQUENCE_REPETITION_RATE_MS; ///< For internal control functions, this controls how often the heartbeat is sent. This should really stay at the standard 100ms defined in ISO11783-7.
			std::uint8_t sequenceCounter = static_cast<std::uint8_t>(SequenceCounterSpecialValue::Initial); ///< The sequence counter used to validate the heartbeat. Counts from 0-250 normally.
		};

		/// @brief Processes a PGN request for a heartbeat.
		/// @param[in] parameterGroupNumber The PGN being requested
		/// @param[in] requestingControlFunction The control function that is requesting the heartbeat
		/// @param[in] targetControlFunction The control function that is being requested to send the heartbeat
		/// @param[in] repetitionRate The repetition rate for the heartbeat
		/// @param[in] parentPointer A context variable to find the relevant instance of this class
		/// @returns True if the request was transmitted, otherwise false.
		static bool process_request_for_heartbeat(std::uint32_t parameterGroupNumber,
		                                          std::shared_ptr<ControlFunction> requestingControlFunction,
		                                          std::shared_ptr<ControlFunction> targetControlFunction,
		                                          std::uint32_t repetitionRate,
		                                          void *parentPointer);

		const CANMessageFrameCallback sendCANFrameCallback; ///< A callback for sending a CAN frame
		EventDispatcher<HeartBeatError, std::shared_ptr<ControlFunction>> heartbeatErrorEventDispatcher; ///< Event dispatcher for heartbeat errors
		EventDispatcher<std::shared_ptr<ControlFunction>> newTrackedHeartbeatEventDispatcher; ///< Event dispatcher for when a heartbeat message from another control function becomes tracked by this interface
		std::list<Heartbeat> trackedHeartbeats; ///< Store tracked heartbeat data, per CF
		bool enabled = true; ///< Attribute that specifies if this interface is enabled. When false, the interface does nothing.
	};
} // namespace isobus

#endif
