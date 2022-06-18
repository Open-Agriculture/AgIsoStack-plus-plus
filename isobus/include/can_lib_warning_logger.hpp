#pragma once

#include <string>

namespace isobus
{
    class CANLibWarningLogger
    {
    public:
        static void LogCANLibWarning(std::string warningText);
    };
} // namespace isobus
