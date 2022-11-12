//================================================================================================
/// @file can_hardware_plugin.hpp
///
/// @brief A base class for a CAN driver. Can be derived into your platform's required interface.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef CAN_HARDEWARE_PLUGIN_HPP
#define CAN_HARDEWARE_PLUGIN_HPP

#include "can_frame.hpp"

class CANHardwarePlugin
{
public:
	virtual bool get_is_valid() const = 0;
	virtual void close() = 0;
	virtual void open() = 0;
	virtual bool read_frame(isobus::HardwareInterfaceCANFrame &canFrame) = 0;
	virtual bool write_frame(const isobus::HardwareInterfaceCANFrame &canFrame) = 0;
};

#endif // CAN_HARDEWARE_PLUGIN_HPP
