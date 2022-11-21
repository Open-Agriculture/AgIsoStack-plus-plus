//================================================================================================
/// @file iop_file_interface.hpp
///
/// @brief A class that manages reading IOP files
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef IOP_FILE_INTERFACE_HPP
#define IOP_FILE_INTERFACE_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace isobus
{
	//================================================================================================
	/// @class IOPFileInterface
	///
	/// @brief A class that manages reading IOP files
	/// @details IOP files are the standard format for ISOBUS object pools. This class will read
	/// an IOP file, and return a vector that represents the object pool stored in the file.
	//================================================================================================
	class IOPFileInterface
	{
	public:
		/// @brief Reads an IOP file given a file name/path
		/// @param[in] filename A string filepath for the IOP file to read
		/// @returns A vector with an object pool in it, or an empty vector if reading failed
		static std::vector<std::uint8_t> read_iop_file(const std::string &filename);
	};
}

#endif // IOP_FILE_INTERFACE_HPP
