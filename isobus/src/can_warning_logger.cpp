//================================================================================================
/// @file can_warning_logger.cpp
///
/// @brief A class that acts as a logging sink. The intent is that someone could make their own
/// derived class of logger and inject it into the CAN stack to get helpful debug logging.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "can_warning_logger.hpp"

namespace isobus
{
    void CANLibWarningLogger::LogCANLibWarning(std::string)
    {
        // TODO replace with logging callbacks
    }
} // namespace isobus
