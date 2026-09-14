//================================================================================================
/// @file board_config.hpp
///
/// @brief Central board wiring configuration for the ESP32 seeder example.
/// @details Every board-specific pin lives in this one file, so the example can be retargeted to a
///          different ESP32-S3 board by editing only here:
///           - the TWAI (CAN) controller pins
///           - the opto-isolated digital input pins
///           - the I2C pins and address of the relay GPIO expander
///          The two feature flags let the example run on boards that lack some of this hardware.
///          When a flag is false, main.cpp skips that hardware entirely: the pins are not claimed,
///          the inputs are not polled, and the relays are not driven. The relay expander is ALSO
///          probed at runtime, so even with HAS_RELAY_EXPANDER true, a missing or unpowered IC just
///          makes relay control a no-op instead of faulting the application.
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef BOARD_CONFIG_HPP
#define BOARD_CONFIG_HPP

#include "driver/gpio.h"

#include <cstdint>

/// @brief Board wiring and optional-hardware flags for this example.
namespace board_config
{
	//------------------------------------------------------------------------------------------------
	// Optional hardware. Set a flag to false to run on a board that does not have that hardware; the
	// application then leaves the related pins alone and skips the related work in its main loop.
	//------------------------------------------------------------------------------------------------
	constexpr bool HAS_RELAY_EXPANDER = true; ///< Board has the TCA9554 I2C relay expander that drives the section relays
	constexpr bool HAS_DIGITAL_INPUTS = true; ///< Board has the opto-isolated digital inputs used as per-section fault feedback

	//------------------------------------------------------------------------------------------------
	// TWAI (CAN). These are the ESP32's built-in CAN controller pins. A CAN transceiver is required
	// between them and the bus; you cannot connect a CAN bus directly to these pins.
	//------------------------------------------------------------------------------------------------
	constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_17; ///< TWAI transmit pin
	constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_18; ///< TWAI receive pin

	//------------------------------------------------------------------------------------------------
	// Digital inputs. Direct GPIOs through bidirectional optocoupler isolation. Each is active-low at
	// the pin (the optocoupler pulls the isolated-side GPIO low when the field-side input is
	// asserted); the driver enables the internal pull-up and inverts the level once, so callers see
	// true = "input active". Entry N maps to the board silkscreen DI(N+1).
	//------------------------------------------------------------------------------------------------
	constexpr std::uint8_t NUMBER_DIGITAL_INPUTS = 8; ///< The number of digital inputs on the board
	constexpr gpio_num_t DIGITAL_INPUT_PINS[NUMBER_DIGITAL_INPUTS] = { GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_8, GPIO_NUM_9, GPIO_NUM_10, GPIO_NUM_11 };

	//------------------------------------------------------------------------------------------------
	// Relay expander. The relays are NOT direct GPIOs; they are driven through a TCA9554PWR I2C GPIO
	// expander. NOTE: SDA/SCL below are bench-confirmed and are reversed compared to the vendor's
	// "Implementation Logic" diagram.
	//------------------------------------------------------------------------------------------------
	constexpr gpio_num_t RELAY_I2C_SDA_PIN = GPIO_NUM_42; ///< I2C SDA for the relay expander (bench-confirmed)
	constexpr gpio_num_t RELAY_I2C_SCL_PIN = GPIO_NUM_41; ///< I2C SCL for the relay expander (bench-confirmed)
	constexpr std::uint16_t RELAY_EXPANDER_ADDRESS = 0x20; ///< The TCA9554PWR 7-bit address
	constexpr std::uint8_t NUMBER_RELAY_CHANNELS = 8; ///< The expander drives 8 relay channels (R1 to R8)
} // namespace board_config

#endif // BOARD_CONFIG_HPP
