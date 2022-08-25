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

namespace isobus
{

	constexpr std::uint64_t DEFAULT_NAME = 0xFFFFFFFFFFFFFFFF;
	constexpr std::uint32_t DEFAULT_IDENTIFIER = 0xFFFFFFFF;
	constexpr std::uint8_t NULL_CAN_ADDRESS = 0xFE;
	constexpr std::uint8_t BROADCAST_CAN_ADDRESS = 0xFF;
	constexpr std::uint8_t CAN_DATA_LENGTH = 8;
	constexpr std::uint32_t CAN_PORT_MAXIMUM = 4;

}

#endif // CAN_CONSTANTS_HPP
