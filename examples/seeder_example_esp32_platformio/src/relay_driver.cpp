//================================================================================================
/// @file relay_driver.cpp
///
/// @brief Implements the TCA9554PWR I2C relay expander driver for this example.
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include "relay_driver.hpp"

#include "board_config.hpp"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

namespace relay_driver
{
	namespace
	{
		constexpr const char *TAG = "relay_driver";
		constexpr gpio_num_t SDA_PIN = board_config::RELAY_I2C_SDA_PIN; ///< Board wiring lives in board_config.hpp
		constexpr gpio_num_t SCL_PIN = board_config::RELAY_I2C_SCL_PIN; ///< Board wiring lives in board_config.hpp
		constexpr i2c_port_num_t I2C_PORT = I2C_NUM_0;
		constexpr std::uint16_t EXPANDER_ADDRESS = board_config::RELAY_EXPANDER_ADDRESS; ///< The TCA9554PWR 7-bit address
		constexpr std::uint8_t REGISTER_OUTPUT_PORT = 0x01; ///< Sets the logic level driven onto each output pin
		constexpr std::uint8_t REGISTER_CONFIG = 0x03; ///< Per-pin direction, where 0 = output and 1 = input

		i2c_master_bus_handle_t bus = nullptr;
		i2c_master_dev_handle_t device = nullptr;
		std::uint8_t outputState = 0x00; ///< Shadow of the Output Port register, where bit 0 = relay channel 1

		/// @brief Writes a single value byte to an expander register.
		/// @param[in] registerAddress The register to write to
		/// @param[in] value The value to write
		/// @returns true if the write was acknowledged
		bool write_register(std::uint8_t registerAddress, std::uint8_t value)
		{
			if (nullptr == device)
			{
				return false;
			}
			std::uint8_t buffer[2] = { registerAddress, value };
			esp_err_t result = i2c_master_transmit(device, buffer, sizeof(buffer), pdMS_TO_TICKS(100));
			if (ESP_OK != result)
			{
				ESP_LOGE(TAG, "Write to register 0x%02X failed: %s", registerAddress, esp_err_to_name(result));
				return false;
			}
			return true;
		}
	} // namespace

	bool init()
	{
		i2c_master_bus_config_t busConfig = {};
		busConfig.i2c_port = I2C_PORT;
		busConfig.sda_io_num = SDA_PIN;
		busConfig.scl_io_num = SCL_PIN;
		busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
		busConfig.glitch_ignore_cnt = 7;
		busConfig.flags.enable_internal_pullup = true;
		if (ESP_OK != i2c_new_master_bus(&busConfig, &bus))
		{
			ESP_LOGE(TAG, "Failed to create the I2C master bus.");
			return false;
		}

		i2c_device_config_t deviceConfig = {};
		deviceConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
		deviceConfig.device_address = EXPANDER_ADDRESS;
		deviceConfig.scl_speed_hz = 100000;
		if (ESP_OK != i2c_master_bus_add_device(bus, &deviceConfig, &device))
		{
			ESP_LOGE(TAG, "Failed to add the TCA9554PWR at address 0x%02X.", EXPANDER_ADDRESS);
			return false;
		}

		// Drive the "all off" level into the Output Port register before switching the
		// Configuration register to outputs, so that no pin glitches through an
		// unintended state while it flips from being an input to an output.
		outputState = 0x00;
		bool succeeded = write_register(REGISTER_OUTPUT_PORT, outputState);
		succeeded = write_register(REGISTER_CONFIG, 0x00) && succeeded; // 0 = output for all 8 pins
		if (!succeeded)
		{
			ESP_LOGE(TAG, "The TCA9554PWR did not respond at address 0x%02X. Is the relay board wired and powered?", EXPANDER_ADDRESS);
		}
		return succeeded;
	}

	bool set_relay(std::uint8_t channel, bool on)
	{
		if ((channel < 1) || (channel > board_config::NUMBER_RELAY_CHANNELS) || (nullptr == device))
		{
			return false;
		}
		std::uint8_t bit = static_cast<std::uint8_t>(1U << (channel - 1));

		// NOTE: This assumes a high expander pin energizes the relay. If the relays on
		// your board are wired active-low, invert `on` here rather than at every call site.
		std::uint8_t newState = on ? static_cast<std::uint8_t>(outputState | bit) : static_cast<std::uint8_t>(outputState & ~bit);
		if (newState == outputState)
		{
			return true; // Nothing changed, so skip the I2C transaction.
		}
		if (!write_register(REGISTER_OUTPUT_PORT, newState))
		{
			return false;
		}
		outputState = newState;
		return true;
	}
} // namespace relay_driver
