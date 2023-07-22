//================================================================================================
/// @file vt_application.cpp
///
/// @brief This is the implementation of the VT portion of the seeder example
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "vt_application.hpp"

#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/utility/iop_file_interface.hpp"
#include "isobus/utility/system_timing.hpp"
#include "object_pool.hpp"

#include <cassert>
#include <iostream>

const std::map<SeederVtApplication::ActiveScreen, std::uint16_t> SeederVtApplication::SCREEN_TO_DATA_MASK_MAP = {
	{ ActiveScreen::Main, mainRunscreen_DataMask },
	{ ActiveScreen::Settings, settingsRunscreen_DataMask },
	{ ActiveScreen::Statistics, statisticsRunscreen_DataMask },
	{ ActiveScreen::Alarms, alarmsRunscreen_DataMask },
	{ ActiveScreen::NoMachineSpeed, noSpeed_AlarmMask },
	{ ActiveScreen::NoTaskController, noTaskController_AlarmMask }
};

const std::map<SeederVtApplication::Statistics, std::uint16_t> SeederVtApplication::STATISTICS_CONTAINER_MAP = {
	{ Statistics::None, UNDEFINED },
	{ Statistics::CANBus, canStatistics_Container },
	{ Statistics::UniversalTerminal, utStatistics_Container },
	{ Statistics::TaskController, tcStatistics_Container },
	{ Statistics::Credits, credits_Container }
};

const std::array<std::uint16_t, SeederVtApplication::NUMBER_ONSCREEN_SECTIONS> SeederVtApplication::SECTION_STATUS_OUTRECTS = {
	section1Status_OutRect,
	section2Status_OutRect,
	section3Status_OutRect,
	section4Status_OutRect,
	section5Status_OutRect,
	section6Status_OutRect
};

const std::array<std::uint16_t, SeederVtApplication::NUMBER_ONSCREEN_SECTIONS> SeederVtApplication::SECTION_SWITCH_STATES = {
	section1EnableState_ObjPtr,
	section2EnableState_ObjPtr,
	section3EnableState_ObjPtr,
	section4EnableState_ObjPtr,
	section5EnableState_ObjPtr,
	section6EnableState_ObjPtr
};

const std::array<SeederVtApplication::ActiveScreen, static_cast<std::uint8_t>(SeederVtApplication::Alarm::NumberOfAlarms)> SeederVtApplication::ALARM_SCREENS = {
	ActiveScreen::NoMachineSpeed,
	ActiveScreen::NoTaskController
};

const std::array<std::uint16_t, static_cast<std::uint8_t>(SeederVtApplication::Alarm::NumberOfAlarms)> SeederVtApplication::ALARM_DESCRIPTION_LINES = {
	NoMachineSpeed_OutStr,
	NoTaskController_OutStr
};

SeederVtApplication::SeederVtApplication(std::shared_ptr<isobus::PartneredControlFunction> VTPartner, std::shared_ptr<isobus::PartneredControlFunction> TCPartner, std::shared_ptr<isobus::InternalControlFunction> source) :
  TCClientInterface(TCPartner, source, nullptr),
  VTClientInterface(VTPartner, source),
  speedMessages(source, false, false, false, false),
  txFlags(static_cast<std::uint32_t>(UpdateVTStateFlags::NumberOfFlags), processFlags, this)
{
}

