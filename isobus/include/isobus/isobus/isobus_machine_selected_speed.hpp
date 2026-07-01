//================================================================================================
/// @file isobus_machine_selected_speed.hpp
///
/// @brief Parses and sends machine selected speed messages.
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_MACHINE_SELECTED_SPEED_HPP
#define ISOBUS_MACHINE_SELECTED_SPEED_HPP

#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/isobus_machine_speed_direction_constants.hpp"

namespace isobus
{
	/// @brief Message that provides the current machine selected speed, direction and source parameters.
	/// @details This is usually the best/authoritative source of speed information as chosen by the machine.
	class MachineSelectedSpeedData
	{
	public:
		/// @brief This parameter is used to indicate why the vehicle speed control unit cannot currently accept
		/// remote commands or has most recently stopped accepting remote commands.
		/// @note Some values are reserved or manufacturer specific. See the SPN definition.
		enum class ExitReasonCode : std::uint8_t
		{
			NoReasonAllClear = 0,
			RequiredLevelOfOperatorPresenceAwarenessNotDetected = 1,
			ImplementReleasedControlOfFunction = 2,
			OperatorOverrideOfFunction = 3,
			OperatorControlNotInValidPosition = 4,
			RemoteCommandTimeout = 5,
			RemoteCommandOutOfRangeInvalid = 6,
			FunctionNotCalibrated = 7,
			OperatorControlFault = 8,
			FunctionFault = 9,
			VehicleTransmissionGearDoesNotAllowRemoteCommands = 22,
			Error = 62,
			NotAvailable = 63
		};

		/// @brief An indication of the speed source that is currently being reported in the machine selected speed parameter.
		/// @details This parameter is an indication of the speed source that is currently being reported in the machine selected speed parameter.
		/// Simulated speed is a system-generated speed message to permit implement operations when the machine is not actually moving.
		/// Blended speed is a speed message that uses a combination of the actual speed sources based on the operator's or the manufacturer's selected logic,
		/// i.e. when a ground-based speed source is less than 0.5 m/s, the speed message will then send the wheel speed source.
		enum class SpeedSource : std::uint8_t
		{
			WheelBasedSpeed = 0, ///< Wheel encoder usually
			GroundBasedSpeed = 1, ///< Radar usually
			NavigationBasedSpeed = 2, ///< GNSS Usually
			Blended = 3, ///< Some combination of source fusion
			Simulated = 4, ///< A test speed
			Reserved_1 = 5, ///< Reserved
			Reserved_2 = 6, ///< Reserved
			NotAvailable = 7 ///< N/A
		};

		/// @brief This parameter is used to report the Tractor ECU's present limit
		/// status associated with a parameter whose commands are persistent.
		/// Similar to other SAEbs03 limit statuses.
		enum class LimitStatus : std::uint8_t
		{
			NotLimited = 0,
			OperatorLimitedControlled = 1, ///< Request cannot be implemented
			LimitedHigh = 2, ///< Only lower command values result in a change
			LimitedLow = 3, ///< Only higher command values result in a change
			Reserved_1 = 4, ///< Reserved
			Reserved_2 = 5, ///< Reserved
			NonRecoverableFault = 6,
			NotAvailable ///< Parameter not supported
		};

		/// @brief Constructor for a MachineSelectedSpeedData
		/// @param[in] sender The control function that is sending this message
		explicit MachineSelectedSpeedData(std::shared_ptr<ControlFunction> sender);

		/// @brief Returns the Actual distance traveled by the machine based on the value of selected machine speed (SPN 4305).
		/// @note When the distance exceeds 4211081215 meters the value shall be reset to zero and incremented as additional distance accrues.
		/// @return The Actual distance traveled by the machine based on the value of selected machine speed (millimeters)
		std::uint32_t get_machine_distance() const;

		/// @brief Sets the Actual distance traveled by the machine based on the value of selected machine speed (SPN 4305).
		/// @note When the distance exceeds 4211081215 meters the value shall be reset to zero and incremented as additional distance accrues.
		/// @param[in] distance The Actual distance traveled by the machine based on the value of selected machine speed (millimeters)
		/// @return True if the set value was different from the stored value otherwise false
		bool set_machine_distance(std::uint32_t distance);

		/// @brief Returns the current machine selected speed.
		/// @details The TECU sends this value as the authoritative speed for the machine.
		/// @returns The current machine selected speed in mm/s
		std::uint16_t get_machine_speed() const;

		/// @brief Sets the machine selected speed.
		/// @param speed The machine selected speed in mm/s
		/// @return True if the set value was different from the stored value otherwise false
		bool set_machine_speed(std::uint16_t speed);

		/// @brief Returns the reason why the vehicle speed control unit cannot currently accept remote commands or
		/// has most recently stopped accepting remote commands
		/// @return why the vehicle speed control unit cannot currently accept remote commands or
		/// has most recently stopped accepting remote commands
		std::uint8_t get_exit_reason_code() const;

		/// @brief Sets the reason why the vehicle speed control unit cannot currently accept remote commands or
		/// has most recently stopped accepting remote commands
		/// @param exitCode The exit/reason code to set
		/// @return True if the set value was different from the stored value otherwise false
		bool set_exit_reason_code(std::uint8_t exitCode);

		/// @brief Returns the speed source that is currently being reported in the machine selected speed parameter (SPN-4305).
		/// @return The speed source that is currently being reported in the machine selected speed parameter (SPN-4305).
		SpeedSource get_speed_source() const;

		/// @brief Sets the speed source that is currently being reported in the machine selected speed parameter (SPN-4305).
		/// @param selectedSource The speed source that is currently being reported in the machine selected speed parameter (SPN-4305).
		/// @return True if the set value was different from the stored value otherwise false
		bool set_speed_source(SpeedSource selectedSource);

		/// @brief Returns The Tractor ECU's present limit status associated with a parameter whose commands are persistent
		/// @return The Tractor ECU's present limit status associated with a parameter whose commands are persistent
		LimitStatus get_limit_status() const;

		/// @brief Sets the Tractor ECU's present limit status associated with a parameter whose commands are persistent
		/// @param statusToSet The Tractor ECU's present limit status associated with a parameter whose commands are persistent
		/// @return True if the set value was different from the stored value otherwise false
		bool set_limit_status(LimitStatus statusToSet);

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
		std::uint32_t machineSelectedSpeedDistance_mm = 0; ///< Stores the machine selected speed distance in millimeters
		std::uint16_t machineSelectedSpeed_mm_per_sec = 0; ///< Stores the machine selected speed in mm/s
		std::uint8_t exitReasonCode = static_cast<std::uint8_t>(ExitReasonCode::NotAvailable); ///< Stores why the machine has most recently stopped accepting remote commands.
		SpeedSource source = SpeedSource::NotAvailable; ///< Stores the speed source that is currently being reported
		LimitStatus limitStatus = LimitStatus::NotAvailable; ///< Stores the tractor ECU limit status
		MachineDirection machineDirectionState = MachineDirection::NotAvailable; ///< Stores direction of travel.
	};
} // namespace isobus

#endif // ISOBUS_MACHINE_SELECTED_SPEED_HPP
