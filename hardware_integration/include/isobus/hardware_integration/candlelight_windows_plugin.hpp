//================================================================================================
/// @file candlelight_windows_plugin.hpp
///
/// @copyright 2026 Mauricio. Distributed under the MIT license.
///
/// @brief AgIsoStack++ CAN driver for candleLight/gs_usb adapters (e.g. the WeAct USB2CANFDV1
/// running its "candlelight" firmware) on Windows, talking directly over WinUSB.
///
/// @details Implements the classic-CAN candleLight/gs_usb vendor control and bulk-transfer
/// protocol over WinUSB. Protocol reference: https://github.com/candle-usb/candleLight_fw
/// Windows reference driver: https://github.com/HubertD/cangaroo
/// Development hardware used Elmue's candleLight firmware on WeActStudio USB2CANFD v1:
/// https://github.com/Elmue/CANable-2.5-firmware-Slcan-and-Candlelight
/// The firmware must expose WinUSB with the candleLight device interface GUID.
/// Classic CAN only (8 byte frames), which is all ISOBUS needs.
//================================================================================================
#ifndef CANDLELIGHT_WINDOWS_PLUGIN_HPP
#define CANDLELIGHT_WINDOWS_PLUGIN_HPP

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_message_frame.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace isobus
{
	/// @brief Windows WinUSB transport for classic CAN using candleLight/gs_usb firmware.
	/// @details One instance owns one USB adapter. CANHardwareInterface owns the receive/transmit
	/// threads and stops them before close(). This plugin does not provide cross-process sharing.
	class CandleLightWindowsPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief Creates a disconnected plugin; no USB operations occur until open().
		/// @param[in] deviceIndex Which candleLight-compatible device to open, if more than one
		/// is plugged in (0 = first found). Use get_last_error() after a failed open() to see why.
		/// @param[in] bitrate CAN bitrate in bit/s. ISOBUS is always 250000.
		/// @param[in] channel Which CAN channel on the adapter to use (0 for a single channel device)
		explicit CandleLightWindowsPlugin(std::uint8_t deviceIndex = 0, std::uint32_t bitrate = 250000, std::uint8_t channel = 0);
		/// @brief Closes the adapter and releases its Windows handles.
		~CandleLightWindowsPlugin() override;

		/// @brief Returns the displayable driver name.
		/// @returns The candleLight/gs_usb WinUSB driver name.
		std::string get_name() const override;
		/// @brief Checks whether initialization succeeded and no fatal USB error was detected.
		/// @returns True when the adapter is ready for CAN I/O.
		bool get_is_valid() const override;
		/// @brief Stops the CAN channel, if usable, and releases acquired resources.
		void close() override;
		/// @brief Opens the adapter and configures its classic CAN channel.
		/// @details Failures leave the plugin invalid and are available through get_last_error().
		void open() override;

		/// @brief Reads one classic CAN data frame with a bounded USB wait.
		/// @param[out] canFrame The received frame; unchanged if no valid frame was read.
		/// @returns True if a frame was received, false on timeout or error.
		bool read_frame(isobus::CANMessageFrame &canFrame) override;
		/// @brief Writes one classic CAN data frame with a bounded USB wait.
		/// @param[in] canFrame The frame to transmit on the configured device channel.
		/// @returns True if USB accepted the entire frame, false on invalid input, timeout or error.
		bool write_frame(const isobus::CANMessageFrame &canFrame) override;

		/// @brief A short, human readable reason the last open() failed, or an empty string
		/// @returns The last initialization error, or an empty string after successful initialization.
		std::string get_last_error() const override;

	private:
		struct Impl; ///< Holds the WinUSB/SetupAPI handles so <windows.h> stays out of this header

		const std::uint8_t deviceIndex; ///< Index among USB interfaces with the candleLight GUID.
		const std::uint32_t bitrate; ///< Requested CAN bit rate in bits per second.
		const std::uint8_t channel; ///< Device CAN channel selected for control and frame transfers.
		std::atomic_bool valid; ///< Initialization and fatal I/O state, shared by stack I/O threads.
		std::unique_ptr<Impl> impl; ///< Windows handles and protocol state owned by this instance.
		std::string lastError; ///< Survives after impl is torn down, unlike Impl::lastError
	};
}

#endif // CANDLELIGHT_WINDOWS_PLUGIN_HPP
