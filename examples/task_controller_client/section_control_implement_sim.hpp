//================================================================================================
/// @file section_control_implement_sim.hpp
///
/// @brief Defines a class that emulates a section control capable ISO implement.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef SECTION_CONTROL_IMPLEMENT_SIM_HPP
#define SECTION_CONTROL_IMPLEMENT_SIM_HPP

#include "isobus/isobus/can_NAME.hpp"
#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"

/// @brief Simulates a sprayer rate controller with section control
/// @note This is just an example. A real rate controller will obviously need to control rate and section
/// states rather than just echoing them back to the task controller.
class SectionControlImplementSimulator
{
public:
	static constexpr std::uint16_t MAX_NUMBER_SECTIONS_SUPPORTED = 256; ///< The most sections any implement can support is 256

	/// @brief Enumerates unique IDs in the implement's DDOP
	enum class ImplementDDOPObjectIDs : std::uint16_t
	{
		Device = 0, ///< Represents the device itself

		MainDeviceElement, ///< The main device element

		DeviceActualWorkState, ///< The actual work state (on/off) for the device
		RequestDefaultProcessData, ///< https://www.isobus.net/isobus/dDEntity/144
		DeviceTotalTime, ///< Accumulated Time in working position

		Connector, ///< Element that represents a connector to which the implement is attached
		ConnectorXOffset, ///< The fore/aft offset of the connector
		ConnectorYOffset, ///< The left/right offset of the connector
		ConnectorType, ///< https://www.isobus.net/isobus/dDEntity/767

		SprayBoom, ///< Element object that represents the boom
		ActualWorkState, ///< The actual on/off work state for the boom
		ActualWorkingWidth, ///< This is the effective / active working width of the boom during operation.
		AreaTotal, ///< An area accumulator that gets reported to the TC for the whole boom
		SetpointWorkState, ///< A settable work state for the entire boom sub-tree of objects
		SectionControlState, ///< If section control is on or off (auto/manual) modes
		BoomXOffset, ///< The offset up/down from the connector where the boom reference point (center) is
		BoomYOffset, ///< The offset left/right from the connector where the boom reference point is
		BoomZOffset, ///< The offset up/down from the connector where the boom reference point is

		Section1, ///< Section 1's device element object
		SectionMax = Section1 + (MAX_NUMBER_SECTIONS_SUPPORTED - 1), ///< Individual device elements for each section
		Section1XOffset, ///< The first section's X (fore/aft) offset
		SectionXOffsetMax = Section1XOffset + (MAX_NUMBER_SECTIONS_SUPPORTED - 1), ///< Individual X offsets (fore/aft) for each section
		Section1YOffset, ///< The first section's Y offset (left/right)
		SectionYOffsetMax = Section1YOffset + (MAX_NUMBER_SECTIONS_SUPPORTED - 1), ///< Individual Y offsets (L/R) for each section
		Section1Width, ///< The first section's width
		SectionWidthMax = Section1Width + (MAX_NUMBER_SECTIONS_SUPPORTED - 1), ///< Individual IDs for each section's width
		ActualCondensedWorkingState1To16, ///< https://www.isobus.net/isobus/dDEntity/183
		ActualCondensedWorkingState17To32, ///< Condensed actual work state for sections 17 to 32
		ActualCondensedWorkingState33To48, ///< Condensed actual work state for sections 33 to 48
		ActualCondensedWorkingState49To64, ///< Condensed actual work state for sections 49 to 64
		ActualCondensedWorkingState65To80, ///< Condensed actual work state for sections 65 to 80
		ActualCondensedWorkingState81To96, ///< Condensed actual work state for sections 81 to 96
		ActualCondensedWorkingState97To112, ///< Condensed actual work state for sections 97 to 112
		ActualCondensedWorkingState113To128, ///< Condensed actual work state for sections 113 to 128
		ActualCondensedWorkingState129To144, ///< Condensed actual work state for sections 129 to 144
		ActualCondensedWorkingState145To160, ///< Condensed actual work state for sections 145 to 160
		ActualCondensedWorkingState161To176, ///< Condensed actual work state for sections 161 to 176
		ActualCondensedWorkingState177To192, ///< Condensed actual work state for sections 177 to 192
		ActualCondensedWorkingState193To208, ///< Condensed actual work state for sections 193 to 208
		ActualCondensedWorkingState209To224, ///< Condensed actual work state for sections 209 to 224
		ActualCondensedWorkingState225To240, ///< Condensed actual work state for sections 225 to 240
		ActualCondensedWorkingState241To256, ///< Condensed actual work state for sections 241 to 256
		SetpointCondensedWorkingState1To16, ///< https://www.isobus.net/isobus/dDEntity/345
		SetpointCondensedWorkingState17To32, ///< Condensed setpoint work state for sections 17 to 32
		SetpointCondensedWorkingState33To48, ///< Condensed setpoint work state for sections 33 to 48
		SetpointCondensedWorkingState49To64, ///< Condensed setpoint work state for sections 49 to 64
		SetpointCondensedWorkingState65To80, ///< Condensed setpoint work state for sections 65 to 80
		SetpointCondensedWorkingState81To96, ///< Condensed setpoint work state for sections 81 to 96
		SetpointCondensedWorkingState97To112, ///< Condensed setpoint work state for sections 97 to 112
		SetpointCondensedWorkingState113To128, ///< Condensed setpoint work state for sections 113 to 128
		SetpointCondensedWorkingState129To144, ///< Condensed setpoint work state for sections 129 to 144
		SetpointCondensedWorkingState145To160, ///< Condensed setpoint work state for sections 145 to 160
		SetpointCondensedWorkingState161To176, ///< Condensed setpoint work state for sections 161 to 176
		SetpointCondensedWorkingState177To192, ///< Condensed setpoint work state for sections 177 to 192
		SetpointCondensedWorkingState193To208, ///< Condensed setpoint work state for sections 193 to 208
		SetpointCondensedWorkingState209To224, ///< Condensed setpoint work state for sections 209 to 224
		SetpointCondensedWorkingState225To240, ///< Condensed setpoint work state for sections 225 to 240
		SetpointCondensedWorkingState241To256, ///< Condensed setpoint work state for sections 241 to 256

