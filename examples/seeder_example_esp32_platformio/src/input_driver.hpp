//================================================================================================
/// @file input_driver.hpp
///
/// @brief Reads the digital inputs (DI) on the Waveshare ESP32-S3-ETH-8DI-8RO-C board.
/// @details The digital inputs are direct GPIOs through bidirectional optocoupler isolation;
///          the pin map is defined in board_config.hpp. Each input is active-low at the pin
///          (the optocoupler pulls the isolated-side GPIO low when the field-side input is
///          asserted), so the driver enables the internal pull-up and inverts the raw level
///          once, letting callers treat a `true` return as "input active". This example maps
///          inputs 1 to 6 onto the seeder's 6 rows: an active input marks that row as
///          reporting a fault, so it reads "not working" while its commanded output (and thus
///          its relay) is left unchanged (see SeederVtApplication::set_section_fault).
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef INPUT_DRIVER_HPP
#define INPUT_DRIVER_HPP

#include <cstdint>

/// @brief A minimal, debounced driver for the board's 8 opto-isolated digital inputs.
namespace input_driver
{
	/// @brief Configures the digital input GPIOs as inputs with internal pull-ups enabled.
	/// @note Only call this when board_config::HAS_DIGITAL_INPUTS is true; main.cpp gates it.
	void init();

	/// @brief Samples all inputs for debouncing. Call cyclically (e.g. once per main loop).
	/// @details A level only becomes the new debounced state after it reads the same way for
	///          NUMBER_DEBOUNCE_SAMPLES consecutive calls, filtering switch bounce and noise.
	void update();

	/// @brief Returns the debounced state of a digital input.
	/// @param[in] channel The input channel, 1 to 8 (matches the board silkscreen DI1 to DI8).
	/// @returns True if the input is active (asserted), otherwise false.
	bool read_debounced(std::uint8_t channel);
} // namespace input_driver

#endif // INPUT_DRIVER_HPP
