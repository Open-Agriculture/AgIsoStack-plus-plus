//================================================================================================
/// @file spi_transaction_frame.hpp
///
/// @brief A class containing the data for a single SPI transaction.
/// Used in combination with SPIHardwarePlugin and allows for multiple transactions to be queued.
/// @author Daan Steenbergen
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef SPI_TRANSACTION_FRAME_HPP
#define SPI_TRANSACTION_FRAME_HPP

#include <cstdint>
#include <vector>

namespace isobus
{
	//================================================================================================
	/// @class SPITransactionFrame
	///
	/// @brief A class containing the data for a single SPI transaction
	//================================================================================================
	class SPITransactionFrame
	{
	public:
		/// @brief Construct a new SPITransactionFrame object
		/// @param[in] txBuffer A pointer to the buffer to transmit
		/// @param[in] read If true, the plugin will read the response to the write operation
		SPITransactionFrame(const std::vector<std::uint8_t> *txBuffer, bool read = false);

		/// @brief Read a byte from the response buffer
		/// @param[in] index The index of the byte to read
		/// @param[in, out] byte The byte to store the read value in
		/// @return True if the byte was read successfully, false otherwise
		bool read_byte(std::size_t index, std::uint8_t &byte) const;

		/// @brief Read multiple bytes from the response buffer
		/// @param[in] index The index of the first byte to read
		/// @param[in, out] buffer The buffer to store the bytes in
		/// @param[in] length The number of bytes to read
		/// @return True if the bytes were read successfully, false otherwise
		bool read_bytes(std::size_t index, std::uint8_t *buffer, std::size_t length) const;

		/// @brief Get the buffer to store the response in
		/// @note This function should only be called by the spi interface
		/// @return The buffer to store the response in
		std::vector<std::uint8_t> &get_rx_buffer();

		/// @brief Get the buffer to transmit
		/// @note This function should only be called by the spi interface
		/// @return The buffer to transmit
		const std::vector<std::uint8_t> *get_tx_buffer() const;

		/// @brief Get whether the interface should read the response to the write operation
		/// @return True if the interface should read the response to the write operation, false otherwise
		bool get_is_read() const;

	private:
		const std::vector<std::uint8_t> *txBuffer; ///< The buffer to transmit
		const bool read; ///< If true, the plugin will read the response to the write operation
		std::vector<std::uint8_t> rxBuffer; ///< The buffer to store the response in
	};
}
#endif // SPI_TRANSACTION_FRAME_HPP
