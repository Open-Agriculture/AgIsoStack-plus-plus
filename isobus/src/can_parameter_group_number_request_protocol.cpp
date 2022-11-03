#include "can_parameter_group_number_request_protocol.hpp"
#include "can_general_parameter_group_numbers.hpp"
#include "can_warning_logger.hpp"

#include <algorithm>

namespace isobus
{
	ParameterGroupNumberRequestProtocol::ParameterGroupNumberRequestProtocol()
	{
	
	}

	ParameterGroupNumberRequestProtocol ::~ParameterGroupNumberRequestProtocol()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest), process_message, this);
			CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepititionRate), process_message, this);
		}
	}

	void ParameterGroupNumberRequestProtocol::initialize(CANLibBadge<CANNetworkManager>)
	{
		if (!initialized)
		{
			initialized = true;
			CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest), process_message, this);
			CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepititionRate), process_message, this);
		}
	}

	bool ParameterGroupNumberRequestProtocol::request_parameter_group_number(std::uint32_t pgn, InternalControlFunction *source, ControlFunction *destination)
	{
		std::array<std::uint8_t, PGN_REQUEST_LENGTH> buffer;
		
		buffer[0] = static_cast<std::uint8_t>(pgn & 0xFF);
		buffer[1] = static_cast<std::uint8_t>((pgn >> 8) & 0xFF);
		buffer[2] = static_cast<std::uint8_t>((pgn >> 16) & 0xFF);

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest),
		                                                      buffer.data(),
		                                                      PGN_REQUEST_LENGTH,
		                                                      source,
		                                                      destination);
	}

	bool ParameterGroupNumberRequestProtocol::request_repetition_rate(std::uint32_t pgn, std::uint16_t repetitionRate_ms, InternalControlFunction *source, ControlFunction *destination)
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

		buffer[0] = static_cast<std::uint8_t>(pgn & 0xFF);
		buffer[1] = static_cast<std::uint8_t>((pgn >> 8) & 0xFF);
		buffer[2] = static_cast<std::uint8_t>((pgn >> 16) & 0xFF);
		buffer[3] = static_cast<std::uint8_t>(repetitionRate_ms & 0xFF);
		buffer[4] = static_cast<std::uint8_t>((repetitionRate_ms >> 8) & 0xFF);
		buffer[5] = 0xFF;
		buffer[6] = 0xFF;
		buffer[7] = 0xFF;

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepititionRate),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      source,
		                                                      destination);
	}

	bool ParameterGroupNumberRequestProtocol::register_pgn_request_callback(std::uint32_t pgn, PGNRequestCallback callback)
	{
		PGNRequestCallbackInfo pgnCallback(callback, pgn);
		bool retVal = false;
		const std::lock_guard<std::mutex> lock(pgnRequestMutex);

		if ((nullptr != callback) && (pgnRequestCallbacks.end() == std::find(pgnRequestCallbacks.begin(), pgnRequestCallbacks.end(), pgnCallback)))
		{
			pgnRequestCallbacks.push_back(pgnCallback);
			retVal = true;
		}
		return retVal;
	}

	bool ParameterGroupNumberRequestProtocol::register_request_for_repetition_rate_callback(std::uint32_t pgn, PGNRequestCallback callback)
	{
		PGNRequestCallbackInfo repetitionRateCallback(callback, pgn);
		bool retVal = false;
		const std::lock_guard<std::mutex> lock(pgnRequestMutex);

		if ((nullptr != callback) && (repetitionRateCallbacks.end() == std::find(repetitionRateCallbacks.begin(), repetitionRateCallbacks.end(), repetitionRateCallback)))
		{
			repetitionRateCallbacks.push_back(repetitionRateCallback);
			retVal = true;
		}
		return retVal;
	}

	ParameterGroupNumberRequestProtocol::PGNRequestCallbackInfo::PGNRequestCallbackInfo(PGNRequestCallback callback, std::uint32_t parameterGroupNumber) :
	  callbackFunction(callback),
	  pgn(parameterGroupNumber)
	{
		
	}

	bool ParameterGroupNumberRequestProtocol::PGNRequestCallbackInfo::operator==(const PGNRequestCallbackInfo &obj)
	{
		return ((obj.callbackFunction == this->callbackFunction) && (obj.pgn == this->pgn));
	}

	void ParameterGroupNumberRequestProtocol::process_message(CANMessage *const message)
	{
		if (nullptr != message)
		{
			switch (message->get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepititionRate):
				{
					// Can't send this request to global, and must be 8 bytes. Ignore illegal message formats
					if ((CAN_DATA_LENGTH == message->get_data_length()) && (nullptr != message->get_destination_control_function()))
					{
						auto data = message->get_data();
						std::uint32_t requestedPGN = data[0];
						requestedPGN |= (static_cast<std::uint32_t>(data[1]) << 8);
						requestedPGN |= (static_cast<std::uint32_t>(data[2]) << 8);

						std::uint16_t requestedRate = data[3];
						requestedRate |= (static_cast<std::uint16_t>(data[4]) << 8);

						const std::lock_guard<std::mutex> lock(pgnRequestMutex);

						for (auto repetitionRateCallback : repetitionRateCallbacks)
						{
							if (repetitionRateCallback.pgn == requestedPGN)
							{
								break;
							}
						}

						// We can just ignore requests for repetition rate if we don't support them for this PGN. No need to NACK.
						// From isobus.net:
						// "CFs are not required to monitor the bus for this message."
						// "If another CF cannot or does not want to use the requested repetition rate, which is necessary for systems with fixed timing control loops, it may ignore this message."
					}
					else
					{
						CANStackLogger::CAN_stack_log("[PR]: Received a malformed or broadcast request for repetition rate message. The message will not be processed.");
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest):
				{
					if (message->get_data_length() >= PGN_REQUEST_LENGTH)
					{
						bool shouldAck = false;
						bool anyCallbackProcessed = false;
						auto data = message->get_data();
						std::uint32_t requestedPGN = data[0];
						requestedPGN |= (static_cast<std::uint32_t>(data[1]) << 8);
						requestedPGN |= (static_cast<std::uint32_t>(data[2]) << 8);

						const std::lock_guard<std::mutex> lock(pgnRequestMutex);

						for (auto pgnRequestCallback : pgnRequestCallbacks)
						{
							if ((pgnRequestCallback.pgn == requestedPGN) &&
							    (pgnRequestCallback.callbackFunction(requestedPGN, message->get_source_control_function(), shouldAck)))
							{
								// If we're here, the callback was able to process the PGN request.
								anyCallbackProcessed = true;

								// Now we need to know if we shoulc ACK it.
								// We should not ACK messages that send the actual PGN as a result of requesting it. This behavior is up to
								// the application layer to do properly.
								if ((shouldAck) && (nullptr != message->get_destination_control_function()))
								{
									send_acknowledgement(AcknowledgementType::Positive,
									                     requestedPGN,
									                     reinterpret_cast<InternalControlFunction *>(message->get_destination_control_function()),
									                     message->get_source_control_function());
								}
								// If this callback was able to process the PGN request, stop processing more.
								break;
							}
						}

						if ((!anyCallbackProcessed) && (nullptr != message->get_destination_control_function()))
						{
							send_acknowledgement(AcknowledgementType::Negative,
							                     requestedPGN,
							                     reinterpret_cast<InternalControlFunction *>(message->get_destination_control_function()),
							                     message->get_source_control_function());
							CANStackLogger::CAN_stack_log("[PR]: NACK-ing PGN request for PGN " + std::to_string(requestedPGN) + " because no callback could handle it.");
						}
					}
					else
					{
						CANStackLogger::CAN_stack_log("[PR]: Received a malformed PGN request message. The message will not be processed.");
					}
				}
				break;

				default:
				{
				}
				break;
			}
		}
	}

	void ParameterGroupNumberRequestProtocol::process_message(CANMessage *const message, void *parent)
	{
		if (nullptr != parent)
		{
			reinterpret_cast<ParameterGroupNumberRequestProtocol *>(parent)->process_message(message);
		}
	}

	bool ParameterGroupNumberRequestProtocol::send_acknowledgement(AcknowledgementType type, std::uint32_t parameterGroupNumber, InternalControlFunction *source, ControlFunction *destination)
	{
		bool retVal = false;

		if ((nullptr != source) && (nullptr != destination))
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(type);
			buffer[1] = 0xFF;
			buffer[2] = 0xFF;
			buffer[3] = 0xFF;
			buffer[4] = destination->get_address();
			buffer[5] = static_cast<std::uint8_t>(parameterGroupNumber & 0xFF);
			buffer[6] = static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF);
			buffer[7] = static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF);

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        source,
			                                                        destination);
		}
		return retVal;
	}

} // namespace isobus
