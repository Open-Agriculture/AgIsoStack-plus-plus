//================================================================================================
/// @file vt_application.hpp
///
/// @brief This is the definition of a class that handles the main application logic for this
/// example application.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef VT_APPLICATION_HPP
#define VT_APPLICATION_HPP

#include "object_pool.hpp"
#include "section_control_implement_sim.hpp"

#include "isobus/isobus/isobus_speed_distance_messages.hpp"
#include "isobus/isobus/isobus_task_controller_client.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client_update_helper.hpp"

/// @brief A class that manages the main application logic for this example program.
class SeederVtApplication
{
public:
	/// @brief Constructor for a SeederVtApplication
	SeederVtApplication(std::shared_ptr<isobus::PartneredControlFunction> VTPartner, std::shared_ptr<isobus::PartneredControlFunction> TCPartner, std::shared_ptr<isobus::InternalControlFunction> source);

	/// @brief Initializes the class. Should be called before update is called the first time
	bool initialize();

	isobus::TaskControllerClient TCClientInterface; ///< An instance of the application's task controller interface
	std::shared_ptr<isobus::VirtualTerminalClient> VTClientInterface; ///< The application's universal/virtual terminal interface
	isobus::VirtualTerminalClientUpdateHelper VTClientUpdateHelper; // A helper class for updating the state of the VT

	/// @brief Cyclically updates the application
	void update();

private:
	/// @brief Enumerates our tolerated speed sources
	enum class SpeedSources
	{
		MachineSelected,
		GroundBased,
		WheelBased
	};

	/// @brief Lists the different alarm conditions that might exist
	enum class AlarmType
	{
		NoMachineSpeed, ///< No MSS message, needed for section control
		NoTaskController, ///< No TC, makes the demo less interesting

		Count ///< The number of alarm types
	};

	/// @brief Stores information associated to if an alarm mask should be shown
	class Alarm
	{
	public:
		/// @brief Constructor for an Alarm
		/// @param[in] activationDelay_ms The delay before the alarm is shown after it is triggered (in milliseconds)
		explicit Alarm(std::uint32_t activationDelay_ms = 10000);

		/// @brief Returns if the alarm is active
		/// @returns True if the alarm is active, otherwise false
		bool is_active() const;

		/// @brief Triggers the alarm if it is not already triggered
		void trigger();

		/// @brief Acknowledges the alarm
		void acknowledge();

		/// @brief Resets the alarm
		void reset();

	private:
		std::uint32_t timestampTriggered_ms = 0;
		std::uint32_t activationDelay_ms;
		bool acknowledged = false;
	};

	/// @brief A callback for handling VT softkey events
	/// @param[in] event The event data to process
	void handle_vt_key_events(const isobus::VirtualTerminalClient::VTKeyEvent &event);

	/// @brief A callback for handling VT numeric value events (user enters a new value, for example)
	/// @param[in] event The event data to process
	void handle_numeric_value_events(const isobus::VirtualTerminalClient::VTChangeNumericValueEvent &event);

	/// @brief A callback for handling machine selected speed events, used to set appropriate VT flags
	/// @param[in] event The event data to process
	void handle_machine_selected_speed(const std::shared_ptr<isobus::SpeedMessagesInterface::MachineSelectedSpeedData> mssData, bool changed);

	/// @brief A callback for handling ground based speed events, used to set appropriate VT flags
	/// @param[in] event The event data to process
	void handle_ground_based_speed(const std::shared_ptr<isobus::SpeedMessagesInterface::GroundBasedSpeedData> mssData, bool changed);

	/// @brief A callback for handling wheel based speed events, used to set appropriate VT flags
	/// @param[in] event The event data to process
	void handle_wheel_based_speed(const std::shared_ptr<isobus::SpeedMessagesInterface::WheelBasedMachineSpeedData> mssData, bool changed);

	/// @brief Aggregates speeds and decides which speed to use
	void process_new_speed(SpeedSources source, std::uint32_t speed);

	/// @brief Toggle a section switch on or off
	/// @param[in] sectionIndex The index of the section to toggle
	void toggle_section(std::uint8_t sectionIndex);

	/// @brief Reflects the section state on the screen
	void update_section_objects(std::uint8_t sectionIndex);

	/// @brief Reflects the speed on the screen
	void update_speedometer_objects(std::uint32_t speed);

	/// @brief Reflects the UT version on the screen
	void update_ut_version_objects(isobus::VirtualTerminalClient::VTVersion version);

	/// @brief Returns if the selected object ID is shown currently
	/// @param[in] objectID The object ID of the object to check the shown state of
	/// @returns True if the selected object ID is shown currently, otherwise false
	bool get_is_object_shown(std::uint16_t objectID) const;

	/// @brief Reverts the current mask to the last previously active data mask
	void revert_to_previous_data_mask();

	/// @brief Called cyclically by the update routine, checks if any alarm masks need to be shown to the user
	void update_alarms();

	static constexpr std::uint8_t NUMBER_ONSCREEN_SECTIONS = 6; ///< The number of sections we can display on the screen

	SectionControlImplementSimulator sectionControl; ///< A class that manages section control
	std::vector<std::uint8_t> objectPool; ///< Stores our object pool
	std::map<AlarmType, Alarm> alarms; ///< Tracks alarm conditions in priority order
	isobus::SpeedMessagesInterface speedMessages; ///< Interface for reading speed from the bus
	std::shared_ptr<isobus::DeviceDescriptorObjectPool> ddop = nullptr; ///< Stores our application's DDOP
	std::uint32_t slowUpdateTimestamp_ms = 0; ///< A timestamp to limit some polled data to 1Hz update rate
	bool languageDataRequested = false; ///< Stores if we've requested the current language data yet
};

#endif // VT_APPLICATION_HPP
