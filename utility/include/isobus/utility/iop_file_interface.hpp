//================================================================================================
/// @file iop_file_interface.hpp
///
/// @brief A class that manages reading IOP files
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
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
	/// @brief A class that manages reading IOP files and provides a simple way to generate
	/// unique versions.
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

		/// @brief Reads an object pool and generates a string version by hashing it
		/// @details Credit for the hash algorithm here goes to "see" on stack overflow.
		/// @param[in] iopData The object pool to hash and generate a version for
		/// @returns A 7 character string that is probably somewhat unique for this pool
		static std::string hash_object_pool_to_version(std::vector<std::uint8_t> &iopData);

		/// @brief Reads an object pool raw data and generates a string version by hashing it
		/// @details This function operates directly on a pointer to the object pool data
		///          and its length. Useful for zero-copy hashing of large object pools.
		/// @param[in] iopDataPtr Pointer to the object pool data
		/// @param[in] length Number of bytes in the object pool
		/// @returns A 7 character string that is probably somewhat unique for this pool
		static std::string hash_object_pool_to_version(const std::uint8_t *iopDataPtr, std::size_t length);
	};
}

#endif // IOP_FILE_INTERFACE_HPP
