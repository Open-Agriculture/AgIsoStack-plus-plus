//================================================================================================
/// @file can_identifier.hpp
///
/// @brief A representation of a classical CAN identifier with utility functions for ectracting
/// values that are encoded inside, along with some helpful constants.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_IDENTIFIER_HPP
#define CAN_IDENTIFIER_HPP

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
	CANIdentifier(Type identifierType,
		            std::uint32_t pgn,
		            CANPriority priority,
		            std::uint8_t destinationAddress,
		            std::uint8_t sourceAddress);
	CANIdentifier(const CANIdentifier &copiedObject);
	~CANIdentifier();

	CANIdentifier& operator= (const CANIdentifier &obj);

	std::uint32_t get_identifier() const;

	Type get_identifier_type() const;

	std::uint32_t get_parameter_group_number() const;

	CANPriority get_priority() const;

	std::uint8_t get_destination_address() const;

	std::uint8_t get_source_address() const;

	bool get_is_valid() const;

	static constexpr std::uint32_t IDENTIFIER_TYPE_BIT_MASK = 0x80000000;
	static constexpr std::uint32_t UNDEFINED_PARAMETER_GROUP_NUMBER = 0xFFFFFFFF;
	static constexpr std::uint8_t GLOBAL_ADDRESS = 0xFF;
	static constexpr std::uint8_t NULL_ADDRESS = 0xFE;

private:
	static constexpr std::uint32_t BROADCAST_PGN_MASK = 0x0003FFFF;
	static constexpr std::uint32_t DESTINATION_SPECIFIC_PGN_MASK = 0x0003FF00;
	static constexpr std::uint32_t PDU2_FORMAT_MASK = 0x00F00000;
	static constexpr std::uint8_t PARAMTER_GROUP_NUMBER_OFFSET = 8;
	static constexpr std::uint8_t PRIORITY_DATA_BIT_OFFSET = 26;

	std::uint32_t m_RawIdentifier;
};

} // namespace isobus

#endif // CAN_IDENTIFIER_HPP
