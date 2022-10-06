//================================================================================================
/// @file iop_file_interface.cpp
///
/// @brief Implementation of class that manages reading IOP files
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "iop_file_interface.hpp"

#include <fstream>
#include <iterator>

namespace isobus
{
	std::vector<std::uint8_t> IOPFileInterface::read_iop_file(const std::string &filename)
	{
		std::vector<std::uint8_t> retVal;
		std::streampos fileSize;

		std::ifstream file(filename, std::ios::binary);

		if (file.is_open())
		{
			// Ignore newlines
			file.unsetf(std::ios::skipws);

			file.seekg(0, std::ios::end);
			fileSize = file.tellg();
			file.seekg(0, std::ios::beg);

			retVal.reserve(fileSize);

			// Read in the data
			retVal.insert(retVal.begin(), std::istream_iterator<std::uint8_t>(file), std::istream_iterator<std::uint8_t>());
		}
		return retVal;
	}
}
