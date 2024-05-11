//================================================================================================
/// @file can_message_data.cpp
///
/// @brief An class that represents a CAN message of arbitrary length being transported.
/// @author Daan Steenbergen
///
/// @copyright 2023 OpenAgriculture
//================================================================================================

#include "isobus/isobus/can_message_data.hpp"

#include <algorithm>

namespace isobus
{
	CANMessageDataVector::CANMessageDataVector(std::size_t size)
	{
		vector::resize(size);
	}

	CANMessageDataVector::CANMessageDataVector(const std::vector<std::uint8_t> &data)
	{
		vector::assign(data.begin(), data.end());
	}

	CANMessageDataVector::CANMessageDataVector(const std::uint8_t *data, std::size_t size)
	{
		vector::assign(data, data + size);
	}

	std::size_t CANMessageDataVector::size() const
	{
		return vector::size();
	}

	std::uint8_t CANMessageDataVector::get_byte(std::size_t index)
	{
		return vector::at(index);
	}

	void CANMessageDataVector::set_byte(std::size_t index, std::uint8_t value)
	{
		vector::at(index) = value;
	}

	CANDataSpan CANMessageDataVector::data() const
	{
		return CANDataSpan(vector::data(), vector::size());
	}

	std::unique_ptr<CANMessageData> CANMessageDataVector::copy_if_not_owned(std::unique_ptr<CANMessageData> self) const
	{
		// We know that a CANMessageDataVector is always owned by itself, so we can just return itself.
		return self;
	}

	CANMessageDataView::CANMessageDataView(const std::uint8_t *ptr, std::size_t len) :
	  CANDataSpan(ptr, len)
	{
	}

	std::size_t CANMessageDataView::size() const
	{
		return DataSpan::size();
	}

	std::uint8_t CANMessageDataView::get_byte(std::size_t index)
	{
		return DataSpan::operator[](index);
	}

	CANDataSpan CANMessageDataView::data() const
	{
		return CANDataSpan(DataSpan::begin(), DataSpan::size());
	}

	std::unique_ptr<CANMessageData> CANMessageDataView::copy_if_not_owned(std::unique_ptr<CANMessageData>) const
	{
		// A view doesn't own the data, so we need to make a copy.
		return std::unique_ptr<CANMessageData>(new CANMessageDataVector(DataSpan::begin(), DataSpan::size()));
	}

	CANMessageDataCallback::CANMessageDataCallback(std::size_t size,
	                                               DataChunkCallback callback,
	                                               void *parentPointer,
	                                               std::size_t chunkSize) :
	  totalSize(size),
	  callback(callback),
	  parentPointer(parentPointer),
	  buffer(chunkSize),
	  bufferSize(chunkSize)
	{
	}

	std::size_t CANMessageDataCallback::size() const
	{
		return totalSize;
	}

	std::uint8_t CANMessageDataCallback::get_byte(std::size_t index)
	{
		if (index >= totalSize)
		{
			return 0;
		}

		if ((index >= dataOffset + bufferSize) || (index < dataOffset) || (!initialized))
		{
			initialized = true;
			dataOffset = index;
			callback(0, dataOffset, std::min(totalSize - dataOffset, bufferSize), buffer.data(), parentPointer);
		}
		return buffer[index - dataOffset];
	}

	std::unique_ptr<CANMessageData> CANMessageDataCallback::copy_if_not_owned(std::unique_ptr<CANMessageData> self) const
	{
		// A callback doesn't own it's data, but it does own the callback function, so we can just return itself.
		return self;
	}

} // namespace isobus
