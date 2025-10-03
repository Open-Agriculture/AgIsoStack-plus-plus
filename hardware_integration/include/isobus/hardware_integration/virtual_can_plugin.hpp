//================================================================================================
/// @file virtual_can_plugin.hpp
///
/// @brief An OS and hardware independent virtual CAN interface driver for testing purposes.
/// Any instance connecting to the same channel and in the same process will be able to communicate.
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef VIRTUAL_CAN_PLUGIN_HPP
#define VIRTUAL_CAN_PLUGIN_HPP

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace isobus
{
	//================================================================================================
	/// @class VirtualCANPlugin
	///
	/// @brief An OS and hardware independent virtual CAN interface driver for testing purposes.
	/// @details Any instance connecting to the same channel and in the same process can communicate.
	/// However, this plugin does not implement rate limiting or any other CAN bus specific features,
	/// like prioritization under heavy load.
	//================================================================================================
	class VirtualCANPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for the virtual CAN driver
		/// @param[in] channel The virtual channel name to use. Free to choose.
		/// @param[in] receiveOwnMessages If `true`, the driver will receive its own messages.
		VirtualCANPlugin(const std::string channel = "", const bool receiveOwnMessages = false);

		/// @brief Destructor for the virtual CAN driver
		virtual ~VirtualCANPlugin();

		/// @brief Returns the displayable name of the plugin
		/// @returns Open-Agriculture Virtual CAN
		std::string get_name() const override;

		/// @brief Returns if the socket connection is valid
		/// @returns `true` if connected, `false` if not connected
		bool get_is_valid() const override;

		/// @brief Returns the assigned virtual channel name
		/// @returns The virtual channel name the bus is assigned to
		std::string get_channel_name() const;

		/// @brief Closes the socket
		void close() override;

		/// @brief Connects to the socket
		void open() override;

		/// @brief Returns a frame from the hardware (synchronous), or `false` if no frame can be read. Times out after 1 second.
		/// @param[in, out] canFrame The CAN frame that was read
		/// @returns `true` if a CAN frame was read, otherwise `false`
		bool read_frame(isobus::CANMessageFrame &canFrame) override;

		/// @brief Returns a frame from the hardware (synchronous), or `false` if no frame can be read.
		/// @param[in, out] canFrame The CAN frame that was read
		/// @param[in] timeout The timeout in milliseconds
		/// @returns `true` if a CAN frame was read, otherwise `false`
		bool read_frame(isobus::CANMessageFrame &canFrame, std::uint32_t timeout) const;

		/// @brief Writes a frame to the bus (synchronous)
		/// @param[in] canFrame The frame to write to the bus
		/// @returns `true` if the frame was written, otherwise `false`
		bool write_frame(const isobus::CANMessageFrame &canFrame) override;

		/// @brief Allows us to write messages as if we received them from the bus
		/// @param[in] canFrame The frame to write to the bus
		void write_frame_as_if_received(const isobus::CANMessageFrame &canFrame) const;

		/// @brief Returns if the internal received message queue is empty or not
		/// @returns `true` if the internal received message queue is empty, otherwise false
		bool get_queue_empty() const;

		/// @brief Clear the internal received message queue
		void clear_queue() const;

	private:
		/// @brief A struct holding information about a virtual CAN device
		struct VirtualDevice
		{
			std::deque<isobus::CANMessageFrame> queue; ///< A queue of CAN frames
			std::condition_variable condition; ///< A condition variable to wake us up when a frame is received
		};

		static constexpr size_t MAX_QUEUE_SIZE = 1000; ///< The maximum size of the queue, mostly arbitrary

		static std::mutex mutex; ///< Mutex to access channels and queues for thread safety
		static std::map<std::string, std::vector<std::shared_ptr<VirtualDevice>>> channels; ///< A channel is a vector of devices

		const std::string channel; ///< The virtual channel name
		const bool receiveOwnMessages; ///< If `true`, the driver will receive its own messages

		std::shared_ptr<VirtualDevice> ourDevice; ///< A pointer to the virtual device of this instance
		std::atomic_bool running; ///< If `true`, the driver is running
	};
}
#endif // VIRTUAL_CAN_PLUGIN_HPP