bool SeederVtApplication::initialize()
{
	bool retVal = true;

	sectionControl.set_number_of_sections(NUMBER_ONSCREEN_SECTIONS);

	objectPool = isobus::IOPFileInterface::read_iop_file("BasePool.iop");

	if (objectPool.empty())
	{
		std::cout << "Failed to load object pool from BasePool.iop" << std::endl;
		return false;
	}
	std::cout << "Loaded object pool from BasePool.iop" << std::endl;

	// Generate a unique version string for this object pool (this is optional, and is entirely application specific behavior)
	std::string objectPoolHash = isobus::IOPFileInterface::hash_object_pool_to_version(objectPool);

	VTClientInterface.set_object_pool(0, isobus::VirtualTerminalClient::VTVersion::Version4, objectPool.data(), objectPool.size(), objectPoolHash);
	softkeyEventListener = VTClientInterface.add_vt_soft_key_event_listener([this](const isobus::VirtualTerminalClient::VTKeyEvent &event) { this->handle_vt_key_events(event); });
	buttonEventListener = VTClientInterface.add_vt_button_event_listener([this](const isobus::VirtualTerminalClient::VTKeyEvent &event) { this->handle_vt_key_events(event); });
	numericValueEventListener = VTClientInterface.add_vt_change_numeric_value_event_listener([this](const isobus::VirtualTerminalClient::VTChangeNumericValueEvent &event) { this->handle_numeric_value_events(event); });
	VTClientInterface.initialize(true);

	for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(UpdateVTStateFlags::NumberOfFlags); i++)
	{
		txFlags.set_flag(i); // Set all flags to bring the pool up with a known state
	}

	speedMessages.initialize();
	machineSelectedSpeedEventHandle = speedMessages.get_machine_selected_speed_data_event_publisher().add_listener([this](const std::shared_ptr<isobus::SpeedMessagesInterface::MachineSelectedSpeedData> mssData, bool changed) { this->handle_machine_selected_speed(mssData, changed); });

	ddop = std::make_shared<isobus::DeviceDescriptorObjectPool>(3);
	if (sectionControl.create_ddop(ddop, TCClientInterface.get_internal_control_function()->get_NAME()))
	{
		TCClientInterface.configure(ddop, 1, 255, 255, true, true, true, false, true);
		TCClientInterface.add_request_value_callback(SectionControlImplementSimulator::request_value_command_callback, &sectionControl);
		TCClientInterface.add_value_command_callback(SectionControlImplementSimulator::value_command_callback, &sectionControl);
		TCClientInterface.initialize(true);
	}
	else
	{
		std::cout << "Failed generating DDOP. TC functionality will not work until the DDOP structure is fixed." << std::endl;
		retVal = false;
	}
	return retVal;
}

void SeederVtApplication::handle_vt_key_events(const isobus::VirtualTerminalClient::VTKeyEvent &event)
{
	switch (event.keyEvent)
	{
		case isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased:
		{
			switch (event.objectID)
			{
				case home_Key:
				{
					set_currently_active_screen(ActiveScreen::Main);
				}
				break;

				case settings_Key:
				{
					set_currently_active_screen(ActiveScreen::Settings);
				}
				break;

				case statistics_Key:
				{
					set_currently_active_screen(ActiveScreen::Statistics);
				}
				break;

				case alarms_Key:
				{
					set_currently_active_screen(ActiveScreen::Alarms);
				}
				break;

				case acknowledgeAlarm_SoftKey:
				{
					for (std::size_t i = 0; i < alarmConditions.size(); i++)
					{
						if ((isobus::SystemTiming::time_expired_ms(alarmConditions.at(i).conditionTimestamp, alarmConditions.at(i).conditionTimeout)) &&
						    (false == alarmConditions.at(i).acknowledged))
						{
							alarmConditions.at(i).acknowledged = true;
							set_currently_active_screen(previousActiveScreen);
							break;
						}
					}
				}
				break;

				case autoManualToggle_Button:
				{
					sectionControl.set_is_mode_auto(!sectionControl.get_is_mode_auto());
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateAutoManual_ObjPtr));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection1Status_OutRect));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection2Status_OutRect));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection3Status_OutRect));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection4Status_OutRect));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection5Status_OutRect));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection6Status_OutRect));
				}
				break;

				case section1Toggle_Button:
				{
					sectionControl.set_switch_state(0, !sectionControl.get_switch_state(0));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection1EnableState_ObjPtr));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection1Status_OutRect));
				}
				break;

				case section2Toggle_Button:
				{
					sectionControl.set_switch_state(1, !sectionControl.get_switch_state(1));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection2EnableState_ObjPtr));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection2Status_OutRect));
				}
				break;

				case section3Toggle_Button:
				{
					sectionControl.set_switch_state(2, !sectionControl.get_switch_state(2));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection3EnableState_ObjPtr));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection3Status_OutRect));
				}
				break;

				case section4Toggle_Button:
				{
					sectionControl.set_switch_state(3, !sectionControl.get_switch_state(3));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection4EnableState_ObjPtr));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection4Status_OutRect));
				}
				break;

				case section5Toggle_Button:
				{
					sectionControl.set_switch_state(4, !sectionControl.get_switch_state(4));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection5EnableState_ObjPtr));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection5Status_OutRect));
				}
				break;

				case section6Toggle_Button:
				{
					sectionControl.set_switch_state(5, !sectionControl.get_switch_state(5));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection6EnableState_ObjPtr));
					txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection6Status_OutRect));
				}
				break;

				default:
					break;
			}
		}
		break;
	}
}

