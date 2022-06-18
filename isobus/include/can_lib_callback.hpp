#pragma once

#include "can_message.hpp"

namespace isobus
{
    typedef void (*CANLibCallback)(CANMessage *message, void *parentPointer);
} // namespace isobus
