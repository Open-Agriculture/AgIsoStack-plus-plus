//================================================================================================
/// @file can_identifier.hpp
///
/// @brief A representation of a classical CAN identifier with utility functions for ectracting
/// values that are encoded inside, along with some helpful constants.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#ifndef CAN_IDENTIFIER_HPP
#define CAN_IDENTIFIER_HPP

#include <cstdint>

namespace isobus
{
	//================================================================================================
	/// @class CANIdentifier
	///
	/// @brief A utility class that allows easy interpretation of a 32 bit CAN identifier
	//================================================================================================
	class CANIdentifier
	{
	public:
		/// @brief Defines all the CAN frame priorities that can be encoded in a frame ID
		enum class CANPriority
		{
			PriorityHighest0 = 0, ///< Highest CAN priority
			Priority1 = 1, ///< Priority highest - 1
			Priority2 = 2, ///< Priority highest - 2
			Priority3 = 3, ///< Priority highest - 3 (Control messages priority)
			Priority4 = 4, ///< Priority highest - 4
			Priority5 = 5, ///< Priority highest - 5
			PriorityDefault6 = 6, ///< The default priority
			PriorityLowest7 = 7 ///< The lowest priority
		};

		/// @brief Defines if a frame is a standard (11 bit) or extended (29 bit) ID frame
		enum class Type
		{
			Standard = 0, ///< Frame is an 11bit ID standard (legacy) message with no PGN and highest priority
			Extended = 1 ///< Frame is a modern 29 bit ID CAN frame
		};

		/// @brief Constructor for a CAN Identifier class based on a raw 32 bit ID
		/// @param[in] rawIdentifierData The raw 32 bit ID to interpret
		explicit CANIdentifier(std::uint32_t rawIdentifierData);

		/// @brief Constructor for a CAN Identifier class based on all discrete components
		/// @param[in] identifierType Type of frame, either standard 11 bit ID, or extended 29 bit ID
		/// @param[in] pgn The parameter group number encoded in the frame (extended only)
		/// @param[in] priority The priority of the frame (extended only)
		/// @param[in] destinationAddress The destination address of the frame
		/// @param[in] sourceAddress The source address of the frame
		CANIdentifier(Type identifierType,
		              std::uint32_t pgn,
		              CANPriority priority,
		              std::uint8_t destinationAddress,
		              std::uint8_t sourceAddress);

		/// @brief Destructor for the CANIdentifier
		~CANIdentifier() = default;

		/// @brief Returns the raw encoded ID of the CAN identifier
		/// @returns The raw encoded ID of the CAN identifier
		std::uint32_t get_identifier() const;

		/// @brief Returns the identifier type (standard vs extended)
		/// @returns The identifier type (standard vs extended)
		Type get_identifier_type() const;

		/// @brief Returns the PGN encoded in the identifier
		/// @returns The PGN encoded in the identifier
		std::uint32_t get_parameter_group_number() const;

		/// @brief Returns the priority of the frame encoded in the identifier
		/// @returns The priority of the frame encoded in the identifier
		CANPriority get_priority() const;

		/// @brief Returns the destination address of the frame encoded in the identifier
		/// @returns The destination address of the frame encoded in the identifier
		std::uint8_t get_destination_address() const;

		/// @brief Returns the source address of the frame encoded in the identifier
		/// @returns The source address of the frame encoded in the identifier
		std::uint8_t get_source_address() const;

		/// @brief Returns if the ID is valid based on some range checking
		/// @returns Frame valid status
		bool get_is_valid() const;

		static constexpr std::uint32_t IDENTIFIER_TYPE_BIT_MASK = 0x80000000; ///< This bit denotes if the frame is standard or extended format
		static constexpr std::uint32_t UNDEFINED_PARAMETER_GROUP_NUMBER = 0xFFFFFFFF; ///< A fake PGN used internally to denote a NULL PGN
		static constexpr std::uint8_t GLOBAL_ADDRESS = 0xFF; ///< The broadcast CAN address
		static constexpr std::uint8_t NULL_ADDRESS = 0xFE; ///< The NULL CAN address as defined by ISO11783

	private:
		static constexpr std::uint32_t BROADCAST_PGN_MASK = 0x0003FFFF; ///< Broadcast PGNs don't mask off the bits used for destination in the PGN
		static constexpr std::uint32_t DESTINATION_SPECIFIC_PGN_MASK = 0x0003FF00; ///< Destination specific PGNs mask the destination out of the PGN itself
		static constexpr std::uint32_t PDU2_FORMAT_MASK = 0x00F00000; ///< Mask that denotes the ID as being PDU2 format
		static constexpr std::uint8_t PARAMETER_GROUP_NUMBER_OFFSET = 8; ///< PGN is offset 8 bits into the ID
		static constexpr std::uint8_t PRIORITY_DATA_BIT_OFFSET = 26; ///< Priority is offset 26 bits into the ID

		std::uint32_t m_RawIdentifier; ///< The raw encoded 29 bit ID
	};

} // namespace isobus

#endif // CAN_IDENTIFIER_HPP
