//================================================================================================
/// @file can_message.cpp
///
/// @brief An abstraction of a CAN message, could be > 8 data bytes.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <cassert>

namespace isobus
{
	CANMessage::CANMessage(Type type,
	                       CANIdentifier identifier,
	                       const std::uint8_t *dataBuffer,
	                       std::uint32_t length,
	                       std::shared_ptr<ControlFunction> source,
	                       std::shared_ptr<ControlFunction> destination,
	                       std::uint8_t CANPort) :
	  messageType(type),
	  identifier(identifier),
	  data(dataBuffer, dataBuffer + length),
	  source(source),
	  destination(destination),
	  CANPortIndex(CANPort)
	{
	}

	CANMessage::CANMessage(Type type,
	                       CANIdentifier identifier,
	                       std::vector<std::uint8_t> data,
	                       std::shared_ptr<ControlFunction> source,
	                       std::shared_ptr<ControlFunction> destination,
	                       std::uint8_t CANPort) :
	  messageType(type),
	  identifier(identifier),
	  data(std::move(data)),
	  source(source),
	  destination(destination),
	  CANPortIndex(CANPort)
	{
	}

	CANMessage CANMessage::create_invalid_message()
	{
		return CANMessage(CANMessage::Type::Receive, CANIdentifier(CANIdentifier::UNDEFINED_PARAMETER_GROUP_NUMBER), {}, nullptr, nullptr, 0);
	}

	CANMessage::Type CANMessage::get_type() const
	{
		return messageType;
	}

	const std::vector<std::uint8_t> &CANMessage::get_data() const
	{
		return data;
	}

	std::uint32_t CANMessage::get_data_length() const
	{
		return static_cast<std::uint32_t>(data.size());
	}

	std::shared_ptr<ControlFunction> CANMessage::get_source_control_function() const
	{
		return source;
	}

	bool CANMessage::has_valid_source_control_function() const
	{
		return (nullptr != source) && source->get_address_valid();
	}

	std::shared_ptr<ControlFunction> CANMessage::get_destination_control_function() const
	{
		return destination;
	}

	bool CANMessage::has_valid_destination_control_function() const
	{
		return (nullptr != destination) && destination->get_address_valid();
	}

	bool CANMessage::is_broadcast() const
	{
		return (!has_valid_destination_control_function()) || (destination->get_address() == CANIdentifier::GLOBAL_ADDRESS);
	}

	bool CANMessage::is_destination_our_device() const
	{
		return has_valid_destination_control_function() && destination->get_type() == ControlFunction::Type::Internal;
	}

	bool CANMessage::is_destination(std::shared_ptr<ControlFunction> controlFunction) const
	{
		return has_valid_destination_control_function() && destination == controlFunction;
	}

	bool CANMessage::is_source(std::shared_ptr<ControlFunction> controlFunction) const
	{
		return has_valid_source_control_function() && source == controlFunction;
	}

	CANIdentifier CANMessage::get_identifier() const
	{
		return identifier;
	}

	bool CANMessage::is_parameter_group_number(CANLibParameterGroupNumber parameterGroupNumber) const
	{
		return identifier.get_parameter_group_number() == static_cast<std::uint32_t>(parameterGroupNumber);
	}

	std::uint8_t CANMessage::get_can_port_index() const
	{
		return CANPortIndex;
	}

	void CANMessage::set_data(const std::uint8_t *dataBuffer, std::uint32_t length)
	{
		assert(length <= ABSOLUTE_MAX_MESSAGE_LENGTH && "CANMessage::set_data() called with length greater than maximum supported");
		assert(nullptr != dataBuffer && "CANMessage::set_data() called with nullptr dataBuffer");

		data.insert(data.end(), dataBuffer, dataBuffer + length);
	}

	void CANMessage::set_data(std::uint8_t dataByte, const std::uint32_t insertPosition)
	{
		assert(insertPosition <= ABSOLUTE_MAX_MESSAGE_LENGTH && "CANMessage::set_data() called with insertPosition greater than maximum supported");

		data[insertPosition] = dataByte;
	}

	void CANMessage::set_data_size(std::uint32_t length)
	{
		data.resize(length);
	}

	void CANMessage::set_identifier(const CANIdentifier &value)
	{
		identifier = value;
	}

	std::uint8_t CANMessage::get_uint8_at(const std::uint32_t index) const
	{
		return data.at(index);
	}

	std::int8_t CANMessage::get_int8_at(const std::uint32_t index) const
	{
		return static_cast<std::int8_t>(data.at(index));
	}

