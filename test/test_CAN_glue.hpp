#pragma once

#include "isobus/isobus/can_frame.hpp"

void update_CAN_network();
void raw_can_glue(isobus::HardwareInterfaceCANFrame &rawFrame, void *parentPointer);