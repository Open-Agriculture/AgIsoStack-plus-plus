//================================================================================================
/// @file vt_application.hpp
///
/// @brief This is the definition of a class that handles the main application logic for this
/// example application.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso and the AgIsoStack++ developers
//================================================================================================
#ifndef VT_APPLICATION_HPP
#define VT_APPLICATION_HPP

#include "isobus/isobus/isobus_speed_distance_messages.hpp"
#include "isobus/isobus/isobus_task_controller_client.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "section_control_implement_sim.hpp"

/// @brief A class that manages the main application logic for this example program.
class SeederVtApplication
{
public:
	/// @brief Enumerates the different screens (data/alarm masks) that the app can display
	enum class ActiveScreen
	{
		Main,
		Settings,
		Statistics,
		Alarms,
		NoMachineSpeed,
		NoTaskController
	};

	/// @brief Constructor for a SeederVtApplication
	SeederVtApplication(std::shared_ptr<isobus::PartneredControlFunction> VTPartner, std::shared_ptr<isobus::PartneredControlFunction> TCPartner, std::shared_ptr<isobus::InternalControlFunction> source);

	/// @brief Initializes the class. Should be called before update is called the first time
	bool initialize();

	isobus::TaskControllerClient TCClientInterface; ///< An instance of the application's task controller interface
	isobus::VirtualTerminalClient VTClientInterface; ///< An instance of the application's universal/virtual terminal interface

	/// @brief Cyclically updates the application
	void update();

private:
	/// @brief A set of flags which are used to manage updating objects in the object pool at runtime.
	enum class UpdateVTStateFlags : std::uint32_t
	{
		UpdateActiveDataMask = 0,

		UpdateSection1EnableState_ObjPtr,
		UpdateSection2EnableState_ObjPtr,
		UpdateSection3EnableState_ObjPtr,
		UpdateSection4EnableState_ObjPtr,
		UpdateSection5EnableState_ObjPtr,
		UpdateSection6EnableState_ObjPtr,
		UpdateSelectedStatisticsContainer_ObjPtr,
		UpdateAutoManual_ObjPtr,
		UpdateSpeedUnits_ObjPtr,
		UpdateCurrentAlarms1_ObjPtr,
		UpdateCurrentAlarms2_ObjPtr,

		UpdateStatisticsSelection_VarNum,
		UpdateCanAddress_VarNum,
		UpdateBusload_VarNum,
		UpdateUtVersion_VarNum,
		UpdateUtAddress_VarNum,
		UpdateCurrentSpeedMeter_VarNum,
		UpdateCurrentSpeedReadout_VarNum,
		UpdateTcVersion_VarNum,
		UpdateTcAddress_VarNum,
		UpdateTcNumberBoomsSupported_VarNum,
		UpdateTcSupportedSections_VarNum,
		UpdateTcControlChannels_VarNum,
		UpdateEnableAlarms_VarNum,

		UpdateSection1Status_OutRect,
		UpdateSection2Status_OutRect,
		UpdateSection3Status_OutRect,
		UpdateSection4Status_OutRect,
		UpdateSection5Status_OutRect,
		UpdateSection6Status_OutRect,

		UpdateSpeed_OutNum,

		NumberOfFlags
	};

	/// @brief Enumerates the statistics in the statistics input list
	enum class Statistics : std::uint16_t
	{
		None = 0,
		CANBus,
		UniversalTerminal,
		TaskController,
		Credits
	};

	/// @brief Enumerates our tolerated speed sources
	enum class SpeedSources
	{
		MachineSelected,
		GroundBased,
		WheelBased
	};

	/// @brief Stores information associated to if an alarm mask should be shown
	class AlarmMaskCondition
	{
	public:
		AlarmMaskCondition() = default;
		std::uint32_t conditionTimestamp = 0;
		std::uint32_t conditionTimeout = 10000;
		bool acknowledged = false;
		bool active = false;
	};

	/// @brief Lists the different alarm conditions that might exist
	enum class Alarm : std::uint8_t
	{
		NoMachineSpeed = 0, ///< No MSS message, needed for section control
		NoTaskController, ///< No TC, makes the demo less interesting

		NumberOfAlarms ///< The number of alarms in this enumeration
	};

