//================================================================================================
/// @file can_transport_protocol.cpp
///
/// @brief Abstract base class for CAN transport protocols.
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#include "isobus/isobus/can_transport_protocol_base.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/utility/system_timing.hpp"

namespace isobus
{
	TransportProtocolSessionBase::TransportProtocolSessionBase(TransportProtocolSessionBase::Direction direction,
	                                                           std::unique_ptr<CANMessageData> data,
	                                                           std::uint32_t parameterGroupNumber,
	                                                           std::uint32_t totalMessageSize,
	                                                           std::shared_ptr<ControlFunction> source,
	                                                           std::shared_ptr<ControlFunction> destination,
	                                                           TransmitCompleteCallback sessionCompleteCallback,
	                                                           void *parentPointer) :
	  direction(direction),
	  parameterGroupNumber(parameterGroupNumber),
	  data(std::move(data)),
	  source(source),
	  destination(destination),
	  totalMessageSize(totalMessageSize),
	  sessionCompleteCallback(sessionCompleteCallback),
	  parent(parentPointer)
	{
	}

	bool TransportProtocolSessionBase::operator==(const TransportProtocolSessionBase &obj) const
	{
		return ((source == obj.source) && (destination == obj.destination) && (parameterGroupNumber == obj.parameterGroupNumber));
	}

	bool TransportProtocolSessionBase::matches(std::shared_ptr<ControlFunction> other_source, std::shared_ptr<ControlFunction> other_destination) const
	{
		return ((source == other_source) && (destination == other_destination));
	}

	TransportProtocolSessionBase::Direction TransportProtocolSessionBase::get_direction() const
	{
		return direction;
	}

	CANMessageData &TransportProtocolSessionBase::get_data() const
	{
		return *data;
	}

	std::uint32_t TransportProtocolSessionBase::get_message_length() const
	{
		return totalMessageSize;
	}

	std::shared_ptr<ControlFunction> TransportProtocolSessionBase::get_source() const
	{
		return source;
	}

	float TransportProtocolSessionBase::get_percentage_bytes_transferred() const
	{
		if (0 == get_message_length())
		{
			return 0.0f;
		}
		return static_cast<float>(get_total_bytes_transferred()) / static_cast<float>(get_message_length()) * 100.0f;
	}

	std::shared_ptr<ControlFunction> TransportProtocolSessionBase::get_destination() const
	{
		return destination;
	}

	std::uint32_t TransportProtocolSessionBase::get_parameter_group_number() const
	{
		return parameterGroupNumber;
	}

	void isobus::TransportProtocolSessionBase::update_timestamp()
	{
		timestamp_ms = SystemTiming::get_timestamp_ms();
	}

	std::uint32_t TransportProtocolSessionBase::get_time_since_last_update() const
	{
		return SystemTiming::get_time_elapsed_ms(timestamp_ms);
	}

	void TransportProtocolSessionBase::complete(bool success) const
	{
		if ((nullptr != sessionCompleteCallback) && (Direction::Transmit == direction))
		{
			sessionCompleteCallback(get_parameter_group_number(),
			                        get_message_length(),
			                        std::static_pointer_cast<InternalControlFunction>(source),
			                        get_destination(),
			                        success,
			                        parent);
		}
	}
}
