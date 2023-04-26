//================================================================================================
/// @file vt_application.hpp
///
/// @brief This is the definition of the VT portion of the seeder example
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef VT_APPLICATION_HPP
#define VT_APPLICATION_HPP

#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "section_control_implement_sim.hpp"

class SeederVtApplication
{
public:
	enum class ActiveScreen
	{
		Main,
		Settings,
		Statistics,
		Info
	};

	SeederVtApplication(std::shared_ptr<isobus::PartneredControlFunction> VTPartner, std::shared_ptr<isobus::InternalControlFunction> source);

	bool Initialize();

	isobus::VirtualTerminalClient VTClientInterface;

	void Update();

private:
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

		UpdateStatisticsSelection_VarNum,
		UpdateCanAddress_VarNum,
		UpdateBusload_VarNum,
		UpdateUtVersion_VarNum,
		UpdateUtAddress_VarNum,
		UpdateCurrentSpeedMeter_VarNum,

		UpdateSection1Status_OutRect,
		UpdateSection2Status_OutRect,
		UpdateSection3Status_OutRect,
		UpdateSection4Status_OutRect,
		UpdateSection5Status_OutRect,
		UpdateSection6Status_OutRect,

		UpdateSpeed_OutNum,

		NumberOfFlags
	};

	enum class Statistics : std::uint16_t
	{
		None = 0,
		CANBus,
		UniversalTerminal,
		TaskController,
		Credits
	};

	static void processFlags(std::uint32_t flag, void *parentPointer);
	static void handle_vt_key_events(const isobus::VirtualTerminalClient::VTKeyEvent &event);
	static void handle_numeric_value_events(const isobus::VirtualTerminalClient::VTChangeNumericValueEvent &event);

	bool get_is_object_shown(std::uint16_t objectID);

	void set_currently_active_screen(ActiveScreen newScreen);
	void set_current_busload(float busload);
	void set_current_can_address(std::uint8_t address);
	void set_selected_statistic(Statistics newSelectedStatistic);
	void set_current_ut_version(isobus::VirtualTerminalClient::VTVersion version);
	void set_current_ut_address(std::uint8_t address);

	static constexpr std::uint8_t NUMBER_ONSCREEN_SECTIONS = 6;
	static const std::map<ActiveScreen, std::uint16_t> SCREEN_TO_DATA_MASK_MAP;
	static const std::map<Statistics, std::uint16_t> STATISTICS_CONTAINER_MAP;
	static const std::array<std::uint16_t, NUMBER_ONSCREEN_SECTIONS> SECTION_STATUS_OUTRECTS;
	static const std::array<std::uint16_t, NUMBER_ONSCREEN_SECTIONS> SECTION_SWITCH_STATES;

	SectionControlImplementSimulator sectionControl;
	isobus::ProcessingFlags txFlags;
	std::vector<std::uint8_t> objectPool;
	std::shared_ptr<void> softkeyEventListener = nullptr;
	std::shared_ptr<void> buttonEventListener = nullptr;
	std::shared_ptr<void> numericValueEventListener = nullptr;
	float currentBusload = 0.0f;
	ActiveScreen currentlyActiveScreen = ActiveScreen::Main;
	Statistics currentlySelectedStatistic = Statistics::None;
	isobus::LanguageCommandInterface::DistanceUnits distanceUnits = isobus::LanguageCommandInterface::DistanceUnits::Metric;
	isobus::VirtualTerminalClient::VTVersion utVersion = isobus::VirtualTerminalClient::VTVersion::ReservedOrUnknown; ///< Stores the UT's version for display in the pool
	std::uint32_t slowUpdateTimestamp_ms = 0; ///< A timestamp to limit some polled data to 1Hz update rate
	std::uint8_t canAddress = 0xFE; ///< Stores our CAN address so we know if it changes and can update it in the pool
	std::uint8_t utAddress = 0xFE; ///< Stores the UT's current address so we can tell if it changes and update it in the pool
};

#endif // VT_APPLICATION_HPP
