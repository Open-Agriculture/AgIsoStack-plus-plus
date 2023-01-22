//================================================================================================
/// @file spi_transaction_frame.cpp
///
/// @brief A frame containing the data for a single SPI transaction.
/// @author Daan Steenbergen
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/hardware_integration/spi_transaction_frame.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#include <cstring>

SPITransactionFrame::SPITransactionFrame(const std::vector<std::uint8_t> *txBuffer, bool read) :
  txBuffer(txBuffer),
  read(read),
  rxBuffer({})
{
	if (read)
	{
		rxBuffer.reserve(txBuffer->size());
	}
}

bool SPITransactionFrame::read_byte(std::size_t index, std::uint8_t &byte) const
{
	bool retVal = false;

	if (read)
	{
		if (index < rxBuffer.size())
		{
			byte = rxBuffer[index];
			retVal = true;
		}
		else
		{
			isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Error, "[SPIFrame] Tried to read byte at index " + isobus::to_string(index) + ", but the buffer only contains " + isobus::to_string(rxBuffer.size()) + " bytes");
		}
	}
	else
	{
		isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Error, "[SPIFrame] The transaction was not configured to read, but tried to read byte at index: " + isobus::to_string(index));
	}
	return retVal;
}

bool SPITransactionFrame::read_bytes(std::size_t index, std::uint8_t *buffer, std::size_t length) const
{
	bool retVal = false;
	if (read)
	{
		if (index + length <= rxBuffer.size())
		{
			std::memcpy(buffer, &rxBuffer[index], length);
			retVal = true;
		}
		else
		{
			isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Error, "[SPIFrame] Tried to read " + isobus::to_string(length) + " bytes at index " + isobus::to_string(index) + ", but the buffer only contains " + isobus::to_string(rxBuffer.size()) + " bytes");
		}
	}
	else
	{
		isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Error, "[SPIFrame] The transaction was not configured to read, but tried to read " + isobus::to_string(length) + " bytes at index: " + isobus::to_string(index));
	}
	return retVal;
}

std::vector<std::uint8_t> &SPITransactionFrame::get_rx_buffer()
{
	return rxBuffer;
}

const std::vector<std::uint8_t> *SPITransactionFrame::get_tx_buffer() const
{
	return txBuffer;
}

bool SPITransactionFrame::get_is_read() const
{
	return read;
}