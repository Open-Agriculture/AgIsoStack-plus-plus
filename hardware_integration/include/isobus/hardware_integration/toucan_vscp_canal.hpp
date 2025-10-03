//================================================================================================
/// @file toucan_vscp_canal.hpp
///
/// @brief An interface for using a Rusoku TouCAN device via the VSCP CANAL api.
/// @note The driver library for this plugin is located at https://github.com/rusoku/
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef TOUCAN_VSCP_CANAL_PLUGIN_HPP
#define TOUCAN_VSCP_CANAL_PLUGIN_HPP

#include <string>

#include "isobus/hardware_integration/canal.h"

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

namespace isobus
{
	//================================================================================================
	/// @class TouCANPlugin
	///
	/// @brief An interface for using a Rusoku TouCAN device via the VSCP CANAL api
	//================================================================================================
	class TouCANPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for a TouCAN hardware plugin object
		/// @param[in] deviceID The id to use for this device
		/// @param[in] serialNumber The serial number of the CAN adapter to connect with
		/// @param[in] baudRate The baud rate in thousands (250K baud would mean you need to pass in 250)
		TouCANPlugin(std::int16_t deviceID, std::uint32_t serialNumber, std::uint16_t baudRate = 250);

		/// @brief Constructor for a TouCAN hardware plugin object
		/// @param[in] deviceName The channel to use. This string has a very specific structure that isn't defined anywhere
		/// except the source code for this specific CANAL implementation.
		/// @attention Only use this if you know what you are doing, otherwise use the other constructor.
		explicit TouCANPlugin(std::string deviceName);

		/// @brief The destructor for TouCANPlugin
		virtual ~TouCANPlugin();

		/// @brief Returns the displayable name of the plugin
		/// @returns Rusoku TouCAN
		std::string get_name() const override;

		/// @brief Returns if the connection with the hardware is valid
		/// @returns `true` if connected, `false` if not connected
		bool get_is_valid() const override;

		/// @brief Closes the connection to the hardware
		void close() override;

		/// @brief Connects to the hardware
		void open() override;

		/// @brief Returns a frame from the hardware (synchronous), or `false` if no frame can be read.
		/// @param[in, out] canFrame The CAN frame that was read
		/// @returns `true` if a CAN frame was read, otherwise `false`
		bool read_frame(isobus::CANMessageFrame &canFrame) override;

		/// @brief Writes a frame to the bus (synchronous)
		/// @param[in] canFrame The frame to write to the bus
		/// @returns `true` if the frame was written, otherwise `false`
		bool write_frame(const isobus::CANMessageFrame &canFrame) override;

		/// @brief Changes previously set configuration parameters. Only works if the device is not open.
		/// @param[in] deviceID The id to use for this device
		/// @param[in] serialNumber The serial number of the CAN adapter to connect with
		/// @param[in] baudRate The baud rate in thousands (250K baud would mean you need to pass in 250)
		/// @returns True if the configuration was changed, otherwise false (if the device is open false will be returned)
		bool reconfigure(std::int16_t deviceID, std::uint32_t serialNumber, std::uint16_t baudRate = 250);

		/// @brief Returns the currently configured serial number of the device which will be used to connect to the hardware
		/// @returns The currently configured serial number of the device
		std::uint32_t get_serial_number() const;

	private:
		/// @brief Generates a device name string for the TouCAN device
		/// @param[in] deviceID The id to use for this device
		/// @param[in] serialNumber The serial number of the CAN adapter to connect with
		/// @param[in] baudRate The baud rate in thousands (250K baud would mean you need to pass in 250)
		void generate_device_name(std::int16_t deviceID, std::uint32_t serialNumber, std::uint16_t baudRate);

		std::string name; ///< A configuration string that is used to connect to the hardware through the CANAL api
		std::uint32_t handle = 0; ///< The handle that the driver returns to us for the open hardware
		std::uint32_t openResult = CANAL_ERROR_NOT_OPEN; ///< Stores the result of the call to begin CAN communication. Used for is_valid check later.
		std::uint32_t currentlyConfiguredSerialNumber = 0; ///< The serial number of the device that is being used
	};
}
#endif // TOUCAN_VSCP_CANAL_PLUGIN_HPP
