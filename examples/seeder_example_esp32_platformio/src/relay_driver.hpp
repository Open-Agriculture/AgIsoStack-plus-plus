//================================================================================================
/// @file relay_driver.hpp
///
/// @brief Drives the relay outputs on the Waveshare ESP32-S3-ETH-8DI-8RO-C board.
/// @details The relays on this board are NOT direct GPIOs. They are driven through a
///          TCA9554PWR I2C GPIO expander; the I2C pins and expander address are defined in
///          board_config.hpp. This example maps the seeder's 6 on-screen sections onto relay
///          channels 1 to 6. init() probes the expander, so on a board without it (or when
///          board_config::HAS_RELAY_EXPANDER is false) the driver becomes a safe no-op.
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef RELAY_DRIVER_HPP
#define RELAY_DRIVER_HPP

#include <cstdint>

/// @brief A minimal driver for the board's TCA9554PWR relay expander.
namespace relay_driver
{
	/// @brief Creates the I2C bus and configures the expander, forcing all relays off.
	/// @note All relays are initialized to off so the implement starts in a safe state.
	/// @returns true if the expander acknowledged, false if it is not wired up or powered.
	bool init();

	/// @brief Energizes or de-energizes a single relay channel.
	/// @param[in] channel The relay channel, 1 to 8 (matches the board silkscreen R1 to R8).
	/// @param[in] on Pass true to energize the relay, false to de-energize it.
	/// @returns true if the requested state is now applied, false on an I2C error.
	bool set_relay(std::uint8_t channel, bool on);
} // namespace relay_driver

#endif // RELAY_DRIVER_HPP
