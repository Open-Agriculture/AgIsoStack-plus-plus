//================================================================================================
/// @file isobus_task_controller_server_options.hpp
///
/// @brief Defines a helper class to assign TC server options.
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_TASK_CONTROLLER_SERVER_OPTIONS_HPP
#define ISOBUS_TASK_CONTROLLER_SERVER_OPTIONS_HPP

#include <cstdint>

namespace isobus
{
	/// @brief A helper class to assign TC server options.
	/// You can use this by doing something like this:
	/// TaskControllerServer::TaskControllerOptions().with_documentation().with_tc_geo_with_position_based_control();
	/// Or you can set the settings yourself, manually.
	class TaskControllerOptions
	{
	public:
		/// @brief Constructor for a TC server options helper class.
		TaskControllerOptions() = default;

		/// @brief Copy constructor for a TC server options helper class.
		TaskControllerOptions(const TaskControllerOptions &) = default;

		/// @brief Assignment operator for a TC server options helper class.
		/// @returns A reference to the current object.
		TaskControllerOptions &operator=(const TaskControllerOptions &) = default;

		/// @brief Alters the settings object to indicate you want to support documentation.
		/// @param[in] supported Whether or not the TC supports documentation.
		/// @returns An updated settings object.
		TaskControllerOptions with_documentation(bool supported = true) const;

		/// @brief Alters the settings object to indicate you want to support tc-geo without position based control.
		/// @param[in] supported Whether or not the TC supports tc-geo without position based control.
		/// @returns An updated settings object.
		TaskControllerOptions with_tc_geo_without_position_based_control(bool supported = true) const;

		/// @brief Alters the settings object to indicate you want to support tc-geo with position based control.
		/// @param[in] supported Whether or not the TC supports tc-geo with position based control.
		/// @returns An updated settings object.
		TaskControllerOptions with_tc_geo_with_position_based_control(bool supported = true) const;

		/// @brief Alters the settings object to indicate you want to support peer control assignment.
		/// @param[in] supported Whether or not the TC supports peer control assignment.
		/// @returns An updated settings object.
		TaskControllerOptions with_peer_control_assignment(bool supported = true) const;

		/// @brief Alters the settings object to indicate you want to support implement section control.
		/// @param[in] supported Whether or not the TC supports implement section control.
		/// @returns An updated settings object.
		TaskControllerOptions with_implement_section_control(bool supported = true) const;

		/// @brief Sets the settings object to the provided options bitfield.
		/// @param[in] supportsDocumentation Whether or not the TC supports documentation.
		/// @param[in] supportsTCGEOWithoutPositionBasedControl Whether or not the TC supports tc-geo without position based control.
		/// @param[in] supportsTCGEOWithPositionBasedControl Whether or not the TC supports tc-geo with position based control.
		/// @param[in] supportsPeerControlAssignment Whether or not the TC supports peer control assignment.
		/// @param[in] supportsImplementSectionControl Whether or not the TC supports implement section control.
		void set_settings(
		  bool supportsDocumentation,
		  bool supportsTCGEOWithoutPositionBasedControl,
		  bool supportsTCGEOWithPositionBasedControl,
		  bool supportsPeerControlAssignment,
		  bool supportsImplementSectionControl);

		/// @brief Gets the options bitfield that was set by the user.
		/// @returns The options bitfield that was set by the user.
		std::uint8_t get_bitfield() const;

	private:
		bool optionDocumentation = false; ///< Bit 0, defines whether or not the TC supports documentation.
		bool optionTCGEOWithoutPositionBasedControl = false; ///< Bit 1, defines whether or not the TC supports tc-geo without position based control.
		bool optionTCGEOWithPositionBasedControl = false; ///< Bit 2, defines whether or not the TC supports tc-geo with position based control.
		bool optionPeerControlAssignment = false; ///< Bit 3, defines whether or not the TC supports peer control assignment.
		bool optionImplementSectionControl = false; ///< Bit 4, defines whether or not the TC supports implement section control.
	};
} // namespace isobus
#endif // ISOBUS_TASK_CONTROLLER_SERVER_OPTIONS_HPP
