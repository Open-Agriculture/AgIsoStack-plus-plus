//================================================================================================
/// @file socket_can_interface.hpp
///
/// @brief An interface for using socket CAN on linux. Mostly for testing, but it could be
/// used in any application to get the stack hooked up to the bus.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef SOCKET_CAN_INTERFACE_HPP
#define SOCKET_CAN_INTERFACE_HPP

#include <string>

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

struct sockaddr_can; ///< Forward declare the linux sockaddr_can struct

namespace isobus
{
	//================================================================================================
	/// @class SocketCANInterface
	///
	/// @brief A CAN Driver for Linux socket CAN
	//================================================================================================
	class SocketCANInterface : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for the socket CAN driver
		/// @param[in] deviceName The device name to use, like "can0" or "vcan0"
		explicit SocketCANInterface(const std::string deviceName);

		/// @brief The destructor for SocketCANInterface
		virtual ~SocketCANInterface();

		/// @brief Returns the displayable name of the plugin
		/// @returns SocketCAN
		std::string get_name() const override;

		/// @brief Returns if the socket connection is valid
		/// @returns `true` if connected, `false` if not connected
		bool get_is_valid() const override;

		/// @brief Returns the device name the driver is using
		/// @returns The device name the driver is using, such as "can0" or "vcan0"
		std::string get_device_name() const;

		/// @brief Closes the socket
		void close() override;

		/// @brief Connects to the socket
		void open() override;

		/// @brief Returns a frame from the hardware (synchronous), or `false` if no frame can be read.
		/// @param[in, out] canFrame The CAN frame that was read
		/// @returns `true` if a CAN frame was read, otherwise `false`
		bool read_frame(isobus::CANMessageFrame &canFrame) override;

		/// @brief Writes a frame to the bus (synchronous)
		/// @param[in] canFrame The frame to write to the bus
		/// @returns `true` if the frame was written, otherwise `false`
		bool write_frame(const isobus::CANMessageFrame &canFrame) override;

		/// @brief Changes the name of the device to use, which only works if the device is not open
		/// @param[in] newName The new name for the device (such as "can0" or "vcan0")
		/// @returns `true` if the name was changed, otherwise `false` (if the device is open this will return false)
		bool set_name(const std::string &newName);

	private:
		struct sockaddr_can *pCANDevice; ///< The structure for CAN sockets
		std::string name; ///< The device name
		int fileDescriptor; ///< File descriptor for the socket
	};
}
#endif // SOCKET_CAN_INTERFACE_HPP
