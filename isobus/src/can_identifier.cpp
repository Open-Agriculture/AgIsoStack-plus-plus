//================================================================================================
/// @file can_identifier.cpp
///
/// @brief A representation of a classical CAN identifier with utility functions for ectracting
/// values that are encoded inside, along with some helpful constants.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_identifier.hpp"

namespace isobus
{
	CANIdentifier::CANIdentifier(std::uint32_t rawIdentifierData) :
	  m_RawIdentifier(rawIdentifierData)
	{
	}

	CANIdentifier::CANIdentifier(Type identifierType,
	                             std::uint32_t pgn,
	                             CANPriority priority,
	                             std::uint8_t destinationAddress,
	                             std::uint8_t sourceAddress) :
	  m_RawIdentifier(0)
	{
		if (Type::Extended == identifierType)
		{
			m_RawIdentifier = (static_cast<std::uint32_t>(priority) << PRIORITY_DATA_BIT_OFFSET);

			if (((pgn << PARAMTER_GROUP_NUMBER_OFFSET) & PDU2_FORMAT_MASK) < PDU2_FORMAT_MASK)
			{
				pgn = (pgn & DESTINATION_SPECIFIC_PGN_MASK);
				pgn = (pgn << PARAMTER_GROUP_NUMBER_OFFSET);
				m_RawIdentifier |= pgn;
				m_RawIdentifier |= (static_cast<std::uint32_t>(destinationAddress) << PARAMTER_GROUP_NUMBER_OFFSET);
			}
			else
			{
				m_RawIdentifier |= ((pgn & BROADCAST_PGN_MASK) << PARAMTER_GROUP_NUMBER_OFFSET);
			}
		}
		m_RawIdentifier |= static_cast<std::uint32_t>(sourceAddress);
	}

	CANIdentifier::CANPriority CANIdentifier::get_priority() const
	{
		const std::uint8_t EXTENDED_IDENTIFIER_MASK = 0x07;

		CANPriority retVal;

		if (Type::Extended == get_identifier_type())
		{
			retVal = static_cast<CANPriority>((m_RawIdentifier >> PRIORITY_DATA_BIT_OFFSET) & EXTENDED_IDENTIFIER_MASK);
		}
		else
		{
			retVal = CANPriority::PriorityHighest0;
		}
		return retVal;
	}

	std::uint32_t CANIdentifier::get_identifier() const
	{
		return (m_RawIdentifier & (~IDENTIFIER_TYPE_BIT_MASK));
	}

	CANIdentifier::Type CANIdentifier::get_identifier_type() const
	{
		const std::uint32_t STANDARD_ID_11_BIT_SIZE = 0x7FF;
		Type retVal;

		if (m_RawIdentifier <= STANDARD_ID_11_BIT_SIZE)
		{
			retVal = Type::Standard;
		}
		else
		{
			retVal = Type::Extended;
		}
		return retVal;
	}

	std::uint32_t CANIdentifier::get_parameter_group_number() const
	{
		std::uint32_t retVal = UNDEFINED_PARAMETER_GROUP_NUMBER;

		if (Type::Extended == get_identifier_type())
		{
			if ((PDU2_FORMAT_MASK & m_RawIdentifier) < PDU2_FORMAT_MASK)
			{
				retVal = ((m_RawIdentifier >> PARAMTER_GROUP_NUMBER_OFFSET) & DESTINATION_SPECIFIC_PGN_MASK);
			}
			else
			{
				retVal = ((m_RawIdentifier >> PARAMTER_GROUP_NUMBER_OFFSET) & BROADCAST_PGN_MASK);
			}
		}
		return retVal;
	}

	std::uint8_t CANIdentifier::get_destination_address() const
	{
		const std::uint8_t ADDRESS_BITS_SIZE = 0xFF;
		const std::uint8_t ADDRESS_DATA_OFFSET = 8;

		std::uint8_t retVal = GLOBAL_ADDRESS;

		if ((CANIdentifier::Type::Extended == get_identifier_type()) &&
		    ((PDU2_FORMAT_MASK & m_RawIdentifier) < PDU2_FORMAT_MASK))
		{
			retVal = ((m_RawIdentifier >> ADDRESS_DATA_OFFSET) & ADDRESS_BITS_SIZE);
		}
		return retVal;
	}

	std::uint8_t CANIdentifier::get_source_address() const
	{
		const std::uint8_t ADDRESS_BITS_SIZE = 0xFF;
		std::uint8_t retVal = CANIdentifier::GLOBAL_ADDRESS;

		if (CANIdentifier::Type::Extended == get_identifier_type())
		{
			retVal = (m_RawIdentifier & ADDRESS_BITS_SIZE);
		}
		return retVal;
	}

	bool CANIdentifier::get_is_valid() const
	{
		const std::uint32_t EXTENDED_ID_29_BIT_SIZE = 0x1FFFFFFF;
		const std::uint32_t STANDARD_ID_11_BIT_SIZE = 0x7FF;
		bool retVal;

		if (Type::Extended == get_identifier_type())
		{
			retVal = (m_RawIdentifier <= EXTENDED_ID_29_BIT_SIZE);
		}
		else
		{
			retVal = (m_RawIdentifier <= STANDARD_ID_11_BIT_SIZE);
		}
		return retVal;
	}

} // namespace isobus
