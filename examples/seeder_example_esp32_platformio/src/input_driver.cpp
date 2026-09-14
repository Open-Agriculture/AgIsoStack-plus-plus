//================================================================================================
/// @file input_driver.cpp
///
/// @brief Implements the opto-isolated digital input driver for this example.
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include "input_driver.hpp"

#include "board_config.hpp"
#include "driver/gpio.h"

namespace input_driver
{
	namespace
	{
		constexpr std::uint8_t NUMBER_INPUTS = board_config::NUMBER_DIGITAL_INPUTS; ///< The board's input count and pin map live in board_config.hpp
		constexpr std::uint8_t NUMBER_DEBOUNCE_SAMPLES = 3; ///< Consecutive agreeing samples before a level is accepted

		bool debouncedStates[NUMBER_INPUTS] = {}; ///< Debounced logical state per input, where true = active
		std::uint8_t matchCounts[NUMBER_INPUTS] = {}; ///< Consecutive raw reads that disagree with the debounced state

		/// @brief Returns the raw active state of an input, inverting the active-low pin level.
		/// @param[in] index The zero-based input index
		/// @returns True if the input is asserted
		bool read_raw(std::uint8_t index)
		{
			// The optocoupler pulls the pin low when the field-side input is driven, so a low level
			// means "active". This is inverted once here so every caller sees true = input active.
			return 0 == gpio_get_level(board_config::DIGITAL_INPUT_PINS[index]);
		}
	} // namespace

	void init()
	{
		std::uint64_t pinMask = 0;
		for (std::uint8_t i = 0; i < NUMBER_INPUTS; ++i)
		{
			pinMask |= (1ULL << board_config::DIGITAL_INPUT_PINS[i]);
		}

		gpio_config_t config = {};
		config.pin_bit_mask = pinMask;
		config.mode = GPIO_MODE_INPUT;
		// Pull-up (not pull-down): the optocoupler leaves the isolated-side pin floating when the
		// input is inactive and pulls it low when active, so an inactive input must read high. This
		// matches the vendor's own demo firmware, which uses INPUT_PULLUP for these pins.
		config.pull_up_en = GPIO_PULLUP_ENABLE;
		config.pull_down_en = GPIO_PULLDOWN_DISABLE;
		config.intr_type = GPIO_INTR_DISABLE;
		gpio_config(&config);
	}

	void update()
	{
		for (std::uint8_t i = 0; i < NUMBER_INPUTS; ++i)
		{
			bool raw = read_raw(i);
			if (raw == debouncedStates[i])
			{
				matchCounts[i] = 0; // Still agrees with the accepted state, so reset the counter
				continue;
			}
			if (++matchCounts[i] >= NUMBER_DEBOUNCE_SAMPLES)
			{
				debouncedStates[i] = raw; // The opposite level held long enough, so accept it
				matchCounts[i] = 0;
			}
		}
	}

	bool read_debounced(std::uint8_t channel)
	{
		if ((channel < 1) || (channel > NUMBER_INPUTS))
		{
			return false;
		}
		return debouncedStates[channel - 1];
	}
} // namespace input_driver
