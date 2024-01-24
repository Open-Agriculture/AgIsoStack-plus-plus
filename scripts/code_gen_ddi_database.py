# copyright 2024 The Open-Agriculture Developers
import os, requests
from datetime import datetime

if os.path.exists("isobus_data_dictionary.cpp") and os.path.isfile("isobus_data_dictionary.cpp"):
    os.remove("isobus_data_dictionary.cpp")

if os.path.exists("isobus_data_dictionary.hpp") and os.path.isfile("isobus_data_dictionary.hpp"):
    os.remove("isobus_data_dictionary.hpp")

if os.path.exists("export.txt") and os.path.isfile("export.txt"):
    os.remove("export.txt")
    print("Removed stale export")

print("Downloading DDI database export")
exportURL = "https://www.isobus.net/isobus/exports/completeTXT"
r = requests.get(exportURL, allow_redirects=True)
print(r.headers.get('content-type'))
open("export.txt", 'wb').write(r.content)

numberOfDDIs = 0
with open("export.txt",'r', encoding="utf8") as f:
    processUnit = False
    for l_no, line in enumerate(f):
        if "DD Entity" in line and "Comment:" not in line:
            numberOfDDIs = numberOfDDIs + 1

outputFile = open("isobus_data_dictionary.hpp", 'w', encoding="utf8")
outputFile.write(
f"""//================================================================================================
/// @file isobus_data_dictionary.hpp
///
/// @brief This file contains the definition of an auto-generated lookup of all ISOBUS DDIs
/// as defined in ISO11783-11, exported from isobus.net.
/// This file was generated {datetime.today().strftime("%B %d, %Y")}.
///
/// @author Adrian Del Grosso
/// @copyright {datetime.today().strftime("%Y")} The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_DATA_DICTIONARY_HPP
#define ISOBUS_DATA_DICTIONARY_HPP

#include <cstdint>
#include <string>

namespace isobus
{{
	/// @brief This class contains the definition of an auto-generated lookup of all ISOBUS DDIs
	class DataDictionary
	{{
	public:
		/// @brief A struct containing the information for a single DDI
		struct Entry
		{{
			const std::uint16_t ddi; ///< The DDI number

			const std::string name; ///< The name of the DDI
			const std::string units; ///< The units of the DDI
			const float resolution; ///< The resolution of the DDI
		}};

        /// @brief Checks the ISO 11783-11 database for the given DDI number
        /// and returns the corresponding entry if found.
        /// @param dataDictionaryIdentifier The DDI number to look up
        /// @return The entry for the given DDI number, or a default entry if not found
		static const Entry &get_entry(std::uint16_t dataDictionaryIdentifier);

	private:
		static const Entry DDI_ENTRIES[{numberOfDDIs}]; ///< A lookup table of all DDI entries in ISO11783-11
		static const Entry DEFAULT_ENTRY; ///< A default "unknown" DDI to return if a DDI is not in the database
	}};
}} // namespace isobus

#endif // ISOBUS_DATA_DICTIONARY_HPP


""")
outputFile.close()

outputFile = open("isobus_data_dictionary.cpp", 'w', encoding="utf8")
outputFile.write(
f"""//================================================================================================
/// @file isobus_data_dictionary.cpp
///
/// @brief This file contains an auto-generated lookup table of all ISOBUS DDIs as defined
/// in ISO11783-11, exported from isobus.net.
/// This file was generated {datetime.today().strftime("%B %d, %Y")}.
/// @author Adrian Del Grosso
/// @copyright {datetime.today().strftime("%Y")} The Open-Agriculture Developers
//================================================================================================
// clang-format off
#include "isobus/isobus/isobus_data_dictionary.hpp"

namespace isobus
{{
	const DataDictionary::Entry &DataDictionary::get_entry(std::uint16_t dataDictionaryIdentifier)
	{{
		for (std::uint_fast16_t i = 0; i < sizeof(DDI_ENTRIES) / sizeof(DataDictionary::Entry); i++)
		{{
			if (DDI_ENTRIES[i].ddi == dataDictionaryIdentifier)
			{{
				return DDI_ENTRIES[i];
			}}
		}}
		return DEFAULT_ENTRY;
	}}

    const DataDictionary::Entry DataDictionary::DEFAULT_ENTRY = {{ 65535, "Unknown", "Unknown", 0.0f }};

	const DataDictionary::Entry DataDictionary::DDI_ENTRIES[{numberOfDDIs}] = {{
""")

with open("export.txt",'r', encoding="utf8") as f:
    processUnit = False
    for l_no, line in enumerate(f):
        if "DD Entity" in line and "Comment:" not in line:
            processUnit = True
            entityLine = line.strip().split(' ', 3)
            strippedEntityLine = []

            for sub in entityLine:
                strippedEntityLine.append(sub.replace("\n", ""))
            
            print("Processing entity", line)
            outputFile.write(f"        {{ {strippedEntityLine[2]}, \"{strippedEntityLine[3]}\",")

        if "Unit:" in line and processUnit:
            entityLine = line.split(' ', 1)
            if "n.a. -" in entityLine[1]:
                entityLine[1] = "None"
            if "not defined - not defined" in entityLine[1]:
                entityLine[1] = "None"
            outputFile.write(f"\"{entityLine[1].strip()}\", ")
            processUnit = False
        
        if "Resolution" in line:
            entityLine = line.split(' ', 1)
            entityLine[1] = entityLine[1].replace(",", ".")
            if '1' == entityLine[1].strip():
                entityLine[1] = '1.0'
            if '0' == entityLine[1].strip():
                entityLine[1] = '0.0'
            outputFile.write(f"{entityLine[1].strip()}f }}, \n")


f.close()

outputFile.write(
f"""}};

}} // namespace isobus
// clang-format on

""")

outputFile.close()
