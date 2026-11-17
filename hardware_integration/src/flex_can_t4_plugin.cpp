//================================================================================================
/// @file flex_can_t4_plugin.cpp
///
/// @brief An interface for using Teensy 4.0/4.1 and Teensy 3.2/3.5/3.6 CAN hardware
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

// The Teensy boards with CAN controllers: 4.0/4.1, 3.6, and 3.2/3.5
#if defined(__IMXRT1062__) || defined(__MK66FX1M0__) || defined(__MK20DX256__) || defined(__MK64FX512__)
#include "isobus/hardware_integration/flex_can_t4_plugin.hpp"
#include "FlexCAN_T4.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <new>

namespace isobus
{
	namespace
	{
		/// @brief Creates a controller object the first time its channel opens, and returns it
		/// @param[in, out] controller The controller's pointer, set when the object is created
		/// @returns The controller, or nullptr if there was no memory for it
		template<typename Controller>
		Controller *get_or_create_controller(Controller *&controller)
		{
			if (nullptr == controller)
			{
				// calloc rather than new. The nullptr check stays: the compiler may assume a plain new
				// never returns nullptr, and the Teensy core's new just returns malloc's result. And the
				// memory is zeroed: FlexCAN_T4's constructor leaves its handler pointers and filter tables
				// as they are, because the library expects a static (zero-initialized) object. In memory
				// that isn't zero, its interrupt calls a garbage transmit handler on the first sent frame.
				void *memory = std::calloc(1, sizeof(Controller));
				if (nullptr != memory)
				{
					controller = new (memory) Controller();
				}
			}
			return controller;
		}

		/// @brief Starts a controller at 250 kbit/s, receiving through its FIFO interrupt
		/// @param[in, out] controller The controller's pointer, set if the object didn't exist yet
		/// @param[in] receiveHandler The interrupt-side receive handler for the controller's channel
		/// @returns `true` if the controller was started, `false` if there was no memory for it
		template<typename Controller>
		bool start_controller(Controller *&controller, _MB_ptr receiveHandler)
		{
			Controller *bus = get_or_create_controller(controller);
			if (nullptr == bus)
			{
				return false;
			}
			bus->begin();
			bus->setBaudRate(250000);
			bus->enableFIFO();
			bus->enableFIFOInterrupt();
			bus->onReceive(receiveHandler);
			return true;
		}
	}

#if defined(__IMXRT1062__)
	FlexCANT4Plugin::Controller<CAN1> *FlexCANT4Plugin::can0 = nullptr;
	FlexCANT4Plugin::Controller<CAN2> *FlexCANT4Plugin::can1 = nullptr;
	FlexCANT4Plugin::Controller<CAN3> *FlexCANT4Plugin::can2 = nullptr;
#elif defined(__MK66FX1M0__)
	FlexCANT4Plugin::Controller<CAN0> *FlexCANT4Plugin::can0 = nullptr;
	FlexCANT4Plugin::Controller<CAN1> *FlexCANT4Plugin::can1 = nullptr;
#else
	FlexCANT4Plugin::Controller<CAN0> *FlexCANT4Plugin::can0 = nullptr;
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

	// Receive by interrupt: the FIFO keeps the order frames arrived in, and its interrupt
	// copies each one into receiveQueue at once. Polled mailboxes (FlexCAN_T4's default
	// after begin()) hold only 4 frames with 29-bit identifiers and are read round-robin,
	// so a burst on the bus while the application was busy lost frames, and more mailboxes
	// would not keep transport-protocol frames in order.
	void FlexCANT4Plugin::open()
	{
		// After an earlier open() the interrupt still hands frames to this plugin: stop that before
		// resetting the queue, and hand the channel back only once the reset is done
		FlexCANT4Plugin::close();
		std::atomic_signal_fence(std::memory_order_seq_cst);
		receiveHead = 0;
		receiveTail = 0;
		receiveHighWater = 0;
		receiveOverflowCount = 0;
		std::atomic_signal_fence(std::memory_order_release);

		if (selectedChannel >= CONTROLLER_COUNT)
		{
			LOG_CRITICAL("[FlexCAN]: Invalid Channel Selected");
			return;
		}

		openPlugins[selectedChannel] = this;
		bool started = false;
		if (0 == selectedChannel)
		{
			started = start_controller(can0, on_receive_channel_0);
		}
#if defined(__IMXRT1062__) || defined(__MK66FX1M0__)
		else if (1 == selectedChannel)
		{
			started = start_controller(can1, on_receive_channel_1);
		}
#endif
#if defined(__IMXRT1062__)
		else if (2 == selectedChannel)
		{
			started = start_controller(can2, on_receive_channel_2);
		}
#endif

		if (started)
		{
			isOpen = true;
		}
		else
		{
			openPlugins[selectedChannel] = nullptr;
			LOG_CRITICAL("[FlexCAN]: Not enough memory for the CAN controller");
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

		// A controller exists only once its channel has been opened
		if ((0 == selectedChannel) && (nullptr != can0))
		{
			retVal = can0->write(message);
		}
#if defined(__IMXRT1062__) || defined(__MK66FX1M0__)
		else if ((1 == selectedChannel) && (nullptr != can1))
		{
			retVal = can1->write(message);
		}
#endif
#if defined(__IMXRT1062__)
		else if ((2 == selectedChannel) && (nullptr != can2))
		{
			retVal = can2->write(message);
		}
#endif
		return retVal;
	}
}
#endif // Teensy 4.0/4.1, 3.6, 3.2/3.5
