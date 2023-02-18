#include "isobus/isobus/isobus_task_controller_client.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <array>
#include <cassert>

namespace isobus
{
	TaskControllerClient::TaskControllerClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  languageCommandInterface(clientSource, partner),
	  partnerControlFunction(partner),
	  myControlFunction(clientSource)
	{
	}

	TaskControllerClient::~TaskControllerClient()
	{
		terminate();
	}

	void TaskControllerClient::initialize(bool spawnThread)
	{
		// You cannot use this interface without having valid control functions.
		assert(nullptr != myControlFunction);
		assert(nullptr != partnerControlFunction);

		partnerControlFunction->add_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData), process_rx_message, this);
		partnerControlFunction->add_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge), process_rx_message, this);
		CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData), process_rx_message, this);
		languageCommandInterface.initialize();

		if (shouldTerminate)
		{
			shouldTerminate = false;
			initialized = false;
		}

		if (!initialized)
		{
			if (spawnThread)
			{
				//workerThread = new std::thread([this]() { worker_thread_function(); }); TODO
			}
			initialized = true;
		}
	}

	void TaskControllerClient::configure(std::uint8_t maxNumberBoomsSupported,
	                                     std::uint8_t maxNumberSectionsSupported,
	                                     std::uint8_t maxNumberChannelsSupportedForPositionBasedControl,
	                                     bool reportToTCSupportsDocumentation,
	                                     bool reportToTCSupportsTCGEOWithoutPositionBasedControl,
	                                     bool reportToTCSupportsTCGEOWithPositionBasedControl,
	                                     bool reportToTCSupportsPeerControlAssignment,
	                                     bool reportToTCSupportsImplementSectionControl)
	{
		numberBoomsSupported = maxNumberBoomsSupported;
		numberSectionsSupported = maxNumberSectionsSupported;
		numberChannelsSupportedForPositionBasedControl = maxNumberChannelsSupportedForPositionBasedControl;
		supportsDocumentation = reportToTCSupportsDocumentation;
		supportsTCGEOWithoutPositionBasedControl = reportToTCSupportsTCGEOWithoutPositionBasedControl;
		supportsTCGEOWithPositionBasedControl = reportToTCSupportsTCGEOWithPositionBasedControl;
		supportsPeerControlAssignment = reportToTCSupportsPeerControlAssignment;
		supportsImplementSectionControl = reportToTCSupportsImplementSectionControl;
	}

	void TaskControllerClient::terminate()
	{
		if (initialized)
		{
			if (nullptr != partnerControlFunction)
			{
				partnerControlFunction->remove_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData), process_rx_message, this);
				partnerControlFunction->remove_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge), process_rx_message, this);
				CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData), process_rx_message, this);
			}

			shouldTerminate = true;

			//if (nullptr != workerThread)
			//{
			//	workerThread->join();
			//	delete workerThread;
			//	workerThread = nullptr;
			//}
		}
	}

	std::uint8_t TaskControllerClient::get_number_booms_supported() const
	{
		return numberBoomsSupported;
	}

	std::uint8_t TaskControllerClient::get_number_sections_supported() const
	{
		return numberSectionsSupported;
	}

	std::uint8_t TaskControllerClient::get_number_channels_supported_for_position_based_control() const
	{
		return numberChannelsSupportedForPositionBasedControl;
	}

	bool TaskControllerClient::get_supports_documentation() const
	{
		return supportsDocumentation;
	}

	bool TaskControllerClient::get_supports_tcgeo_without_position_based_control() const
	{
		return supportsTCGEOWithoutPositionBasedControl;
	}

	bool TaskControllerClient::get_supports_tcgeo_with_position_based_control() const
	{
		return supportsTCGEOWithPositionBasedControl;
	}

	bool TaskControllerClient::get_supports_peer_control_assignment() const
	{
		return supportsPeerControlAssignment;
	}

	bool TaskControllerClient::get_supports_implement_section_control() const
	{
		return supportsImplementSectionControl;
	}

	bool TaskControllerClient::get_is_initialized() const
	{
		return initialized;
	}

	bool TaskControllerClient::get_is_connected() const
	{
		return (StateMachineState::Connected == currentState);
	}

	void TaskControllerClient::update()
	{
		switch (currentState)
		{
			case StateMachineState::Disconnected:
			{
				// Waiting to initiate communication, or conditions to communicate not met.
				enableStatusMessage = false;
			}
			break;

			case StateMachineState::WaitForStartUpDelay:
			{
				if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, SIX_SECOND_TIMEOUT_MS))
				{
					CANStackLogger::debug("[TC]: Startup delay complete, waiting for TC server status message.");
					set_state(StateMachineState::WaitForServerStatusMessage);
				}
			}
			break;

			case StateMachineState::WaitForServerStatusMessage:
			{
				if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, SIX_SECOND_TIMEOUT_MS))
				{
					CANStackLogger::error("[TC]: Timeout waiting for TC status message. Resetting client conenction.");
					set_state(StateMachineState::Disconnected);
				}
			}
			break;

			case StateMachineState::SendWorkingSetMaster:
			{
				if (send_working_set_master())
				{
					set_state(StateMachineState::SendStatusMessage);
				}
				else if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, TWO_SECOND_TIMEOUT_MS))
				{
					CANStackLogger::error("[TC]: Timeout sending working set master message. Resetting client conenction.");
					set_state(StateMachineState::Disconnected);
				}
			}
			break;

			case StateMachineState::SendStatusMessage:
			{
				// Start sending the status message
				if (send_status())
				{
					enableStatusMessage = true;
					statusMessageTimestamp_ms = SystemTiming::get_timestamp_ms();
					set_state(StateMachineState::RequestVersion);
				}
				else if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, TWO_SECOND_TIMEOUT_MS))
				{
					CANStackLogger::error("[TC]: Timeout sending first status message. Resetting client conenction.");
					set_state(StateMachineState::Disconnected);
				}
			}
			break;

			case StateMachineState::RequestVersion:
			{
				if (send_version_request())
				{
					set_state(StateMachineState::WaitForRequestVersionResponse);
				}
				else if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, TWO_SECOND_TIMEOUT_MS))
				{
					CANStackLogger::error("[TC]: Timeout sending version request message. Resetting client conenction.");
					set_state(StateMachineState::Disconnected);
				}
			}
			break;

			case StateMachineState::WaitForRequestVersionResponse:
			{
				if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, TWO_SECOND_TIMEOUT_MS))
				{
					CANStackLogger::error("[TC]: Timeout waiting for version request response. Resetting client conenction.");
					set_state(StateMachineState::Disconnected);
				}
			}
			break;

			case StateMachineState::WaitForRequestVersionFromServer:
			{
				if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, SIX_SECOND_TIMEOUT_MS))
				{
					CANStackLogger::warn("[TC]: Timeout waiting for version request from TC. This is not required, so proceeding anways.");
					set_state(StateMachineState::RequestLanguage);
				}
			}
			break;

			case StateMachineState::SendRequestVersionResponse:
			{
				if (send_request_version_response())
				{
					set_state(StateMachineState::RequestLanguage);
				}
				else if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, TWO_SECOND_TIMEOUT_MS))
				{
					CANStackLogger::error("[TC]: Timeout sending version request response. Resetting client conenction.");
					set_state(StateMachineState::Disconnected);
				}
			}
			break;

			case StateMachineState::RequestLanguage:
			{
				if (languageCommandInterface.send_request_language_command())
				{
					set_state(StateMachineState::WaitForLanguageResponse);
				}
				else if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, SIX_SECOND_TIMEOUT_MS))
				{
					CANStackLogger::error("[TC]: Timeout trying to send request for language command message. Resetting client conenction.");
					set_state(StateMachineState::Disconnected);
				}
			}
			break;

			case StateMachineState::WaitForLanguageResponse:
			{
				if ((SystemTiming::get_time_elapsed_ms(languageCommandInterface.get_language_command_timestamp()) < SIX_SECOND_TIMEOUT_MS) &&
				    ("" != languageCommandInterface.get_language_code()))
				{
					// set_state() Todo
				}
			}
			break;

			case StateMachineState::Connected:
			{
				//! @todo check server status timestamp
			}
			break;

			default:
			{
				assert(false); // Unknown state? File a bug on GitHub if you see this happen.
			}
			break;
		}

		if ((enableStatusMessage) &&
		    (SystemTiming::time_expired_ms(statusMessageTimestamp_ms, TWO_SECOND_TIMEOUT_MS)) &&
		    (send_status()))
		{
			statusMessageTimestamp_ms = SystemTiming::get_timestamp_ms();
		}
	}

	void TaskControllerClient::process_rx_message(CANMessage *message, void *parentPointer)
	{
		if ((nullptr != message) &&
		    (nullptr != parentPointer) &&
		    (CAN_DATA_LENGTH <= message->get_data_length()))
		{
			TaskControllerClient *parentTC = static_cast<TaskControllerClient *>(parentPointer);
			std::vector<std::uint8_t> &messageData = message->get_data();

			switch (message->get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge):
				{
					if (AcknowledgementType::Negative == static_cast<AcknowledgementType>(message->get_uint8_at(0)))
					{
						std::uint32_t targetParameterGroupNumber = message->get_uint24_at(5);
						if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData) == targetParameterGroupNumber)
						{
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TC]: The TC Server is NACK-ing our messages. Disconnecting.");
							parentTC->set_state(StateMachineState::Disconnected);
						}
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData):
				{
					switch (static_cast<ProcessDataCommands>(messageData[0] & 0x0F))
					{
						case ProcessDataCommands::TechnicalCapabilities:
						{
							switch (static_cast<TechnicalDataMessageCommands>(messageData[0] >> 4))
							{
								case TechnicalDataMessageCommands::ParameterVersion:
								{
									parentTC->serverVersion = messageData[1];
									parentTC->maxServerBootTime_s = messageData[2];
									parentTC->serverOptionsByte1 = messageData[3];
									parentTC->serverOptionsByte2 = messageData[4];
									parentTC->serverNumberOfBoomsForSectionControl = messageData[5];
									parentTC->serverNumberOfSectionsForSectionControl = messageData[6];
									parentTC->serverNumberOfChannelsForPositionBasedControl = messageData[7];

									if (messageData[1] > static_cast<std::uint8_t>(Version::SecondPublishedEdition))
									{
										CANStackLogger::warn("[TC]: Server version is newer than client's maximum supported version.");
									}
									CANStackLogger::debug("[TC]: TC Server supports " +
									                      isobus::to_string(static_cast<int>(messageData[5])) +
									                      " booms, " +
									                      isobus::to_string(static_cast<int>(messageData[6])) +
									                      " sections, and " +
									                      isobus::to_string(static_cast<int>(messageData[7])) +
									                      "position based control channels.");

									if (StateMachineState::WaitForRequestVersionResponse == parentTC->get_state())
									{
										parentTC->set_state(StateMachineState::WaitForRequestVersionFromServer);
									}
								}
								break;

								default:
								{
									CANStackLogger::warn("[TC]: Unsupported process data technical data message received. Message will be dropped.");
								}
								break;
							}
						}
						break;

						case ProcessDataCommands::StatusMessage:
						{
							// Many values in the status message were undefined in version 2 and before, so the
							// standard explicitly tells us to ignore those attributes. The only things that really
							// matter are that we got the mesesage, and bytes 5, 6 and 7.
							parentTC->tcStatusBitfield = messageData[4];
							parentTC->sourceAddressOfCommandBeingExecuted = messageData[5];
							parentTC->commandBeingExecuted = messageData[6];
							if (StateMachineState::WaitForServerStatusMessage == parentTC->currentState)
							{
								parentTC->set_state(StateMachineState::SendWorkingSetMaster);
							}
						}
						break;

						default:
						{
						}
						break;
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

	bool TaskControllerClient::send_request_version_response() const
	{
		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { (static_cast<std::uint8_t>(TechnicalDataMessageCommands::ParameterVersion) << 4),
			                                                         static_cast<std::uint8_t>(Version::SecondPublishedEdition),
			                                                         0xFF, // Must be 0xFF when a client sends it (boot time)
			                                                         static_cast<std::uint8_t>(static_cast<std::uint8_t>(supportsDocumentation) |
			                                                                                   (static_cast<std::uint8_t>(supportsTCGEOWithoutPositionBasedControl) << 1) |
			                                                                                   (static_cast<std::uint8_t>(supportsTCGEOWithPositionBasedControl) << 2) |
			                                                                                   (static_cast<std::uint8_t>(supportsPeerControlAssignment) << 3) |
			                                                                                   (static_cast<std::uint8_t>(supportsImplementSectionControl) << 4)),
			                                                         0x00,
			                                                         numberBoomsSupported,
			                                                         numberSectionsSupported,
			                                                         numberChannelsSupportedForPositionBasedControl };

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get());
	}

	bool TaskControllerClient::send_status() const
	{
		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(ProcessDataCommands::ClientTask) | 0xF0,
			                                                         0xFF, // Element number N/A
			                                                         0xFF, // DDI N/A
			                                                         0xFF, // DDI N/A
			                                                         static_cast<std::uint8_t>(tcStatusBitfield & 0x01), // Actual TC or DL status
			                                                         0x00, // Reserved (0)
			                                                         0x00, // Reserved (0)
			                                                         0x00 }; // Reserved (0)

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get());
	}

	bool TaskControllerClient::send_version_request() const
	{
		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(TechnicalDataMessageCommands::ParameterRequestVersion),
			                                                         0xFF,
			                                                         0xFF,
			                                                         0xFF,
			                                                         0xFF,
			                                                         0xFF,
			                                                         0xFF,
			                                                         0xFF };

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get());
	}

	bool TaskControllerClient::send_working_set_master() const
	{
		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { numberOfWorkingSetMembers, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      nullptr);
	}

	void TaskControllerClient::set_state(StateMachineState newState)
	{
		if (newState != currentState)
		{
			stateMachineTimestamp_ms = SystemTiming::get_timestamp_ms();
			currentState = newState;
		}
	}

	TaskControllerClient::StateMachineState TaskControllerClient::get_state() const
	{
		return currentState;
	}

	std::uint8_t TaskControllerClient::get_connected_tc_number_booms_supported() const
	{
		return serverNumberOfBoomsForSectionControl;
	}

	std::uint8_t TaskControllerClient::get_connected_tc_number_sections_supported() const
	{
		return serverNumberOfSectionsForSectionControl;
	}

	std::uint8_t TaskControllerClient::get_connected_tc_number_channels_supported() const
	{
		return serverNumberOfChannelsForPositionBasedControl;
	}

	std::uint8_t TaskControllerClient::get_connected_tc_max_boot_time() const
	{
		return maxServerBootTime_s;
	}

	bool TaskControllerClient::get_connected_tc_option_supported(ServerOptions option) const
	{
		return (0 != (static_cast<std::uint8_t>(option) & serverOptionsByte1));
	}

	TaskControllerClient::Version TaskControllerClient::get_connected_tc_version() const
	{
		Version retVal = Version::Unknown;

		if (serverVersion <= static_cast<std::uint8_t>(Version::SecondPublishedEdition))
		{
			retVal = static_cast<Version>(serverVersion);
		}
		return retVal;
	}

} // namespace isobus
