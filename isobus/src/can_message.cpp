//================================================================================================
/// @file can_message.cpp
///
/// @brief An abstraction  of a CAN message, could be > 8 data bytes.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_message.hpp"

namespace isobus
{
	std::uint32_t CANMessage::lastGeneratedUniqueID = 0;

	CANMessage::CANMessage(std::uint8_t CANPort) :
	  source(nullptr),
	  destination(nullptr),
	  identifier(0),
	  messageType(Type::Receive),
	  messageUniqueID(lastGeneratedUniqueID++),
	  CANPortIndex(CANPort)
	{
	}

	CANMessage::Type CANMessage::get_type() const
	{
		return messageType;
	}

	std::vector<std::uint8_t> &CANMessage::get_data()
	{
		return data;
	}

	std::uint32_t CANMessage::get_data_length() const
	{
		return data.size();
	}

	ControlFunction *CANMessage::get_source_control_function() const
	{
		return source;
	}

	ControlFunction *CANMessage::get_destination_control_function() const
	{
		return destination;
	}

	CANIdentifier CANMessage::get_identifier() const
	{
		return identifier;
	}

	std::uint32_t CANMessage::get_message_unique_id() const
	{
		return messageUniqueID;
	}

	std::uint8_t CANMessage::get_can_port_index() const
	{
		return CANPortIndex;
	}

	std::uint8_t CANMessage::get_uint8_at(const std::size_t index)
	{
		std::uint8_t retVal = 0;
		if (index + 1 <= get_data_length())
		{
			retVal = data[index];
		}
		return retVal;
	}

	std::uint16_t CANMessage::get_uint16_at(const std::size_t index, const ByteFormat format)
	{
		std::uint16_t retVal = 0;
		if (index + 2 <= get_data_length())
		{
			if (ByteFormat::LittleEndian == format)
			{
				retVal = data[index];
				retVal |= static_cast<std::uint16_t>(data[index + 1]) << 8;
			}
			else
			{
				retVal = static_cast<std::uint16_t>(data[index]) << 8;
				retVal |= data[index + 1];
			}
		}
		return retVal;
	}

	std::uint32_t CANMessage::get_uint32_at(const std::size_t index, const ByteFormat format)
	{
		std::uint32_t retVal = 0;
		if (index + 4 <= get_data_length())
		{
			if (ByteFormat::LittleEndian == format)
			{
				retVal = data[index];
				retVal |= static_cast<std::uint32_t>(data[index + 1]) << 8;
				retVal |= static_cast<std::uint32_t>(data[index + 2]) << 16;
				retVal |= static_cast<std::uint32_t>(data[index + 3]) << 24;
			}
			else
			{
				retVal = static_cast<std::uint32_t>(data[index]) << 24;
				retVal |= static_cast<std::uint32_t>(data[index + 1]) << 16;
				retVal |= static_cast<std::uint32_t>(data[index + 2]) << 8;
				retVal |= data[index + 3];
			}
		}
		return retVal;
	}

	std::uint64_t CANMessage::get_uint64_at(const std::size_t index, const ByteFormat format)
	{
		std::uint64_t retVal = 0;
		if (index + 8 <= get_data_length())
		{
			if (ByteFormat::LittleEndian == format)
			{
				retVal = data[index];
				retVal |= static_cast<std::uint64_t>(data[index + 1]) << 8;
				retVal |= static_cast<std::uint64_t>(data[index + 2]) << 16;
				retVal |= static_cast<std::uint64_t>(data[index + 3]) << 24;
				retVal |= static_cast<std::uint64_t>(data[index + 4]) << 32;
				retVal |= static_cast<std::uint64_t>(data[index + 5]) << 40;
				retVal |= static_cast<std::uint64_t>(data[index + 6]) << 48;
				retVal |= static_cast<std::uint64_t>(data[index + 7]) << 56;
			}
			else
			{
				retVal = static_cast<std::uint64_t>(data[index]) << 56;
				retVal |= static_cast<std::uint64_t>(data[index + 1]) << 48;
				retVal |= static_cast<std::uint64_t>(data[index + 2]) << 40;
				retVal |= static_cast<std::uint64_t>(data[index + 3]) << 32;
				retVal |= static_cast<std::uint64_t>(data[index + 4]) << 24;
				retVal |= static_cast<std::uint64_t>(data[index + 5]) << 16;
				retVal |= static_cast<std::uint64_t>(data[index + 6]) << 8;
				retVal |= data[index + 7];
			}
		}
		return retVal;
	}
	bool isobus::CANMessage::get_bool_at(const std::size_t byteIndex, const std::uint8_t bitIndex, const std::uint8_t length)
	{
		bool retVal = false;
		if (length <= 8 - bitIndex)
		{
			uint8_t mask = ((1 << length) - 1) << bitIndex;
			retVal = (get_uint8_at(byteIndex) & mask) == mask;
		}
		return retVal;
	}

} // namespace isobus
