//================================================================================================
/// @file can_extended_transport_protocol.cpp
///
/// @brief A protocol class that handles the ISO11783 extended transport protocol.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "can_extended_transport_protocol.hpp"

#include "can_general_parameter_group_numbers.hpp"
#include "can_network_configuration.hpp"
#include "can_warning_logger.hpp"
#include "system_timing.hpp"

namespace isobus
{

	ExtendedTransportProtocolManager ExtendedTransportProtocolManager::Protocol;

	ExtendedTransportProtocolManager::ExtendedTransportProtocolManager()
	{
	}

	ExtendedTransportProtocolManager ::~ExtendedTransportProtocolManager()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolDataTransfer), process_message, this);
			CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement), process_message, this);
		}
	}

	void ExtendedTransportProtocolManager::initialize(CANLibBadge<CANNetworkManager>)
	{
		if (!initialized)
		{
			initialized = true;
			CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolDataTransfer), process_message, this);
			CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement), process_message, this);
		}
	}

	void ExtendedTransportProtocolManager::process_message(CANMessage *const message)
	{
	}

	void ExtendedTransportProtocolManager::process_message(CANMessage *const message, void *parent)
	{
	}

	bool ExtendedTransportProtocolManager::protocol_transmit_message(std::uint32_t parameterGroupNumber,
	                                                                 std::uint8_t *data,
	                                                                 std::uint32_t messageLength,
	                                                                 ControlFunction *source,
	                                                                 ControlFunction *destination)
	{
		return false;
	}

	void ExtendedTransportProtocolManager::update(CANLibBadge<CANNetworkManager>)
	{
	
	}

	bool ExtendedTransportProtocolManager::abort_session(ExtendedTransportProtocolSession *session, ConnectionAbortReason reason)
	{
		bool retVal = false;

		if (nullptr != session)
		{
			InternalControlFunction *myControlFunction;
			ControlFunction *partnerControlFunction;
			std::array<std::uint8_t, CAN_DATA_LENGTH> data;
			std::uint32_t pgn = session->sessionMessage.get_identifier().get_parameter_group_number();

			if (ExtendedTransportProtocolSession::Direction::Transmit == session->sessionDirection)
			{
				myControlFunction = CANNetworkManager::CANNetwork.get_internal_control_function(session->sessionMessage.get_source_control_function());
				partnerControlFunction = session->sessionMessage.get_destination_control_function();
			}
			else
			{
				myControlFunction = CANNetworkManager::CANNetwork.get_internal_control_function(session->sessionMessage.get_destination_control_function());
				partnerControlFunction = session->sessionMessage.get_source_control_function();
			}

			data[0] = EXTENDED_CONNECTION_ABORT_MULTIPLEXOR;
			data[1] = static_cast<std::uint8_t>(reason);
			data[2] = 0xFF;
			data[3] = 0xFF;
			data[4] = 0xFF;
			data[5] = static_cast<std::uint8_t>(pgn & 0xFF);
			data[6] = static_cast<std::uint8_t>((pgn >> 8) & 0xFF);
			data[7] = static_cast<std::uint8_t>((pgn >> 16) & 0xFF);
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
			                                                        data.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        myControlFunction,
			                                                        partnerControlFunction,
			                                                        CANIdentifier::CANPriority::PriorityLowest7);
		}
		return retVal;
	}

	bool ExtendedTransportProtocolManager::abort_session(std::uint32_t parameterGroupNumber, ConnectionAbortReason reason, InternalControlFunction *source, ControlFunction *destination)
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> data;

		data[0] = EXTENDED_CONNECTION_ABORT_MULTIPLEXOR;
		data[1] = static_cast<std::uint8_t>(reason);
		data[2] = 0xFF;
		data[3] = 0xFF;
		data[4] = 0xFF;
		data[5] = static_cast<std::uint8_t>(parameterGroupNumber & 0xFF);
		data[6] = static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF);
		data[7] = static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF);
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
		                                                      data.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      source,
		                                                      destination,
		                                                      CANIdentifier::CANPriority::PriorityLowest7);
	}

} // namespace isobus
