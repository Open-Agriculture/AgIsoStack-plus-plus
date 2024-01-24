//================================================================================================
/// @file isobus_data_dictionary.hpp
///
/// @brief This file contains the definition of an auto-generated lookup of all ISOBUS DDIs
/// as defined in ISO11783-11, exported from isobus.net.
/// This file was generated January 23, 2024.
///
/// @author Adrian Del Grosso
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_DATA_DICTIONARY_HPP
#define ISOBUS_DATA_DICTIONARY_HPP

#include <cstdint>
#include <string>

namespace isobus
{
	/// @brief This class contains the definition of an auto-generated lookup of all ISOBUS DDIs
	class DataDictionary
	{
	public:
		/// @brief A struct containing the information for a single DDI
		struct Entry
		{
			const std::uint16_t ddi; ///< The DDI number

			const std::string name; ///< The name of the DDI
			const std::string units; ///< The units of the DDI
			const float resolution; ///< The resolution of the DDI
		};

		/// @brief Checks the ISO 11783-11 database for the given DDI number
		/// and returns the corresponding entry if found.
		/// @param dataDictionaryIdentifier The DDI number to look up
		/// @return The entry for the given DDI number, or a default entry if not found
		static const Entry &get_entry(std::uint16_t dataDictionaryIdentifier);

	private:
		static const Entry DDI_ENTRIES[715]; ///< A lookup table of all DDI entries in ISO11783-11
		static const Entry DEFAULT_ENTRY; ///< A default "unknown" DDI to return if a DDI is not in the database
	};
} // namespace isobus

#endif // ISOBUS_DATA_DICTIONARY_HPP
