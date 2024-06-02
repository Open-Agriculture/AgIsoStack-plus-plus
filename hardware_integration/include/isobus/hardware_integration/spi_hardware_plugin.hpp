//================================================================================================
/// @file spi_hardware_plugin.hpp
///
/// @brief A base class for SPI communication between hardware devices.
/// Can be derived into your platform's required interface.
/// @author Daan Steenbergen
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#ifndef SPI_HARDWARE_PLUGIN_HPP
#define SPI_HARDWARE_PLUGIN_HPP

#include "isobus/hardware_integration/spi_transaction_frame.hpp"

#include <cstdint>
#include <vector>

namespace isobus
{
	//================================================================================================
	/// @class SPIHardwarePlugin
	///
	/// @brief An abstract base class for SPI communication.
	//================================================================================================
	class SPIHardwarePlugin
	{
	public:
		/// @brief Begin a transaction on the SPI bus. This should be called before any of the read/write operations.
		/// @details Here the SPI bus can be acquired and prepared for a new transaction.
		/// @note If any error occurs, end_transaction() should return false to mark a failed transaction
		virtual void begin_transaction(){};

		/// @brief Write a frame to the SPI bus. This should only be called after begin_transaction().
		/// The result should only be read after end_transaction().
		/// @param frame A pointer to the frame to transmit
		/// @note If any error occurs, end_transaction() should return false to mark a failed transaction.
		virtual void transmit(SPITransactionFrame *frame) = 0;

		/// @brief End a transaction on the SPI bus. This must be called after all write operations and before any read operation.
		/// @details Here the SPI bus will be released and the transaction will be finalized.
		/// @return True if the transaction was successful, false otherwise
		virtual bool end_transaction() = 0;
	};
}
#endif // SPI_HARDWARE_PLUGIN_HPP
