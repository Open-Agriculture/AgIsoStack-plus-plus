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
#include <cassert>

namespace isobus
{
	ParameterGroupNumberRequestProtocol::ParameterGroupNumberRequestProtocol(std::shared_ptr<InternalControlFunction> internalControlFunction, CANLibBadge<InternalControlFunction>) :
	  myControlFunction(internalControlFunction)
	{
		assert(nullptr != myControlFunction && "ParameterGroupNumberRequestProtocol::ParameterGroupNumberRequestProtocol() called with nullptr internalControlFunction");
		CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest), process_message, this);
		CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepetitionRate), process_message, this);
	}

	ParameterGroupNumberRequestProtocol::~ParameterGroupNumberRequestProtocol()
	{
		CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest), process_message, this);
		CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepetitionRate), process_message, this);
	}

	bool ParameterGroupNumberRequestProtocol::request_parameter_group_number(std::uint32_t pgn, std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination)
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

	bool ParameterGroupNumberRequestProtocol::request_repetition_rate(std::uint32_t pgn, std::uint16_t repetitionRate_ms, std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination)
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
#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
		const std::lock_guard<std::mutex> lock(pgnRequestMutex);
#endif

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
#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
		const std::lock_guard<std::mutex> lock(pgnRequestMutex);
#endif

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
#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
		const std::lock_guard<std::mutex> lock(pgnRequestMutex);
#endif

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
#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
		const std::lock_guard<std::mutex> lock(pgnRequestMutex);
#endif

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

	bool ParameterGroupNumberRequestProtocol::PGNRequestCallbackInfo::operator==(const PGNRequestCallbackInfo &obj) const
	{
		return ((obj.callbackFunction == this->callbackFunction) && (obj.pgn == this->pgn) && (obj.parent == this->parent));
	}

	bool ParameterGroupNumberRequestProtocol::PGNRequestForRepetitionRateCallbackInfo::operator==(const PGNRequestForRepetitionRateCallbackInfo &obj) const
	{
		return ((obj.callbackFunction == this->callbackFunction) && (obj.pgn == this->pgn) && (obj.parent == this->parent));
	}

	void ParameterGroupNumberRequestProtocol::process_message(const CANMessage &message)
	{
		if (((nullptr == message.get_destination_control_function()) &&
		     (BROADCAST_CAN_ADDRESS == message.get_identifier().get_destination_address())) ||
		    (message.get_destination_control_function() == myControlFunction))
		{
			switch (message.get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepetitionRate):
				{
					// Can't send this request to global, and must be 8 bytes. Ignore illegal message formats
					if ((CAN_DATA_LENGTH == message.get_data_length()) && (nullptr != message.get_destination_control_function()))
					{
						std::uint32_t requestedPGN = message.get_uint24_at(0);
						std::uint16_t requestedRate = message.get_uint16_at(3);

#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
						const std::lock_guard<std::mutex> lock(pgnRequestMutex);
#endif
						for (const auto &repetitionRateCallback : repetitionRateCallbacks)
						{
							if (((repetitionRateCallback.pgn == requestedPGN) ||
							     (static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::Any) == repetitionRateCallback.pgn)) &&
							    (repetitionRateCallback.callbackFunction(requestedPGN, message.get_source_control_function(), requestedRate, repetitionRateCallback.parent)))
							{
								// If the callback was able to process the PGN request, stop processing more.
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
						CANStackLogger::warn("[PR]: Received a malformed or broadcast request for repetition rate message. The message will not be processed.");
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest):
				{
					if (message.get_data_length() >= PGN_REQUEST_LENGTH)
					{
						bool shouldAck = false;
						AcknowledgementType ackType = AcknowledgementType::Negative;
						bool anyCallbackProcessed = false;

						std::uint32_t requestedPGN = message.get_uint24_at(0);

#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
						const std::lock_guard<std::mutex> lock(pgnRequestMutex);
#endif
						for (const auto &pgnRequestCallback : pgnRequestCallbacks)
						{
							if (((pgnRequestCallback.pgn == requestedPGN) ||
							     (static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::Any) == pgnRequestCallback.pgn)) &&
							    (pgnRequestCallback.callbackFunction(requestedPGN, message.get_source_control_function(), shouldAck, ackType, pgnRequestCallback.parent)))
							{
								// If we're here, the callback was able to process the PGN request.
								anyCallbackProcessed = true;

								// Now we need to know if we shoulc ACK it.
								// We should not ACK messages that send the actual PGN as a result of requesting it. This behavior is up to
								// the application layer to do properly.
								if (shouldAck && (nullptr != message.get_destination_control_function()))
								{
									send_acknowledgement(ackType,
									                     requestedPGN,
									                     message.get_source_control_function());
								}
								// If this callback was able to process the PGN request, stop processing more.
								break;
							}
						}

						if ((!anyCallbackProcessed) && (nullptr != message.get_destination_control_function()))
						{
							send_acknowledgement(AcknowledgementType::Negative,
							                     requestedPGN,
							                     message.get_source_control_function());
							CANStackLogger::warn("[PR]: NACK-ing PGN request for PGN " + isobus::to_string(requestedPGN) + " because no callback could handle it.");
						}
					}
					else
					{
						CANStackLogger::warn("[PR]: Received a malformed PGN request message. The message will not be processed.");
					}
				}
				break;

				default:
					break;
			}
		}
	}

	void ParameterGroupNumberRequestProtocol::process_message(const CANMessage &message, void *parent)
	{
		if (nullptr != parent)
		{
			reinterpret_cast<ParameterGroupNumberRequestProtocol *>(parent)->process_message(message);
		}
	}

	bool ParameterGroupNumberRequestProtocol::send_acknowledgement(AcknowledgementType type, std::uint32_t parameterGroupNumber, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
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
			                                                        myControlFunction,
			                                                        nullptr);
		}
		return retVal;
	}

} // namespace isobus
