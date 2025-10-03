//================================================================================================
/// @file sys_tec_windows_plugin.hpp
///
/// @brief An interface for using a SYS TEC sysWORXX USB CAN device.
/// @attention Make sure you've installed the appropriate driver software for this device before
/// you use this plugin. Visit https://www.systec-electronic.com/ for the needed software.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef SYS_TEC_WINDOWS_PLUGIN_HPP
#define SYS_TEC_WINDOWS_PLUGIN_HPP

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

// These needs to be included before Usbcan32.h for definitions of
// anachronistic Microsoft C types like WORD, DWORD, and TCHAR
#include <Windows.h>
#include <tchar.h>

#include "isobus/hardware_integration/Usbcan32.h"

namespace isobus
{
	/// @brief A Windows CAN Driver for SYS TEC electronic AG USB CAN modules.
	class SysTecWindowsPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for the Windows SYS TEC plugin
		/// @param[in] channel The channel to use. See definitions in USBCAN32.h, such as USBCAN_CHANNEL_CH0
		/// @param[in] baudrate The baudrate to use when connecting to the bus, typically 250k
		SysTecWindowsPlugin(std::uint8_t channel = USBCAN_CHANNEL_CH0, std::uint32_t baudrate = USBCAN_BAUD_250kBit);

		/// @brief Constructor for the Windows SYS TEC plugin
		/// @param[in] serialNumber The serial number of the device to connect to
		/// @param[in] baudrate The baudrate to use when connecting to the bus, typically 250k
		SysTecWindowsPlugin(std::uint32_t serialNumber, std::uint32_t baudrate = USBCAN_BAUD_250kBit);

		/// @brief The destructor for PCANBasicWindowsPlugin
		virtual ~SysTecWindowsPlugin();

		/// @brief Returns the displayable name of the plugin
		/// @returns SYS TEC
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
		std::uint32_t serialNumber = 0; ///< The serial number to connect to, or 0 if not used
		std::uint16_t baudrateConstant = USBCAN_BAUD_250kBit; ///< The constant used to configure the adapter's baudrate
		std::uint8_t channelIndex = 0; ///< The channel for the device, used if you have a multi-channel device
		std::uint8_t handle = USBCAN_INVALID_HANDLE; ///< The handle for the device, used to interact with the DLL
		bool openResult = false; ///< Stores the result of the call to begin CAN communication. Used for is_valid check later.
	};
}
#endif // SYS_TEC_WINDOWS_PLUGIN_HPP
