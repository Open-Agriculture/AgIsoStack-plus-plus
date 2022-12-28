//================================================================================================
/// @file spi_transaction_plugin.hpp
///
/// @brief A base class for the communication between a CAN driver and SPI.
/// Can be derived into your platform's required interface.
/// @author Daan Steenbergen
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef SPI_TRANSACTION_PLUGIN_HPP
#define SPI_TRANSACTION_PLUGIN_HPP

#include <cstdint>

//================================================================================================
/// @class SPITransactionPlugin
///
/// @brief An abstract base class for SPI communication.
//================================================================================================
class SPITransactionPlugin
{
public:
	/// @brief Returns if the the transaction was valid and no errors occurred
	/// @returns `true` if no errors occured and transaction was succesfull,
	///          `false` if there was an error and the response should not be used.
	virtual bool get_is_valid() const = 0;

	/// @brief Opens a transaction with the SPI bus
	/// @details This function can be overridden if a transaction wishes to be used.
	/// Any errors that occur should be reported by returning `false` in get_is_valid().
	virtual void begin_transaction()
	{
	}

	/// @brief Writes a frame to the SPI bus and reads the response
	/// @details The returned response should only be used after end_transaction() AND get_is_valid() returns true.
	/// Any errors that occur should be reported by returning `false` in get_is_valid().
	/// @param[in] txFrame The frame to write to the bus
	/// @returns The frame that was read from the bus
	virtual std::uint8_t read_write(const std::uint8_t txFrame) = 0;

	/// @brief Writes a frame to the SPI bus
	/// @details Can be overwriten if there is different native functions for reading and writing.
	/// Any errors that occur should be reported by returning `false` in get_is_valid().
	/// @param[in] txFrame The frame to write to the bus
	virtual void write(const std::uint8_t txFrame)
	{
		read_write(txFrame);
	};

	/// @brief Ends a transaction with the SPI bus
	/// @details This function can be overridden if a transaction wishes to be used.
	/// Any errors that occur should be reported by returning `false` in get_is_valid().
	virtual void end_transaction()
	{
	}
};

#endif // SPI_TRANSACTION_PLUGIN_HPP