void SeederVtApplication::handle_numeric_value_events(const isobus::VirtualTerminalClient::VTChangeNumericValueEvent &event)
{
	switch (event.objectID)
	{
		case statisticsSelection_VarNum:
		{
			set_selected_statistic(static_cast<Statistics>(event.value));
		}
		break;

		default:
			break;
	}
}

void SeederVtApplication::handle_machine_selected_speed(const std::shared_ptr<isobus::SpeedMessagesInterface::MachineSelectedSpeedData>, bool changed)
{
	if (changed)
	{
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateCurrentSpeedMeter_VarNum));
	}
}

void SeederVtApplication::update()
{
	// Update some polled data or other things that don't need as frequent updates
	if (isobus::SystemTiming::time_expired_ms(slowUpdateTimestamp_ms, 1000))
	{
		auto VTClientControlFunction = VTClientInterface.get_internal_control_function();
		auto VTControlFunction = VTClientInterface.get_partner_control_function();
		auto TCControlFunction = TCClientInterface.get_partner_control_function();

		if (nullptr != VTClientControlFunction)
		{
			// These are used for displaying to the user. Address is not really needed to be known.
			set_current_can_address(VTClientControlFunction->get_address());
			set_current_ut_address(VTControlFunction->get_address());

			if (!languageDataRequested)
			{
				languageDataRequested = VTClientInterface.languageCommandInterface.send_request_language_command();
			}
		}
		set_current_busload(isobus::CANNetworkManager::CANNetwork.get_estimated_busload(0));
		set_current_ut_version(VTClientInterface.get_connected_vt_version());

		if (distanceUnits != VTClientInterface.languageCommandInterface.get_commanded_distance_units())
		{
			distanceUnits = VTClientInterface.languageCommandInterface.get_commanded_distance_units();
			txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSpeed_OutNum));
			txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSpeedUnits_ObjPtr));
		}

		if (nullptr != TCControlFunction)
		{
			set_current_tc_address(TCControlFunction->get_address());
		}
		set_current_tc_number_booms(TCClientInterface.get_connected_tc_number_booms_supported());
		set_current_tc_number_channels(TCClientInterface.get_connected_tc_number_channels_supported());
		set_current_tc_number_sections(TCClientInterface.get_connected_tc_number_sections_supported());
		set_current_tc_version(TCClientInterface.get_connected_tc_version());
		update_alarms();

		slowUpdateTimestamp_ms = isobus::SystemTiming::get_timestamp_ms();
	}
	speedMessages.update();

	// Send CAN messages
	txFlags.process_all_flags();
}

