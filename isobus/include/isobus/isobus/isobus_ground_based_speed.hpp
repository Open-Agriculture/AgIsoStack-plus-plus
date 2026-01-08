//================================================================================================
/// @file isobus_ground_based_speed.hpp
///
/// @brief Parses and sends ground-based speed messages.
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_GROUND_BASED_SPEED_HPP
#define ISOBUS_GROUND_BASED_SPEED_HPP

#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/isobus_machine_speed_direction_constants.hpp"

namespace isobus
{
	/// @brief Message normally sent by the Tractor ECU on the implement bus on construction and
	/// agricultural implements providing to connected systems the current measured ground speed
	/// (also includes a free-running distance counter and an indication of the direction of travel).
	///
	/// @note Accuracies of both wheel-based and ground-based sources can be speed-dependent and degrade at low speeds.
	/// Wheel-based information might not be updated at the 100ms rate at low speeds.
	class GroundBasedSpeedData
	{
	public:
		/// @brief Constructor for a GroundBasedSpeedData
		/// @param[in] sender The control function that is sending this message
		explicit GroundBasedSpeedData(std::shared_ptr<ControlFunction> sender);

		/// @brief Actual distance traveled by a machine, based on measurements from a sensor such as that is not susceptible to wheel slip
		/// (e.g. radar, GPS, LIDAR, or stationary object tracking)
		/// @note This distance is usually provided by radar.
		/// @return Actual distance traveled by a machine, based on measurements from a sensor such as that is not susceptible to wheel slip (millimeters)
		std::uint32_t get_machine_distance() const;

		/// @brief Sets the actual distance traveled by a machine, based on measurements from a sensor such as that is not susceptible to wheel slip.
		/// @note This distance is usually provided by radar.
		/// @param distance The actual distance traveled by a machine (millimeters)
		/// @return True if the set value was different from the stored value otherwise false
		bool set_machine_distance(std::uint32_t distance);

		/// @brief Returns the actual ground speed of a machine, measured by a sensor such as that is not susceptible to wheel slip in mm/s
		/// @note This speed is usually provided by radar.
		/// @return The actual ground speed of a machine, measured by a sensor such as that is not susceptible to wheel slip in mm/s
		std::uint16_t get_machine_speed() const;

		/// @brief Sets the actual ground speed of a machine, measured by a sensor such as that is not susceptible to wheel slip in mm/s
		/// @note This speed is usually provided by radar.
		/// @param speed The actual ground speed of a machine, measured by a sensor such as that is not susceptible to wheel slip in mm/s
		/// @return True if the set value was different from the stored value otherwise false
		bool set_machine_speed(std::uint16_t speed);

		/// @brief Returns A measured signal indicating either forward or reverse as the direction of travel.
		/// @note When the speed is zero, this indicates the last travel direction until a different
		/// direction is detected or selected and engaged.
		/// @return The measured direction of travel for the machine
		MachineDirection get_machine_direction_of_travel() const;

		/// @brief Sets a measured signal indicating either forward or reverse as the direction of travel.
		/// @note The "Not Off" key switch state does not always mean "On" so use care when using it.
		/// @param directionOfTravel The measured direction of travel for the machine
		/// @return True if the set value was different from the stored value otherwise false
		bool set_machine_direction_of_travel(MachineDirection directionOfTravel);

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
		std::uint32_t groundBasedMachineDistance_mm = 0; ///< Stores the ground-based speed's distance in millimeters
		std::uint16_t groundBasedMachineSpeed_mm_per_sec = 0; ///< Stores the ground-based speed in mm/s
		MachineDirection machineDirectionState = MachineDirection::NotAvailable; ///< Stores direction of travel.
	};
} // namespace isobus

#endif // ISOBUS_GROUND_BASED_SPEED_HPP