		LiquidProduct, ///< The main tank/bin element that describes the sprayer's main product
		TankCapacity, ///< The max volume content for the liquid product device element
		TankVolume, ///< Actual Device Element Content specified as volume
		LifetimeApplicationVolumeTotal, ///< https://www.isobus.net/isobus/dDEntity/400
		PrescriptionControlState, ///< https://www.isobus.net/isobus/dDEntity/203
		ActualCulturalPractice, ///< https://www.isobus.net/isobus/dDEntity/205
		TargetRate, ///< The target rate for the rate controller main product
		ActualRate, ///< The actual rate of the rate controller main product

		AreaPresentation, ///< Describes to the TC how to display area units
		TimePresentation, ///< Describes to the TC how to display time units
		ShortWidthPresentation, ///< Describes to the TC how to display small width units
		LongWidthPresentation, ///< Describes to the TC how to display large width units
		VolumePresentation, ///< Describes to the TC how to display volume units
		VolumePerAreaPresentation ///< Describes to the TC how to display volume per area units
	};

	/// @brief Constructor for the simulator
	SectionControlImplementSimulator() = default;

	/// @brief Sets the number of section states to track
	/// @param[in] value The number of sections to track for section control
	void set_number_of_sections(std::uint8_t value);

	/// @brief Returns the number of sections that the sim is configured for
	/// @returns The number of sections the sim is configured for
	std::uint8_t get_number_of_sections() const;

	/// @brief Sets the current section state by index
	/// @param[in] index The index of the section state to set
	/// @param[in] value The new state for the section
	void set_section_state(std::uint8_t index, bool value);

	/// @brief Returns the current section state by index
	/// @param[in] index The index of the section to get the state for
	bool get_section_state(std::uint8_t index) const;

	/// @brief Returns the target rate as the actual rate
	/// @returns The "actual" rate which is just the target rate
	std::uint32_t get_actual_rate() const;

	/// @brief Sets the target rate
	/// @param[in] value The rate to set
	void set_target_rate(std::uint32_t value);

	/// @brief Returns the actual work state of the device
	bool get_actual_work_state() const;

	/// @brief Sets the target work state for the device
	/// @param[in] value The new works state to set
	void set_target_work_state(bool value);

	/// @brief Returns the current prescription control state
	/// @returns The current prescription control state (always 1)
	constexpr std::uint32_t get_prescription_control_state() const;

	/// @brief Returns the current section control state
	/// @returns The current section control state (always 1)
	constexpr std::uint32_t get_section_control_state() const;

	/// @brief Generates a DDOP to send to the TC
	/// @param[in] poolToPopulate A pointer to the DDOP that will be populated
	/// @param[in] clientName The ISO NAME to generate the DDOP for
	/// @returns true if the DDOP was successfully created, otherwise false
	bool create_ddop(std::shared_ptr<isobus::DeviceDescriptorObjectPool> poolToPopulate, isobus::NAME clientName) const;

	/// @brief A callback that will be used by the TC client to read values
	/// @param[in] elementNumber The element number associated to the value being requested
	/// @param[in] DDI The ddi of the value in the element being requested
	/// @param[in,out] value The value to report back to the TC client
	/// @param[in] parentPointer A pointer to the class instance this callback is for
	static bool request_value_command_callback(std::uint16_t elementNumber,
	                                           std::uint16_t DDI,
	                                           std::int32_t &value,
	                                           void *parentPointer);

	/// @brief A callback that will be used by the TC client to set values
	/// @param[in] elementNumber The element number being commanded
	/// @param[in] DDI The DDI being commanded for the specified element number
	/// @param[in] processVariableValue The value being commanded
	/// @param[in] parentPointer the pointer to the class instance the callback is for
	/// @returns true
	static bool value_command_callback(std::uint16_t elementNumber,
	                                   std::uint16_t DDI,
	                                   std::int32_t processVariableValue,
	                                   void *parentPointer);

private:
	static constexpr std::uint8_t NUMBER_SECTIONS_PER_CONDENSED_MESSAGE = 16; ///< Number of section states in a condensed working state message

	std::vector<bool> sectionStates; ///< Stores the commanded section states as a set of boolean values
	std::uint32_t targetRate = 0; ///< The target rate
	bool setpointWorkState = true; ///< The overall work state
};

#endif // SECTION_CONTROL_IMPLEMENT_SIM_HPP