	std::uint16_t CANMessage::get_uint16_at(const std::uint32_t index, const ByteFormat format) const
	{
		std::uint16_t retVal;
		if (ByteFormat::LittleEndian == format)
		{
			retVal = data.at(index);
			retVal |= static_cast<std::uint16_t>(static_cast<std::uint16_t>(data.at(index + 1)) << 8);
		}
		else
		{
			retVal = static_cast<std::uint16_t>(static_cast<std::uint16_t>(data.at(index)) << 8);
			retVal |= data.at(index + 1);
		}
		return retVal;
	}

	std::int16_t CANMessage::get_int16_at(const std::uint32_t index, const ByteFormat format) const
	{
		std::int16_t retVal;
		if (ByteFormat::LittleEndian == format)
		{
			retVal = static_cast<std::int16_t>(data.at(index));
			retVal |= static_cast<std::int16_t>(static_cast<std::int16_t>(data.at(index + 1)) << 8);
		}
		else
		{
			retVal = static_cast<std::int16_t>(static_cast<std::int16_t>(data.at(index)) << 8);
			retVal |= static_cast<std::int16_t>(data.at(index + 1));
		}
		return retVal;
	}

	std::uint32_t CANMessage::get_uint24_at(const std::uint32_t index, const ByteFormat format) const
	{
		std::uint32_t retVal;
		if (ByteFormat::LittleEndian == format)
		{
			retVal = data.at(index);
			retVal |= static_cast<std::uint32_t>(data.at(index + 1)) << 8;
			retVal |= static_cast<std::uint32_t>(data.at(index + 2)) << 16;
		}
		else
		{
			retVal = static_cast<std::uint32_t>(data.at(index + 2)) << 16;
			retVal |= static_cast<std::uint32_t>(data.at(index + 1)) << 8;
			retVal |= data.at(index + 2);
		}
		return retVal;
	}

	std::int32_t CANMessage::get_int24_at(const std::uint32_t index, const ByteFormat format) const
	{
		std::int32_t retVal;
		if (ByteFormat::LittleEndian == format)
		{
			retVal = static_cast<std::int32_t>(data.at(index));
			retVal |= static_cast<std::int32_t>(data.at(index + 1)) << 8;
			retVal |= static_cast<std::int32_t>(data.at(index + 2)) << 16;
		}
		else
		{
			retVal = static_cast<std::int32_t>(data.at(index + 2)) << 16;
			retVal |= static_cast<std::int32_t>(data.at(index + 1)) << 8;
			retVal |= static_cast<std::int32_t>(data.at(index + 2));
		}
		return retVal;
	}

	std::uint32_t CANMessage::get_uint32_at(const std::uint32_t index, const ByteFormat format) const
	{
		std::uint32_t retVal;
		if (ByteFormat::LittleEndian == format)
		{
			retVal = data.at(index);
			retVal |= static_cast<std::uint32_t>(data.at(index + 1)) << 8;
			retVal |= static_cast<std::uint32_t>(data.at(index + 2)) << 16;
			retVal |= static_cast<std::uint32_t>(data.at(index + 3)) << 24;
		}
		else
		{
			retVal = static_cast<std::uint32_t>(data.at(index)) << 24;
			retVal |= static_cast<std::uint32_t>(data.at(index + 1)) << 16;
			retVal |= static_cast<std::uint32_t>(data.at(index + 2)) << 8;
			retVal |= data.at(index + 3);
		}
		return retVal;
	}

	std::int32_t CANMessage::get_int32_at(const std::uint32_t index, const ByteFormat format) const
	{
		std::int32_t retVal;
		if (ByteFormat::LittleEndian == format)
		{
			retVal = static_cast<std::int32_t>(data.at(index));
			retVal |= static_cast<std::int32_t>(data.at(index + 1)) << 8;
			retVal |= static_cast<std::int32_t>(data.at(index + 2)) << 16;
			retVal |= static_cast<std::int32_t>(data.at(index + 3)) << 24;
		}
		else
		{
			retVal = static_cast<std::int32_t>(data.at(index)) << 24;
			retVal |= static_cast<std::int32_t>(data.at(index + 1)) << 16;
			retVal |= static_cast<std::int32_t>(data.at(index + 2)) << 8;
			retVal |= static_cast<std::int32_t>(data.at(index + 3));
		}
		return retVal;
	}

