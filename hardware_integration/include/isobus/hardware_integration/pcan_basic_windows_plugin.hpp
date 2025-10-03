//================================================================================================
/// @file pcan_basic_windows_plugin.hpp
///
/// @brief An interface for using a PEAK PCAN device.
/// @attention Use of the PEAK driver is governed in part by their license, and requires you
/// to install their driver first, which in-turn requires you to agree to their terms and conditions.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef PCAN_BASIC_WINDOWS_PLUGIN_HPP
#define PCAN_BASIC_WINDOWS_PLUGIN_HPP

#include <string>

// This needs to be included before PCANBasic for definitions of
// anachronistic windows C types like WORD, DWORD etc
#include <Windows.h>

#include "isobus/hardware_integration/PCANBasic.h"
#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

namespace isobus
{
	//================================================================================================
	/// @class PCANBasicWindowsPlugin
	///
	/// @brief A Windows CAN Driver for PEAK PCAN Devices
	//================================================================================================
	class PCANBasicWindowsPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for the Windows version of the PEAK PCAN Basic CAN driver
		/// @param[in] channel The channel to use. See definitions in PCANBasic.g such as `PCAN_USBBUS1`
		explicit PCANBasicWindowsPlugin(WORD channel);

		/// @brief The destructor for PCANBasicWindowsPlugin
		virtual ~PCANBasicWindowsPlugin();

		/// @brief Returns the displayable name of the plugin
		/// @returns PEAK CAN
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
#endif // PCAN_BASIC_WINDOWS_PLUGIN_HPP
