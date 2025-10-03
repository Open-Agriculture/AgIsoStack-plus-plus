//================================================================================================
/// @file flex_can_t4_plugin.hpp
///
/// @brief An interface for using FlexCAN_T4 on a Teensy4/4.1 device.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef FLEX_CAN_T4_PLUGIN_HPP
#define FLEX_CAN_T4_PLUGIN_HPP

#include "FlexCAN_T4.hpp"
#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

namespace isobus
{
	/// @brief An interface for using FlexCAN_T4 on a Teensy4/4.1 device
	class FlexCANT4Plugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for the FlexCANT4Plugin
		/// @param[in] channel The channel to use on the device
		explicit FlexCANT4Plugin(std::uint8_t channel);

		/// @brief The destructor for FlexCANT4Plugin
		virtual ~FlexCANT4Plugin() = default;

		/// @brief Returns the displayable name of the plugin
		/// @returns FlexCANT4
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
#if defined(__IMXRT1062__)
		static FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_512> can0;
		static FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_512> can1;
		static FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_512> can2;
#elif defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
		static FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_512> can0;
		static FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_512> can1;
#endif
		std::uint8_t selectedChannel; ///< The channel that this plugin is assigned to
		bool isOpen = false; ///< Tracks if the connection is open/connected
	};
}
#endif // FLEX_CAN_T4_PLUGIN_HPP
