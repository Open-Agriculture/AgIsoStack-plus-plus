//================================================================================================
/// @file innomaker_usb2can_windows_plugin.hpp
///
/// @brief An interface for using an InnoMaker USB2CAN device.
/// @attention This is not legal advice. The InnoMaker USB2CAN driver uses libusb, which is
/// an LGPL-2.1 library. Be sure you understand the implications of this before proceeding.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef INNOMAKER_USB2CAN_PLUGIN_HPP
#define INNOMAKER_USB2CAN_PLUGIN_HPP

#include <cstdint>
#include <memory>
#include <string>

#include "isobus/hardware_integration/InnoMakerUsb2CanLib.h"
#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

namespace isobus
{
	//================================================================================================
	/// @class InnoMakerUSB2CANWindowsPlugin
	///
	/// @brief A Windows CAN Driver for InnoMaker USB2CAN devices
	//================================================================================================
	class InnoMakerUSB2CANWindowsPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief The baudrates supported by the InnoMaker USB2CAN device
		enum Baudrate
		{
			B20k, ///< 20 kbps baudrate
			B33k3, ///< 33.3 kbps baudrate
			B40k, ///< 40 kbps baudrate
			B50k, ///< 50 kbps baudrate
			B66k6, ///< 66.6 kbps baudrate
			B80k, ///< 80 kbps baudrate
			B83k3, ///< 83.3 kbps baudrate
			B100k, ///< 100 kbps baudrate
			B125k, ///< 125 kbps baudrate
			B200k, ///< 200 kbps baudrate
			B250k, ///< 250 kbps baudrate
			B400k, ///< 400 kbps baudrate
			B500k, ///< 500 kbps baudrate
			B666k, ///< 666 kbps baudrate
			B800k, ///< 800 kbps baudrate
			B1000k, ///< 1000 kbps baudrate
		};

		/// @brief Constructor for the Windows version of the InnoMaker USB2CAN Windows CAN driver
		/// @param[in] channel The channel to use by index, passed directly to the consuming driver
		/// @param[in] baudrate The baud rate to configure the device for. Typically 250k baud
		explicit InnoMakerUSB2CANWindowsPlugin(int channel, Baudrate baudrate = B250k);

		/// @brief The destructor for InnoMakerUSB2CANWindowsPlugin
		virtual ~InnoMakerUSB2CANWindowsPlugin();

		/// @brief Returns the displayable name of the plugin
		/// @returns INNO-Maker USB2CAN
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
		static constexpr InnoMakerUsb2CanLib::UsbCanMode CAN_MODE = InnoMakerUsb2CanLib::UsbCanModeNormal; ///< The mode to use for the CAN device
		static constexpr std::uint32_t CAN_EFF_FLAG = 0x80000000; ///< Set if the frame is extended
		static constexpr std::uint32_t CAN_SFF_MASK = 0x000007FF; ///< The mask for standard frames
		static constexpr std::uint32_t CAN_EFF_MASK = 0x1FFFFFFF; ///< The mask for extended frames

		static std::unique_ptr<InnoMakerUsb2CanLib> driverInstance; ///< The driver itself
		const int channel; ///< Stores the channel associated with this object
		const std::uint32_t baudrate; ///< Stores the baud rate associated with this object
		std::unique_ptr<InnoMakerUsb2CanLib::innomaker_can> txContexts; ///< Stores Tx tickets for the driver
	};
}
#endif // INNOMAKER_USB2CAN_PLUGIN_HPP
