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
    class CANLibWarningLogger
    {
    public:
        static void LogCANLibWarning(std::string warningText);
    };
} // namespace isobus

#endif // CAN_WARNING_LOGGER_HPP
