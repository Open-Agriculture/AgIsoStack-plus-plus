//================================================================================================
/// @file can_stack_logger.hpp
///
/// @brief A class that acts as a logging sink. The intent is that someone could make their own
/// derived class of logger and inject it into the CAN stack to get helpful debug logging.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef CAN_STACK_LOGGER_HPP
#define CAN_STACK_LOGGER_HPP

#include <mutex>
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
		/// @brief Enumerates the various log message severities
		enum class LoggingLevel
		{
			Debug = 0, ///< Verbose information
			Info, ///< General status info messages and messages about how things are working under normal conditions
			Warning, ///< Warnings indicate issues that do not stop normal operation, but should be noted for troubleshooting
			Error, ///< Errors are issues that interrupt normal operation
			Critical ///< Critical issues are fundamental problems that must be solved for the stack to work properly
		};

		/// @brief The constructor for a CANStackLogger
		CANStackLogger();

		/// @brief The destructor for a CANStackLogger
		~CANStackLogger();

		/// @brief Gets called from the CAN stack to log information. Wraps sink_CAN_stack_log.
		/// @param[in] level The log level for this text
		/// @param[in] logText The text to be logged
		static void CAN_stack_log(LoggingLevel level, const std::string &logText);

		/// @brief Logs a string to the log sink with `Debug` severity. Wraps sink_CAN_stack_log.
		/// @param[in] logText The text to be logged at `Debug` severity
		static void debug(const std::string &logText);

		/// @brief Logs a string to the log sink with `Info` severity. Wraps sink_CAN_stack_log.
		/// @param[in] logText The text to be logged at `Info` severity
		static void info(const std::string &logText);

		/// @brief Logs a string to the log sink with `Warning` severity. Wraps sink_CAN_stack_log.
		/// @param[in] logText The text to be logged at `Warning` severity
		static void warn(const std::string &logText);

		/// @brief Logs a string to the log sink with `Error` severity. Wraps sink_CAN_stack_log.
		/// @param[in] logText The text to be logged at `Error` severity
		static void error(const std::string &logText);

		/// @brief Logs a string to the log sink with `Critical` severity. Wraps sink_CAN_stack_log.
		/// @param[in] logText The text to be logged at `Critical` severity
		static void critical(const std::string &logText);

		/// @brief Assigns a derived logger class to be used as the log sink
		/// @param[in] logSink A pointer to a derived CANStackLogger class
		static void set_can_stack_logger_sink(CANStackLogger *logSink);

		/// @brief Returns the current logging level
		/// @details Log statements below the current level will be dropped
		/// and not passed to the log sink
		/// @returns The current log level
		static LoggingLevel get_log_level();

		/// @brief Sets the current logging level
		/// @details Log statements below the new level will be dropped
		/// and not passed to the log sink
		/// @param[in] newLogLevel The new logging level
		static void set_log_level(LoggingLevel newLogLevel);

		/// @brief Override this to make a log sink for your application
		/// @param[in] level The severity level of the log text
		/// @param[in] logText The information being logged
		virtual void sink_CAN_stack_log(LoggingLevel level, const std::string &logText);

	private:
		/// @brief Provides a pointer to the static instance of the logger, and returns if the pointer is valid
		/// @param[out] canStackLogger The static logger instance
		/// @returns true if the logger is not `nullptr` or false if it is `nullptr`
		static bool get_can_stack_logger(CANStackLogger *&canStackLogger);

		static CANStackLogger *logger; ///< A static pointer to an instance of a logger
		static LoggingLevel currentLogLevel; ///< The current log level. Logs for levels below the current one will be dropped.
		static std::mutex loggerMutex; ///< A mutex that protects the logger so it can be used from multiple threads
	};
} // namespace isobus

#endif // CAN_STACK_LOGGER_HPP