	/// @brief Catches UpdateVTStateFlags and uses them to transmit messages to the VT at runtime to adjust the object pool's state
	static void processFlags(std::uint32_t flag, void *parentPointer);

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

	/// @brief Returns if the selected object ID is shown currently
	/// @param[in] objectID The object ID of the object to check the shown state of
	/// @returns True if the selected object ID is shown currently, otherwise false
	bool get_is_object_shown(std::uint16_t objectID);

	/// @brief Returns the number of active alarms
	/// @returns The number of currently active alarms
	std::size_t get_number_active_alarms() const;

	/// @brief Returns active alarms by index/priority
	/// @returns Active alarms by index/priority or NumberOfAlarms if index is invalid
	Alarm get_active_alarm_by_priority_index(std::size_t index) const;

	/// @brief Sets the currently active screen, which sets the appropriate VT flag to change the active mask if needed
	/// @param[in] newScreen The screen to set as the active screen
	void set_currently_active_screen(ActiveScreen newScreen);

	/// @brief Sets the current busload, which is used to compare against the NetworkManager's reported
	/// load each loop to update the displayed load if needed.
	/// @param[in] busload The busload to set, in percent
	void set_current_busload(float busload);

	/// @brief Sets the current address of our ICF, used to tell if it ever changes when
	/// compared against the address reported by the stack so the VT flag can be set.
	/// @param[in] address The address to set
	void set_current_can_address(std::uint8_t address);

	/// @brief Sets the statistic that is selected in the stats data mask's input list
	/// @param[in] newSelectedStatistic The statistic to set as the currently selected one
	void set_selected_statistic(Statistics newSelectedStatistic);

	/// @brief Sets the connected UT's reported version, used to update the displayed value in the object pool
	/// @param[in] version The UT version to set
	void set_current_ut_version(isobus::VirtualTerminalClient::VTVersion version);

	/// @brief Sets the connected UT's CAN address, used to update the displayed value in the object pool
	/// @param[in] address The UT's address to set
	void set_current_ut_address(std::uint8_t address);

	/// @brief Sets the connected TC's CAN address, used to update the displayed value in the object pool
	/// @param[in] address The TC's address to set
	void set_current_tc_address(std::uint8_t address);

	/// @brief Sets the connected TC's reported version, used to update the displayed value in the object pool
	/// @param[in] version The TC's version to set
	void set_current_tc_version(isobus::TaskControllerClient::Version version);

	/// @brief Sets the connected TC's reported number of supported booms, used to update the displayed value in the object pool
	/// @param[in] numberBooms The TC's reported number of booms to set
	void set_current_tc_number_booms(std::uint8_t numberBooms);

	/// @brief Sets the connected TC's reported number of supported channels, used to update the displayed value in the object pool
	/// @param[in] numberChannels The TC's reported number of channels to set
	void set_current_tc_number_channels(std::uint8_t numberChannels);

	/// @brief Sets the connected TC's reported number of supported sections, used to update the displayed value in the object pool
	/// @param[in] numberSections The TC's reported number of sections to set
	void set_current_tc_number_sections(std::uint8_t numberSections);

	/// @brief Called cyclically by the update routine, checks if any alarm masks need to be shown to the user
	void update_alarms();

	static constexpr std::uint8_t NUMBER_ONSCREEN_SECTIONS = 6; ///< The number of sections we can display on the screen
	static const std::map<ActiveScreen, std::uint16_t> SCREEN_TO_DATA_MASK_MAP; ///< A map between our screen enum and the corresponding data masks' object IDs
	static const std::map<Statistics, std::uint16_t> STATISTICS_CONTAINER_MAP; ///< Maps selected statistics to corresponding container object IDs
	static const std::array<std::uint16_t, NUMBER_ONSCREEN_SECTIONS> SECTION_STATUS_OUTRECTS; ///< An array of object IDs corresponding to the on screen section status objects
	static const std::array<std::uint16_t, NUMBER_ONSCREEN_SECTIONS> SECTION_SWITCH_STATES; ///< An array of object IDs corresponding to the on screen section switch objects
	static const std::array<ActiveScreen, static_cast<std::uint8_t>(Alarm::NumberOfAlarms)> ALARM_SCREENS; ///< Used to convert index of alarm to active screen
	static const std::array<std::uint16_t, static_cast<std::uint8_t>(Alarm::NumberOfAlarms)> ALARM_DESCRIPTION_LINES; ///< Stores object IDs corresponding to each alarm's one line description