	std::uint64_t CANMessage::get_uint64_at(const std::uint32_t index, const ByteFormat format) const
	{
		std::uint64_t retVal;
		if (ByteFormat::LittleEndian == format)
		{
			retVal = data.at(index);
			retVal |= static_cast<std::uint64_t>(data.at(index + 1)) << 8;
			retVal |= static_cast<std::uint64_t>(data.at(index + 2)) << 16;
			retVal |= static_cast<std::uint64_t>(data.at(index + 3)) << 24;
			retVal |= static_cast<std::uint64_t>(data.at(index + 4)) << 32;
			retVal |= static_cast<std::uint64_t>(data.at(index + 5)) << 40;
			retVal |= static_cast<std::uint64_t>(data.at(index + 6)) << 48;
			retVal |= static_cast<std::uint64_t>(data.at(index + 7)) << 56;
		}
		else
		{
			retVal = static_cast<std::uint64_t>(data.at(index)) << 56;
			retVal |= static_cast<std::uint64_t>(data.at(index + 1)) << 48;
			retVal |= static_cast<std::uint64_t>(data.at(index + 2)) << 40;
			retVal |= static_cast<std::uint64_t>(data.at(index + 3)) << 32;
			retVal |= static_cast<std::uint64_t>(data.at(index + 4)) << 24;
			retVal |= static_cast<std::uint64_t>(data.at(index + 5)) << 16;
			retVal |= static_cast<std::uint64_t>(data.at(index + 6)) << 8;
			retVal |= data.at(index + 7);
		}
		return retVal;
	}

	std::int64_t CANMessage::get_int64_at(const std::uint32_t index, const ByteFormat format) const
	{
		std::int64_t retVal;
		if (ByteFormat::LittleEndian == format)
		{
			retVal = static_cast<std::int64_t>(data.at(index));
			retVal |= static_cast<std::int64_t>(data.at(index + 1)) << 8;
			retVal |= static_cast<std::int64_t>(data.at(index + 2)) << 16;
			retVal |= static_cast<std::int64_t>(data.at(index + 3)) << 24;
			retVal |= static_cast<std::int64_t>(data.at(index + 4)) << 32;
			retVal |= static_cast<std::int64_t>(data.at(index + 5)) << 40;
			retVal |= static_cast<std::int64_t>(data.at(index + 6)) << 48;
			retVal |= static_cast<std::int64_t>(data.at(index + 7)) << 56;
		}
		else
		{
			retVal = static_cast<std::int64_t>(data.at(index)) << 56;
			retVal |= static_cast<std::int64_t>(data.at(index + 1)) << 48;
			retVal |= static_cast<std::int64_t>(data.at(index + 2)) << 40;
			retVal |= static_cast<std::int64_t>(data.at(index + 3)) << 32;
			retVal |= static_cast<std::int64_t>(data.at(index + 4)) << 24;
			retVal |= static_cast<std::int64_t>(data.at(index + 5)) << 16;
			retVal |= static_cast<std::int64_t>(data.at(index + 6)) << 8;
			retVal |= static_cast<std::int64_t>(data.at(index + 7));
		}
		return retVal;
	}

	bool isobus::CANMessage::get_bool_at(const std::uint32_t byteIndex, const std::uint8_t bitIndex, const std::uint8_t length) const
	{
		assert(length <= 8 - bitIndex && "length must be less than or equal to 8 - bitIndex");
		auto mask = static_cast<std::uint8_t>(((1 << length) - 1) << bitIndex);
		return (get_uint8_at(byteIndex) & mask) == mask;
	}

	std::uint64_t CANMessage::get_data_custom_length(const std::uint32_t startBitIndex, const std::uint32_t length, const isobus::CANMessage::ByteFormat format) const
	{
		std::uint64_t retVal = 0;
		std::uint8_t currentByte = 0;
		std::uint32_t endBitIndex = startBitIndex + length - 1;
		std::uint32_t bitCounter = 0;
		std::uint32_t amountOfBytesLeft = (length + 8 - 1) / 8;
		std::uint32_t startAmountOfBytes = amountOfBytesLeft;
		std::uint8_t indexOfFinalByteBit = 7;

		if (endBitIndex > 8 * data.size() || length < 1 || startBitIndex >= 8 * data.size())
		{
			LOG_ERROR("End bit index is greater than length or startBitIndex is wrong or startBitIndex is greater than endBitIndex");
			return retVal;
		}

		for (auto i = startBitIndex; i <= endBitIndex; i++)
		{
			auto byteIndex = i / 8;
			auto bitIndexWithinByte = i % 8;
			auto bit = (data.at(byteIndex) >> (indexOfFinalByteBit - bitIndexWithinByte)) & 1;
			if (length - bitCounter < 8)
			{
				currentByte |= static_cast<uint8_t>(bit) << (length - 1 - bitCounter);
			}
			else
			{
				currentByte |= static_cast<uint8_t>(bit) << (indexOfFinalByteBit - bitIndexWithinByte);
			}

			if ((bitCounter + 1) % 8 == 0 || i == endBitIndex)
			{
				if (ByteFormat::LittleEndian == format)
				{
					retVal |= (static_cast<uint64_t>(currentByte) << (startAmountOfBytes - amountOfBytesLeft) * 8);
				}
				else
				{
					retVal |= (static_cast<uint64_t>(currentByte) << ((amountOfBytesLeft * 8) - 8));
				}
				currentByte = 0;
				amountOfBytesLeft--;
			}

			bitCounter++;
		}

		return retVal;
	}

} // namespace isobus
