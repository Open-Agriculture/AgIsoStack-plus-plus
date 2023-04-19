//================================================================================================
/// @file isobus_functionalities.cpp
///
/// @brief Implements the management of the ISOBUS control function functionalities message.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/isobus_functionalities.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <limits>

namespace isobus
{
	void ControlFunctionFunctionalities::set_functionality_is_supported(Functionalities functionality, std::uint8_t functionalityGeneration, bool isSupported)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(functionality);

		if (supportedFunctionalities.end() == existingFunctionality)
		{
			FunctionalityData newFunctionality(functionality);
			newFunctionality.configure_default_data();
			newFunctionality.generation = functionalityGeneration;
			supportedFunctionalities.emplace_back(newFunctionality);
		}
	}

	bool ControlFunctionFunctionalities::get_functionality_is_supported(Functionalities functionality)
	{
		bool retVal = false;
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(functionality);

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			retVal = true;
		}
		return retVal;
	}

	std::uint8_t ControlFunctionFunctionalities::get_functionality_generation(Functionalities functionality)
	{
		std::uint8_t retVal = 0;

		auto existingFunctionality = get_functionality(functionality);

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			retVal = existingFunctionality->generation;
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::set_aux_O_inputs_option_state(AuxOOptions option, bool optionState)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::AuxOInputs);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < AuxOOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(0, static_cast<std::uint8_t>(option), optionState);
		}
	}

	void ControlFunctionFunctionalities::set_aux_O_functions_option_state(AuxOOptions option, bool optionState)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::AuxOFunctions);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < AuxOOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(0, static_cast<std::uint8_t>(option), optionState);
		}
	}

	void ControlFunctionFunctionalities::set_aux_N_inputs_option_state(AuxNOptions option, bool optionState)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::AuxNInputs);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < AuxNOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(option > AuxNOptions::SupportsType7Function ? 1 : 0,
			                                         option > AuxNOptions::SupportsType7Function ? static_cast<std::uint8_t>(static_cast<std::uint16_t>(option) >> 8) : static_cast<std::uint8_t>(option),
			                                         optionState);
		}
	}

	void ControlFunctionFunctionalities::set_aux_N_functions_option_state(AuxNOptions option, bool optionState)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::AuxNFunctions);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < AuxNOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(option > AuxNOptions::SupportsType7Function ? 1 : 0,
			                                         option > AuxNOptions::SupportsType7Function ? static_cast<std::uint8_t>(static_cast<std::uint16_t>(option) >> 8) : static_cast<std::uint8_t>(option),
			                                         optionState);
		}
	}

	void ControlFunctionFunctionalities::set_task_controller_geo_server_option_state(TaskControllerGeoServerOptions option, bool optionState)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TaskControllerGeoServer);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < TaskControllerGeoServerOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(0, static_cast<std::uint8_t>(option), optionState);
		}
	}

	void ControlFunctionFunctionalities::set_task_controller_geo_client_option(std::uint8_t numberOfControlChannels)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TaskControllerGeoClient);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (numberOfControlChannels > 0))
		{
			existingFunctionality->serializedValue.at(0) = numberOfControlChannels;
		}
	}

	void ControlFunctionFunctionalities::set_task_controller_section_control_server_option_state(std::uint8_t numberOfSupportedBooms, std::uint8_t numberOfSupportedSections, bool optionState)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TaskControllerSectionControlServer);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (numberOfSupportedBooms > 0) &&
		    (numberOfSupportedSections > 0))
		{
			existingFunctionality->serializedValue.at(0) = numberOfSupportedBooms;
			existingFunctionality->serializedValue.at(1) = numberOfSupportedSections;
		}
	}

	void ControlFunctionFunctionalities::set_task_controller_section_control_client_option_state(std::uint8_t numberOfSupportedBooms, std::uint8_t numberOfSupportedSections, bool optionState)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TaskControllerSectionControlClient);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (numberOfSupportedBooms > 0) &&
		    (numberOfSupportedSections > 0))
		{
			existingFunctionality->serializedValue.at(0) = numberOfSupportedBooms;
			existingFunctionality->serializedValue.at(1) = numberOfSupportedSections;
		}
	}

	void ControlFunctionFunctionalities::set_basic_tractor_ECU_server_option_state(BasicTractorECUOptions option, bool optionState)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::BasicTractorECUServer);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < BasicTractorECUOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(0, static_cast<std::uint8_t>(option), optionState);
		}
	}

	void ControlFunctionFunctionalities::set_basic_tractor_ECU_implement_client_option_state(BasicTractorECUOptions option, bool optionState)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::BasicTractorECUImplementClient);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < BasicTractorECUOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(0, static_cast<std::uint8_t>(option), optionState);
		}
	}
	void ControlFunctionFunctionalities::set_tractor_implement_management_server_option_state(TractorImplementManagementOptions option, bool optionState)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementServer);

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			existingFunctionality->set_bit_in_option(get_tim_option_byte_index(option), get_tim_option_bit_index(option), optionState);
		}
	}

	void ControlFunctionFunctionalities::set_tractor_implement_management_server_aux_valve_option(std::uint8_t auxValveIndex, bool stateSupported, bool flowSupported)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementServer);

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			existingFunctionality->set_bit_in_option(auxValveIndex / 4, 2 * (auxValveIndex % 4), stateSupported);
			existingFunctionality->set_bit_in_option(auxValveIndex / 4, 2 * (auxValveIndex % 4) + 1, flowSupported);
		}
	}

	void ControlFunctionFunctionalities::set_tractor_implement_management_client_option_state(TractorImplementManagementOptions option, bool optionState)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementClient);

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			existingFunctionality->set_bit_in_option(get_tim_option_byte_index(option), get_tim_option_bit_index(option), optionState);
		}
	}

	void ControlFunctionFunctionalities::set_tractor_implement_management_client_aux_valve_option(std::uint8_t auxValveIndex, bool stateSupported, bool flowSupported)
	{
		const std::lock_guard<std::mutex> lock(functionalitiesMutex);
	}

	ControlFunctionFunctionalities::FunctionalityData::FunctionalityData(Functionalities functionalityToStore) :
	  functionality(functionalityToStore)
	{
	}

	void ControlFunctionFunctionalities::FunctionalityData::configure_default_data()
	{
		switch (functionality)
		{
			case Functionalities::MinimumControlFunction:
			case Functionalities::UniversalTerminalServer:
			case Functionalities::UniversalTerminalWorkingSet:
			case Functionalities::AuxOInputs:
			case Functionalities::AuxOFunctions:
			case Functionalities::TaskControllerBasicServer:
			case Functionalities::TaskControllerBasicClient:
			case Functionalities::TaskControllerGeoServer:
			case Functionalities::TaskControllerGeoClient:
			case Functionalities::BasicTractorECUServer:
			case Functionalities::BasicTractorECUImplementClient:
			case Functionalities::FileServer:
			case Functionalities::FileServerClient:
			{
				serializedValue.resize(1);
				serializedValue.at(0) = 0x00; // No options
			}
			break;

			case Functionalities::AuxNInputs:
			case Functionalities::AuxNFunctions:
			{
				serializedValue.resize(2);
				serializedValue.at(0) = 0x00; // No options
				serializedValue.at(1) = 0x00; // No options
			}
			break;

			case Functionalities::TaskControllerSectionControlServer:
			case Functionalities::TaskControllerSectionControlClient:
			{
				serializedValue.resize(2);
				serializedValue.at(0) = 0x01; // 1 Boom supported (1 is the minimum)
				serializedValue.at(1) = 0x01; // 1 Section Supported (1 is the minimum)
			}
			break;

			case Functionalities::TractorImplementManagementServer:
			case Functionalities::TractorImplementManagementClient:
			{
				CANStackLogger::warn("[DP]: You have configured TIM as a CF functionality, but the library doesn't support TIM at this time. Do you have an external TIM implementation?");
				serializedValue.resize(15); // TIM has a lot of options. https://www.isobus.net/isobus/option
				std::fill(serializedValue.begin(), serializedValue.end(), 0x00); // Support nothing by default
			}
			break;

			default:
			{
				CANStackLogger::error("[DP]: An invalid control function functionality was added. It's values will be ignored.");
			}
			break;
		}
	}

	void ControlFunctionFunctionalities::FunctionalityData::set_bit_in_option(std::uint8_t byteIndex, std::uint8_t bit, bool value)
	{
		if ((byteIndex < serializedValue.size()) && (bit < std::numeric_limits<std::uint8_t>::digits))
		{
			if (value)
			{
				serializedValue.at(byteIndex) = (serializedValue.at(byteIndex) | (1 << bit));
			}
			else
			{
				serializedValue.at(byteIndex) = (serializedValue.at(byteIndex) & ~(1 << bit));
			}
		}
	}

	std::list<ControlFunctionFunctionalities::FunctionalityData>::iterator ControlFunctionFunctionalities::get_functionality(Functionalities functionalityToRetreive)
	{
		return std::find_if(supportedFunctionalities.begin(), supportedFunctionalities.end(), [functionalityToRetreive](FunctionalityData &currentFunctionality) { return currentFunctionality.functionality == functionalityToRetreive; });
	}

	void ControlFunctionFunctionalities::get_message_content(std::vector<std::uint8_t> &messageData)
	{
		messageData.clear();
		messageData.push_back(0xFF); // Each control function shall respond with byte 1 set to FF
		messageData.push_back(static_cast<std::uint8_t>(supportedFunctionalities.size()));
	}

	std::uint8_t ControlFunctionFunctionalities::get_tim_option_byte_index(TractorImplementManagementOptions option) const
	{
		std::uint8_t retVal = 0xFF;

		switch (option)
		{
			case TractorImplementManagementOptions::FrontPTODisengagementIsSupported:
			case TractorImplementManagementOptions::FrontPTOEngagementCCWIsSupported:
			case TractorImplementManagementOptions::FrontPTOengagementCWIsSupported:
			case TractorImplementManagementOptions::FrontPTOspeedCCWIsSupported:
			case TractorImplementManagementOptions::FrontPTOspeedCWIsSupported:
			{
				retVal = 9;
			}
			break;

			case TractorImplementManagementOptions::RearPTODisengagementIsSupported:
			case TractorImplementManagementOptions::RearPTOEngagementCCWIsSupported:
			case TractorImplementManagementOptions::RearPTOEngagementCWIsSupported:
			case TractorImplementManagementOptions::RearPTOSpeedCCWIsSupported:
			case TractorImplementManagementOptions::RearPTOSpeedCWIsSupported:
			{
				retVal = 10;
			}
			break;

			case TractorImplementManagementOptions::FrontHitchMotionIsSupported:
			case TractorImplementManagementOptions::FrontHitchPositionIsSupported:
			{
				retVal = 11;
			}
			break;

			case TractorImplementManagementOptions::RearHitchMotionIsSupported:
			case TractorImplementManagementOptions::RearHitchPositionIsSupported:
			{
				retVal = 12;
			}
			break;

			case TractorImplementManagementOptions::VehicleSpeedInForwardDirectionIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedInReverseDirectionIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedStartMotionIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedStopMotionIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedForwardSetByServerIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedReverseSetByServerIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedChangeDirectionIsSupported:
			{
				retVal = 13;
			}
			break;

			case TractorImplementManagementOptions::GuidanceCurvatureIsSupported:
			{
				retVal = 14;
			}
			break;

			default:
			{
			}
			break;
		}
		return retVal;
	}

	std::uint8_t ControlFunctionFunctionalities::get_tim_option_bit_index(TractorImplementManagementOptions option) const
	{
		std::uint8_t retVal = 0xFF;

		switch (option)
		{
			case TractorImplementManagementOptions::FrontPTODisengagementIsSupported:
			case TractorImplementManagementOptions::RearPTODisengagementIsSupported:
			case TractorImplementManagementOptions::FrontHitchMotionIsSupported:
			case TractorImplementManagementOptions::RearHitchMotionIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedInForwardDirectionIsSupported:
			case TractorImplementManagementOptions::GuidanceCurvatureIsSupported:
			{
				retVal = 0;
			}
			break;

			case TractorImplementManagementOptions::FrontPTOEngagementCCWIsSupported:
			case TractorImplementManagementOptions::RearPTOEngagementCCWIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedInReverseDirectionIsSupported:
			{
				retVal = 1;
			}
			break;

			case TractorImplementManagementOptions::FrontHitchPositionIsSupported:
			case TractorImplementManagementOptions::RearHitchPositionIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedStartMotionIsSupported:
			{
				retVal = 2;
			}
			break;

			case TractorImplementManagementOptions::FrontPTOengagementCWIsSupported:
			case TractorImplementManagementOptions::RearPTOEngagementCWIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedStopMotionIsSupported:
			{
				retVal = 3;
			}
			break;

			case TractorImplementManagementOptions::FrontPTOspeedCCWIsSupported:
			case TractorImplementManagementOptions::RearPTOSpeedCCWIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedForwardSetByServerIsSupported:
			{
				retVal = 4;
			}
			break;

			case TractorImplementManagementOptions::FrontPTOspeedCWIsSupported:
			case TractorImplementManagementOptions::RearPTOSpeedCWIsSupported:
			case TractorImplementManagementOptions::VehicleSpeedReverseSetByServerIsSupported:
			{
				retVal = 5;
			}
			break;

			case TractorImplementManagementOptions::VehicleSpeedChangeDirectionIsSupported:
			{
				retVal = 6;
			}
			break;

			default:
			{
			}
			break;
		}
		return retVal;
	}
} // namespace isobus
