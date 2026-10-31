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

#include <cstddef>
#include <cstdint>

namespace isobus
{
	/// @brief An interface for using FlexCAN_T4 on a Teensy4/4.1 device
	class FlexCANT4Plugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for the FlexCANT4Plugin
		/// @param[in] channel The channel to use on the device
		explicit FlexCANT4Plugin(std::uint8_t channel);

		/// @brief The destructor for FlexCANT4Plugin, stops the receive interrupt handing frames to this plugin
		virtual ~FlexCANT4Plugin();

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

		/// @brief The most frames the receive queue has held at once since open()
		/// @returns The receive queue's high-water mark, in frames
		std::size_t get_receive_queue_high_water() const;

		/// @brief Frames dropped since open() because the receive queue was full
		/// @returns The number of dropped frames
		std::uint32_t get_receive_queue_overflow_count() const;

		/// @brief How many frames the receive queue holds
		static constexpr std::size_t RECEIVE_QUEUE_SIZE = 64;

	private:
		/// @brief Called from the FlexCAN FIFO interrupt: copies a frame into the receive queue
		/// @param[in] message The frame the controller received
		void queue_received_frame(const CAN_message_t &message);

		/// @brief The interrupt-side receive handler for FlexCAN channel 0
		/// @param[in] message The frame the controller received
		static void on_receive_channel_0(const CAN_message_t &message);

		/// @brief The interrupt-side receive handler for FlexCAN channel 1
		/// @param[in] message The frame the controller received
		static void on_receive_channel_1(const CAN_message_t &message);

		/// @brief The interrupt-side receive handler for FlexCAN channel 2
		/// @param[in] message The frame the controller received
		static void on_receive_channel_2(const CAN_message_t &message);

		static FlexCANT4Plugin *openPlugins[3]; ///< The plugin that opened each channel, for the interrupt handlers

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

		// Received frames, in arrival order: the FIFO interrupt writes (head), read_frame() reads (tail).
		// Single producer, single consumer on one core, so the two indices are the only shared state:
		// the interrupt publishes a slot by moving head past it, read_frame() frees it by moving tail.
		// Each index has one writer, and signal fences keep the slot copies on the right side of
		// the index updates.
		CANMessageFrame receiveQueue[RECEIVE_QUEUE_SIZE]; ///< The frames themselves
		volatile std::size_t receiveHead = 0; ///< Next slot the interrupt writes
		volatile std::size_t receiveTail = 0; ///< Next slot read_frame() reads
		volatile std::size_t receiveHighWater = 0; ///< The most frames held at once
		volatile std::uint32_t receiveOverflowCount = 0; ///< Frames dropped on a full queue
	};
}
#endif // FLEX_CAN_T4_PLUGIN_HPP
