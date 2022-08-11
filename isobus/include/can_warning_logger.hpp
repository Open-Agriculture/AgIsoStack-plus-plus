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
    class CANStackLogger
    {
    public:
        CANStackLogger();
        ~CANStackLogger();

        // Gets called from the CAN stack. Wraps LogCANLibWarning
        static void CAN_stack_log(const std::string &warningText);

        static void set_can_stack_logger_sink(CANStackLogger *logSink);

        // Override this to make a log sink for your application
        virtual void LogCANLibWarning(const std::string &warningText);

    private:
        static bool get_can_stack_logger(CANStackLogger *canStackLogger);

        static CANStackLogger *logger;
    };
} // namespace isobus

#endif // CAN_WARNING_LOGGER_HPP