	SectionControlImplementSimulator sectionControl; ///< A class that manages section control
	isobus::ProcessingFlags txFlags; ///< A set of flags to handle transmitting VT messages and retries as needed
	std::vector<std::uint8_t> objectPool; ///< Stores our object pool
	std::array<AlarmMaskCondition, static_cast<std::uint8_t>(Alarm::NumberOfAlarms)> alarmConditions; ///< Tracks alarm conditions in priority order
	std::shared_ptr<void> softkeyEventListener = nullptr; ///< Stores a handle for soft key event callbacks
	std::shared_ptr<void> buttonEventListener = nullptr; ///< Stores a handle for button event callbacks
	std::shared_ptr<void> numericValueEventListener = nullptr; ///< Stores a handle for numeric value event callbacks
	float currentBusload = 0.0f; ///< Stores the current CAN bus load, used to detect changes and update the UT value
	ActiveScreen currentlyActiveScreen = ActiveScreen::Main; ///< The currently active data mask
	ActiveScreen previousActiveScreen = ActiveScreen::Main; ///< Stores the previous active screen so it can be returned to if needed
	Statistics currentlySelectedStatistic = Statistics::None; ///< The currently selected statistic on the stats data mask
	SpeedSources currentSpeedSource = SpeedSources::MachineSelected; ///< Keeps track of which speed source is currently being used
	isobus::LanguageCommandInterface::DistanceUnits distanceUnits = isobus::LanguageCommandInterface::DistanceUnits::Metric; ///< Stores the current displayed distance units
	isobus::VirtualTerminalClient::VTVersion utVersion = isobus::VirtualTerminalClient::VTVersion::ReservedOrUnknown; ///< Stores the UT's version for display in the pool
	isobus::TaskControllerClient::Version tcVersion = isobus::TaskControllerClient::Version::Unknown; ///< Stores the TC's version for display in the object pool
	isobus::SpeedMessagesInterface speedMessages; ///< Interface for reading speed from the bus
	std::shared_ptr<isobus::DeviceDescriptorObjectPool> ddop = nullptr; ///< Stores our application's DDOP
	std::shared_ptr<std::function<void(const std::shared_ptr<isobus::SpeedMessagesInterface::MachineSelectedSpeedData> &, const bool &)>> machineSelectedSpeedEventHandle; ///< Handle for MSS events
	std::shared_ptr<std::function<void(const std::shared_ptr<isobus::SpeedMessagesInterface::GroundBasedSpeedData> &, const bool &)>> groundBasedSpeedEventHandle; ///< Handle for ground based speed events
	std::shared_ptr<std::function<void(const std::shared_ptr<isobus::SpeedMessagesInterface::WheelBasedMachineSpeedData> &, const bool &)>> wheelBasedSpeedEventHandle; ///< Handle for wheel based speed events
	std::uint32_t slowUpdateTimestamp_ms = 0; ///< A timestamp to limit some polled data to 1Hz update rate
	std::uint32_t lastMachineSpeed = 0; ///< Used to help track speed source timeouts
	std::uint8_t canAddress = 0xFE; ///< Stores our CAN address so we know if it changes and can update it in the pool
	std::uint8_t utAddress = 0xFE; ///< Stores the UT's current address so we can tell if it changes and update it in the pool
	std::uint8_t tcAddress = 0xFE; ///< Stores the TC's current address so we can tell if it changes and update it in the pool
	std::uint8_t tcNumberBooms = 0; ///< Stores the TC's number of supported booms so we can tell if it changes and update it in the pool
	std::uint8_t tcNumberSections = 0; ///< Stores the TC's number of supported sections so we can tell if it changes and update it in the pool
	std::uint8_t tcNumberChannels = 0; ///< Stores the TC's number of control channels so we can tell if it changes and update it in the pool
	bool languageDataRequested = false; ///< Stores if we've requested the current language data yet
	bool alarmsEnabled = true; ///< Enables or disables showing alarms
};

#endif // VT_APPLICATION_HPP