void SeederVtApplication::processFlags(std::uint32_t flag, void *parentPointer)
{
	auto currentFlag = static_cast<UpdateVTStateFlags>(flag);
	auto seeder = reinterpret_cast<SeederVtApplication *>(parentPointer);
	bool transmitSuccessful = false;

	assert(nullptr != seeder);

	if (seeder->VTClientInterface.get_is_connected())
	{
		switch (currentFlag)
		{
			case UpdateVTStateFlags::UpdateActiveDataMask:
			{
				std::uint16_t dataMask = mainRunscreen_DataMask;

				auto correspondingDataMask = SCREEN_TO_DATA_MASK_MAP.find(seeder->currentlyActiveScreen);

				if (SCREEN_TO_DATA_MASK_MAP.end() != correspondingDataMask)
				{
					dataMask = correspondingDataMask->second;
				}
				transmitSuccessful = seeder->VTClientInterface.send_change_active_mask(example_WorkingSet, dataMask);
			}
			break;

			case UpdateVTStateFlags::UpdateSection1EnableState_ObjPtr:
			case UpdateVTStateFlags::UpdateSection2EnableState_ObjPtr:
			case UpdateVTStateFlags::UpdateSection3EnableState_ObjPtr:
			case UpdateVTStateFlags::UpdateSection4EnableState_ObjPtr:
			case UpdateVTStateFlags::UpdateSection5EnableState_ObjPtr:
			case UpdateVTStateFlags::UpdateSection6EnableState_ObjPtr:
			{
				std::uint32_t sectionIndex = flag - static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection1EnableState_ObjPtr);

				if (seeder->get_is_object_shown(SECTION_SWITCH_STATES.at(sectionIndex)))
				{
					std::uint16_t newObject = offButtonSliderSmall_OutPict;

					if (seeder->sectionControl.get_switch_state(sectionIndex))
					{
						newObject = onButtonSliderSmall_OutPict;
					}
					transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(SECTION_SWITCH_STATES.at(sectionIndex), newObject);
				}
			}
			break;

			case UpdateVTStateFlags::UpdateAutoManual_ObjPtr:
			{
				if (seeder->get_is_object_shown(autoManual_ObjPtr))
				{
					transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(autoManual_ObjPtr, seeder->sectionControl.get_is_mode_auto() ? autoMode_Container : manualMode_Container);
				}
			}
			break;

			case UpdateVTStateFlags::UpdateSpeedUnits_ObjPtr:
			{
				if (seeder->get_is_object_shown(speedUnits_ObjPtr))
				{
					if (isobus::LanguageCommandInterface::DistanceUnits::ImperialUS == seeder->VTClientInterface.languageCommandInterface.get_commanded_distance_units())
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(speedUnits_ObjPtr, unitMph_OutStr);
					}
					else
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(speedUnits_ObjPtr, unitKph_OutStr);
					}
				}
			}
			break;

			case UpdateVTStateFlags::UpdateCurrentAlarms1_ObjPtr:
			{
				if (seeder->get_number_active_alarms() > 0)
				{
					auto alarmToShow = seeder->get_active_alarm_by_priority_index(0);

					if (Alarm::NumberOfAlarms != alarmToShow)
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(currentAlarms1_ObjPtr, ALARM_DESCRIPTION_LINES.at(static_cast<std::uint8_t>(alarmToShow)));
					} // else, process the flag again on the next loop
				}
				else
				{
					transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(currentAlarms1_ObjPtr, noActiveAlarms_OutStr);
				}
			}
			break;

			case UpdateVTStateFlags::UpdateCurrentAlarms2_ObjPtr:
			{
				if (seeder->get_number_active_alarms() > 1)
				{
					auto alarmToShow = seeder->get_active_alarm_by_priority_index(1);

					if (Alarm::NumberOfAlarms != alarmToShow)
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(currentAlarms2_ObjPtr, ALARM_DESCRIPTION_LINES.at(static_cast<std::uint8_t>(alarmToShow)));
					} // else, process the flag again on the next loop
				}
				else
				{
					transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(currentAlarms2_ObjPtr, UNDEFINED);
				}
			}
			break;

			case UpdateVTStateFlags::UpdateStatisticsSelection_VarNum:
			{
				if (seeder->get_is_object_shown(statisticsSelection_VarNum))
				{
					transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(statisticsSelection_VarNum, static_cast<std::uint32_t>(seeder->currentlySelectedStatistic));
				}
			}
			break;

			case UpdateVTStateFlags::UpdateSelectedStatisticsContainer_ObjPtr:
			{
				if (seeder->get_is_object_shown(selectedStatisticsContainer_ObjPtr))
				{
					std::uint16_t newContainer = UNDEFINED;

					auto correspondingContainer = STATISTICS_CONTAINER_MAP.find(seeder->currentlySelectedStatistic);

					if (STATISTICS_CONTAINER_MAP.end() != correspondingContainer)
					{
						newContainer = correspondingContainer->second;
					}
					transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(selectedStatisticsContainer_ObjPtr, newContainer);
				}
			}
			break;

			case UpdateVTStateFlags::UpdateCanAddress_VarNum:
			{
				if (seeder->get_is_object_shown(canAddress_VarNum))
				{
					transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(canAddress_VarNum, seeder->canAddress);
				}
			}
			break;

			case UpdateVTStateFlags::UpdateBusload_VarNum:
			{
				if (seeder->get_is_object_shown(busload_VarNum))
				{
					transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(busload_VarNum, seeder->currentBusload * 100.0f);
				}
			}
			break;

			case UpdateVTStateFlags::UpdateUtAddress_VarNum:
			{
				if (seeder->get_is_object_shown(utAddress_VarNum))
				{
					transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(utAddress_VarNum, seeder->utAddress);
				}
			}
			break;

			case UpdateVTStateFlags::UpdateUtVersion_VarNum:
			{
				if (seeder->get_is_object_shown(utVersion_VarNum))
				{
					std::uint8_t integerVersion = 0xFF;

					switch (seeder->utVersion)
					{
						case isobus::VirtualTerminalClient::VTVersion::Version2OrOlder:
						{
							integerVersion = 2;
						}
						break;

						case isobus::VirtualTerminalClient::VTVersion::Version3:
						{
							integerVersion = 3;
						}
						break;

						case isobus::VirtualTerminalClient::VTVersion::Version4:
						{
							integerVersion = 4;
						}
						break;

						case isobus::VirtualTerminalClient::VTVersion::Version5:
						{
							integerVersion = 5;
						}
						break;

						case isobus::VirtualTerminalClient::VTVersion::Version6:
						{
							integerVersion = 6;
						}
						break;

						default:
							break;
					}
					transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(utVersion_VarNum, integerVersion);
				}
			}
			break;

			case UpdateVTStateFlags::UpdateCurrentSpeedMeter_VarNum:
			{
				if (seeder->get_is_object_shown(currentSpeedMeter_VarNum))
				{
					auto mss = seeder->speedMessages.get_received_machine_selected_speed(0);
					if (nullptr != mss)
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(currentSpeedMeter_VarNum, mss->get_machine_speed());
					}
					else
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(currentSpeedMeter_VarNum, 0);
					}
				}
			}
			break;

			case UpdateVTStateFlags::UpdateTcVersion_VarNum:
			{
				if (seeder->get_is_object_shown(tcVersion_VarNum))
				{
					transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(tcVersion_VarNum, static_cast<std::uint32_t>(seeder->TCClientInterface.get_connected_tc_version()));
				}
			}
			break;

			case UpdateVTStateFlags::UpdateTcAddress_VarNum:
			{
				if (seeder->get_is_object_shown(tcAddress_VarNum))
				{
					if (nullptr != seeder->TCClientInterface.get_partner_control_function())
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(tcAddress_VarNum, seeder->TCClientInterface.get_partner_control_function()->get_address());
					}
					else
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(tcAddress_VarNum, 0xFE);
					}
				}
			}
			break;

			case UpdateVTStateFlags::UpdateTcNumberBoomsSupported_VarNum:
			{
				if (seeder->get_is_object_shown(tcNumberBoomsSupported_VarNum))
				{
					if (nullptr != seeder->TCClientInterface.get_partner_control_function())
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(tcNumberBoomsSupported_VarNum, seeder->TCClientInterface.get_connected_tc_number_booms_supported());
					}
					else
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(tcNumberBoomsSupported_VarNum, 0);
					}
				}
			}
			break;

			case UpdateVTStateFlags::UpdateTcSupportedSections_VarNum:
			{
				if (seeder->get_is_object_shown(tcSupportedSections_VarNum))
				{
					if (nullptr != seeder->TCClientInterface.get_partner_control_function())
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(tcSupportedSections_VarNum, seeder->TCClientInterface.get_connected_tc_number_sections_supported());
					}
					else
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(tcSupportedSections_VarNum, 0);
					}
				}
			}
			break;

			case UpdateVTStateFlags::UpdateTcControlChannels_VarNum:
			{
				if (seeder->get_is_object_shown(tcControlChannels_VarNum))
				{
					if (nullptr != seeder->TCClientInterface.get_partner_control_function())
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(tcControlChannels_VarNum, seeder->TCClientInterface.get_connected_tc_number_channels_supported());
					}
					else
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_numeric_value(tcControlChannels_VarNum, 0);
					}
				}
			}
			break;

			case UpdateVTStateFlags::UpdateSection1Status_OutRect:
			case UpdateVTStateFlags::UpdateSection2Status_OutRect:
			case UpdateVTStateFlags::UpdateSection3Status_OutRect:
			case UpdateVTStateFlags::UpdateSection4Status_OutRect:
			case UpdateVTStateFlags::UpdateSection5Status_OutRect:
			case UpdateVTStateFlags::UpdateSection6Status_OutRect:
			{
				std::uint32_t sectionIndex = flag - static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSection1Status_OutRect);

				if (seeder->get_is_object_shown(SECTION_STATUS_OUTRECTS.at(sectionIndex)))
				{
					std::uint32_t fillAttribute = solidRed_FillAttr;

					if (seeder->sectionControl.get_actual_work_state())
					{
						fillAttribute = solidGreen_FillAttr;
					}
					else if ((!seeder->sectionControl.get_is_mode_auto()) && (seeder->sectionControl.get_switch_state(sectionIndex)))
					{
						fillAttribute = solidYellow_FillAttr;
					}
					transmitSuccessful = seeder->VTClientInterface.send_change_attribute(SECTION_STATUS_OUTRECTS.at(sectionIndex), 5, fillAttribute); // 5 Is the attribute ID of the fill attribute
				}
			}
			break;

			case UpdateVTStateFlags::UpdateSpeed_OutNum:
			{
				if (seeder->get_is_object_shown(speed_OutNum))
				{
					if (isobus::LanguageCommandInterface::DistanceUnits::ImperialUS == seeder->VTClientInterface.languageCommandInterface.get_commanded_distance_units())
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_attribute(speed_OutNum, 8, 0.0022369363f); // mm/s to mph
					}
					else
					{
						transmitSuccessful = seeder->VTClientInterface.send_change_attribute(speed_OutNum, 8, 0.0036f); // mm/s to kph
					}
				}
			}
			break;

			default:
			{
				transmitSuccessful = true;
			}
			break;
		}
	}

	if (!transmitSuccessful)
	{
		seeder->txFlags.set_flag(flag);
	}
}

