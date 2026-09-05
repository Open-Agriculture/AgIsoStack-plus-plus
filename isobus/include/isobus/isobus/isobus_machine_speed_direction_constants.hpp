//================================================================================================
/// @file isobus_machine_speed_direction_constants.hpp
///
/// @brief Defines a common constants for machine speed, distance, and direction.
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_MACHINE_SPEED_DIRECTION_CONSTANTS_HPP
#define ISOBUS_MACHINE_SPEED_DIRECTION_CONSTANTS_HPP

#include <cstdint>

namespace isobus
{
	/// @brief Enumerates commonly used values of the direction of travel for the machine.
	enum class MachineDirection : std::uint8_t
	{
		Reverse = 0,
		Forward = 1,
		Error = 2,
		NotAvailable = 3
	};

	constexpr std::uint32_t SPEED_DISTANCE_MESSAGE_TX_INTERVAL_MS = 100; ///< The interval (in milliseconds) defined in ISO11783-7 for sending this class's messages
	constexpr std::uint32_t SPEED_DISTANCE_MESSAGE_RX_TIMEOUT_MS = 3 * SPEED_DISTANCE_MESSAGE_TX_INTERVAL_MS; ///< A (somewhat arbitrary) timeout for detecting stale messages.
	constexpr std::uint32_t SAEds05_MAX_VALUE = 4211081215; ///< The maximum valid value for a SAEds05 slot (see J1939)
	constexpr std::uint16_t SAEvl01_MAX_VALUE = 64255; ///< The maximum valid value for a SAEvl01 slot (see J1939)
} // namespace isobus

#endif // ISOBUS_MACHINE_SPEED_DIRECTION_CONSTANTS_HPP
