//================================================================================================
/// @file can_hardware_interface_single_thread.hpp
///
/// @brief The hardware abstraction layer that separates the stack from the underlying CAN driver
/// @note This version of the CAN Hardware Interface is meant for systems without multi-threading.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef CAN_HARDWARE_INTERFACE_SINGLE_THREAD_HPP
#define CAN_HARDWARE_INTERFACE_SINGLE_THREAD_HPP

#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"
#include "isobus/utility/event_dispatcher.hpp"

namespace isobus
{
	//================================================================================================
	/// @class CANHardwareInterface
	///
	/// @brief Provides a common queuing layer for running the CAN stack and all CAN drivers and is
	/// meant for systems that are single threaded or do not support std::thread and its friends.
	///
	/// @details The `CANHardwareInterface` class was created to provide a common queuing
	/// layer for running the CAN stack and all CAN drivers to simplify integration and crucially to
	/// provide a consistent, safe order of operations for all the function calls needed to properly
	/// drive the stack.
	//================================================================================================
	class CANHardwareInterface
	{
	public:
		/// @brief Returns the number of configured CAN channels that the class is managing
		/// @returns The number of configured CAN channels that the class is managing
		static std::uint8_t get_number_of_can_channels();

		/// @brief Sets the number of CAN channels to manage
		/// @details Allocates the proper number of `CanHardware` objects to track
		/// each CAN channel's Tx and Rx message queues. If you pass in a smaller number than what was
		/// already configured, it will delete the unneeded `CanHardware` objects.
		/// @note The function will fail if the channel is already assigned to a driver or the interface is already started
		/// @param value The number of CAN channels to manage
		/// @returns `true` if the channel count was set, otherwise `false`.
		static bool set_number_of_can_channels(std::uint8_t value);

		/// @brief Assigns a CAN driver to a channel
		/// @param[in] channelIndex The channel to assign to
		/// @param[in] canDriver The driver to assign to the channel
		/// @note The function will fail if the channel is already assigned to a driver or the interface is already started
		/// @returns `true` if the driver was assigned to the channel, otherwise `false`
		static bool assign_can_channel_frame_handler(std::uint8_t channelIndex, std::shared_ptr<CANHardwarePlugin> canDriver);

		/// @brief Removes a CAN driver from a channel
		/// @param[in] channelIndex The channel to remove the driver from
		/// @note The function will fail if the channel is already assigned to a driver or the interface is already started
		/// @returns `true` if the driver was removed from the channel, otherwise `false`
		static bool unassign_can_channel_frame_handler(std::uint8_t channelIndex);

		/// @brief Initializes the hardware interface
		/// @returns `true` if the interface was initialized, otherwise false (perhaps you already called start)
		static bool start();

		/// @brief Cleans up and discards all remaining messages in the Tx and Rx queues.
		/// @returns `true` if the interface was stopped, otherwise `false`
		static bool stop();

		/// @brief Checks if the CAN stack and CAN drivers are running
		/// @returns `true` if the threads are running, otherwise `false`
		static bool is_running();

		/// @brief Called externally, adds a message to a CAN channel's Tx queue
		/// @param[in] frame The frame to add to the Tx queue
		/// @returns `true` if the frame was accepted, otherwise `false` (maybe wrong channel assigned)
		static bool transmit_can_frame(const isobus::CANMessageFrame &frame);

		/// @brief Get the event dispatcher for when a CAN message frame is received from hardware event
		/// @returns The event dispatcher which can be used to register callbacks/listeners to
		static isobus::EventDispatcher<const isobus::CANMessageFrame &> &get_can_frame_received_event_dispatcher();

		/// @brief Get the event dispatcher for when a CAN message frame will be send to hardware event
		/// @returns The event dispatcher which can be used to register callbacks/listeners to
		static isobus::EventDispatcher<const isobus::CANMessageFrame &> &get_can_frame_transmitted_event_dispatcher();

		/// @brief Call this periodically. Does most of the work of this class.
		/// @note You must call this very often, such as least every millisecond to ensure CAN messages get retrieved from the hardware
		static void update();

	private:
		/// @brief Stores the Tx/Rx queues, mutexes, and driver needed to run a single CAN channel
		struct CANHardware
		{
			std::deque<isobus::CANMessageFrame> messagesToBeTransmitted; ///< Tx message queue for a CAN channel
			std::deque<isobus::CANMessageFrame> receivedMessages; ///< Rx message queue for a CAN channel
			std::shared_ptr<CANHardwarePlugin> frameHandler; ///< The CAN driver to use for a CAN channel
		};

		/// @brief Singleton instance of the CANHardwareInterface class
		/// @details This is a little hack that allows to have the destructor called
		static CANHardwareInterface SINGLETON;

		/// @brief Constructor for the CANHardwareInterface class
		CANHardwareInterface() = default;

		/// @brief Destructor for the CANHardwareInterface class
		~CANHardwareInterface() = default;

		/// @brief The receive thread(s) execute this function
		/// @param[in] channelIndex The associated CAN channel for the thread
		static void receive_can_frame(std::uint8_t channelIndex);

		/// @brief Attempts to write a frame using the driver assigned to a frame's channel
		/// @param[in] frame The frame to try and write to the bus
		static bool transmit_can_frame_from_buffer(const isobus::CANMessageFrame &frame);

		static isobus::EventDispatcher<const isobus::CANMessageFrame &> frameReceivedEventDispatcher; ///< The event dispatcher for when a CAN message frame is received from hardware event
		static isobus::EventDispatcher<const isobus::CANMessageFrame &> frameTransmittedEventDispatcher; ///< The event dispatcher for when a CAN message has been transmitted via hardware

		static std::vector<std::unique_ptr<CANHardware>> hardwareChannels; ///< A list of all CAN channel's metadata
		static bool started; ///< Stores if the threads have been started
	};
}
#endif // CAN_HARDWARE_INTERFACE_SINGLE_THREAD_HPP
