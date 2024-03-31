//================================================================================================
/// @file stm32l4x_hal.hpp
///
/// @brief An interface for using CAN peripherals on STM32L4x devices.
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef STM32L4X_HAL_PLUGIN_HPP
#define STM32L4X_HAL_PLUGIN_HPP

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

#include "isobus/utility/thread_synchronization.hpp"

#include "stm32l4xx_hal.h"

namespace isobus
{
	/// @brief An interface for using CAN peripherals on STM32L4x devices.
	/// This uses the ST Microelectronics hardware abstraction layer.
	class STM32L4xHALPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for the STM32L4xHALPlugin
		/// @param[in] handle The handle for the CAN peripheral to use.
		/// @attention You must have already set up the CAN peripheral for the proper
		/// bit timing before constructing this.
		/// The reason for this is that we don't know your CAN peripheral's clock rate!
		/// Generally, if you're using STM32CubeMX, you shouldn't need to do anything
		/// special except to construct this AFTER the CAN is set up in main
		explicit STM32L4xHALPlugin(const CAN_HandleTypeDef &handle);

		/// @brief The destructor for STM32L4xHALPlugin
		virtual ~STM32L4xHALPlugin() = default;

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

		static void can1_rx_callback();
		static void can2_rx_callback();

		static CAN_HandleTypeDef &get_can1_handle();

#ifdef CAN2
		static CAN_HandleTypeDef &get_can2_handle();
#endif

	private:
		enum class PeripheralChannel
		{
			Channel1,
			Channel2
		};
		bool getCANMessage(isobus::CANMessageFrame &canFrame);
		CAN_HandleTypeDef &get_channel();
#ifdef USE_CMSIS_RTOS2_THREADING
		static osSemaphoreId_t can1RxSemaphore;
#ifdef CAN2
		static osSemaphoreId_t can2RxSemaphore;
#endif
#endif
		static CAN_HandleTypeDef can1Handle; ///< A copy of the handle for CAN 1, which is static so that we can access it in an interrupt

#ifdef CAN2
		static CAN_HandleTypeDef can2Handle; ///< A copy of the handle for CAN 2, which is static so that we can access it in an interrupt
#endif
		static LockFreeQueue<CANMessageFrame> can1Queue;
#ifdef CAN2
		static LockFreeQueue<CANMessageFrame> can2Queue;
#endif
		static constexpr std::size_t DEFAULT_QUEUE_SIZE = 40; ///< The size of the lock free buffer(s) holding CAN messages from the interrupt
		PeripheralChannel channel = PeripheralChannel::Channel1; ///< This object's specific channel that it's managing
		bool isOpen = false; ///< Stores if connecting to the peripheral succeeded or not
	};
}
#endif // STM32L4X_HAL_PLUGIN_HPP