bool SeederVtApplication::get_is_object_shown(std::uint16_t objectID)
{
	auto myControlFunction = VTClientInterface.get_internal_control_function();
	bool retVal = false;

	if ((nullptr != myControlFunction) &&
	    (VTClientInterface.get_active_working_set_master_address() == myControlFunction->get_address()))
	{
		switch (objectID)
		{
			case section1Status_OutRect:
			case section2Status_OutRect:
			case section3Status_OutRect:
			case section4Status_OutRect:
			case section5Status_OutRect:
			case section6Status_OutRect:
			case autoManual_Container:
			case autoManual_ObjPtr:
			case mainRunscreen_SoftKeyMask:
			case Title_OutStr:
			case planterRunscreenStatus_Container:
			case planter_OutPict:
			case sectionButtons_Container:
			case section1Switch_Container:
			case section2Switch_Container:
			case section3Switch_Container:
			case section4Switch_Container:
			case section5Switch_Container:
			case section6Switch_Container:
			case speed_OutNum:
			case speedReadout_Container:
			case speedUnits_ObjPtr:
			case currentSpeedReadout_VarNum:
			case currentSpeedMeter_VarNum:
			{
				retVal = (ActiveScreen::Main == currentlyActiveScreen);
			}
			break;

			case statisticsHeader_OutStr:
			case statisticsDropdown_Container:
			case statistics_InList:
			case selectedStatisticsContainer_ObjPtr:
			{
				retVal = (ActiveScreen::Statistics == currentlyActiveScreen);
			}
			break;

			case returnHome_SKeyMask:
			{
				retVal = (ActiveScreen::Main != currentlyActiveScreen);
			}
			break;

			case busload_VarNum:
			case canAddress_VarNum:
			{
				retVal = ((ActiveScreen::Statistics == currentlyActiveScreen) &&
				          (Statistics::CANBus == currentlySelectedStatistic));
			}
			break;

			case utAddress_VarNum:
			case utVersion_VarNum:
			{
				retVal = ((ActiveScreen::Statistics == currentlyActiveScreen) &&
				          (Statistics::UniversalTerminal == currentlySelectedStatistic));
			}
			break;

			case tcVersion_VarNum:
			case tcAddress_VarNum:
			case tcNumberBoomsSupported_VarNum:
			case tcSupportedSections_VarNum:
			case tcControlChannels_VarNum:
			{
				retVal = ((ActiveScreen::Statistics == currentlyActiveScreen) &&
				          (Statistics::TaskController == currentlySelectedStatistic));
			}
			break;

			case machineSpeedNotDetectedSummary_OutStr:
			{
				retVal = (ActiveScreen::NoMachineSpeed == currentlyActiveScreen);
			}
			break;

			case TCNotConnectedSummary_OutStr:
			case noTCTitle_OutStr:
			{
				retVal = (ActiveScreen::NoTaskController == currentlyActiveScreen);
			}
			break;

			case warning_OutPict:
			case alarm_SKeyMask:
			{
				retVal = ((ActiveScreen::NoTaskController == currentlyActiveScreen) ||
				          (ActiveScreen::NoMachineSpeed == currentlyActiveScreen));
			}
			break;

			case currentAlarms1_ObjPtr:
			case currentAlarms2_ObjPtr:
			case currentAlarmsHeader_OutStr:
			{
				retVal = (ActiveScreen::Alarms == currentlyActiveScreen);
			}
			break;

			default:
			{
				retVal = true;
			}
			break;
		}
	}
	return retVal;
}

