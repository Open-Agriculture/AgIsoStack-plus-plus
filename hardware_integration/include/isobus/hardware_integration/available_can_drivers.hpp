//================================================================================================
/// @file available_can_drivers.hpp
///
/// @brief A utility to include all of the available CAN drivers. Should mainly be
/// used for testing purposes.
/// @author Daan Steenbergen
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef AVAILABLE_CAN_DRIVERS_HPP
#define AVAILABLE_CAN_DRIVERS_HPP

#ifdef ISOBUS_SOCKETCAN_AVAILABLE
#include "isobus/hardware_integration/socket_can_interface.hpp"
#endif

#ifdef ISOBUS_WINDOWSPCANBASIC_AVAILABLE
#include "isobus/hardware_integration/pcan_basic_windows_plugin.hpp"
#endif

#ifdef ISOBUS_VIRTUALCAN_AVAILABLE
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#endif

#ifdef ISOBUS_TWAI_AVAILABLE
#include "isobus/hardware_integration/twai_plugin.hpp"
#endif

#ifdef ISOBUS_MCP2515_AVAILABLE
#include "isobus/hardware_integration/mcp2515_can_interface.hpp"
#endif

#ifdef ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE
#include "isobus/hardware_integration/innomaker_usb2can_windows_plugin.hpp"
#endif

#endif // AVAILABLE_CAN_DRIVERS_HPP