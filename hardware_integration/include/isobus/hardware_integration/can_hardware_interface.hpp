//================================================================================================
/// @file can_hardware_interface.hpp
///
/// @brief The hardware abstraction layer that separates the stack from the underlying CAN driver
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef CAN_HARDWARE_INTERFACE_HPP
#define CAN_HARDWARE_INTERFACE_HPP

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_frame.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"

//================================================================================================
/// @class CANHardwareInterface
///
/// @brief Provides a common queuing and thread layer for running the CAN stack and all CAN drivers
///
/// @details The `CANHardwareInterface` class was created to provide a common queuing and thread
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

	/// @brief Starts the threads for managing the CAN stack and CAN drivers
	/// @returns `true` if the threads were started, otherwise false (perhaps they are already running)
	static bool start();

	/// @brief Stops all CAN management threads and discards all remaining messages in the Tx and Rx queues.
	/// @returns `true` if the threads were stopped, otherwise `false`
	static bool stop();

	/// @brief Called externally, adds a message to a CAN channel's Tx queue
	/// @param[in] packet The packet to add to the Tx queue
	/// @returns `true` if the packet was accepted, otherwise `false` (maybe wrong channel assigned)
	static bool transmit_can_message(const isobus::HardwareInterfaceCANFrame &packet);

	/// @brief Adds an Rx callback. The added callback will be called any time a CAN message is received.
	/// @param[in] callback The callback to add
	/// @param[in] parentPointer Generic context variable, usually a pointer to the owner class for this callback
	/// @returns `true` if the callback was added, `false` if it was already in the list
	static bool add_raw_can_message_rx_callback(void (*callback)(isobus::HardwareInterfaceCANFrame &rxFrame, void *parentPointer), void *parentPointer);

	/// @brief Removes a Rx callback
	/// @param[in] callback The callback to remove
	/// @param[in] parentPointer Generic context variable, usually a pointer to the owner class for this callback
	/// @returns `true` if the callback was removed, `false` if no callback matched the two parameters
	static bool remove_raw_can_message_rx_callback(void (*callback)(isobus::HardwareInterfaceCANFrame &rxFrame, void *parentPointer), void *parentPointer);

	/// @brief Set the period between calls to the can lib update callback in milliseconds
	/// @param[in] value The period between update calls in milliseconds
	/// @note All changes to the update delay will be ignored if `start` has been called and the threads are running
	static void set_can_driver_update_period(std::uint32_t value);

	/// @brief Adds a periodic update callback
	/// @param[in] callback The callback to add
	/// @param[in] parentPointer Generic context variable, usually a pointer to the owner class for this callback
	/// @returns `true` if the callback was added, `false` if it was already in the list
	static bool add_can_lib_update_callback(void (*callback)(void *parentPointer), void *parentPointer);

	/// @brief Removes a periodic update callback
	/// @param[in] callback The callback to remove
	/// @param[in] parentPointer Generic context variable, usually a pointer to the owner class for this callback
	/// @returns `true` if the callback was removed, `false` if no callback matched the two parameters
	static bool remove_can_lib_update_callback(void (*callback)(void *parentPointer), void *parentPointer);

private:
	/// @brief A class to store information about CAN lib update callbacks
	class CanLibUpdateCallbackInfo
	{
	public:
		/// @brief Allows easy comparison of callback data
		/// @param obj the object to compare against
		bool operator==(const CanLibUpdateCallbackInfo &obj) const;

		void (*callback)(void *parentPointer) = nullptr; ///< The callback
		void *parent = nullptr; ///< Context variable, the owner of the callback
	};

	/// @brief A class to store information about Rx callbacks
	class RawCanMessageCallbackInfo
	{
	public:
		/// @brief Allows easy comparison of callback data
		/// @param obj the object to compare against
		bool operator==(const RawCanMessageCallbackInfo &obj) const;

		void (*callback)(isobus::HardwareInterfaceCANFrame &rxFrame, void *parentPointer) = nullptr; ///< The callback
		void *parent = nullptr; ///< Context variable, the owner of the callback
	};

	/// @brief Stores the Tx/Rx queues, mutexes, and driver needed to run a single CAN channel
	struct CANHardware
	{
		std::mutex messagesToBeTransmittedMutex; ///< Mutex to protect the Tx queue
		std::deque<isobus::HardwareInterfaceCANFrame> messagesToBeTransmitted; ///< Tx message queue for a CAN channel

		std::mutex receivedMessagesMutex; ///< Mutex to protect the Rx queue
		std::deque<isobus::HardwareInterfaceCANFrame> receivedMessages; ///< Rx message queue for a CAN channel

		std::unique_ptr<std::thread> receiveMessageThread; ///< Thread to manage getting messages from a CAN channel

		std::shared_ptr<CANHardwarePlugin> frameHandler; ///< The CAN driver to use for a CAN channel
	};

	/// @brief Singleton instance of the CANHardwareInterface class
	/// @details This is a little hack that allows to have the destructor called
	static CANHardwareInterface SINGLETON;

	/// @brief Constructor for the CANHardwareInterface class
	CANHardwareInterface() = default;

	/// @brief Deconstructor for the CANHardwareInterface class
	virtual ~CANHardwareInterface();

	/// @brief The default update interval for the CAN stack. Mostly arbitrary
	static constexpr std::uint32_t PERIODIC_UPDATE_INTERVAL = 4;

	/// @brief The main CAN thread executes this function. Does most of the work of this class
	static void can_thread_function();

	/// @brief The receive thread(s) execute this function
	/// @param[in] channelIndex The associated CAN channel for the thread
	static void receive_message_thread_function(std::uint8_t channelIndex);

	/// @brief Attempts to write a frame using the driver assigned to a packet's channel
	/// @param[in] packet The packet to try and write to the bus
	static bool transmit_can_message_from_buffer(isobus::HardwareInterfaceCANFrame &packet);

	/// @brief The periodic update thread executes this function
	static void update_can_lib_periodic_function();

	/// @brief Stops all threads related to the hardware interface
	static void stop_threads();

	static std::unique_ptr<std::thread> canThread; ///< The main CAN thread
	static std::unique_ptr<std::thread> periodicUpdateThread; ///< A thread that periodically wakes up to update the CAN stack

	static std::vector<std::unique_ptr<CANHardware>> hardwareChannels; ///< A list of all CAN channel's metadata
	static std::vector<RawCanMessageCallbackInfo> rxCallbacks; ///< A list of all registered Rx callbacks
	static std::vector<CanLibUpdateCallbackInfo> periodicUpdateCallbacks; ///< A list of all registered periodic update callbacks

	static std::mutex hardwareChannelsMutex; ///< Mutex to protect `hardwareChannels`
	static std::mutex threadMutex; ///< A mutex for the main CAN thread
	static std::mutex rxCallbacksMutex; ///< A mutex for protecting the `rxCallbacks`
	static std::mutex periodicUpdateCallbacksMutex; ///< A mutex for protecting the `periodicUpdateCallbacks`
	static std::condition_variable threadConditionVariable; ///< A condition variable to allow for signaling the CAN thread from `periodicUpdateThread`
	static std::atomic_bool threadsStarted; ///< Stores if `start` has been called yet
	static std::atomic_bool canLibNeedsUpdate; ///< Stores if the CAN thread needs to update the CAN stack this iteration
	static std::uint32_t canLibUpdatePeriod; ///< The period between calls to the CAN stack update function in milliseconds
};

#endif // CAN_HARDWARE_INTERFACE_HPP
