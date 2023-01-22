//================================================================================================
/// @file spi_interface_esp.cpp
///
/// @brief A driver for (synchronous) SPI communication on ESP platforms.
/// @author Daan Steenbergen
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "isobus/hardware_integration/spi_interface_esp.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#include <cstring>

SPIInterfaceESP::SPIInterfaceESP(const spi_device_interface_config_t *deviceConfig, const spi_host_device_t hostDevice) :
  deviceConfig(deviceConfig),
  hostDevice(hostDevice),
  spiMutex(xSemaphoreCreateMutex()),
  initialized(false),
  success(true) {}

SPIInterfaceESP::~SPIInterfaceESP()
{
	deinit();
	vSemaphoreDelete(spiMutex);
}

bool SPIInterfaceESP::init()
{
	esp_err_t error = spi_bus_add_device(hostDevice, deviceConfig, &spiDevice);
	if (ESP_OK == error)
	{
		initialized = true;
	}
	else
	{
		isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Critical, "[SPI-ESP] Failed to add SPI device: " + isobus::to_string(esp_err_to_name(error)));
	}

	return ESP_OK == error;
}

void SPIInterfaceESP::deinit()
{
	esp_err_t error = spi_bus_remove_device(spiDevice);
	if (ESP_OK != error)
	{
		isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Error, "[SPI-ESP] Failed to remove SPI device: " + isobus::to_string(esp_err_to_name(error)));
	}
	if (ESP_ERR_INVALID_STATE == error || ESP_OK == error)
	{
		initialized = false;
	}
}

void SPIInterfaceESP::transmit(SPITransactionFrame *frame)
{
	if (initialized)
	{
		if (xSemaphoreTake(spiMutex, MAX_TIME_TO_WAIT) == pdTRUE)
		{
			spi_transaction_t transaction;
			std::memset(&transaction, 0, sizeof(transaction));

			transaction.length = frame->get_tx_buffer()->size() * 8;
			transaction.tx_buffer = frame->get_tx_buffer()->data();
			if (frame->get_is_read())
			{
				frame->get_rx_buffer().resize(frame->get_tx_buffer()->size());
				transaction.rx_buffer = frame->get_rx_buffer().data();
			}
			else
			{
				transaction.rx_buffer = nullptr;
			}

			esp_err_t error = spi_device_transmit(spiDevice, &transaction);
			if (ESP_OK != error)
			{
				success = false;
				isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Warning, "[SPI-ESP] Failed to transmit SPI transaction frame: " + isobus::to_string(esp_err_to_name(error)));
			}
			xSemaphoreGive(spiMutex);
		}
		else
		{
			success = false;
			isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Error, "[SPI-ESP] Failed to obtain SPI mutex in transmit().");
		}
	}
	else
	{
		success = false;
		isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Critical, "[SPI-ESP] SPI device not initialized, pherhaps you forgot to call init()?");
	}
}

bool SPIInterfaceESP::end_transaction()
{
	bool retVal = success;
	success = true; // Reset success flag for next transaction
	return retVal;
}