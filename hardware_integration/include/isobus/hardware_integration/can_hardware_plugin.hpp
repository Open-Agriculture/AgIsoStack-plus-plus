//================================================================================================
/// @file can_hardware_plugin.hpp
///
/// @brief A base class for a CAN driver. Can be derived into your platform's required interface.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef CAN_HARDEWARE_PLUGIN_HPP
#define CAN_HARDEWARE_PLUGIN_HPP

#include <string>
#include "isobus/isobus/can_message_frame.hpp"

namespace isobus
{
	//================================================================================================
	/// @class CANHardwarePlugin
	///
	/// @brief An abstract base class for a CAN driver
	//================================================================================================
	class CANHardwarePlugin
	{
	public:
		virtual ~CANHardwarePlugin() = default;

		/// @brief Returns with the name of the plugin in a format which is suitable to be displayed
		/// to the user for e.g. on a ComboBox
		/// @returns the name of the plugin
		virtual std::string get_name() const = 0;

		/// @brief Returns if the driver is ready and in a good state
		/// @details This should return `false` until `open` is called, and after `close` is called, or
		/// if anything happens that causes the driver to be invalid, like the hardware is disconnected.
		/// @returns `true` if the driver is good/connected, `false` if the driver is not usable
		virtual bool get_is_valid() const = 0;

		/// @brief Disconnects the driver from the hardware.
		virtual void close() = 0;

		/// @brief Connects the driver to the hardware. This will be called to initialize the driver
		/// and connect it to the hardware.
		virtual void open() = 0;

		/// @brief Reads one frame from the bus synchronously
		/// @param[in, out] canFrame The CAN frame that was read
		/// @returns `true` if a CAN frame was read, otherwise `false`
		virtual bool read_frame(isobus::CANMessageFrame &canFrame) = 0;

		/// @brief Writes a frame to the bus (synchronous)
		/// @param[in] canFrame The frame to write to the bus
		/// @returns `true` if the frame was written, otherwise `false`
		virtual bool write_frame(const isobus::CANMessageFrame &canFrame) = 0;
	};
}
#endif // CAN_HARDEWARE_PLUGIN_HPP
