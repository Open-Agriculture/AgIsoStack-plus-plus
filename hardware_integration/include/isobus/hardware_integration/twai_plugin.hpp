//================================================================================================
/// @file twai_plugin.hpp
///
/// @brief A driver for using the Two-Wire Automotive Interface (TWAI) with the stack.
/// @author Daan Steenbergen
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef TWAI_PLUGIN_HPP
#define TWAI_PLUGIN_HPP
#ifdef ESP_PLATFORM

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

#include "driver/twai.h"

#include <string>

namespace isobus
{
	//================================================================================================
	/// @class TWAIPlugin
	///
	/// @brief A driver for Two-Wire Automotive Interface (TWAI).
	//================================================================================================
	class TWAIPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for the socket CAN driver
		/// @param[in] filterConfig A reference to the filter configuration for the TWAI driver
		/// @param[in] timingConfig A reference to the timing configuration for the TWAI driver
		/// @param[in] generalConfig A reference to the general configuration for the TWAI driver
		explicit TWAIPlugin(const twai_general_config_t *generalConfig, const twai_timing_config_t *timingConfig, const twai_filter_config_t *filterConfig);

		/// @brief The destructor for TWAIPlugin
		virtual ~TWAIPlugin();

		/// @brief Returns the displayable name of the plugin
		/// @returns TWAI
		std::string get_name() const override;

		/// @brief Returns if the socket connection is valid
		/// @returns `true` if connected, `false` if not connected
		bool get_is_valid() const override;

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

	private:
		const twai_general_config_t *generalConfig;
		const twai_timing_config_t *timingConfig;
		const twai_filter_config_t *filterConfig;
	};
}
#endif // ESP_PLATFORM
#endif // TWAI_PLUGIN_HPP
