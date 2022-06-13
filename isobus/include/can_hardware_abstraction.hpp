#pragma once

#include "can_frame.hpp"

#include <cstdint>

namespace isobus
{

bool send_can_message_to_hardware(HardwareInterfaceCANFrame frame);

} // namespace isobus