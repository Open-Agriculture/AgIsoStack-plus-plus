//================================================================================================
/// @file isobus_functionalities.cpp
///
/// @brief Implements the management of the ISOBUS control function functionalities message.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_functionalities.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

namespace isobus
{
	ControlFunctionFunctionalities::ControlFunctionFunctionalities(std::shared_ptr<InternalControlFunction> sourceControlFunction) :
	  myControlFunction(sourceControlFunction),
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberOfFlags), process_flags, this)
	{
		set_functionality_is_supported(Functionalities::MinimumControlFunction, 1, true); // Support the absolute minimum by default

		if (auto pgnRequestProtocol = sourceControlFunction->get_pgn_request_protocol().lock())
		{
			pgnRequestProtocol->register_pgn_request_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ControlFunctionFunctionalities), pgn_request_handler, this);
		}
		else
		{
			LOG_ERROR("[DP]: Failed to register PGN request callback for ControlFunctionFunctionalities due to the protocol being expired");
		}
	}

	ControlFunctionFunctionalities::~ControlFunctionFunctionalities()
	{
		if (auto pgnRequestProtocol = myControlFunction->get_pgn_request_protocol().lock())
		{
			pgnRequestProtocol->remove_pgn_request_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ControlFunctionFunctionalities), pgn_request_handler, this);
		}
	}

	void ControlFunctionFunctionalities::set_functionality_is_supported(Functionalities functionality, std::uint8_t functionalityGeneration, bool isSupported)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(functionality);

		if ((supportedFunctionalities.end() == existingFunctionality) && isSupported)
		{
			FunctionalityData newFunctionality(functionality);
			newFunctionality.configure_default_data();
			newFunctionality.generation = functionalityGeneration;
			supportedFunctionalities.emplace_back(newFunctionality);
		}
		else if ((supportedFunctionalities.end() != existingFunctionality) && (!isSupported))
		{
			if (Functionalities::MinimumControlFunction == functionality)
			{
				LOG_WARNING("[DP]: You are disabling minimum control function functionality reporting! This is not recommended.");
			}
			supportedFunctionalities.erase(existingFunctionality);
		}
	}

	bool ControlFunctionFunctionalities::get_functionality_is_supported(Functionalities functionality)
	{
		bool retVal = false;
		LOCK_GUARD(Mutex, functionalitiesMutex);

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

	void ControlFunctionFunctionalities::set_minimum_control_function_option_state(MinimumControlFunctionOptions option, bool optionState)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::MinimumControlFunction);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < MinimumControlFunctionOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(0, static_cast<std::uint8_t>(option), optionState);
		}
	}

	bool ControlFunctionFunctionalities::get_minimum_control_function_option_state(MinimumControlFunctionOptions option)
	{
		return get_functionality_byte_option(Functionalities::MinimumControlFunction, 0, static_cast<std::uint8_t>(option));
	}

	void ControlFunctionFunctionalities::set_aux_O_inputs_option_state(AuxOOptions option, bool optionState)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::AuxOInputs);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < AuxOOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(0, static_cast<std::uint8_t>(option), optionState);
		}
	}

	bool ControlFunctionFunctionalities::get_aux_O_inputs_option_state(AuxOOptions option)
	{
		return get_functionality_byte_option(Functionalities::AuxOInputs, 0, static_cast<std::uint8_t>(option));
	}

	void ControlFunctionFunctionalities::set_aux_O_functions_option_state(AuxOOptions option, bool optionState)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::AuxOFunctions);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < AuxOOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(0, static_cast<std::uint8_t>(option), optionState);
		}
	}

	bool ControlFunctionFunctionalities::get_aux_O_functions_option_state(AuxOOptions option)
	{
		return get_functionality_byte_option(Functionalities::AuxOFunctions, 0, static_cast<std::uint8_t>(option));
	}

	void ControlFunctionFunctionalities::set_aux_N_inputs_option_state(AuxNOptions option, bool optionState)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::AuxNInputs);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < AuxNOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(option > AuxNOptions::SupportsType7Function ? 1 : 0,
			                                         option > AuxNOptions::SupportsType7Function ? static_cast<std::uint8_t>(static_cast<std::uint16_t>(option) >> 8) : static_cast<std::uint8_t>(option),
			                                         optionState);
		}
	}

	bool ControlFunctionFunctionalities::get_aux_N_inputs_option_state(AuxNOptions option)
	{
		return get_functionality_byte_option(Functionalities::AuxNInputs, 0, static_cast<std::uint8_t>(option));
	}

	void ControlFunctionFunctionalities::set_aux_N_functions_option_state(AuxNOptions option, bool optionState)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::AuxNFunctions);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < AuxNOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(option > AuxNOptions::SupportsType7Function ? 1 : 0,
			                                         option > AuxNOptions::SupportsType7Function ? static_cast<std::uint8_t>(static_cast<std::uint16_t>(option) >> 8) : static_cast<std::uint8_t>(option),
			                                         optionState);
		}
	}

	bool ControlFunctionFunctionalities::get_aux_N_functions_option_state(AuxNOptions option)
	{
		return get_functionality_byte_option(Functionalities::AuxNFunctions, 0, static_cast<std::uint8_t>(option));
	}

	void ControlFunctionFunctionalities::set_task_controller_geo_server_option_state(TaskControllerGeoServerOptions option, bool optionState)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TaskControllerGeoServer);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < TaskControllerGeoServerOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(0, static_cast<std::uint8_t>(option), optionState);
		}
	}

	bool ControlFunctionFunctionalities::get_task_controller_geo_server_option_state(TaskControllerGeoServerOptions option)
	{
		return get_functionality_byte_option(Functionalities::TaskControllerGeoServer, 0, static_cast<std::uint8_t>(option));
	}

	void ControlFunctionFunctionalities::set_task_controller_geo_client_option(std::uint8_t numberOfControlChannels)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TaskControllerGeoClient);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (numberOfControlChannels > 0))
		{
			existingFunctionality->serializedValue.at(0) = numberOfControlChannels;
		}
	}

	std::uint8_t ControlFunctionFunctionalities::get_task_controller_geo_client_option()
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		auto existingFunctionality = get_functionality(Functionalities::TaskControllerGeoClient);
		std::uint8_t retVal = 0;

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			retVal = existingFunctionality->serializedValue.at(0);
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::set_task_controller_section_control_server_option_state(std::uint8_t numberOfSupportedBooms, std::uint8_t numberOfSupportedSections)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TaskControllerSectionControlServer);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (numberOfSupportedBooms > 0) &&
		    (numberOfSupportedSections > 0))
		{
			existingFunctionality->serializedValue.at(0) = numberOfSupportedBooms;
			existingFunctionality->serializedValue.at(1) = numberOfSupportedSections;
		}
	}

	std::uint8_t ControlFunctionFunctionalities::get_task_controller_section_control_server_number_supported_booms()
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		auto existingFunctionality = get_functionality(Functionalities::TaskControllerSectionControlServer);
		std::uint8_t retVal = 0;

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			retVal = existingFunctionality->serializedValue.at(0);
		}
		return retVal;
	}

	std::uint8_t ControlFunctionFunctionalities::get_task_controller_section_control_server_number_supported_sections()
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		auto existingFunctionality = get_functionality(Functionalities::TaskControllerSectionControlServer);
		std::uint8_t retVal = 0;

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			retVal = existingFunctionality->serializedValue.at(1);
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::set_task_controller_section_control_client_option_state(std::uint8_t numberOfSupportedBooms, std::uint8_t numberOfSupportedSections)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TaskControllerSectionControlClient);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (numberOfSupportedBooms > 0) &&
		    (numberOfSupportedSections > 0))
		{
			existingFunctionality->serializedValue.at(0) = numberOfSupportedBooms;
			existingFunctionality->serializedValue.at(1) = numberOfSupportedSections;
		}
	}

	std::uint8_t ControlFunctionFunctionalities::get_task_controller_section_control_client_number_supported_booms()
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		auto existingFunctionality = get_functionality(Functionalities::TaskControllerSectionControlClient);
		std::uint8_t retVal = 0;

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			retVal = existingFunctionality->serializedValue.at(0);
		}
		return retVal;
	}

	std::uint8_t ControlFunctionFunctionalities::get_task_controller_section_control_client_number_supported_sections()
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		auto existingFunctionality = get_functionality(Functionalities::TaskControllerSectionControlClient);
		std::uint8_t retVal = 0;

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			retVal = existingFunctionality->serializedValue.at(1);
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::set_basic_tractor_ECU_server_option_state(BasicTractorECUOptions option, bool optionState)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::BasicTractorECUServer);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < BasicTractorECUOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(0, static_cast<std::uint8_t>(option), optionState);
		}
	}

	bool ControlFunctionFunctionalities::get_basic_tractor_ECU_server_option_state(BasicTractorECUOptions option)
	{
		bool retVal = false;

		// This one is handled differently to handle the 0 value
		if (BasicTractorECUOptions::TECUNotMeetingCompleteClass1Requirements == option)
		{
			LOCK_GUARD(Mutex, functionalitiesMutex);
			auto existingFunctionality = get_functionality(Functionalities::BasicTractorECUServer);

			if (supportedFunctionalities.end() != existingFunctionality)
			{
				retVal = (0 == existingFunctionality->serializedValue.at(0));
			}
		}
		else
		{
			retVal = get_functionality_byte_option(Functionalities::BasicTractorECUServer, 0, static_cast<std::uint8_t>(option));
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::set_basic_tractor_ECU_implement_client_option_state(BasicTractorECUOptions option, bool optionState)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::BasicTractorECUImplementClient);

		if ((supportedFunctionalities.end() != existingFunctionality) &&
		    (option < BasicTractorECUOptions::Reserved))
		{
			existingFunctionality->set_bit_in_option(0, static_cast<std::uint8_t>(option), optionState);
		}
	}

	bool ControlFunctionFunctionalities::get_basic_tractor_ECU_implement_client_option_state(BasicTractorECUOptions option)
	{
		bool retVal = false;

		// This one is handled differently to handle the 0 value
		if (BasicTractorECUOptions::TECUNotMeetingCompleteClass1Requirements == option)
		{
			LOCK_GUARD(Mutex, functionalitiesMutex);
			auto existingFunctionality = get_functionality(Functionalities::BasicTractorECUImplementClient);

			if (supportedFunctionalities.end() != existingFunctionality)
			{
				retVal = (0 == existingFunctionality->serializedValue.at(0));
			}
		}
		else
		{
			retVal = get_functionality_byte_option(Functionalities::BasicTractorECUImplementClient, 0, static_cast<std::uint8_t>(option));
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::set_tractor_implement_management_server_option_state(TractorImplementManagementOptions option, bool optionState)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementServer);

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			if (TractorImplementManagementOptions::NoOptions != option)
			{
				existingFunctionality->set_bit_in_option(get_tim_option_byte_index(option), 1 << get_tim_option_bit_index(option), optionState);
			}
			else
			{
				LOG_DEBUG("[DP]: Can't set the No Options TIM option, disable the other ones instead.");
			}
		}
	}

	bool ControlFunctionFunctionalities::get_tractor_implement_management_server_option_state(TractorImplementManagementOptions option)
	{
		bool retVal = true;

		if (TractorImplementManagementOptions::NoOptions == option)
		{
			LOCK_GUARD(Mutex, functionalitiesMutex);
			auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementServer);

			if (supportedFunctionalities.end() != existingFunctionality)
			{
				for (const auto &currentByte : existingFunctionality->serializedValue)
				{
					retVal &= (0 == currentByte);
				}
			}
		}
		else
		{
			std::uint8_t optionBit = get_tim_option_bit_index(option);

			if (optionBit < 0xFF)
			{
				retVal = get_functionality_byte_option(Functionalities::TractorImplementManagementServer, get_tim_option_byte_index(option), 1 << get_tim_option_bit_index(option));
			}
			else
			{
				retVal = false;
			}
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::set_tractor_implement_management_server_aux_valve_option(std::uint8_t auxValveIndex, bool stateSupported, bool flowSupported)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementServer);

		if ((supportedFunctionalities.end() != existingFunctionality) && (auxValveIndex < NUMBER_TIM_AUX_VALVES))
		{
			existingFunctionality->set_bit_in_option(auxValveIndex / 4, 1 << (2 * (auxValveIndex % 4)), stateSupported);
			existingFunctionality->set_bit_in_option(auxValveIndex / 4, 1 << (2 * (auxValveIndex % 4) + 1), flowSupported);
		}
	}

	bool ControlFunctionFunctionalities::get_tractor_implement_management_server_aux_valve_state_supported(std::uint8_t auxValveIndex)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		bool retVal = false;

		auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementServer);

		if ((supportedFunctionalities.end() != existingFunctionality) && (auxValveIndex < NUMBER_TIM_AUX_VALVES))
		{
			retVal = existingFunctionality->get_bit_in_option(auxValveIndex / 4, 1 << (2 * (auxValveIndex % 4)));
		}
		return retVal;
	}

	bool ControlFunctionFunctionalities::get_tractor_implement_management_server_aux_valve_flow_supported(std::uint8_t auxValveIndex)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		bool retVal = false;

		auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementServer);

		if ((supportedFunctionalities.end() != existingFunctionality) && (auxValveIndex < NUMBER_TIM_AUX_VALVES))
		{
			retVal = existingFunctionality->get_bit_in_option(auxValveIndex / 4, 1 << (2 * (auxValveIndex % 4) + 1));
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::set_tractor_implement_management_client_option_state(TractorImplementManagementOptions option, bool optionState)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);

		auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementClient);

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			existingFunctionality->set_bit_in_option(get_tim_option_byte_index(option), 1 << get_tim_option_bit_index(option), optionState);
		}
	}

	bool ControlFunctionFunctionalities::get_tractor_implement_management_client_option_state(TractorImplementManagementOptions option)
	{
		bool retVal = true;

		if (TractorImplementManagementOptions::NoOptions == option)
		{
			LOCK_GUARD(Mutex, functionalitiesMutex);
			auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementClient);

			if (supportedFunctionalities.end() != existingFunctionality)
			{
				for (const auto &currentByte : existingFunctionality->serializedValue)
				{
					retVal &= (0 == currentByte);
				}
			}
		}
		else
		{
			std::uint8_t optionBit = get_tim_option_bit_index(option);

			if (optionBit < std::numeric_limits<std::uint8_t>::digits)
			{
				retVal = get_functionality_byte_option(Functionalities::TractorImplementManagementClient, get_tim_option_byte_index(option), 1 << get_tim_option_bit_index(option));
			}
			else
			{
				// This should be impossible, if you see this please report a bug on our GitHub.
				// Generally this means that a new TIM option was added but not considered in get_tim_option_bit_index.
				assert(false);
				retVal = false;
			}
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::set_tractor_implement_management_client_aux_valve_option(std::uint8_t auxValveIndex, bool stateSupported, bool flowSupported)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementClient);

		if ((supportedFunctionalities.end() != existingFunctionality) && (auxValveIndex < NUMBER_TIM_AUX_VALVES))
		{
			existingFunctionality->set_bit_in_option(auxValveIndex / 4, 1 << (2 * (auxValveIndex % 4)), stateSupported);
			existingFunctionality->set_bit_in_option(auxValveIndex / 4, 1 << (2 * (auxValveIndex % 4) + 1), flowSupported);
		}
	}

	bool ControlFunctionFunctionalities::get_tractor_implement_management_client_aux_valve_state_supported(std::uint8_t auxValveIndex)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		bool retVal = false;

		auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementClient);

		if ((supportedFunctionalities.end() != existingFunctionality) && (auxValveIndex < NUMBER_TIM_AUX_VALVES))
		{
			retVal = existingFunctionality->get_bit_in_option(auxValveIndex / 4, 1 << (2 * (auxValveIndex % 4)));
		}
		return retVal;
	}

	bool ControlFunctionFunctionalities::get_tractor_implement_management_client_aux_valve_flow_supported(std::uint8_t auxValveIndex)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		bool retVal = false;

		auto existingFunctionality = get_functionality(Functionalities::TractorImplementManagementClient);

		if ((supportedFunctionalities.end() != existingFunctionality) && (auxValveIndex < NUMBER_TIM_AUX_VALVES))
		{
			retVal = existingFunctionality->get_bit_in_option(auxValveIndex / 4, 1 << (2 * (auxValveIndex % 4) + 1));
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::update()
	{
		txFlags.process_all_flags();
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
				LOG_WARNING("[DP]: You have configured TIM as a CF functionality, but the library doesn't support TIM at this time. Do you have an external TIM implementation?");
				serializedValue.resize(15); // TIM has a lot of options. https://www.isobus.net/isobus/option
				std::fill(serializedValue.begin(), serializedValue.end(), 0x00); // Support nothing by default
			}
			break;

			default:
			{
				LOG_ERROR("[DP]: An invalid control function functionality was added. It's values will be ignored.");
			}
			break;
		}
	}

	void ControlFunctionFunctionalities::FunctionalityData::set_bit_in_option(std::uint8_t byteIndex, std::uint8_t bit, bool value)
	{
		if ((byteIndex < serializedValue.size()) && (bit < std::numeric_limits<std::uint8_t>::max()))
		{
			if (value)
			{
				serializedValue.at(byteIndex) = (serializedValue.at(byteIndex) | bit);
			}
			else
			{
				serializedValue.at(byteIndex) = (serializedValue.at(byteIndex) & ~bit);
			}
		}
	}

	bool ControlFunctionFunctionalities::FunctionalityData::get_bit_in_option(std::uint8_t byteIndex, std::uint8_t bit)
	{
		bool retVal = false;

		if ((byteIndex < serializedValue.size()) && (bit < std::numeric_limits<std::uint8_t>::max()))
		{
			retVal = (0 != (serializedValue.at(byteIndex) & bit));
		}
		return retVal;
	}

	std::list<ControlFunctionFunctionalities::FunctionalityData>::iterator ControlFunctionFunctionalities::get_functionality(Functionalities functionalityToRetrieve)
	{
		return std::find_if(supportedFunctionalities.begin(), supportedFunctionalities.end(), [functionalityToRetrieve](const FunctionalityData &currentFunctionality) { return currentFunctionality.functionality == functionalityToRetrieve; });
	}

	bool ControlFunctionFunctionalities::get_functionality_byte_option(Functionalities functionality, std::uint8_t byteIndex, std::uint8_t option)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		bool retVal = false;

		auto existingFunctionality = get_functionality(functionality);

		if (supportedFunctionalities.end() != existingFunctionality)
		{
			retVal = existingFunctionality->get_bit_in_option(byteIndex, option);
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::get_message_content(std::vector<std::uint8_t> &messageData)
	{
		LOCK_GUARD(Mutex, functionalitiesMutex);
		messageData.clear();
		messageData.reserve(supportedFunctionalities.size() * 4); // Approximate, but pretty close unless you have TIM.
		messageData.push_back(0xFF); // Each control function shall respond with byte 1 set to FF
		messageData.push_back(static_cast<std::uint8_t>(supportedFunctionalities.size()));

		for (const auto &functionality : supportedFunctionalities)
		{
			messageData.push_back(static_cast<std::uint8_t>(functionality.functionality));
			messageData.push_back(functionality.generation);
			messageData.push_back(static_cast<std::uint8_t>(functionality.serializedValue.size()));

			for (const auto &dataByte : functionality.serializedValue)
			{
				messageData.push_back(dataByte);
			}
		}

		while (messageData.size() < CAN_DATA_LENGTH)
		{
			messageData.push_back(0xFF);
		}
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
				// Option not handled. This is normal for NoOption but others should be considered.
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
				// Option not handled. This is normal for NoOption but others should be considered.
			}
			break;
		}
		return retVal;
	}

	bool ControlFunctionFunctionalities::pgn_request_handler(std::uint32_t parameterGroupNumber,
	                                                         std::shared_ptr<ControlFunction>,
	                                                         bool &acknowledge,
	                                                         AcknowledgementType &,
	                                                         void *parentPointer)
	{
		assert(nullptr != parentPointer);
		auto targetInterface = static_cast<ControlFunctionFunctionalities *>(parentPointer);
		bool retVal = false;

		if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::ControlFunctionFunctionalities) == parameterGroupNumber)
		{
			acknowledge = false;
			targetInterface->txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::ControlFunctionFunctionalitiesMessage));
			retVal = true;
		}
		return retVal;
	}

	void ControlFunctionFunctionalities::process_flags(std::uint32_t flag, void *parentPointer)
	{
		assert(nullptr != parentPointer);
		auto targetInterface = static_cast<ControlFunctionFunctionalities *>(parentPointer);
		bool transmitSuccessful = true;

		if (static_cast<std::uint32_t>(TransmitFlags::ControlFunctionFunctionalitiesMessage) == flag)
		{
			std::vector<std::uint8_t> messageBuffer;
			targetInterface->get_message_content(messageBuffer);
			transmitSuccessful = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ControlFunctionFunctionalities),
			                                                                    messageBuffer.data(),
			                                                                    messageBuffer.size(),
			                                                                    targetInterface->myControlFunction,
			                                                                    nullptr);
		}

		if (!transmitSuccessful)
		{
			targetInterface->txFlags.set_flag(flag);
		}
	}
} // namespace isobus
