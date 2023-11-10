//================================================================================================
/// @file can_constants.hpp
///
/// @brief General constants used throughout this library
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef CAN_CONSTANTS_HPP
#define CAN_CONSTANTS_HPP

#include <cstdint>

namespace isobus
{
	constexpr std::uint64_t DEFAULT_NAME = 0xFFFFFFFFFFFFFFFF; ///< An invalid NAME used as a default
	constexpr std::uint32_t DEFAULT_IDENTIFIER = 0xFFFFFFFF; ///< An invalid identifier used as a default
	constexpr std::uint8_t NULL_CAN_ADDRESS = 0xFE; ///< The NULL CAN address defined by J1939 and ISO11783
	constexpr std::uint8_t BROADCAST_CAN_ADDRESS = 0xFF; ///< The global/broadcast CAN address
	constexpr std::uint8_t CAN_DATA_LENGTH = 8; ///< The length of a classical CAN frame
	constexpr std::uint32_t CAN_PORT_MAXIMUM = 4; ///< An arbitrary limit for memory consumption

}

#endif // CAN_CONSTANTS_HPP
