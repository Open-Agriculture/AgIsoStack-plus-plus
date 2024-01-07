//================================================================================================
/// @file can_message_data.hpp
///
/// @brief An interface class that represents data payload of a CAN message of arbitrary length.
/// @author Daan Steenbergen
///
/// @copyright 2023 - The OpenAgriculture Developers
//================================================================================================

#ifndef CAN_MESSAGE_DATA_HPP
#define CAN_MESSAGE_DATA_HPP

#include "isobus/isobus/can_callbacks.hpp"
#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/utility/data_span.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace isobus
{
	/// @brief A interface class that represents data payload of a CAN message of arbitrary length.
	class CANMessageData
	{
	public:
		/// @brief Default destructor.
		virtual ~CANMessageData() = default;

		/// @brief Get the size of the data.
		/// @return The size of the data.
		virtual std::size_t size() const = 0;

		/// @brief Get the byte at the given index.
		/// @param[in] index The index of the byte to get.
		/// @return The byte at the given index.
		virtual std::uint8_t get_byte(std::size_t index) = 0;

		/// @brief If the data isn't owned by this class, make a copy of the data.
		/// @param[in] self A pointer to this object.
		/// @return A copy of the data if it isn't owned by this class, otherwise a moved pointer.
		virtual std::unique_ptr<CANMessageData> copy_if_not_owned(std::unique_ptr<CANMessageData> self) const = 0;
	};

	/// @brief A class that represents data of a CAN message by holding a vector of bytes.
	class CANMessageDataVector : public CANMessageData
	  , public std::vector<std::uint8_t>
	{
	public:
		/// @brief Construct a new CANMessageDataVector object.
		/// @param[in] size The size of the data.
		explicit CANMessageDataVector(std::size_t size);

		/// @brief Construct a new CANMessageDataVector object.
		/// @param[in] data The data to copy.
		explicit CANMessageDataVector(const std::vector<std::uint8_t> &data);

		/// @brief Construct a new CANMessageDataVector object.
		/// @param[in] data A pointer to the data to copy.
		/// @param[in] size The size of the data to copy.
		CANMessageDataVector(const std::uint8_t *data, std::size_t size);

		/// @brief Get the size of the data.
		/// @return The size of the data.
		std::size_t size() const override;

		/// @brief Get the byte at the given index.
		/// @param[in] index The index of the byte to get.
		/// @return The byte at the given index.
		std::uint8_t get_byte(std::size_t index) override;

		/// @brief Set the byte at the given index.
		/// @param[in] index The index of the byte to set.
		/// @param[in] value The value to set the byte to.
		void set_byte(std::size_t index, std::uint8_t value);

		/// @brief Get the data span.
		/// @return The data span.
		CANDataSpan data() const;

		/// @brief If the data isn't owned by this class, make a copy of the data.
		/// @param[in] self A pointer to this object.
		/// @return A copy of the data if it isn't owned by this class, otherwise it returns itself.
		std::unique_ptr<CANMessageData> copy_if_not_owned(std::unique_ptr<CANMessageData> self) const override;
	};

	/// @brief A class that represents data of a CAN message by holding a view of an array of bytes.
	/// The view is not owned by this class, it is simply holding a pointer to the array of bytes.
	class CANMessageDataView : public CANMessageData
	  , public CANDataSpan
	{
	public:
		/// @brief Construct a new CANMessageDataView object.
		/// @param[in] ptr The pointer to the array of bytes.
		/// @param[in] len The length of the array of bytes.
		CANMessageDataView(const std::uint8_t *ptr, std::size_t len);

		/// @brief Get the size of the data.
		/// @return The size of the data.
		std::size_t size() const override;

		/// @brief Get the byte at the given index.
		/// @param[in] index The index of the byte to get.
		/// @return The byte at the given index.
		std::uint8_t get_byte(std::size_t index) override;

		/// @brief Get the data span.
		/// @return The data span.
		CANDataSpan data() const;

		/// @brief If the data isn't owned by this class, make a copy of the data.
		/// @param[in] self A pointer to this object.
		/// @return A copy of the data if it isn't owned by this class, otherwise it returns itself.
		std::unique_ptr<CANMessageData> copy_if_not_owned(std::unique_ptr<CANMessageData> self) const override;
	};

	/// @brief A class that represents data of a CAN message by using a callback function.
	class CANMessageDataCallback : public CANMessageData
	{
	public:
		/// @brief Constructor for transport data that uses a callback function.
		/// @param[in] size The size of the data.
		/// @param[in] callback The callback function to be called for each data chunk.
		/// @param[in] parentPointer The parent object that owns this callback (optional).
		/// @param[in] chunkSize The size of each data chunk (optional, default is 7).
		CANMessageDataCallback(std::size_t size,
		                       DataChunkCallback callback,
		                       void *parentPointer = nullptr,
		                       std::size_t chunkSize = 7);

		/// @brief Get the size of the data.
		/// @return The size of the data.
		std::size_t size() const override;

		/// @brief Get the byte at the given index.
		/// @param[in] index The index of the byte to get.
		/// @return The byte at the given index.
		std::uint8_t get_byte(std::size_t index) override;

		/// @brief If the data isn't owned by this class, make a copy of the data.
		/// @param[in] self A pointer to this object.
		/// @return A copy of the data if it isn't owned by this class, otherwise it returns itself.
		std::unique_ptr<CANMessageData> copy_if_not_owned(std::unique_ptr<CANMessageData> self) const override;

	private:
		std::size_t totalSize; ///< The total size of the data.
		DataChunkCallback callback; ///< The callback function to be called for each data chunk.
		void *parentPointer; ///< The parent object that gets passed to the callback function.
		std::vector<std::uint8_t> buffer; ///< The buffer to store the data chunks.
		std::size_t bufferSize; ///< The size of the buffer.
		std::size_t dataOffset = 0; ///< The offset of the data in the buffer.
		bool initialized = false; ///< Whether the buffer has been initialized.
	};
} // namespace isobus

#endif // CAN_MESSAGE_DATA_HPP
