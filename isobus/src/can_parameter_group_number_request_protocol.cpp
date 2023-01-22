//================================================================================================
/// @file can_parameter_group_number_request_protocol.cpp
///
/// @brief A protocol that handles PGN requests
/// @details The purpose of this protocol is to simplify and standardize how PGN requests
/// are made and responded to.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>

namespace isobus
{
	std::list<ParameterGroupNumberRequestProtocol *> ParameterGroupNumberRequestProtocol::pgnRequestProtocolList;

	void ParameterGroupNumberRequestProtocol::initialize(CANLibBadge<CANNetworkManager>)
	{
		if (!initialized)
		{
			initialized = true;
			CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest), process_message, this);
			CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepetitionRate), process_message, this);
		}
	}

	bool ParameterGroupNumberRequestProtocol::assign_pgn_request_protocol_to_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction)
	{
		bool retVal = true;

		for (auto protocolLocation : pgnRequestProtocolList)
		{
			if (protocolLocation->myControlFunction == internalControlFunction)
			{
				retVal = false;
				break;
			}
		}

		if (retVal)
		{
			ParameterGroupNumberRequestProtocol *newProtocol = new ParameterGroupNumberRequestProtocol(internalControlFunction);
			pgnRequestProtocolList.push_back(newProtocol);
		}
		return retVal;
	}

	bool ParameterGroupNumberRequestProtocol::deassign_pgn_request_protocol_to_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction)
	{
		bool retVal = false;

		for (auto protocolLocation = pgnRequestProtocolList.begin(); protocolLocation != pgnRequestProtocolList.end(); protocolLocation++)
		{
			if ((*protocolLocation)->myControlFunction == internalControlFunction)
			{
				retVal = true;
				delete *protocolLocation;
				pgnRequestProtocolList.erase(protocolLocation);
				break;
			}
		}
		return retVal;
	}

	ParameterGroupNumberRequestProtocol *ParameterGroupNumberRequestProtocol::get_pgn_request_protocol_by_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction)
	{
		ParameterGroupNumberRequestProtocol *retVal = nullptr;
		for (auto protocol : pgnRequestProtocolList)
		{
			if (protocol->myControlFunction == internalControlFunction)
			{
				retVal = protocol;
				break;
			}
		}
		return retVal;
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

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepetitionRate),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      source,
		                                                      destination);
	}

	bool ParameterGroupNumberRequestProtocol::register_pgn_request_callback(std::uint32_t pgn, PGNRequestCallback callback, void *parentPointer)
	{
		PGNRequestCallbackInfo pgnCallback(callback, pgn, parentPointer);
		bool retVal = false;
		const std::lock_guard<std::mutex> lock(pgnRequestMutex);

		if ((nullptr != callback) && (pgnRequestCallbacks.end() == std::find(pgnRequestCallbacks.begin(), pgnRequestCallbacks.end(), pgnCallback)))
		{
			pgnRequestCallbacks.push_back(pgnCallback);
			retVal = true;
		}
		return retVal;
	}

	bool ParameterGroupNumberRequestProtocol::register_request_for_repetition_rate_callback(std::uint32_t pgn, PGNRequestForRepetitionRateCallback callback, void *parentPointer)
	{
		PGNRequestForRepetitionRateCallbackInfo repetitionRateCallback(callback, pgn, parentPointer);
		bool retVal = false;
		const std::lock_guard<std::mutex> lock(pgnRequestMutex);

		if ((nullptr != callback) && (repetitionRateCallbacks.end() == std::find(repetitionRateCallbacks.begin(), repetitionRateCallbacks.end(), repetitionRateCallback)))
		{
			repetitionRateCallbacks.push_back(repetitionRateCallback);
			retVal = true;
		}
		return retVal;
	}

	bool ParameterGroupNumberRequestProtocol::remove_pgn_request_callback(std::uint32_t pgn, PGNRequestCallback callback, void *parentPointer)
	{
		PGNRequestCallbackInfo repetitionRateCallback(callback, pgn, parentPointer);
		bool retVal = false;
		const std::lock_guard<std::mutex> lock(pgnRequestMutex);

		auto callbackLocation = find(pgnRequestCallbacks.begin(), pgnRequestCallbacks.end(), repetitionRateCallback);

		if (pgnRequestCallbacks.end() != callbackLocation)
		{
			pgnRequestCallbacks.erase(callbackLocation);
			retVal = true;
		}
		return retVal;
	}

	bool ParameterGroupNumberRequestProtocol::remove_request_for_repetition_rate_callback(std::uint32_t pgn, PGNRequestForRepetitionRateCallback callback, void *parentPointer)
	{
		PGNRequestForRepetitionRateCallbackInfo repetitionRateCallback(callback, pgn, parentPointer);
		bool retVal = false;
		const std::lock_guard<std::mutex> lock(pgnRequestMutex);

		auto callbackLocation = find(repetitionRateCallbacks.begin(), repetitionRateCallbacks.end(), repetitionRateCallback);

		if (repetitionRateCallbacks.end() != callbackLocation)
		{
			repetitionRateCallbacks.erase(callbackLocation);
			retVal = true;
		}
		return retVal;
	}

	std::size_t ParameterGroupNumberRequestProtocol::get_number_registered_pgn_request_callbacks() const
	{
		return pgnRequestCallbacks.size();
	}

	std::size_t ParameterGroupNumberRequestProtocol::get_number_registered_request_for_repetition_rate_callbacks() const
	{
		return repetitionRateCallbacks.size();
	}

	void ParameterGroupNumberRequestProtocol::update(CANLibBadge<CANNetworkManager>)
	{
	}

	ParameterGroupNumberRequestProtocol::PGNRequestCallbackInfo::PGNRequestCallbackInfo(PGNRequestCallback callback, std::uint32_t parameterGroupNumber, void *parentPointer) :
	  callbackFunction(callback),
	  pgn(parameterGroupNumber),
	  parent(parentPointer)
	{
	}

	ParameterGroupNumberRequestProtocol::PGNRequestForRepetitionRateCallbackInfo::PGNRequestForRepetitionRateCallbackInfo(PGNRequestForRepetitionRateCallback callback, std::uint32_t parameterGroupNumber, void *parentPointer) :
	  callbackFunction(callback),
	  pgn(parameterGroupNumber),
	  parent(parentPointer)
	{
	}

	bool ParameterGroupNumberRequestProtocol::PGNRequestCallbackInfo::operator==(const PGNRequestCallbackInfo &obj)
	{
		return ((obj.callbackFunction == this->callbackFunction) && (obj.pgn == this->pgn) && (obj.parent == this->parent));
	}

	bool ParameterGroupNumberRequestProtocol::PGNRequestForRepetitionRateCallbackInfo::operator==(const PGNRequestForRepetitionRateCallbackInfo &obj)
	{
		return ((obj.callbackFunction == this->callbackFunction) && (obj.pgn == this->pgn) && (obj.parent == this->parent));
	}

	void ParameterGroupNumberRequestProtocol::process_message(CANMessage *const message)
	{
		if ((nullptr != message) &&
		    (((nullptr == message->get_destination_control_function()) &&
		      (BROADCAST_CAN_ADDRESS == message->get_identifier().get_destination_address())) ||
		     (message->get_destination_control_function() == myControlFunction.get())))
		{
			switch (message->get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepetitionRate):
				{
					// Can't send this request to global, and must be 8 bytes. Ignore illegal message formats
					if ((CAN_DATA_LENGTH == message->get_data_length()) && (nullptr != message->get_destination_control_function()))
					{
						std::vector<std::uint8_t> &data = message->get_data();
						std::uint32_t requestedPGN = data[0];
						requestedPGN |= (static_cast<std::uint32_t>(data[1]) << 8);
						requestedPGN |= (static_cast<std::uint32_t>(data[2]) << 8);

						std::uint16_t requestedRate = data[3];
						requestedRate |= (static_cast<std::uint16_t>(data[4]) << 8);

						const std::lock_guard<std::mutex> lock(pgnRequestMutex);

						for (auto repetitionRateCallback : repetitionRateCallbacks)
						{
							if (((repetitionRateCallback.pgn == requestedPGN) ||
							     (static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::Any) == repetitionRateCallback.pgn)) &&
							    (repetitionRateCallback.callbackFunction(requestedPGN, message->get_source_control_function(), requestedRate, repetitionRateCallback.parent)))
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
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[PR]: Received a malformed or broadcast request for repetition rate message. The message will not be processed.");
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest):
				{
					if (message->get_data_length() >= PGN_REQUEST_LENGTH)
					{
						bool shouldAck = false;
						AcknowledgementType ackType = AcknowledgementType::Negative;
						bool anyCallbackProcessed = false;
						std::vector<std::uint8_t> &data = message->get_data();
						std::uint32_t requestedPGN = data[0];
						requestedPGN |= (static_cast<std::uint32_t>(data[1]) << 8);
						requestedPGN |= (static_cast<std::uint32_t>(data[2]) << 8);

						const std::lock_guard<std::mutex> lock(pgnRequestMutex);

						for (auto pgnRequestCallback : pgnRequestCallbacks)
						{
							if (((pgnRequestCallback.pgn == requestedPGN) ||
							     (static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::Any) == pgnRequestCallback.pgn)) &&
							    (pgnRequestCallback.callbackFunction(requestedPGN, message->get_source_control_function(), shouldAck, ackType, pgnRequestCallback.parent)))
							{
								// If we're here, the callback was able to process the PGN request.
								anyCallbackProcessed = true;

								// Now we need to know if we shoulc ACK it.
								// We should not ACK messages that send the actual PGN as a result of requesting it. This behavior is up to
								// the application layer to do properly.
								if ((shouldAck) && (nullptr != message->get_destination_control_function()))
								{
									send_acknowledgement(ackType,
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
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[PR]: NACK-ing PGN request for PGN " + isobus::to_string(requestedPGN) + " because no callback could handle it.");
						}
					}
					else
					{
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[PR]: Received a malformed PGN request message. The message will not be processed.");
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

	ParameterGroupNumberRequestProtocol::ParameterGroupNumberRequestProtocol(std::shared_ptr<InternalControlFunction> internalControlFunction) :
	  myControlFunction(internalControlFunction)
	{
	}

	ParameterGroupNumberRequestProtocol ::~ParameterGroupNumberRequestProtocol()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest), process_message, this);
			CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepetitionRate), process_message, this);
		}
	}

	void ParameterGroupNumberRequestProtocol::process_message(CANMessage *const message, void *parent)
	{
		if (nullptr != parent)
		{
			reinterpret_cast<ParameterGroupNumberRequestProtocol *>(parent)->process_message(message);
		}
	}

	bool ParameterGroupNumberRequestProtocol::protocol_transmit_message(std::uint32_t,
	                                                                    const std::uint8_t *,
	                                                                    std::uint32_t,
	                                                                    ControlFunction *,
	                                                                    ControlFunction *,
	                                                                    TransmitCompleteCallback,
	                                                                    void *,
	                                                                    DataChunkCallback)
	{
		return false; // This protocol is not a transport layer, so just return false
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
			                                                        nullptr);
		}
		return retVal;
	}

} // namespace isobus
