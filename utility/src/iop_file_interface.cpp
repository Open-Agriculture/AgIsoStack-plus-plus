//================================================================================================
/// @file iop_file_interface.cpp
///
/// @brief Implementation of class that manages reading IOP files
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/utility/iop_file_interface.hpp"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>

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

	std::string IOPFileInterface::hash_object_pool_to_version(std::vector<std::uint8_t> &iopData)
	{
		std::size_t seed = iopData.size();
		std::stringstream stream;

		for (std::uint32_t x : iopData)
		{
			x = ((x >> 16) ^ x) * 0x45d9f3b;
			x = ((x >> 16) ^ x) * 0x45d9f3b;
			x = (x >> 16) ^ x;
			seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		stream << std::hex << seed;
		return stream.str();
	}
}
