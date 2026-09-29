//================================================================================================
/// @file mac_can_pcan_plugin.hpp
///
/// @brief An interface for using a PEAK PCAN device through the MacCAN PCBUSB driver.
/// @attention Use of this is governed in part by the MacCAN EULA
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef MAC_CAN_PCAN_PLUGIN_HPP
#define MAC_CAN_PCAN_PLUGIN_HPP

#include <string>

#include "isobus/hardware_integration/PCBUSB.h"
#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

namespace isobus
{
	//================================================================================================
	/// @class MacCANPCANPlugin
	///
	/// @brief A Mac OS CAN Driver for PEAK PCAN Devices
	//================================================================================================
	class MacCANPCANPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for the MacCAN PCAN plugin
		/// @param[in] channel The channel to use. See definitions in PCBUSB.h such as `PCAN_USBBUS1`
		explicit MacCANPCANPlugin(WORD channel);

		/// @brief The destructor for MacCANPCANPlugin
		virtual ~MacCANPCANPlugin();

		/// @brief Returns the displayable name of the plugin
		/// @returns MacCAN
		std::string get_name() const override;

		/// @brief Returns if the connection with the hardware is valid
		/// @returns `true` if connected, `false` if not connected
		bool get_is_valid() const override;

		/// @brief Closes the connection to the hardware
		void close() override;

		/// @brief Connects to the hardware you specified in the constructor's channel argument
		void open() override;

		/// @brief Returns a frame from the hardware (synchronous), or `false` if no frame can be read.
		/// @param[in, out] canFrame The CAN frame that was read
		/// @returns `true` if a CAN frame was read, otherwise `false`
		bool read_frame(isobus::CANMessageFrame &canFrame) override;

		/// @brief Writes a frame to the bus (synchronous)
		/// @param[in] canFrame The frame to write to the bus
		/// @returns `true` if the frame was written, otherwise `false`
		bool write_frame(const isobus::CANMessageFrame &canFrame) override;

	private:
		TPCANHandle handle; ///< The handle as defined in the PCAN driver API
		TPCANStatus openResult; ///< Stores the result of the call to begin CAN communication. Used for is_valid check later.
	};
}
#endif // MAC_CAN_PCAN_PLUGIN_HPP
