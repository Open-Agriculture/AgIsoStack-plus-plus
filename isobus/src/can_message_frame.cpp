//================================================================================================
/// @file can_message_frame.cpp
///
/// @brief Implements helper functions for CANMessageFrame
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/can_message_frame.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_identifier.hpp"

namespace isobus
{
	std::uint32_t CANMessageFrame::get_number_bits_in_message() const
	{
		constexpr std::uint32_t MAX_CONSECUTIVE_SAME_BITS = 5; // After 5 consecutive bits, 6th will be opposite
		const std::uint32_t dataLengthBits = CAN_DATA_LENGTH * dataLength;
		std::uint32_t retVal = 0;

		if (isExtendedFrame)
		{
			constexpr std::uint32_t EXTENDED_ID_BEST_NON_DATA_LENGTH = 67; // SOF, ID, Control, CRC, ACK, EOF, and interframe space
			constexpr std::uint32_t EXTENDED_ID_WORST_NON_DATA_LENGTH = 78;
			retVal = ((dataLengthBits + EXTENDED_ID_BEST_NON_DATA_LENGTH) + (dataLengthBits + (dataLengthBits / MAX_CONSECUTIVE_SAME_BITS) + EXTENDED_ID_WORST_NON_DATA_LENGTH));
		}
		else
		{
			constexpr std::uint32_t STANDARD_ID_BEST_NON_DATA_LENGTH = 47; // SOF, ID, Control, CRC, ACK, EOF, and interframe space
			constexpr std::uint32_t STANDARD_ID_WORST_NON_DATA_LENGTH = 54;
			retVal = ((dataLengthBits + STANDARD_ID_BEST_NON_DATA_LENGTH) + (dataLengthBits + (dataLengthBits / MAX_CONSECUTIVE_SAME_BITS) + STANDARD_ID_WORST_NON_DATA_LENGTH));
		}
		return retVal / 2;
	}
} // namespace isobus
