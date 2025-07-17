//================================================================================================
/// @file can_constants.hpp
///
/// @brief General constants used throughout this library
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef CAN_CONSTANTS_HPP
#define CAN_CONSTANTS_HPP

#ifndef CAN_PORT_MAXIMUM_VALUE
/// @brief Maximum number of CAN ports that can be used simultaneously
/// @details This value can be configured at build time using CMake option -DCAN_PORT_MAXIMUM_VALUE=<value>
/// The default value is 4. Valid range is 1-16.
#define CAN_PORT_MAXIMUM_VALUE 4
#endif

#include <cstdint>

namespace isobus
{
	constexpr std::uint64_t DEFAULT_NAME = 0xFFFFFFFFFFFFFFFF; ///< An invalid NAME used as a default
	constexpr std::uint32_t DEFAULT_IDENTIFIER = 0xFFFFFFFF; ///< An invalid identifier used as a default
	constexpr std::uint8_t NULL_CAN_ADDRESS = 0xFE; ///< The NULL CAN address defined by J1939 and ISO11783
	constexpr std::uint8_t BROADCAST_CAN_ADDRESS = 0xFF; ///< The global/broadcast CAN address
	constexpr std::uint8_t CAN_DATA_LENGTH = 8; ///< The length of a classical CAN frame
	constexpr std::uint32_t CAN_PORT_MAXIMUM = CAN_PORT_MAXIMUM_VALUE; ///< An arbitrary limit for memory consumption (configurable via CMake)
	constexpr std::uint16_t NULL_OBJECT_ID = 65535; ///< Special ID used to indicate no object

}

#endif // CAN_CONSTANTS_HPP
