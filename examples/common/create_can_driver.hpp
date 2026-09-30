//================================================================================================
/// @file create_can_driver.hpp
///
/// @brief A class that configures and returns with CAN driver based on the interfaceName
/// and driver parameters (which are usually coming from the saved settings or command line aguments)
/// @author Miklos Marton
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#pragma once

#include "isobus/hardware_integration/can_hardware_plugin.hpp"

#include <memory>
#include <string>

class CANDriverFactory
{
public:
	static std::shared_ptr<isobus::CANHardwarePlugin> create(const std::string &interfaceName = "", const std::string &driver = "");

private:
	static void printAvailableDriverList();
};
