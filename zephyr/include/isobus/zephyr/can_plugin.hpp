//================================================================================================
/// @file can_plugin.hpp
/// @brief Zephyr native classic CAN hardware plugin.
//================================================================================================
#ifndef ISOBUS_ZEPHYR_CAN_PLUGIN_HPP
#define ISOBUS_ZEPHYR_CAN_PLUGIN_HPP

#include "isobus/hardware_integration/can_hardware_plugin.hpp"

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

#include <array>
#include <atomic>

namespace isobus
{
	/// @brief Adapts a Zephyr CAN controller to AgIsoStack++'s hardware plugin interface.
	class ZephyrCANPlugin : public CANHardwarePlugin
	{
	public:
		explicit ZephyrCANPlugin(const struct device *canDevice);
		~ZephyrCANPlugin() override;

		std::string get_name() const override;
		bool get_is_valid() const override;
		void close() override;
		void open() override;
		bool read_frame(isobus::CANMessageFrame &canFrame) override;
		bool write_frame(const isobus::CANMessageFrame &canFrame) override;

	private:
		static constexpr std::size_t RX_QUEUE_SIZE = 16;

		static void receive_callback(const struct device *dev, struct can_frame *frame, void *userData);
		void enqueue_received_frame(const struct can_frame &frame);

		const struct device *canDevice;
		std::atomic_bool running{ false };
		int standardFilterId = -1;
		int extendedFilterId = -1;
		std::array<isobus::CANMessageFrame, RX_QUEUE_SIZE> receiveQueue{};
		std::size_t receiveHead = 0;
		std::size_t receiveCount = 0;
		struct k_spinlock receiveLock{};
	};
} // namespace isobus

#endif // ISOBUS_ZEPHYR_CAN_PLUGIN_HPP
