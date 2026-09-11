#pragma once

#include "isobus/hardware_integration/can_hardware_plugin.hpp"

#include <memory>
#include <string>

class CANDriverFactory
{
public:
	static std::shared_ptr<isobus::CANHardwarePlugin> create(const std::string &interfaceName = "");
};