std::size_t SeederVtApplication::get_number_active_alarms() const
{
	std::size_t retVal = 0;

	for (auto &alarm : alarmConditions)
	{
		if (alarm.active)
		{
			retVal++;
		}
	}
	return retVal;
}

SeederVtApplication::Alarm SeederVtApplication::get_active_alarm_by_priority_index(std::size_t index) const
{
	std::size_t numberOfProcessedAlarms = 0;
	Alarm retVal = Alarm::NumberOfAlarms;

	for (std::size_t i = 0; i < alarmConditions.size(); i++)
	{
		if (alarmConditions.at(i).active)
		{
			numberOfProcessedAlarms++;
		}

		if (numberOfProcessedAlarms == (index + 1))
		{
			retVal = static_cast<Alarm>(i);
			break;
		}
	}
	return retVal;
}

void SeederVtApplication::set_currently_active_screen(ActiveScreen newScreen)
{
	if (newScreen != currentlyActiveScreen)
	{
		previousActiveScreen = currentlyActiveScreen;
		currentlyActiveScreen = newScreen;
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateActiveDataMask));
	}
}

void SeederVtApplication::set_current_busload(float busload)
{
	if (busload != currentBusload)
	{
		currentBusload = busload;
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateBusload_VarNum));
	}
}

