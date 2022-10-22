//================================================================================================
/// @file isobus_diagnostic_protocol.cpp
///
/// @brief A protocol that handles the ISO-11783 Active DTC Protocol.
/// @details The ISO-11783 definition of DM1 is based on the J1939 definition with some tweaks.
/// This protocol reports active diagnostic trouble codes as defined by
/// SAE J1939-73. The message this protocol sends is sent via BAM, which has some
/// implications to your application, as only 1 BAM can be active at a time. This message
/// is sent at 1 Hz. Unlike in J1939, the message is discontinued when no DTCs are active to
/// minimize bus load. Also, ISO-11783 does not utilize or support lamp status.
/// You can revert to the standard J1939 behavior though if you want.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "isobus_diagnostic_protocol.hpp"

#include "can_general_parameter_group_numbers.hpp"
#include "can_network_manager.hpp"
#include "system_timing.hpp"

#include <algorithm>

namespace isobus
{
	std::list<DiagnosticProtocol *> DiagnosticProtocol::diagnosticProtocolList;

	DiagnosticProtocol::DiagnosticTroubleCode::DiagnosticTroubleCode() :
	  suspectParameterNumber(0xFFFFFFFF),
	  failureModeIdentifier(static_cast<std::uint8_t>(FailureModeIdentifier::ConditionExists)),
	  lampState(LampStatus::None)
	{
	}

	DiagnosticProtocol::DiagnosticTroubleCode::DiagnosticTroubleCode(std::uint32_t spn, FailureModeIdentifier fmi, LampStatus lamp) :
	  suspectParameterNumber(spn),
	  failureModeIdentifier(static_cast<std::uint8_t>(fmi)),
	  lampState(lamp)
	{
	}

	bool DiagnosticProtocol::DiagnosticTroubleCode::operator==(const DiagnosticTroubleCode &obj)
	{
		return ((suspectParameterNumber == obj.suspectParameterNumber) &&
		        (failureModeIdentifier == obj.failureModeIdentifier) &&
		        (lampState == obj.lampState));
	}

	std::uint8_t DiagnosticProtocol::DiagnosticTroubleCode::get_occurrance_count() const
	{
		return occuranceCount;
	}

