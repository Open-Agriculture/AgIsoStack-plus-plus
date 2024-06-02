//================================================================================================
/// @file can_message_frame.hpp
///
/// @brief A classical CAN frame, with 8 data bytes
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#ifndef CAN_MESSAGE_FRAME_HPP
#define CAN_MESSAGE_FRAME_HPP

#include <cstdint>

namespace isobus
{
	//================================================================================================
	/// @class CANMessageFrame
	///
	/// @brief A CAN frame for interfacing with a hardware layer, like socket CAN or other interface
	//================================================================================================
	class CANMessageFrame
	{
	public:
		/// Returns the number of bits in a CAN message with averaged bit stuffing
		/// @returns The number of bits in the message (with average bit stuffing)
		std::uint32_t get_number_bits_in_message() const;

		std::uint64_t timestamp_us; ///< A microsecond timestamp
		std::uint32_t identifier; ///< The 32 bit identifier of the frame
		std::uint8_t channel; ///< The CAN channel index associated with the frame
		std::uint8_t data[8]; ///< The data payload of the frame
		std::uint8_t dataLength; ///< The length of the data used in the frame
		bool isExtendedFrame; ///< Denotes if the frame is extended format
	};

} // namespace isobus

#endif // CAN_MESSAGE_FRAME_HPP
