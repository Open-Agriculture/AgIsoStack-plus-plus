//================================================================================================
/// @file isobus_time_date_interface.cpp
///
/// @brief Implements an interface to handle to transmit the time and date information using the
/// Time/Date (TD) PGN.
///
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_time_date_interface.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#include <cassert>

#ifndef DISABLE_CAN_STACK_LOGGER
#include <iomanip>
#include <sstream>
#endif

namespace isobus
{
	TimeDateInterface::TimeDateInterface(std::shared_ptr<InternalControlFunction> sourceControlFunction, const std::function<bool(TimeAndDate &timeAndDateToPopulate)> &timeAndDateCallback) :
	  myControlFunction(sourceControlFunction),
	  userTimeDateCallback(timeAndDateCallback)
	{
		if (nullptr != sourceControlFunction)
		{
			assert(nullptr != timeAndDateCallback); // You need a callback to populate the time and date information... the interface needs to know the time and date to send it out on the bus!
		}
	}

	TimeDateInterface::~TimeDateInterface()
	{
		if (initialized && (nullptr != myControlFunction))
		{
			auto pgnRequestProtocol = myControlFunction->get_pgn_request_protocol().lock();

			if (nullptr != pgnRequestProtocol)
			{
				pgnRequestProtocol->remove_pgn_request_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TimeDate), process_request_for_time_date, this);
			}
		}
	}

	void TimeDateInterface::initialize()
	{
		if (!initialized)
		{
			CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TimeDate),
			                                                                         process_rx_message,
			                                                                         this);

			if (nullptr != myControlFunction)
			{
				auto pgnRequestProtocol = myControlFunction->get_pgn_request_protocol().lock();

				if (nullptr != pgnRequestProtocol)
				{
					pgnRequestProtocol->register_pgn_request_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TimeDate), process_request_for_time_date, this);
				}
			}
			initialized = true;
		}
	}

	bool TimeDateInterface::is_initialized() const
	{
		return initialized;
	}

	EventDispatcher<TimeDateInterface::TimeAndDateInformation> &TimeDateInterface::get_event_dispatcher()
	{
		return timeAndDateEventDispatcher;
	}

	bool TimeDateInterface::send_time_and_date(const TimeAndDate &timeAndDateToSend) const
	{
		// If you hit any of these assertions, it's because you are trying to send an invalid time and date.
		// Sending invalid values on the network is bad.
		// Please check the values you are trying to send and make sure they are within the valid ranges noted below.
		// These values can also be found in the ISO11783-7 standard (on isobus.net), or in the J1939 standard.
		// Also, please only send the time and date if you have a good RTC or GPS source. Sending bad time and date information can cause issues for other devices on the network.
		assert(timeAndDateToSend.year >= 1985 && timeAndDateToSend.year <= 2235); // The year must be between 1985 and 2235
		assert(timeAndDateToSend.month >= 1 && timeAndDateToSend.month <= 12); // The month must be between 1 and 12
		assert(timeAndDateToSend.day <= 31); // The day must be between 0 and 31
		assert(timeAndDateToSend.hours <= 23); // The hours must be between 0 and 23
		assert(timeAndDateToSend.minutes <= 59); // The minutes must be between 0 and 59
		assert(timeAndDateToSend.seconds <= 59); // The seconds must be between 0 and 59
		assert(timeAndDateToSend.quarterDays <= 3); // The quarter days must be between 0 and 3
		assert(timeAndDateToSend.milliseconds == 0 || timeAndDateToSend.milliseconds == 250 || timeAndDateToSend.milliseconds == 500 || timeAndDateToSend.milliseconds == 750); // The milliseconds must be 0, 250, 500, or 750
		assert(timeAndDateToSend.localHourOffset >= -23 && timeAndDateToSend.localHourOffset <= 23); // The local hour offset must be between -23 and 23
		assert(timeAndDateToSend.localMinuteOffset >= -59 && timeAndDateToSend.localMinuteOffset <= 59); // The local minute offset must be between -59 and 59

		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
			static_cast<std::uint8_t>(timeAndDateToSend.seconds * 4 + (timeAndDateToSend.milliseconds / 250)),
			timeAndDateToSend.minutes,
			timeAndDateToSend.hours,
			timeAndDateToSend.month,
			static_cast<std::uint8_t>(timeAndDateToSend.day * 4 + timeAndDateToSend.quarterDays),
			static_cast<std::uint8_t>(timeAndDateToSend.year - 1985),
			static_cast<std::uint8_t>(timeAndDateToSend.localMinuteOffset + 125),
			static_cast<std::uint8_t>(timeAndDateToSend.localHourOffset + 125)
		};
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TimeDate),
		                                                      buffer.data(),
		                                                      buffer.size(),
		                                                      myControlFunction,
		                                                      nullptr,
		                                                      CANIdentifier::CANPriority::PriorityDefault6);
	}

	bool TimeDateInterface::request_time_and_date(std::shared_ptr<InternalControlFunction> requestingControlFunction, std::shared_ptr<ControlFunction> optionalDestination) const
	{
		bool retVal = false;

		if (nullptr != requestingControlFunction)
		{
			retVal = ParameterGroupNumberRequestProtocol::request_parameter_group_number(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TimeDate),
			                                                                             requestingControlFunction,
			                                                                             optionalDestination);
		}
		return retVal;
	}

	std::shared_ptr<InternalControlFunction> TimeDateInterface::get_control_function() const
	{
		return myControlFunction;
	}

	void TimeDateInterface::process_rx_message(const CANMessage &message, void *parentPointer)
	{
		auto timeDateInterface = static_cast<TimeDateInterface *>(parentPointer);

		if ((nullptr != timeDateInterface) &&
		    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::TimeDate) == message.get_identifier().get_parameter_group_number()) &&
		    (nullptr != message.get_source_control_function()))
		{
			if (CAN_DATA_LENGTH == message.get_data_length())
			{
				TimeAndDateInformation timeAndDateInformation;

				timeAndDateInformation.controlFunction = message.get_source_control_function();
				timeAndDateInformation.timeAndDate.seconds = message.get_uint8_at(0) / 4; // This is SPN 959
				timeAndDateInformation.timeAndDate.milliseconds = static_cast<std::uint16_t>((message.get_uint8_at(0) % 4) * 250); // This is also part of SPN 959
				timeAndDateInformation.timeAndDate.minutes = message.get_uint8_at(1); // This is SPN 960
				timeAndDateInformation.timeAndDate.hours = message.get_uint8_at(2); // This is SPN 961
				timeAndDateInformation.timeAndDate.month = message.get_uint8_at(3); // This is SPN 963
				timeAndDateInformation.timeAndDate.day = message.get_uint8_at(4) / 4; // This is SPN 962
				timeAndDateInformation.timeAndDate.quarterDays = message.get_uint8_at(4) % 4; // This is also part of SPN 962
				timeAndDateInformation.timeAndDate.year = static_cast<std::uint16_t>(message.get_uint8_at(5) + 1985); // This is SPN 964
				timeAndDateInformation.timeAndDate.localMinuteOffset = static_cast<std::int8_t>(message.get_uint8_at(6) - 125); // This is SPN 1601
				timeAndDateInformation.timeAndDate.localHourOffset = static_cast<std::int8_t>(message.get_int8_at(7) - 125); // This is SPN 1602

#ifndef DISABLE_CAN_STACK_LOGGER
				if (CANStackLogger::get_log_level() == CANStackLogger::LoggingLevel::Debug) // This is a very heavy log statement, so only do it if we are logging at debug level
				{
					std::ostringstream oss;
					oss << "[Time/Date]: Control Function 0x";
					oss << std::setfill('0') << std::setw(16) << std::hex << message.get_source_control_function()->get_NAME().get_full_name();
					oss << " at address " << static_cast<int>(message.get_source_control_function()->get_address());
					oss << " reports it is: " << isobus::to_string(static_cast<int>(timeAndDateInformation.timeAndDate.hours)) << ":";
					oss << isobus::to_string(static_cast<int>(timeAndDateInformation.timeAndDate.minutes)) << ":";
					oss << isobus::to_string(static_cast<int>(timeAndDateInformation.timeAndDate.seconds));
					oss << " on day " << isobus::to_string(static_cast<int>(timeAndDateInformation.timeAndDate.day));
					oss << " of month " << isobus::to_string(static_cast<int>(timeAndDateInformation.timeAndDate.month));
					oss << " in the year " << isobus::to_string(static_cast<int>(timeAndDateInformation.timeAndDate.year));
					oss << " with a local offset of " << isobus::to_string(static_cast<int>(timeAndDateInformation.timeAndDate.localHourOffset));
					oss << " hours and " << isobus::to_string(static_cast<int>(timeAndDateInformation.timeAndDate.localMinuteOffset)) << " minutes.";
					LOG_DEBUG(oss.str());
				}
#endif
				timeDateInterface->timeAndDateEventDispatcher.invoke(std::move(timeAndDateInformation));
			}
			else
			{
				LOG_WARNING("[Time/Date]: Received a Time/Date message with an invalid data length. DLC must be 8.");
			}
		}
	}

	bool TimeDateInterface::process_request_for_time_date(std::uint32_t parameterGroupNumber,
	                                                      std::shared_ptr<ControlFunction>,
	                                                      bool &acknowledge,
	                                                      AcknowledgementType &,
	                                                      void *parentPointer)
	{
		bool retVal = false;

		if ((nullptr != parentPointer) &&
		    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::TimeDate) == parameterGroupNumber))
		{
			auto interface = static_cast<TimeDateInterface *>(parentPointer);

			if ((nullptr != interface->myControlFunction) &&
			    (nullptr != interface->userTimeDateCallback))
			{
				TimeAndDate timeAndDateInformation;
				if (interface->userTimeDateCallback(timeAndDateInformation)) // Getting the time and date information from the user callback
				{
					LOG_DEBUG("[Time/Date]: Received a request for Time/Date information and interface is configured to reply. Sending Time/Date.");
					retVal = interface->send_time_and_date(timeAndDateInformation);
					acknowledge = false;
				}
				else
				{
					LOG_ERROR("[Time/Date]: Your application failed to provide Time/Date information when requested! You are probably doing something wrong. The request may be NACKed as a result.");
				}
			}
		}
		return retVal;
	}
} // namespace isobus
