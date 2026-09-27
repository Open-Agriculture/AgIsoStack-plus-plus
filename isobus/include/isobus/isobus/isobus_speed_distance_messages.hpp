//================================================================================================
/// @file isobus_speed_distance_messages.hpp
///
/// @brief Defines classes for processing/sending ISOBUS speed messages.
/// @attention These classes are meant to be used in the ISOBUS odometry interface, not used
/// directly by a consuming application.
/// @note The full list of standardized speeds can be found at "isobus.net"
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_SPEED_MESSAGES_HPP
#define ISOBUS_SPEED_MESSAGES_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/isobus_ground_based_speed.hpp"
#include "isobus/isobus/isobus_machine_selected_speed.hpp"
#include "isobus/isobus/isobus_machine_selected_speed_command.hpp"
#include "isobus/isobus/isobus_wheel_based_speed.hpp"
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
		/// @param[in] enableSendingMachineSelectedSpeedCommandPeriodically If true, machine-selected speed command will be sent periodically. (Normally you will not want to send this unless you are intending to cause machine motion)
		SpeedMessagesInterface(std::shared_ptr<InternalControlFunction> source,
		                       bool enableSendingGroundBasedSpeedPeriodically = false,
		                       bool enableSendingWheelBasedSpeedPeriodically = false,
		                       bool enableSendingMachineSelectedSpeedPeriodically = false,
		                       bool enableSendingMachineSelectedSpeedCommandPeriodically = false);

		/// @brief Destructor for SpeedMessagesInterface. Cleans up PGN registrations if needed.
		~SpeedMessagesInterface();

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

		/// @brief Use this to configure transmission of the machine selected speed command message.
		/// If you pass in an internal control function to the constructor of this class, then this message is available to be sent.
		MachineSelectedSpeedCommandData machineSelectedSpeedCommandTransmitData;

		/// @brief Returns the number of received, unique wheel-based speed message sources
		/// @returns The number of CFs sending the wheel-based speed message
		std::size_t get_number_received_wheel_based_speed_sources() const;

		/// @brief Returns the number of received, unique ground-based speed message sources
		/// @returns The number of CFs sending the ground-based speed message
		std::size_t get_number_received_ground_based_speed_sources() const;

		/// @brief Returns the number of received, unique machine selected speed message sources
		/// @returns The number of CFs sending the machine selected speed message
		std::size_t get_number_received_machine_selected_speed_sources() const;

		/// @brief Returns the number of received, unique machine selected speed command message sources
		/// @returns The number of CFs sending the machine selected speed command message
		std::size_t get_number_received_machine_selected_speed_command_sources() const;

		/// @brief Returns the content of the machine selected speed message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the machine selected speed message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these commands. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		/// @returns The parsed content of the machine selected speed message
		std::shared_ptr<MachineSelectedSpeedData> get_received_machine_selected_speed(std::size_t index);

		/// @brief Returns the content of the wheel-based speed message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the wheel-based speed message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these commands. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		/// @returns The parsed content of the wheel-based speed message
		std::shared_ptr<WheelBasedMachineSpeedData> get_received_wheel_based_speed(std::size_t index);

		/// @brief Returns the content of the ground-based speed message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the ground-based speed message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these commands. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		/// @returns The parsed content of the ground-based speed message
		std::shared_ptr<GroundBasedSpeedData> get_received_ground_based_speed(std::size_t index);

		/// @brief Returns the content of the machine selected speed command message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the machine selected speed command message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these commands. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		/// @returns The parsed content of the machine selected speed command message
		std::shared_ptr<MachineSelectedSpeedCommandData> get_received_machine_selected_speed_command(std::size_t index);

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated wheel-based speed messages are received.
		/// @returns The event publisher for wheel-based speed messages
		EventDispatcher<const std::shared_ptr<WheelBasedMachineSpeedData>, bool> &get_wheel_based_machine_speed_data_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated machine selected speed messages are received.
		/// @returns The event publisher for machine selected speed messages
		EventDispatcher<const std::shared_ptr<MachineSelectedSpeedData>, bool> &get_machine_selected_speed_data_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated ground-based speed messages are received.
		/// @returns The event publisher for ground-based speed messages
		EventDispatcher<const std::shared_ptr<GroundBasedSpeedData>, bool> &get_ground_based_machine_speed_data_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated machine selected speed command messages are received.
		/// @returns The event publisher for machine selected speed command messages
		EventDispatcher<const std::shared_ptr<MachineSelectedSpeedCommandData>, bool> &get_machine_selected_speed_command_data_event_publisher();

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
			SendMachineSelectedSpeedCommand, ///< A flag to manage sending the machine selected speed command message

			NumberOfFlags ///< The number of flags in this enumeration
		};

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

		/// @brief Sends the machine selected speed command message
		/// @returns true if the message was sent, otherwise false
		bool send_machine_selected_speed_command() const;

		ProcessingFlags txFlags; ///< Tx flag for sending messages periodically
		EventDispatcher<const std::shared_ptr<WheelBasedMachineSpeedData>, bool> wheelBasedMachineSpeedDataEventPublisher; ///< An event publisher for notifying when new wheel-based speed messages are received
		EventDispatcher<const std::shared_ptr<MachineSelectedSpeedData>, bool> machineSelectedSpeedDataEventPublisher; ///< An event publisher for notifying when new machine selected speed messages are received
		EventDispatcher<const std::shared_ptr<GroundBasedSpeedData>, bool> groundBasedSpeedDataEventPublisher; ///< An event publisher for notifying when new ground-based speed messages are received
		EventDispatcher<const std::shared_ptr<MachineSelectedSpeedCommandData>, bool> machineSelectedSpeedCommandDataEventPublisher; ///< An event publisher for notifying when new machine selected speed command messages are received
		std::vector<std::shared_ptr<WheelBasedMachineSpeedData>> receivedWheelBasedSpeedMessages; ///< A list of all received wheel-based speed messages
		std::vector<std::shared_ptr<MachineSelectedSpeedData>> receivedMachineSelectedSpeedMessages; ///< A list of all received machine selected speed messages
		std::vector<std::shared_ptr<GroundBasedSpeedData>> receivedGroundBasedSpeedMessages; ///< A list of all received ground-based speed messages
		std::vector<std::shared_ptr<MachineSelectedSpeedCommandData>> receivedMachineSelectedSpeedCommandMessages; ///< A list of all received ground-based speed messages
		std::uint32_t wheelBasedSpeedTransmitTimestamp_ms = 0; ///< Timestamp used to know when to transmit the wheel-based speed message in milliseconds
		std::uint32_t machineSelectedSpeedTransmitTimestamp_ms = 0; ///< Timestamp used to know when to transmit the machine selected speed message in milliseconds
		std::uint32_t groundBasedSpeedTransmitTimestamp_ms = 0; ///< Timestamp used to know when to transmit the ground-based speed message in milliseconds
		std::uint32_t machineSelectedSpeedCommandTransmitTimestamp_ms = 0; ///< Timestamp used to know when to transmit the ground-based speed message in milliseconds
		bool initialized = false; ///< Stores if the interface has been initialized
	};
} // namespace isobus

#endif // ISOBUS_SPEED_MESSAGES_HPP
