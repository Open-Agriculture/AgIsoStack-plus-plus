//================================================================================================
/// @file isobus_wheel_based_speed.hpp
///
/// @brief Parses and sends wheel-based speed messages.
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_WHEEL_BASED_SPEED_HPP
#define ISOBUS_WHEEL_BASED_SPEED_HPP

#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/isobus_machine_speed_direction_constants.hpp"

namespace isobus
{

	/// @brief Groups the data encoded in an ISO "Wheel-based Speed and Distance" message
	class WheelBasedMachineSpeedData
	{
	public:
		/// @brief Enumerates the key switch states of the tractor or power unit.
		enum class KeySwitchState : std::uint8_t
		{
			Off = 0, ///< Key is off
			NotOff = 1, ///< Key is not off (does not always mean that it's on!)
			Error = 2,
			NotAvailable = 3
		};

		/// @brief Enumerates the states of a switch or operator input to start or enable implement operations
		enum class ImplementStartStopOperations : std::uint8_t
		{
			StopDisableImplementOperations = 0,
			StartEnableImplementOperations = 1,
			Error = 2,
			NotAvailable = 3
		};

		/// @brief This parameter indicates whether the reported direction is reversed from the perspective of the operator
		enum class OperatorDirectionReversed : std::uint8_t
		{
			NotReversed = 0,
			Reversed = 1,
			Error = 2,
			NotAvailable = 3
		};

		/// @brief Constructor for a WheelBasedMachineSpeedData
		/// @param[in] sender The control function that is sending this message
		explicit WheelBasedMachineSpeedData(std::shared_ptr<ControlFunction> sender);

		/// @brief Returns The distance traveled by a machine as calculated from wheel or tail-shaft speed.
		/// @note When the distance exceeds 4211081215m the value shall be reset to zero and incremented as additional distance accrues.
		/// @returns The distance traveled by a machine as calculated from wheel or tail-shaft speed. (millimeters)
		std::uint32_t get_machine_distance() const;

		/// @brief Sets the distance traveled by a machine as calculated from wheel or tail-shaft speed.
		/// @param distance The distance traveled by a machine as calculated from wheel or tail-shaft speed. (millimeters)
		/// @return True if the set value was different from the stored value otherwise false
		bool set_machine_distance(std::uint32_t distance);

		/// @brief Returns the value of the speed of a machine as calculated from the measured wheel or tail-shaft speed.
		/// @returns The value of the speed of a machine as calculated from the measured wheel or tail-shaft speed.
		std::uint16_t get_machine_speed() const;

		/// @brief Sets the value of the speed of a machine as calculated from the measured wheel or tail-shaft speed.
		/// @param speed The value of the speed of a machine as calculated from the measured wheel or tail-shaft speed.
		/// @return True if the set value was different from the stored value otherwise false
		bool set_machine_speed(std::uint16_t speed);

		/// @brief Returns the maximum time (in minutes) of remaining tractor or power-unit-supplied electrical power at the current load.
		/// @return The maximum time of remaining tractor or power-unit-supplied electrical power at the current load (in minutes)
		std::uint8_t get_maximum_time_of_tractor_power() const;

		/// @brief Sets the maximum time (in minutes) of remaining tractor or power-unit-supplied electrical power at the current load.
		/// @param maxTime The maximum time of remaining tractor or power-unit-supplied electrical power at the current load (in minutes)
		/// @return True if the set value was different from the stored value otherwise false
		bool set_maximum_time_of_tractor_power(std::uint8_t maxTime);

		/// @brief Returns A measured signal indicating either forward or reverse as the direction of travel.
		/// @note When the speed is zero, this indicates the last travel direction until a different
		/// direction is detected or selected and engaged.
		/// @return The measured direction of travel for the machine
		MachineDirection get_machine_direction_of_travel() const;

		/// @brief Sets a measured signal indicating either forward or reverse as the direction of travel.
		/// @note The "Not Off" key switch state does not always mean "On" so use care when using it.
		/// @param direction The measured direction of travel for the machine
		/// @return True if the set value was different from the stored value otherwise false
		bool set_machine_direction_of_travel(MachineDirection direction);

		/// @brief Returns the key switch state of the tractor or power unit.
		/// @return The key switch state of the tractor or power unit.
		KeySwitchState get_key_switch_state() const;

		/// @brief Sets the reported key switch state of the tractor or power unit.
		/// @note The "Not Off" key switch state does not always mean "On" so use care when using it.
		/// @param state The key switch state of the tractor or power unit.
		/// @return True if the set value was different from the stored value otherwise false
		bool set_key_switch_state(KeySwitchState state);

		/// @brief Returns the state of a switch or other operator input to start or enable implement operations.
		/// @details The start or enabled state can be the result of the implement being positioned in an operating position.
		/// It can be generated by an operator placing a switch to an ON state. Also called "Master ON/OFF" switch.
		/// @return The state of a switch or other operator input to start or enable implement operations.
		ImplementStartStopOperations get_implement_start_stop_operations_state() const;

		/// @brief Sets the state of a switch or other operator input to start or enable implement operations.
		/// @details The start or enabled state can be the result of the implement being positioned in an operating position.
		/// It can be generated by an operator placing a switch to an ON state. Also called "Master ON/OFF" switch.
		/// @param state The state of a switch or other operator input to start or enable implement operations.
		/// @return True if the set value was different from the stored value otherwise false
		bool set_implement_start_stop_operations_state(ImplementStartStopOperations state);

		/// @brief Returns whether the reported direction is reversed from the perspective of the operator
		/// @return Whether the reported direction is reversed from the perspective of the operator or not
		OperatorDirectionReversed get_operator_direction_reversed_state() const;

		/// @brief Sets whether the reported direction is reversed from the perspective of the operator.
		/// @param reverseState The state of whether the reported direction is reversed from the perspective of the operator
		/// @return True if the set value was different from the stored value otherwise false
		bool set_operator_direction_reversed_state(OperatorDirectionReversed reverseState);

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
		std::uint32_t wheelBasedMachineDistance_mm = 0; ///< Stores the decoded machine wheel-based distance in millimeters
		std::uint16_t wheelBasedMachineSpeed_mm_per_sec = 0; ///< Stores the decoded wheel-based machine speed in mm/s
		std::uint8_t maximumTimeOfTractorPower_min = 0; ///< Stores the maximum time of remaining tractor or power-unit-supplied electrical power at the current load
		MachineDirection machineDirectionState = MachineDirection::NotAvailable; ///< Stores direction of travel.
		KeySwitchState keySwitchState = KeySwitchState::NotAvailable; ///< Stores the key switch state of the tractor or power unit.
		ImplementStartStopOperations implementStartStopOperationsState = ImplementStartStopOperations::NotAvailable; ///< Stores the state of a switch or other operator input to start or enable implement operations.
		OperatorDirectionReversed operatorDirectionReversedState = OperatorDirectionReversed::NotAvailable; ///< Stores whether the reported direction is reversed from the perspective of the operator
	};

} // namespace isobus

#endif // ISOBUS_WHEEL_BASED_SPEED_HPP
