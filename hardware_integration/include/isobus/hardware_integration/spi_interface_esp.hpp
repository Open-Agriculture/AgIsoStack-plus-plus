//================================================================================================
/// @file spi_interface_esp.hpp
///
/// @brief A (synchronous) implementation for SPI communication with (CAN) hardware devices on
/// ESP platforms.
/// @author Daan Steenbergen
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#ifndef SPI_INTERFACE_ESP_SYNC_HPP
#define SPI_INTERFACE_ESP_SYNC_HPP

#include "isobus/hardware_integration/spi_hardware_plugin.hpp"

#include "driver/spi_master.h"
#include "freertos/semphr.h"

namespace isobus
{
	//================================================================================================
	/// @class SPIInterfaceESP
	///
	/// @brief A driver for (synchronous) SPI communication on ESP platforms.
	//================================================================================================
	class SPIInterfaceESP : public SPIHardwarePlugin
	{
	public:
		static constexpr std::uint32_t MAX_TIME_TO_WAIT = 5000 / portTICK_PERIOD_MS; ///< timeout of 5 seconds for spi related calls, mostly arbitrary.

		/// @brief Constructor of a SPI device on an ESP platform.
		/// @param deviceConfig A pointer to the configuration of the SPI device
		/// @param hostDevice The host device of the SPI device, e.g. SPI2_HOST
		SPIInterfaceESP(const spi_device_interface_config_t *deviceConfig, const spi_host_device_t hostDevice);

		/// @brief Destructor of a SPI device on an ESP platform.
		virtual ~SPIInterfaceESP();

		/// @brief Initialize the SPI device.
		/// @return True if the initialization was successful, false otherwise
		bool init();

		/// @brief Deinitialize the SPI device.
		void deinit();

		/// @brief Write (and read) a frame to the SPI bus
		/// @param[in, out] frame A reference to the frame to transmit/receive
		void transmit(SPITransactionFrame *frame) override;

		/// @brief End the transaction. This function returns the status since the last end_transaction().
		/// @return True if the transaction was successful, false otherwise
		bool end_transaction() override;

	private:
		const spi_device_interface_config_t *deviceConfig; ///< The configuration of the SPI device
		const spi_host_device_t hostDevice; ///< The host spi device
		const SemaphoreHandle_t spiMutex; ///< A mutex to prevent concurrent access to the SPI bus
		spi_device_handle_t spiDevice; ///< A handle to the SPI device
		bool initialized; ///< The status of the device
		bool success; ///< The status of the current transaction
	};
}
#endif // SPI_INTERFACE_ESP_SYNC_HPP
