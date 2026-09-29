//================================================================================================
/// @file can_plugin.cpp
/// @brief Implements the Zephyr native classic CAN hardware plugin.
//================================================================================================

#include "isobus/zephyr/can_plugin.hpp"

#include "isobus/isobus/can_stack_logger.hpp"

#include <algorithm>

namespace isobus
{
	ZephyrCANPlugin::ZephyrCANPlugin(const struct device *canDevice) :
	  canDevice(canDevice)
	{
	}

	ZephyrCANPlugin::~ZephyrCANPlugin()
	{
		close();
	}

	std::string ZephyrCANPlugin::get_name() const
	{
		return "Zephyr CAN";
	}

	bool ZephyrCANPlugin::get_is_valid() const
	{
		return running.load();
	}

	void ZephyrCANPlugin::open()
	{
		if (running.load())
		{
			return;
		}
		if ((nullptr == canDevice) || !device_is_ready(canDevice))
		{
			LOG_ERROR("[Zephyr CAN] CAN device is not ready");
			return;
		}

		struct can_filter standardFilter{};
		standardFilter.id = 0;
		standardFilter.mask = 0;
		standardFilter.flags = 0;
		standardFilterId = can_add_rx_filter(canDevice, receive_callback, this, &standardFilter);
		if (standardFilterId < 0)
		{
			LOG_ERROR("[Zephyr CAN] Failed to install standard ID receive filter (%d)", standardFilterId);
			return;
		}

		struct can_filter extendedFilter{};
		extendedFilter.id = 0;
		extendedFilter.mask = 0;
		extendedFilter.flags = CAN_FILTER_IDE;
		extendedFilterId = can_add_rx_filter(canDevice, receive_callback, this, &extendedFilter);
		if (extendedFilterId < 0)
		{
			LOG_ERROR("[Zephyr CAN] Failed to install extended ID receive filter (%d)", extendedFilterId);
			can_remove_rx_filter(canDevice, standardFilterId);
			standardFilterId = -1;
			return;
		}

		if (0 != can_start(canDevice))
		{
			LOG_ERROR("[Zephyr CAN] Failed to start CAN controller");
			can_remove_rx_filter(canDevice, extendedFilterId);
			can_remove_rx_filter(canDevice, standardFilterId);
			extendedFilterId = -1;
			standardFilterId = -1;
			return;
		}

		const k_spinlock_key_t key = k_spin_lock(&receiveLock);
		receiveHead = 0;
		receiveCount = 0;
		k_spin_unlock(&receiveLock, key);
		running.store(true);
		LOG_INFO("[Zephyr CAN] Started CAN controller");
	}

	void ZephyrCANPlugin::close()
	{
		if (!running.exchange(false))
		{
			return;
		}

		if (extendedFilterId >= 0)
		{
			can_remove_rx_filter(canDevice, extendedFilterId);
			extendedFilterId = -1;
		}
		if (standardFilterId >= 0)
		{
			can_remove_rx_filter(canDevice, standardFilterId);
			standardFilterId = -1;
		}

		if (0 != can_stop(canDevice))
		{
			LOG_WARNING("[Zephyr CAN] Failed to stop CAN controller");
		}

		const k_spinlock_key_t key = k_spin_lock(&receiveLock);
		receiveHead = 0;
		receiveCount = 0;
		k_spin_unlock(&receiveLock, key);
	}

	bool ZephyrCANPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		const k_spinlock_key_t key = k_spin_lock(&receiveLock);
		if (0 == receiveCount)
		{
			k_spin_unlock(&receiveLock, key);
			k_msleep(1);
			return false;
		}

		canFrame = receiveQueue[receiveHead];
		receiveHead = (receiveHead + 1) % RX_QUEUE_SIZE;
		--receiveCount;
		k_spin_unlock(&receiveLock, key);
		return true;
	}

	bool ZephyrCANPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		if (!running.load() || (canFrame.dataLength > sizeof(canFrame.data)))
		{
			return false;
		}
		if (canFrame.isExtendedFrame ? (canFrame.identifier > CAN_EXT_ID_MASK) : (canFrame.identifier > CAN_STD_ID_MASK))
		{
			return false;
		}

		struct can_frame frame{};
		frame.id = canFrame.identifier;
		frame.dlc = canFrame.dataLength;
		frame.flags = canFrame.isExtendedFrame ? CAN_FRAME_IDE : 0;
		std::copy_n(canFrame.data, canFrame.dataLength, frame.data);

		if (0 != can_send(canDevice, &frame, K_MSEC(100), nullptr, nullptr))
		{
			LOG_WARNING("[Zephyr CAN] Failed to send CAN frame 0x%08X", canFrame.identifier);
			return false;
		}
		return true;
	}

	void ZephyrCANPlugin::receive_callback(const struct device *dev, struct can_frame *frame, void *userData)
	{
		ARG_UNUSED(dev);
		if ((nullptr == frame) || (nullptr == userData))
		{
			return;
		}
		static_cast<ZephyrCANPlugin *>(userData)->enqueue_received_frame(*frame);
	}

	void ZephyrCANPlugin::enqueue_received_frame(const struct can_frame &frame)
	{
		if (!running.load() || (frame.flags & (CAN_FRAME_FDF | CAN_FRAME_RTR)) || (frame.dlc > 8))
		{
			return;
		}

		isobus::CANMessageFrame convertedFrame{};
		convertedFrame.timestamp_us = k_ticks_to_us_floor64(k_uptime_ticks());
		convertedFrame.identifier = frame.id;
		convertedFrame.dataLength = frame.dlc;
		convertedFrame.isExtendedFrame = (frame.flags & CAN_FRAME_IDE) != 0;
		std::copy_n(frame.data, frame.dlc, convertedFrame.data);

		const k_spinlock_key_t key = k_spin_lock(&receiveLock);
		if (receiveCount < RX_QUEUE_SIZE)
		{
			const std::size_t tail = (receiveHead + receiveCount) % RX_QUEUE_SIZE;
			receiveQueue[tail] = convertedFrame;
			++receiveCount;
		}
		k_spin_unlock(&receiveLock, key);
	}
} // namespace isobus
