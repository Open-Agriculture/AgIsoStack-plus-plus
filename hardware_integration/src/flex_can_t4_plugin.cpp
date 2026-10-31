//================================================================================================
/// @file flex_can_t4_plugin.cpp
///
/// @brief An interface for using Teensy4/4.1 CAN hardware
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

// Teensy 4.x only: the Teensy 3.x declarations in the header use bus names FlexCAN_T4 rejects there
#if defined(__IMXRT1062__)
#include "isobus/hardware_integration/flex_can_t4_plugin.hpp"
#include "FlexCAN_T4.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <atomic>
#include <cstring>

namespace isobus
{
#if defined(__IMXRT1062__)
	FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_512> FlexCANT4Plugin::can0;
	FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_512> FlexCANT4Plugin::can1;
	FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_512> FlexCANT4Plugin::can2;
#elif defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
	FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_512> FlexCANT4Plugin::can0;
	FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_512> FlexCANT4Plugin::can1;
#endif

	FlexCANT4Plugin *FlexCANT4Plugin::openPlugins[3] = { nullptr, nullptr, nullptr };

	FlexCANT4Plugin::FlexCANT4Plugin(std::uint8_t channel) :
	  selectedChannel(channel)
	{
	}

	FlexCANT4Plugin::~FlexCANT4Plugin()
	{
		FlexCANT4Plugin::close();
	}

	std::string FlexCANT4Plugin::get_name() const
	{
		return "FlexCANT4";
	}

	bool FlexCANT4Plugin::get_is_valid() const
	{
		return isOpen;
	}

	void FlexCANT4Plugin::close()
	{
		// Flex CAN doesn't have a way to stop it, but its interrupt stops handing frames to this plugin
		if ((selectedChannel < 3) && (this == openPlugins[selectedChannel]))
		{
			openPlugins[selectedChannel] = nullptr;
		}
		isOpen = false;
	}

	// Receive by interrupt (#248 in NeptuneGPS_Triton): the FIFO keeps the order frames
	// arrived in, and its interrupt copies each one into receiveQueue at once. Polled
	// mailboxes (FlexCAN_T4's default after begin()) hold only 4 frames with 29-bit
	// identifiers and are read round-robin, so a burst on the bus while the application
	// was busy lost frames, and more mailboxes would not keep transport-protocol frames
	// in order.
	void FlexCANT4Plugin::open()
	{
		receiveHead = 0;
		receiveTail = 0;
		receiveHighWater = 0;
		receiveOverflowCount = 0;

		if (0 == selectedChannel)
		{
			openPlugins[0] = this;
			can0.begin();
			can0.setBaudRate(250000);
			can0.enableFIFO();
			can0.enableFIFOInterrupt();
			can0.onReceive(on_receive_channel_0);
			isOpen = true;
		}
		else if (1 == selectedChannel)
		{
			openPlugins[1] = this;
			can1.begin();
			can1.setBaudRate(250000);
			can1.enableFIFO();
			can1.enableFIFOInterrupt();
			can1.onReceive(on_receive_channel_1);
			isOpen = true;
		}
#if defined(__IMXRT1062__)
		else if (2 == selectedChannel)
		{
			openPlugins[2] = this;
			can2.begin();
			can2.setBaudRate(250000);
			can2.enableFIFO();
			can2.enableFIFOInterrupt();
			can2.onReceive(on_receive_channel_2);
			isOpen = true;
		}
#endif
		else
		{
			LOG_CRITICAL("[FlexCAN]: Invalid Channel Selected");
		}
	}

	bool FlexCANT4Plugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		const std::size_t tail = receiveTail;
		if (tail == receiveHead)
		{
			return false;
		}
		// Copy the slot only after seeing head past it, and free it only after the copy
		std::atomic_signal_fence(std::memory_order_acquire);
		canFrame = receiveQueue[tail];
		std::atomic_signal_fence(std::memory_order_release);
		receiveTail = (tail + 1) % RECEIVE_QUEUE_SIZE;
		return true;
	}

	std::size_t FlexCANT4Plugin::get_receive_queue_high_water() const
	{
		return receiveHighWater;
	}

	std::uint32_t FlexCANT4Plugin::get_receive_queue_overflow_count() const
	{
		return receiveOverflowCount;
	}

	void FlexCANT4Plugin::queue_received_frame(const CAN_message_t &message)
	{
		const std::size_t head = receiveHead;
		const std::size_t next = (head + 1) % RECEIVE_QUEUE_SIZE;
		if (next == receiveTail)
		{
			receiveOverflowCount = receiveOverflowCount + 1;
			return;
		}
		CANMessageFrame &frame = receiveQueue[head];
		frame.timestamp_us = micros();
		frame.identifier = message.id;
		frame.channel = selectedChannel;
		memcpy(frame.data, message.buf, 8);
		frame.dataLength = message.len;
		frame.isExtendedFrame = message.flags.extended;
		std::atomic_signal_fence(std::memory_order_release);
		receiveHead = next; // publish the frame only once it is complete

		const std::size_t used = (next + RECEIVE_QUEUE_SIZE - receiveTail) % RECEIVE_QUEUE_SIZE;
		if (used > receiveHighWater)
		{
			receiveHighWater = used;
		}
	}

	void FlexCANT4Plugin::on_receive_channel_0(const CAN_message_t &message)
	{
		if (nullptr != openPlugins[0])
		{
			openPlugins[0]->queue_received_frame(message);
		}
	}

	void FlexCANT4Plugin::on_receive_channel_1(const CAN_message_t &message)
	{
		if (nullptr != openPlugins[1])
		{
			openPlugins[1]->queue_received_frame(message);
		}
	}

	void FlexCANT4Plugin::on_receive_channel_2(const CAN_message_t &message)
	{
		if (nullptr != openPlugins[2])
		{
			openPlugins[2]->queue_received_frame(message);
		}
	}

	bool FlexCANT4Plugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		CAN_message_t message;
		bool retVal = false;

		message.id = canFrame.identifier;
		message.len = canFrame.dataLength;
		message.flags.extended = true;
		message.seq = true; // Ask for sequential transmission
		memcpy(message.buf, canFrame.data, canFrame.dataLength);

		if (0 == selectedChannel)
		{
			retVal = can0.write(message);
		}
		else if (1 == selectedChannel)
		{
			retVal = can1.write(message);
		}
#if defined(__IMXRT1062__)
		else if (2 == selectedChannel)
		{
			retVal = can2.write(message);
		}
#endif
		return retVal;
	}
}
#endif // __IMXRT1062__
