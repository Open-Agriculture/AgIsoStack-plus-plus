//================================================================================================
/// @file isobus_language_command_interface.cpp
///
/// @brief Defines a set of values found in the isobus language command message from
/// ISO11783-7 commonly used in VT and TC communication
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/isobus_language_command_interface.hpp"

#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <string>

namespace isobus
{
	LanguageCommandInterface::LanguageCommandInterface(std::shared_ptr<InternalControlFunction> sourceControlFunction, bool shouldRespondToRequests) :
	  myControlFunction(sourceControlFunction),
	  myPartner(nullptr),
	  respondToRequests(shouldRespondToRequests)
	{
	}

	LanguageCommandInterface::LanguageCommandInterface(std::shared_ptr<InternalControlFunction> sourceControlFunction, std::shared_ptr<PartneredControlFunction> filteredControlFunction) :
	  myControlFunction(sourceControlFunction),
	  myPartner(filteredControlFunction)
	{
	}

	LanguageCommandInterface ::~LanguageCommandInterface()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::LanguageCommand), process_rx_message, this);

			if (respondToRequests && (!myControlFunction->get_pgn_request_protocol().expired()))
			{
				myControlFunction->get_pgn_request_protocol().lock()->remove_pgn_request_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::LanguageCommand), on_language_request, this);
			}
		}
	}

	void LanguageCommandInterface::initialize()
	{
		if (!initialized)
		{
			if (nullptr != myControlFunction)
			{
				CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::LanguageCommand), process_rx_message, this);

				if (respondToRequests && (!myControlFunction->get_pgn_request_protocol().expired()))
				{
					myControlFunction->get_pgn_request_protocol().lock()->register_pgn_request_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::LanguageCommand), on_language_request, this);
				}
				initialized = true;
			}
			else
			{
				CANStackLogger::error("[VT/TC]: Language command interface is missing an internal control function, and will not be functional.");
			}
		}
		else
		{
			CANStackLogger::warn("[VT/TC]: Language command interface has been initialized, but is being initialized again.");
		}
	}

	void LanguageCommandInterface::set_partner(std::shared_ptr<PartneredControlFunction> filteredControlFunction)
	{
		myPartner = filteredControlFunction;
	}

	bool LanguageCommandInterface::get_initialized() const
	{
		return initialized;
	}

	bool LanguageCommandInterface::send_request_language_command() const
	{
		bool retVal = false;

		if (initialized)
		{
			retVal = ParameterGroupNumberRequestProtocol::request_parameter_group_number(static_cast<std::uint32_t>(CANLibParameterGroupNumber::LanguageCommand), myControlFunction, myPartner);
		}
		else
		{
			// Make sure you call initialize first!
			CANStackLogger::error("[VT/TC]: Language command interface is being used without being initialized!");
		}
		return retVal;
	}

	bool LanguageCommandInterface::send_language_command() const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
			static_cast<std::uint8_t>(languageCode[0]),
			static_cast<std::uint8_t>(languageCode[1]),
			static_cast<std::uint8_t>((static_cast<std::uint8_t>(timeFormat) << 4) |
			                          (static_cast<std::uint8_t>(decimalSymbol) << 6)),
			static_cast<std::uint8_t>(dateFormat),
			static_cast<std::uint8_t>(static_cast<std::uint8_t>(massUnitSystem) |
			                          (static_cast<std::uint8_t>(volumeUnitSystem) << 2) |
			                          (static_cast<std::uint8_t>(areaUnitSystem) << 4) |
			                          (static_cast<std::uint8_t>(distanceUnitSystem) << 6)),
			static_cast<std::uint8_t>(static_cast<std::uint8_t>(genericUnitSystem) |
			                          (static_cast<std::uint8_t>(forceUnitSystem) << 2) |
			                          (static_cast<std::uint8_t>(pressureUnitSystem) << 4) |
			                          (static_cast<std::uint8_t>(temperatureUnitSystem) << 6)),
			static_cast<std::uint8_t>(countryCode[0]),
			static_cast<std::uint8_t>(countryCode[1])
		};
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::LanguageCommand),
		                                                      buffer.data(),
		                                                      static_cast<std::uint32_t>(buffer.size()),
		                                                      myControlFunction,
		                                                      nullptr);
	}

	std::string LanguageCommandInterface::get_country_code() const
	{
		return countryCode;
	}

	void LanguageCommandInterface::set_country_code(std::string country)
	{
		if (country.length() > 2)
		{
			CANStackLogger::warn("[VT/TC]: Language command country code should not be more than 2 characters! It will be truncated.");
		}
		else if (country.length() < 2)
		{
			CANStackLogger::warn("[VT/TC]: Language command country code should not be less than 2 characters! It will be padded.");
		}

		while (country.length() < 2)
		{
			country.push_back(' ');
		}
		countryCode = country;
	}

	std::string LanguageCommandInterface::get_language_code() const
	{
		return languageCode;
	}

	void LanguageCommandInterface::set_language_code(std::string language)
	{
		if (language.length() > 2)
		{
			CANStackLogger::warn("[VT/TC]: Language command language code should not be more than 2 characters! It will be truncated.");
		}
		else if (language.length() < 2)
		{
			CANStackLogger::warn("[VT/TC]: Language command language code should not be less than 2 characters! It will be padded.");
		}

		while (language.length() < 2)
		{
			language.push_back(' ');
		}
		languageCode = language;
	}

	std::uint32_t LanguageCommandInterface::get_language_command_timestamp() const
	{
		return languageCommandTimestamp_ms;
	}

	LanguageCommandInterface::DecimalSymbols LanguageCommandInterface::get_commanded_decimal_symbol() const
	{
		return decimalSymbol;
	}

	void LanguageCommandInterface::set_commanded_decimal_symbol(DecimalSymbols decimals)
	{
		decimalSymbol = decimals;
	}

	LanguageCommandInterface::TimeFormats LanguageCommandInterface::get_commanded_time_format() const
	{
		return timeFormat;
	}

	void LanguageCommandInterface::set_commanded_time_format(TimeFormats format)
	{
		timeFormat = format;
	}

	LanguageCommandInterface::DateFormats LanguageCommandInterface::get_commanded_date_format() const
	{
		return dateFormat;
	}

	void LanguageCommandInterface::set_commanded_date_format(DateFormats format)
	{
		dateFormat = format;
	}

	LanguageCommandInterface::DistanceUnits LanguageCommandInterface::get_commanded_distance_units() const
	{
		return distanceUnitSystem;
	}

	void LanguageCommandInterface::set_commanded_distance_units(DistanceUnits units)
	{
		distanceUnitSystem = units;
	}

	LanguageCommandInterface::AreaUnits LanguageCommandInterface::get_commanded_area_units() const
	{
		return areaUnitSystem;
	}

	void LanguageCommandInterface::set_commanded_area_units(AreaUnits units)
	{
		areaUnitSystem = units;
	}

	LanguageCommandInterface::VolumeUnits LanguageCommandInterface::get_commanded_volume_units() const
	{
		return volumeUnitSystem;
	}

	void LanguageCommandInterface::set_commanded_volume_units(VolumeUnits units)
	{
		volumeUnitSystem = units;
	}

	LanguageCommandInterface::MassUnits LanguageCommandInterface::get_commanded_mass_units() const
	{
		return massUnitSystem;
	}

	void LanguageCommandInterface::set_commanded_mass_units(MassUnits units)
	{
		massUnitSystem = units;
	}

	LanguageCommandInterface::TemperatureUnits LanguageCommandInterface::get_commanded_temperature_units() const
	{
		return temperatureUnitSystem;
	}

	void LanguageCommandInterface::set_commanded_temperature_units(TemperatureUnits units)
	{
		temperatureUnitSystem = units;
	}

	LanguageCommandInterface::PressureUnits LanguageCommandInterface::get_commanded_pressure_units() const
	{
		return pressureUnitSystem;
	}

	void LanguageCommandInterface::set_commanded_pressure_units(PressureUnits units)
	{
		pressureUnitSystem = units;
	}

	LanguageCommandInterface::ForceUnits LanguageCommandInterface::get_commanded_force_units() const
	{
		return forceUnitSystem;
	}

	void LanguageCommandInterface::set_commanded_force_units(ForceUnits units)
	{
		forceUnitSystem = units;
	}

	LanguageCommandInterface::UnitSystem LanguageCommandInterface::get_commanded_generic_units() const
	{
		return genericUnitSystem;
	}

	void LanguageCommandInterface::set_commanded_generic_units(UnitSystem units)
	{
		genericUnitSystem = units;
	}

	const std::array<std::uint8_t, 7> LanguageCommandInterface::get_localization_raw_data() const
	{
		std::array<std::uint8_t, 7> retVal = { 0 };

		if (languageCode.size() >= 2)
		{
			retVal[0] = languageCode[0];
			retVal[1] = languageCode[1];
		}
		else
		{
			retVal[0] = ' ';
			retVal[1] = ' ';
		}
		retVal[2] = ((static_cast<std::uint8_t>(timeFormat) << 4) |
		             (static_cast<std::uint8_t>(decimalSymbol) << 6));
		retVal[3] = static_cast<std::uint8_t>(dateFormat);
		retVal[4] = (static_cast<std::uint8_t>(massUnitSystem) |
		             (static_cast<std::uint8_t>(volumeUnitSystem) << 2) |
		             (static_cast<std::uint8_t>(areaUnitSystem) << 4) |
		             (static_cast<std::uint8_t>(distanceUnitSystem) << 6));
		retVal[5] = (static_cast<std::uint8_t>(genericUnitSystem) |
		             (static_cast<std::uint8_t>(forceUnitSystem) << 2) |
		             (static_cast<std::uint8_t>(pressureUnitSystem) << 4) |
		             (static_cast<std::uint8_t>(temperatureUnitSystem) << 6));
		retVal[6] = 0xFF;
		return retVal;
	}

	void LanguageCommandInterface::process_rx_message(const CANMessage &message, void *parentPointer)
	{
		auto *parentInterface = reinterpret_cast<LanguageCommandInterface *>(parentPointer);

		if ((nullptr != parentInterface) &&
		    (CAN_DATA_LENGTH <= message.get_data_length()) &&
		    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::LanguageCommand) == message.get_identifier().get_parameter_group_number()) &&
		    ((nullptr == parentInterface->myPartner) ||
		     (message.get_source_control_function()->get_NAME() == parentInterface->myPartner->get_NAME())))
		{
			const auto &data = message.get_data();
			parentInterface->languageCommandTimestamp_ms = SystemTiming::get_timestamp_ms();
			parentInterface->languageCode.clear();
			parentInterface->languageCode.push_back(static_cast<char>(data.at(0)));
			parentInterface->languageCode.push_back(static_cast<char>(data.at(1)));
			parentInterface->timeFormat = static_cast<TimeFormats>((data.at(2) >> 4) & 0x03);
			parentInterface->decimalSymbol = static_cast<DecimalSymbols>((data.at(2) >> 6) & 0x03);
			parentInterface->dateFormat = static_cast<DateFormats>(data.at(3));
			parentInterface->massUnitSystem = static_cast<MassUnits>(data.at(4) & 0x03);
			parentInterface->volumeUnitSystem = static_cast<VolumeUnits>((data.at(4) >> 2) & 0x03);
			parentInterface->areaUnitSystem = static_cast<AreaUnits>((data.at(4) >> 4) & 0x03);
			parentInterface->distanceUnitSystem = static_cast<DistanceUnits>((data.at(4) >> 6) & 0x03);
			parentInterface->genericUnitSystem = static_cast<UnitSystem>(data.at(5) & 0x03);
			parentInterface->forceUnitSystem = static_cast<ForceUnits>((data.at(5) >> 2) & 0x03);
			parentInterface->pressureUnitSystem = static_cast<PressureUnits>((data.at(5) >> 4) & 0x03);
			parentInterface->temperatureUnitSystem = static_cast<TemperatureUnits>((data.at(5) >> 6) & 0x03);
			parentInterface->countryCode.clear();

			if ((0xFF != data.at(6)) || (0xFF != data.at(7)))
			{
				parentInterface->countryCode.push_back(static_cast<char>(data.at(6)));
				parentInterface->countryCode.push_back(static_cast<char>(data.at(7)));
			}

			CANStackLogger::debug("[VT/TC]: Language and unit data received from control function " +
			                        isobus::to_string(static_cast<int>(message.get_identifier().get_source_address())) +
			                        " language is: " +
			                        parentInterface->languageCode.c_str(),
			                      " and country code is ",
			                      parentInterface->countryCode.empty() ? "unknown." : parentInterface->countryCode.c_str());
		}
	}

	bool LanguageCommandInterface::on_language_request(std::uint32_t parameterGroupNumber,
	                                                   std::shared_ptr<ControlFunction>,
	                                                   bool &acknowledge,
	                                                   AcknowledgementType &acknowledgeType,
	                                                   void *parentPointer)
	{
		bool retVal = false;

		if ((nullptr != parentPointer) && (static_cast<std::uint32_t>(CANLibParameterGroupNumber::LanguageCommand) == parameterGroupNumber))
		{
			auto targetInterface = static_cast<LanguageCommandInterface *>(parentPointer);
			acknowledge = false;
			acknowledgeType = AcknowledgementType::Positive;
			retVal = true;
			targetInterface->send_language_command();
		}
		return retVal;
	}
}
