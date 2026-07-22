//================================================================================================
/// @file isobus_machine_selected_speed_command.hpp
///
/// @brief Parses and encodes machine selected speed command messages.
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_MACHINE_SELECTED_SPEED_COMMAND_HPP
#define ISOBUS_MACHINE_SELECTED_SPEED_COMMAND_HPP

#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/isobus_machine_speed_direction_constants.hpp"

namespace isobus
{
	/// @brief Message that provides the control of the machine speed and direction.
	/// If you receive this message, you can sniff the speed commands being sent to the TECU or act
	/// as the TECU or propulsion interface yourself.
	/// @attention Use extreme caution if you choose to send this message, as you may cause machine motion!
	class MachineSelectedSpeedCommandData
	{
	public:
		/// @brief Constructor for a MachineSelectedSpeedCommandData
		/// @param[in] sender The control function that is sending this message
		explicit MachineSelectedSpeedCommandData(std::shared_ptr<ControlFunction> sender);

		/// @brief Returns the commanded setpoint value of the machine speed as measured by the selected source in mm/s
		/// @return The commanded setpoint value of the machine speed as measured by the selected source in mm/s
		std::uint16_t get_machine_speed_setpoint_command() const;

		/// @brief Sets The commanded setpoint value of the machine speed as measured by the selected source in mm/s
		/// @attention This is used to set the speed of the machine! Use with caution!
		/// @param speed The commanded setpoint value of the machine speed as measured by the selected source in mm/s
		/// @return True if the set value was different from the stored value otherwise false
		bool set_machine_speed_setpoint_command(std::uint16_t speed);

		/// @brief Returns the machine's maximum allowed speed in mm/s, which get's communicated to the tractor/machine
		/// @returns The machine's maximum allowed speed in mm/s, which get's communicated to the tractor/machine
		std::uint16_t get_machine_selected_speed_setpoint_limit() const;

		/// @brief Sets the maximum allowed machine speed in mm/s, which gets communicated to the tractor/machine
		/// @param[in] speedLimit The maximum allowed machine speed in mm/s
		/// @return True if the set value was different from the stored value otherwise false
		bool set_machine_selected_speed_setpoint_limit(std::uint16_t speedLimit);

		/// @brief Returns The commanded direction of the machine.
		/// @return The commanded direction of the machine.
		MachineDirection get_machine_direction_command() const;

		/// @brief Sets the commanded direction of the machine.
		/// @param commandedDirection The commanded direction of travel for the machine
		/// @return True if the set value was different from the stored value otherwise false
		bool set_machine_direction_of_travel(MachineDirection commandedDirection);

		/// @brief Returns a pointer to the sender of the message. If an ICF is the sender, returns the ICF being used to transmit from.
		/// @attention The only way you could get an invalid pointer here is if you register a partner, it sends this message, then you delete the partner and
		/// call this function, as that is the only time the stack deletes a control function. That would be abnormal program flow, but at some point
		/// the stack will be updated to return a shared or weak pointer instead, but for now please be aware of that limitation.
		/// Eventually though the message will time-out normally and you can get a new pointer for
		/// the external CF that replaces the deleted partner.
		/// @returns The control function sending this instance of the guidance system command message
		std::shared_ptr<ControlFunction> get_sender_control_function() const;

		/// @brief Sets the timestamp for when the message was received or sent
		/// @param[in] timestamp The timestamp, in milliseconds, when the message was sent or received
		void set_timestamp_ms(std::uint32_t timestamp);

		/// @brief Returns the timestamp for when the message was received, in milliseconds
		/// @returns The timestamp for when the message was received, in milliseconds
		std::uint32_t get_timestamp_ms() const;

	private:
		std::shared_ptr<ControlFunction> const controlFunction; ///< The CF that is sending the message
		std::uint32_t timestamp_ms = 0; ///< A timestamp for when the message was released in milliseconds
		std::uint16_t speedCommandedSetpoint = 0; ///< Stores the commanded speed setpoint in mm/s
		std::uint16_t speedSetpointLimit = 0; ///< Stores the maximum allowed speed in mm/s
		MachineDirection machineDirectionCommand = MachineDirection::NotAvailable; ///< Stores commanded direction of travel.
	};
} // namespace isobus

#endif // ISOBUS_MACHINE_SELECTED_SPEED_COMMAND_HPP
