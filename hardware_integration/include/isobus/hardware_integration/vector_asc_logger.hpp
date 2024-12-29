//================================================================================================
/// @file vector_asc_logger.hpp
///
/// @brief Defines a CAN logger that saves messages in a Vector .asc file using a hook in the
/// hardware interface.
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef VECTOR_ASC_LOGGER_HPP
#define VECTOR_ASC_LOGGER_HPP

#include "isobus/hardware_integration/can_hardware_interface.hpp"

#include <ctime>
#include <fstream>

namespace isobus
{
	/// @brief Logs to Vector .asc file
	class VectorASCLogger
	{
	public:
		/// @brief Constructor for a logger, which uses a default file name
		VectorASCLogger();

		/// @brief Constructor for a logger, which uses a user provided file name
		/// @param filename The name/path of the file to log to
		explicit VectorASCLogger(std::string filename);

		/// @brief Destructor for a logger
		~VectorASCLogger();

		/// @brief Deleted copy constructor
		VectorASCLogger(VectorASCLogger &) = delete;

	private:
		/// @brief Opens the target log file, and passes it the current time information
		/// @param[in] filePath The path to the file to open
		/// @param[in] headerTime A string with a properly formatted .asc file date header in it
		void openFile(const std::string &filePath, const std::string &headerTime);

		/// @brief Builds a vector ascii log file date header
		/// @param[in] currentTime The current time
		/// @returns A string with a properly formatted .asc file date header in it
		std::string constructHeaderTime(const std::time_t &currentTime);

		std::fstream logFileStream; ///< The file to log to
		isobus::EventCallbackHandle canFrameReceivedListener = 0; ///< A listener for received frames
		isobus::EventCallbackHandle canFrameSentListener = 0; ///< A listener for sent frames
		std::uint32_t initialTimestamp; ///< The initial timestamp of the logger
	};
}

#endif // VECTOR_ASC_LOGGER_HPP