	DiagnosticProtocol::DiagnosticProtocol(std::shared_ptr<InternalControlFunction> internalControlFunction) :
	  myControlFunction(internalControlFunction),
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberOfFlags), process_flags, this),
	  lastDM1SentTimestamp(0),
	  lastDM2SentTimestamp(0),
	  j1939Mode(false)
	{
		diagnosticProtocolList.push_back(this);
	}

	DiagnosticProtocol::~DiagnosticProtocol()
	{
		auto protocolLocation = find(diagnosticProtocolList.begin(), diagnosticProtocolList.end(), this);

		if (diagnosticProtocolList.end() != protocolLocation)
		{
			diagnosticProtocolList.erase(protocolLocation);
		}

		if (initialized)
		{
			initialized = false;
			CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest), process_message, this);
		}
	}

	bool DiagnosticProtocol::assign_diagnostic_protocol_to_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction)
	{
		bool retVal = true;

		for (auto protocolLocation : diagnosticProtocolList)
		{
			if (protocolLocation->myControlFunction == internalControlFunction)
			{
				retVal = false;
				break;
			}
		}

		if (retVal)
		{
			DiagnosticProtocol *newProtocol = new DiagnosticProtocol(internalControlFunction);
			diagnosticProtocolList.push_back(newProtocol);
		}
		return retVal;
	}

	bool DiagnosticProtocol::deassign_diagnostic_protocol_to_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction)
	{
		bool retVal = false;

		for (auto protocolLocation = diagnosticProtocolList.begin(); protocolLocation != diagnosticProtocolList.end(); protocolLocation++)
		{
			if ((*protocolLocation)->myControlFunction == internalControlFunction)
			{
				retVal = true;
				delete *protocolLocation;
				diagnosticProtocolList.erase(protocolLocation);
				break;
			}
		}
		return retVal;
	}

	DiagnosticProtocol *DiagnosticProtocol::get_diagnostic_protocol_by_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction)
	{
		DiagnosticProtocol *retVal = nullptr;
		for (auto protocol : diagnosticProtocolList)
		{
			if (protocol->myControlFunction == internalControlFunction)
			{
				retVal = protocol;
				break;
			}
		}
		return retVal;
	}

	void DiagnosticProtocol::initialize(CANLibBadge<CANNetworkManager>)
	{
		if (!initialized)
		{
			initialized = true;
			CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest), process_message, this);
		}
	}

	void DiagnosticProtocol::set_j1939_mode(bool value)
	{
		j1939Mode = value;
	}

	bool DiagnosticProtocol::get_j1939_mode() const
	{
		return j1939Mode;
	}

	void DiagnosticProtocol::clear_active_diagnostic_trouble_codes()
	{
		inactiveDTCList.insert(std::end(inactiveDTCList), std::begin(activeDTCList), std::end(activeDTCList));
		activeDTCList.clear();
	}

	void DiagnosticProtocol::clear_inactive_diagnostic_trouble_codes()
	{
		inactiveDTCList.clear();
	}

	bool DiagnosticProtocol::set_diagnostic_trouble_code_active(const DiagnosticTroubleCode &dtc, bool active)
	{
		bool retVal = false;

		if (active)
		{
			// First check to see if it's already active
			auto activeLocation = std::find(activeDTCList.begin(), activeDTCList.end(), dtc);

			if (activeDTCList.end() == activeLocation)
			{
				// Not already active. This is valid
				auto inactiveLocation = std::find(inactiveDTCList.begin(), inactiveDTCList.end(), dtc);

				if (inactiveDTCList.end() != inactiveLocation)
				{
					inactiveLocation->occuranceCount++;
					activeDTCList.push_back(*inactiveLocation);
					inactiveDTCList.erase(inactiveLocation);
				}
				else
				{
					activeDTCList.push_back(dtc);
					activeDTCList[activeDTCList.size() - 1].occuranceCount = 1;

					if (SystemTiming::get_time_elapsed_ms(lastDM1SentTimestamp) > DM_MAX_FREQUENCY_MS)
					{
						txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::DM1));
						lastDM1SentTimestamp = SystemTiming::get_timestamp_ms();
					}
				}
			}
			else
			{
				// Already active!
				retVal = false;
			}
		}
		else
		{
			/// First check to see if it's already in the inactive list
			auto inactiveLocation = std::find(inactiveDTCList.begin(), inactiveDTCList.end(), dtc);

			if (inactiveDTCList.end() == inactiveLocation)
			{
				auto activeLocation = std::find(activeDTCList.begin(), activeDTCList.end(), dtc);

				if (activeDTCList.end() != activeLocation)
				{
					inactiveDTCList.push_back(*activeLocation);
					activeDTCList.erase(activeLocation);

					if (SystemTiming::get_time_elapsed_ms(lastDM2SentTimestamp) > DM_MAX_FREQUENCY_MS)
					{
						txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::DM2));
						lastDM2SentTimestamp = SystemTiming::get_timestamp_ms();
					}
				}
			}
			else
			{
				// Already inactive!
				retVal = false;
			}
		}
		return retVal;
	}

	void DiagnosticProtocol::update(CANLibBadge<CANNetworkManager>)
	{
		if (j1939Mode)
		{
			if (SystemTiming::time_expired_ms(lastDM1SentTimestamp, DM_MAX_FREQUENCY_MS))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::DM1));
				lastDM1SentTimestamp = SystemTiming::get_timestamp_ms();
			}

			if (SystemTiming::time_expired_ms(lastDM2SentTimestamp, DM_MAX_FREQUENCY_MS))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::DM2));
				lastDM2SentTimestamp = SystemTiming::get_timestamp_ms();
			}
		}
		else
		{
			if ((0 != activeDTCList.size()) &&
			    (SystemTiming::time_expired_ms(lastDM1SentTimestamp, DM_MAX_FREQUENCY_MS)))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::DM1));
				lastDM1SentTimestamp = SystemTiming::get_timestamp_ms();
			}

			if ((0 != inactiveDTCList.size()) &&
			    (SystemTiming::time_expired_ms(lastDM2SentTimestamp, DM_MAX_FREQUENCY_MS)))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::DM2));
				lastDM2SentTimestamp = SystemTiming::get_timestamp_ms();
			}
		}
		txFlags.process_all_flags();
	}

	std::uint8_t DiagnosticProtocol::convert_flash_state_to_byte(FlashState flash)
	{
		std::uint8_t retVal = 0;

		switch (flash)
		{
			case FlashState::Slow:
			{
				retVal = 0x00;
			}
			break;

			case FlashState::Fast:
			{
				retVal = 0x01;
			}
			break;

			case FlashState::Solid:
			default:
			{
				retVal = 0x03;
			}
			break;
		}
		return retVal;
	}

	void DiagnosticProtocol::get_active_list_lamp_state_and_flash_state(Lamps targetLamp, FlashState &flash, bool &lampOn)
	{
		flash = FlashState::Solid;
		lampOn = false;

		for (auto dtc : activeDTCList)
		{
			switch (targetLamp)
			{
				case Lamps::AmberWarningLamp:
				{
					if (dtc.lampState == LampStatus::AmberWarningLampSolid)
					{
						lampOn = true;
					}
					else if (dtc.lampState == LampStatus::AmberWarningLampSlowFlash)
					{
						lampOn = true;
						if (flash != FlashState::Fast)
						{
							flash = FlashState::Slow;
						}
					}
					else if (dtc.lampState == LampStatus::AmberWarningLampFastFlash)
					{
						lampOn = true;
						flash = FlashState::Fast;
					}
				}
				break;

				case Lamps::MalfunctionIndicatorLamp:
				{
					if (dtc.lampState == LampStatus::MalfunctionIndicatorLampSolid)
					{
						lampOn = true;
					}
					else if (dtc.lampState == LampStatus::MalfuctionIndicatorLampSlowFlash)
					{
						lampOn = true;
						if (flash != FlashState::Fast)
						{
							flash = FlashState::Slow;
						}
					}
					else if (dtc.lampState == LampStatus::MalfunctionIndicatorLampFastFlash)
					{
						lampOn = true;
						flash = FlashState::Fast;
					}
				}
				break;

				case Lamps::ProtectLamp:
				{
					if (dtc.lampState == LampStatus::EngineProtectLampSolid)
					{
						lampOn = true;
					}
					else if (dtc.lampState == LampStatus::EngineProtectLampSlowFlash)
					{
						lampOn = true;
						if (flash != FlashState::Fast)
						{
							flash = FlashState::Slow;
						}
					}
					else if (dtc.lampState == LampStatus::EngineProtectLampFastFlash)
					{
						lampOn = true;
						flash = FlashState::Fast;
					}
				}
				break;

				case Lamps::RedStopLamp:
				{
					if (dtc.lampState == LampStatus::RedStopLampSolid)
					{
						lampOn = true;
					}
					else if (dtc.lampState == LampStatus::RedStopLampSlowFlash)
					{
						lampOn = true;
						if (flash != FlashState::Fast)
						{
							flash = FlashState::Slow;
						}
					}
					else if (dtc.lampState == LampStatus::RedStopLampFastFlash)
					{
						lampOn = true;
						flash = FlashState::Fast;
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

	void DiagnosticProtocol::get_inactive_list_lamp_state_and_flash_state(Lamps targetLamp, FlashState &flash, bool &lampOn)
	{
		flash = FlashState::Solid;
		lampOn = false;

		for (auto dtc : inactiveDTCList)
		{
			switch (targetLamp)
			{
				case Lamps::AmberWarningLamp:
				{
					if (dtc.lampState == LampStatus::AmberWarningLampSolid)
					{
						lampOn = true;
					}
					else if (dtc.lampState == LampStatus::AmberWarningLampSlowFlash)
					{
						lampOn = true;
						if (flash != FlashState::Fast)
						{
							flash = FlashState::Slow;
						}
					}
					else if (dtc.lampState == LampStatus::AmberWarningLampFastFlash)
					{
						lampOn = true;
						flash = FlashState::Fast;
					}
				}
				break;

				case Lamps::MalfunctionIndicatorLamp:
				{
					if (dtc.lampState == LampStatus::MalfunctionIndicatorLampSolid)
					{
						lampOn = true;
					}
					else if (dtc.lampState == LampStatus::MalfuctionIndicatorLampSlowFlash)
					{
						lampOn = true;
						if (flash != FlashState::Fast)
						{
							flash = FlashState::Slow;
						}
					}
					else if (dtc.lampState == LampStatus::MalfunctionIndicatorLampFastFlash)
					{
						lampOn = true;
						flash = FlashState::Fast;
					}
				}
				break;

				case Lamps::ProtectLamp:
				{
					if (dtc.lampState == LampStatus::EngineProtectLampSolid)
					{
						lampOn = true;
					}
					else if (dtc.lampState == LampStatus::EngineProtectLampSlowFlash)
					{
						lampOn = true;
						if (flash != FlashState::Fast)
						{
							flash = FlashState::Slow;
						}
					}
					else if (dtc.lampState == LampStatus::EngineProtectLampFastFlash)
					{
						lampOn = true;
						flash = FlashState::Fast;
					}
				}
				break;

				case Lamps::RedStopLamp:
				{
					if (dtc.lampState == LampStatus::RedStopLampSolid)
					{
						lampOn = true;
					}
					else if (dtc.lampState == LampStatus::RedStopLampSlowFlash)
					{
						lampOn = true;
						if (flash != FlashState::Fast)
						{
							flash = FlashState::Slow;
						}
					}
					else if (dtc.lampState == LampStatus::RedStopLampFastFlash)
					{
						lampOn = true;
						flash = FlashState::Fast;
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

	bool DiagnosticProtocol::protocol_transmit_message(std::uint32_t,
	                                                   const std::uint8_t *,
	                                                   std::uint32_t,
	                                                   ControlFunction *,
	                                                   ControlFunction *,
	                                                   TransmitCompleteCallback,
	                                                   void *,
	                                                   DataChunkCallback)
	{
		return false;
	}

	bool DiagnosticProtocol::send_diagnostic_message_1()
	{
		bool retVal = false;

		if (nullptr != myControlFunction)
		{
			std::uint16_t payloadSize = (activeDTCList.size() * DM_PAYLOAD_BYTES_PER_DTC) + 2; // 2 Bytes (0 and 1) are reserved
			std::vector<std::uint8_t> buffer;

			buffer.resize(payloadSize);

			if (get_j1939_mode())
			{
				bool tempLampState = false;
				FlashState tempLampFlashState = FlashState::Solid;
				get_active_list_lamp_state_and_flash_state(Lamps::ProtectLamp, tempLampFlashState, tempLampState);

				/// Encode Protect state and flash
				buffer[0] = tempLampState;
				buffer[1] = convert_flash_state_to_byte(tempLampFlashState);

				get_active_list_lamp_state_and_flash_state(Lamps::AmberWarningLamp, tempLampFlashState, tempLampState);

				/// Encode amber warning lamp state and flash
				buffer[0] |= (tempLampState << 2);
				buffer[1] |= (convert_flash_state_to_byte(tempLampFlashState) << 2);

				get_active_list_lamp_state_and_flash_state(Lamps::RedStopLamp, tempLampFlashState, tempLampState);

				/// Encode red stop lamp state and flash
				buffer[0] |= (tempLampState << 4);
				buffer[1] |= (convert_flash_state_to_byte(tempLampFlashState) << 4);

				get_active_list_lamp_state_and_flash_state(Lamps::MalfunctionIndicatorLamp, tempLampFlashState, tempLampState);

				/// Encode malfunction indicator lamp state and flash
				buffer[0] |= (tempLampState << 6);
				buffer[1] |= (convert_flash_state_to_byte(tempLampFlashState) << 6);
			}
			else
			{
				// ISO 11783 does not use lamp state or lamp flash bytes
				buffer[0] = 0xFF;
				buffer[1] = 0xFF;
			}

			if (0 == activeDTCList.size())
			{
				buffer[2] = 0x00;
				buffer[3] = 0x00;
				buffer[4] = 0x00;
				buffer[5] = 0x00;
				buffer[6] = 0xFF;
				buffer[7] = 0xFF;
				retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::DiagnosticMessage1),
				                                                        buffer.data(),
				                                                        CAN_DATA_LENGTH,
				                                                        myControlFunction.get());
			}
			else
			{
				for (std::uint16_t i = 0; i < activeDTCList.size(); i++)
				{
					buffer[2 + (DM_PAYLOAD_BYTES_PER_DTC * i)] = static_cast<std::uint8_t>(activeDTCList[i].suspectParameterNumber & 0xFF);
					buffer[3 + (DM_PAYLOAD_BYTES_PER_DTC * i)] = static_cast<std::uint8_t>((activeDTCList[i].suspectParameterNumber >> 8) & 0xFF);
					buffer[4 + (DM_PAYLOAD_BYTES_PER_DTC * i)] = ((static_cast<std::uint8_t>((activeDTCList[i].suspectParameterNumber >> 16) & 0xFF) << 5) | static_cast<std::uint8_t>(activeDTCList[i].failureModeIdentifier & 0x1F));
					buffer[5 + (DM_PAYLOAD_BYTES_PER_DTC * i)] = (activeDTCList[i].occuranceCount & 0x7F);
				}
				retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::DiagnosticMessage1),
				                                                        buffer.data(),
				                                                        payloadSize,
				                                                        myControlFunction.get());
			}
		}
		return retVal;
	}

	bool DiagnosticProtocol::send_diagnostic_message_2()
	{
		bool retVal = false;

		if (nullptr != myControlFunction)
		{
			std::uint16_t payloadSize = (inactiveDTCList.size() * DM_PAYLOAD_BYTES_PER_DTC) + 2; // 2 Bytes (0 and 1) are reserved or used for lamp + flash
			std::vector<std::uint8_t> buffer;

			buffer.resize(payloadSize);

			if (get_j1939_mode())
			{
				bool tempLampState = false;
				FlashState tempLampFlashState = FlashState::Solid;
				get_inactive_list_lamp_state_and_flash_state(Lamps::ProtectLamp, tempLampFlashState, tempLampState);

				/// Encode Protect state and flash
				buffer[0] = tempLampState;
				buffer[1] = convert_flash_state_to_byte(tempLampFlashState);

				get_inactive_list_lamp_state_and_flash_state(Lamps::AmberWarningLamp, tempLampFlashState, tempLampState);

				/// Encode amber warning lamp state and flash
				buffer[0] |= (tempLampState << 2);
				buffer[1] |= (convert_flash_state_to_byte(tempLampFlashState) << 2);

				get_inactive_list_lamp_state_and_flash_state(Lamps::RedStopLamp, tempLampFlashState, tempLampState);

				/// Encode red stop lamp state and flash
				buffer[0] |= (tempLampState << 4);
				buffer[1] |= (convert_flash_state_to_byte(tempLampFlashState) << 4);

				get_inactive_list_lamp_state_and_flash_state(Lamps::MalfunctionIndicatorLamp, tempLampFlashState, tempLampState);

				/// Encode malfunction indicator lamp state and flash
				buffer[0] |= (tempLampState << 6);
				buffer[1] |= (convert_flash_state_to_byte(tempLampFlashState) << 6);
			}
			else
			{
				// ISO 11783 does not use lamp state or lamp flash bytes
				buffer[0] = 0xFF;
				buffer[1] = 0xFF;
			}

			if (0 == inactiveDTCList.size())
			{
				buffer[2] = 0x00;
				buffer[3] = 0x00;
				buffer[4] = 0x00;
				buffer[5] = 0x00;
				buffer[6] = 0xFF;
				buffer[7] = 0xFF;
				retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::DiagnosticMessage2),
				                                                        buffer.data(),
				                                                        CAN_DATA_LENGTH,
				                                                        myControlFunction.get());
			}
			else
			{
				for (std::uint16_t i = 0; i < inactiveDTCList.size(); i++)
				{
					buffer[2 + (DM_PAYLOAD_BYTES_PER_DTC * i)] = static_cast<std::uint8_t>(inactiveDTCList[i].suspectParameterNumber & 0xFF);
					buffer[3 + (DM_PAYLOAD_BYTES_PER_DTC * i)] = static_cast<std::uint8_t>((inactiveDTCList[i].suspectParameterNumber >> 8) & 0xFF);
					buffer[4 + (DM_PAYLOAD_BYTES_PER_DTC * i)] = ((static_cast<std::uint8_t>((inactiveDTCList[i].suspectParameterNumber >> 16) & 0xFF) << 5) | static_cast<std::uint8_t>(inactiveDTCList[i].failureModeIdentifier & 0x1F));
					buffer[5 + (DM_PAYLOAD_BYTES_PER_DTC * i)] = (inactiveDTCList[i].occuranceCount & 0x7F);
				}
				retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::DiagnosticMessage2),
				                                                        buffer.data(),
				                                                        payloadSize,
				                                                        myControlFunction.get());
			}
		}
		return retVal;
	}

	bool DiagnosticProtocol::send_diagnostic_message_3_ack(ControlFunction *destination)
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;
		bool retVal = false;

		// "A positive or negative acknowledgement is not required in response to a global request."
		if (nullptr != destination)
		{
			constexpr std::uint32_t DM3PGN = static_cast<std::uint32_t>(CANLibParameterGroupNumber::DiagnosticMessage3);

			buffer[0] = 0x00; // Positive acknowledgement
			buffer[1] = 0xFF; // Group function value
			buffer[2] = 0xFF; // Reserved
			buffer[3] = 0xFF; // Reserved
			buffer[4] = destination->get_address(); // Address acknowledged
			buffer[5] = (DM3PGN & 0xFF); // PGN LSB
			buffer[6] = ((DM3PGN >> 8) & 0xFF); // PGN middle byte
			buffer[7] = ((DM3PGN >> 16) & 0xFF); // PGN MSB

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        myControlFunction.get(),
			                                                        nullptr);
		}
		return retVal;
	}

	bool DiagnosticProtocol::send_diagnostic_protocol_identification()
	{
		bool retVal = false;

		if (nullptr != myControlFunction)
		{
			/// Bit 1 = J1939-73, Bit 2 = ISO 14230, Bit 3 = ISO 15765-3, else Reserved
			constexpr std::uint8_t SUPPORTED_DIAGNOSTIC_PROTOCOLS_BITFIELD = 0x01;
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer.fill(0xFF); // Reserved bytes
			buffer[0] = SUPPORTED_DIAGNOSTIC_PROTOCOLS_BITFIELD;
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::DiagnosticProtocolIdentification),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        myControlFunction.get());
		}
		return retVal;
	}

	void DiagnosticProtocol::process_message(CANMessage *const message)
	{
		if (nullptr != message)
		{
			switch (message->get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest):
				{
					if (3 == message->get_data_length()) /// PGN requests DLC is 3
					{
						std::vector<std::uint8_t> messageData = message->get_data();
						std::uint32_t requestedPGN =  static_cast<std::uint32_t>(messageData.at(0));
						requestedPGN |= (static_cast<std::uint32_t>(messageData.at(1)) << 8);
						requestedPGN |= (static_cast<std::uint32_t>(messageData.at(2)) << 16);

						if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::DiagnosticMessage3) == requestedPGN)
						{
							clear_inactive_diagnostic_trouble_codes();
							send_diagnostic_message_3_ack(message->get_source_control_function());
						}
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

	void DiagnosticProtocol::process_message(CANMessage *const message, void *parent)
	{
		if (nullptr != parent)
		{
			reinterpret_cast<DiagnosticProtocol *>(parent)->process_message(message);
		}
	}

	void DiagnosticProtocol::process_flags(std::uint32_t flag, void *parentPointer)
	{
		if (nullptr != parentPointer)
		{
			DiagnosticProtocol *parent = reinterpret_cast<DiagnosticProtocol *>(parentPointer);
			bool transmitSuccessful = false;

			switch (flag)
			{
				case static_cast<std::uint32_t>(TransmitFlags::DM1):
				{
					transmitSuccessful = parent->send_diagnostic_message_1();

					if (transmitSuccessful)
					{
						parent->lastDM1SentTimestamp = SystemTiming::get_timestamp_ms();
					}
				}
				break;

				case static_cast<std::uint32_t>(TransmitFlags::DM2):
				{
					transmitSuccessful = parent->send_diagnostic_message_2();

					if (transmitSuccessful)
					{
						parent->lastDM2SentTimestamp = SystemTiming::get_timestamp_ms();
					}
				}
				break;

				case static_cast<std::uint32_t>(TransmitFlags::DiagnosticProtocolID):
				{
					transmitSuccessful = parent->send_diagnostic_protocol_identification();
				}
				break;

				default:
				{
				}
				break;
			}

			if (false == transmitSuccessful)
			{
				parent->txFlags.set_flag(flag);
			}
		}
	}

} // namespace isobus
