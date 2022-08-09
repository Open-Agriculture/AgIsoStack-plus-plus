//================================================================================================
/// @file can_hardware_abstraction.hpp
///
/// @brief An abstraction between this CAN stack and any hardware layer
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_HARDWARE_ABSTRACTION
#define CAN_HARDWARE_ABSTRACTION

#include "can_frame.hpp"

#include <cstdint>

namespace isobus
{

	bool send_can_message_to_hardware(HardwareInterfaceCANFrame frame);

} // namespace isobus

#endif // CAN_HARDWARE_ABSTRACTION