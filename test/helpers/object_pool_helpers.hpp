//================================================================================================
/// @file object_pool_helpers.hpp
///
/// @brief Gives unit tests access to the object pools stored in the repository.
/// @author Open-Agriculture
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef OBJECT_POOL_HELPERS_HPP
#define OBJECT_POOL_HELPERS_HPP

#include "isobus/utility/iop_file_interface.hpp"

#include <cstdint>
#include <vector>

namespace test_helpers
{
	/// @brief Reads the version 3 example object pool, VT3TestPool.iop
	/// @details The path is set by CMake, so this works from any working directory and without building the examples
	/// @returns The object pool, or an empty vector if it could not be read
	inline std::vector<std::uint8_t> read_vt3_test_pool()
	{
		return isobus::IOPFileInterface::read_iop_file(VT3_TEST_POOL_PATH);
	}
} // namespace test_helpers

#endif // OBJECT_POOL_HELPERS_HPP
