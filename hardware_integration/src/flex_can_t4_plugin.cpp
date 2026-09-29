//================================================================================================
/// @file flex_can_t4_plugin.cpp
///
/// @brief An interface for using Teensy4/4.1 CAN hardware
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#include "isobus/hardware_integration/flex_can_t4_plugin.hpp"
#include "FlexCAN_T4.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

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

	FlexCANT4Plugin::FlexCANT4Plugin(std::uint8_t channel) :
	  selectedChannel(channel)
	{
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
		// Flex CAN doesn't have a way to stop it...
		isOpen = false;
	}

	void FlexCANT4Plugin::open()
	{
		if (0 == selectedChannel)
		{
			can0.begin();
			can0.setBaudRate(250000);
			isOpen = true;
		}
		else if (1 == selectedChannel)
		{
			can1.begin();
			can1.setBaudRate(250000);
			isOpen = true;
		}
#if defined(__IMXRT1062__)
		else if (2 == selectedChannel)
		{
			can2.begin();
			can2.setBaudRate(250000);
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
		CAN_message_t message;
		bool retVal = false;

		if (0 == selectedChannel)
		{
			retVal = can0.read(message);
			canFrame.channel = 0;
		}
		else if (1 == selectedChannel)
		{
			retVal = can1.read(message);
			canFrame.channel = 1;
		}
#if defined(__IMXRT1062__)
		else if (2 == selectedChannel)
		{
			retVal = can2.read(message);
			canFrame.channel = 2;
		}
#endif

		memcpy(canFrame.data, message.buf, 8);
		canFrame.identifier = message.id;
		canFrame.dataLength = message.len;
		canFrame.isExtendedFrame = message.flags.extended;
		return retVal;
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