void SeederVtApplication::set_current_can_address(std::uint8_t address)
{
	if (address != canAddress)
	{
		canAddress = address;
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateCanAddress_VarNum));
	}
}

void SeederVtApplication::set_selected_statistic(Statistics newSelectedStatistic)
{
	if (newSelectedStatistic != currentlySelectedStatistic)
	{
		currentlySelectedStatistic = newSelectedStatistic;
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateStatisticsSelection_VarNum));
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateSelectedStatisticsContainer_ObjPtr));
	}
}

void SeederVtApplication::set_current_ut_version(isobus::VirtualTerminalClient::VTVersion version)
{
	if (version != utVersion)
	{
		utVersion = version;
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateUtVersion_VarNum));
	}
}

void SeederVtApplication::set_current_ut_address(std::uint8_t address)
{
	if (address != utAddress)
	{
		utAddress = address;
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateUtAddress_VarNum));
	}
}

void SeederVtApplication::set_current_tc_address(std::uint8_t address)
{
	if (address != tcAddress)
	{
		tcAddress = address;
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateTcAddress_VarNum));
	}
}

void SeederVtApplication::set_current_tc_version(isobus::TaskControllerClient::Version version)
{
	if (version != tcVersion)
	{
		tcVersion = version;
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateTcVersion_VarNum));
	}
}

