//================================================================================================
/// @file isobus_task_controller_server_options.cpp
///
/// @brief Implements a helper class to assign TC server options.
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_task_controller_server_options.hpp"

namespace isobus
{
	TaskControllerOptions TaskControllerOptions::with_documentation(bool supported) const
	{
		TaskControllerOptions copy = *this;
		copy.optionDocumentation = supported;
		return copy;
	}

	TaskControllerOptions TaskControllerOptions::with_tc_geo_without_position_based_control(bool supported) const
	{
		TaskControllerOptions copy = *this;
		copy.optionTCGEOWithoutPositionBasedControl = supported;
		return copy;
	}

	TaskControllerOptions TaskControllerOptions::with_tc_geo_with_position_based_control(bool supported) const
	{
		TaskControllerOptions copy = *this;
		copy.optionTCGEOWithPositionBasedControl = supported;
		return copy;
	}

	TaskControllerOptions TaskControllerOptions::with_peer_control_assignment(bool supported) const
	{
		TaskControllerOptions copy = *this;
		copy.optionPeerControlAssignment = supported;
		return copy;
	}

	TaskControllerOptions TaskControllerOptions::with_implement_section_control(bool supported) const
	{
		TaskControllerOptions copy = *this;
		copy.optionImplementSectionControl = supported;
		return copy;
	}

	void TaskControllerOptions::set_settings(
	  bool supportsDocumentation,
	  bool supportsTCGEOWithoutPositionBasedControl,
	  bool supportsTCGEOWithPositionBasedControl,
	  bool supportsPeerControlAssignment,
	  bool supportsImplementSectionControl)
	{
		optionDocumentation = supportsDocumentation;
		optionTCGEOWithoutPositionBasedControl = supportsTCGEOWithoutPositionBasedControl;
		optionTCGEOWithPositionBasedControl = supportsTCGEOWithPositionBasedControl;
		optionPeerControlAssignment = supportsPeerControlAssignment;
		optionImplementSectionControl = supportsImplementSectionControl;
	}

	std::uint8_t TaskControllerOptions::get_bitfield() const
	{
		return static_cast<std::uint8_t>(static_cast<std::uint8_t>(optionDocumentation) |
		                                 (static_cast<std::uint8_t>(optionTCGEOWithoutPositionBasedControl) << 1) |
		                                 (static_cast<std::uint8_t>(optionTCGEOWithPositionBasedControl) << 2) |
		                                 (static_cast<std::uint8_t>(optionPeerControlAssignment) << 3) |
		                                 (static_cast<std::uint8_t>(optionImplementSectionControl) << 4));
	}
} // namespace isobus
