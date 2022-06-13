#pragma once

#include "can_frame.hpp"

void update_CAN_network();
void raw_can_glue(isobus::HardwareInterfaceCANFrame &rawFrame, void *parentPointer);