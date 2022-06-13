#pragma once

#include <cstdint>

namespace isobus
{

class CANIdentifier
{
public:
	enum CANPriority
	{
		PriorityHighest0 = 0,
		Priority1 = 1,
		Priority2 = 2,
		Priority3 = 3,
		Priority4 = 4,
		Priority5 = 5,
		PriorityDefault6 = 6,
		PriorityLowest7 = 7
	};

	enum Type
	{
		Standard = 0,
		Extended = 1
	};

	explicit CANIdentifier(std::uint32_t rawIdentifierData);
	~CANIdentifier();

	std::uint32_t get_identifier() const;

	Type get_identifier_type() const;

	std::uint32_t get_parameter_group_number() const;

	CANPriority get_priority() const;

	std::uint8_t get_destination_address() const;

	std::uint8_t get_source_address() const;

	bool get_is_valid() const;

	static const std::uint32_t IDENTIFIER_TYPE_BIT_MASK = 0x80000000;
	static const std::uint32_t UNDEFINED_PARAMETER_GROUP_NUMBER = 0xFFFFFFFF;
	static const std::uint8_t GLOBAL_ADDRESS = 0xFF;
	static const std::uint8_t NULL_ADDRESS = 0xFE;

private:
	static const std::uint32_t BROADCAST_PGN_MASK = 0x0003FFFF;
	static const std::uint32_t DESTINATION_SPECIFIC_PGN_MASK = 0x0003FF00;
	static const std::uint32_t PDU2_FORMAT_MASK = 0x00F00000;

	std::uint32_t m_RawIdentifier;
};

} // namespace isobus