void SeederVtApplication::set_current_tc_number_booms(std::uint8_t numberBooms)
{
	if (numberBooms != tcNumberBooms)
	{
		tcNumberBooms = numberBooms;
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateTcNumberBoomsSupported_VarNum));
	}
}

void SeederVtApplication::set_current_tc_number_channels(std::uint8_t numberChannels)
{
	if (numberChannels != tcNumberChannels)
	{
		tcNumberChannels = numberChannels;
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateTcControlChannels_VarNum));
	}
}

void SeederVtApplication::set_current_tc_number_sections(std::uint8_t numberSections)
{
	if (numberSections != tcNumberSections)
	{
		tcNumberSections = numberSections;
		txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateTcSupportedSections_VarNum));
	}
}

void SeederVtApplication::update_alarms()
{
	bool updateShownMask = false;

	if (0 == speedMessages.get_number_received_machine_selected_speed_command_sources())
	{
		if (isobus::SystemTiming::time_expired_ms(alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoMachineSpeed)).conditionTimestamp, alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoMachineSpeed)).conditionTimeout))
		{
			updateShownMask = true;
			alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoMachineSpeed)).active = true;
		}
		else
		{
			alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoMachineSpeed)).active = false;
		}
	}
	else
	{
		alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoMachineSpeed)).conditionTimestamp = isobus::SystemTiming::get_timestamp_ms();
		alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoMachineSpeed)).acknowledged = false;
		alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoMachineSpeed)).active = false;
	}

	if (false == TCClientInterface.get_is_connected())
	{
		if (isobus::SystemTiming::time_expired_ms(alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoTaskController)).conditionTimestamp, alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoTaskController)).conditionTimeout))
		{
			updateShownMask = true;
			alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoTaskController)).active = true;
		}
		else
		{
			alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoTaskController)).active = false;
		}
	}
	else
	{
		alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoTaskController)).conditionTimestamp = isobus::SystemTiming::get_timestamp_ms();
		alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoTaskController)).acknowledged = false;
		alarmConditions.at(static_cast<std::uint8_t>(Alarm::NoTaskController)).active = false;
	}

	if (updateShownMask)
	{
		for (std::size_t i = 0; i < alarmConditions.size(); i++)
		{
			if ((alarmConditions.at(i).active) &&
			    (false == alarmConditions.at(i).acknowledged))
			{
				set_currently_active_screen(ALARM_SCREENS.at(i));
				break;
			}
		}
	}
	// Not ideal, but since we filter on shown screen it's not that bad. Todo, update these flags to be event driven
	txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateCurrentAlarms1_ObjPtr));
	txFlags.set_flag(static_cast<std::uint32_t>(UpdateVTStateFlags::UpdateCurrentAlarms2_ObjPtr));
}
