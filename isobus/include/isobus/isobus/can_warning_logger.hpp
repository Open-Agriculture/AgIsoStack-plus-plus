//================================================================================================
/// @file can_warning_logger.hpp
///
/// @brief A class that acts as a logging sink. The intent is that someone could make their own
/// derived class of logger and inject it into the CAN stack to get helpful debug logging.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef CAN_WARNING_LOGGER_HPP
#define CAN_WARNING_LOGGER_HPP

#include <string>

namespace isobus
{
	//================================================================================================
	/// @class CANStackLogger
	///
	/// @brief A base class for a CAN logger, used to get diagnostic info from the CAN stack
	/// @details The CAN stack prints helpful text that may inform you of issues in either the stack
	/// or your application. You can override a function in this class to begin consuming this
	/// logging text.
	//================================================================================================
	class CANStackLogger
	{
	public:
		/// @brief The constructor for a CANStackLogger
		CANStackLogger();

		/// @brief The destructor for a CANStackLogger
		~CANStackLogger();

		/// @brief Gets called from the CAN stack to log information. Wraps LogCANLibWarning.
		/// @param[in] warningText The text to be logged
		static void CAN_stack_log(const std::string &warningText);

		/// @brief Assigns a derived logger class to be used as the log sink
		/// @param[in] logSink A pointer to a derived CANStackLogger class
		static void set_can_stack_logger_sink(CANStackLogger *logSink);

		/// @brief Override this to make a log sink for your application
		/// @param[in] warningText The information being logged
		virtual void LogCANLibWarning(const std::string &warningText);

	private:
		/// @brief Provides a pointer to the static instance of the logger, and returns if the pointer is valid
		/// @param[out] canStackLogger The static logger instance
		/// @returns true if the logger is not `nullptr` or false if it is `nullptr`
		static bool get_can_stack_logger(CANStackLogger *&canStackLogger);

		static CANStackLogger *logger; ///< A static pointer to an instance of a logger
	};
} // namespace isobus

#endif // CAN_WARNING_LOGGER_HPP
