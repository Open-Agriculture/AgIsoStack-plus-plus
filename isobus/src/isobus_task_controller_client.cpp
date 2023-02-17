#include "isobus/isobus/isobus_task_controller_client.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

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

	bool TaskControllerClient::get_is_initialized() const
	{
		return initialized;
	}

	bool TaskControllerClient::get_is_connected() const
	{
		return false;
	}

	void TaskControllerClient::update()
	{
		switch (currentState)
		{
			case StateMachineState::Disconnected:
			{
				// Waiting to initiate communication, or conditions to communicate not met.
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

			case StateMachineState::BeginSendingWorkingSetMaster:
			{
			}
			break;

			case StateMachineState::SendStatusMessage:
			{
			}
			break;

			case StateMachineState::RequestVersion:
			{
			}
			break;

			case StateMachineState::WaitForRequestVersionResponse:
			{
			}
			break;

			case StateMachineState::WaitForRequestVersionFromServer:
			{
			}
			break;

			case StateMachineState::SendRequestVersionResponse:
			{
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

			default:
			{
				assert(false); // Unknown state? File a bug on GitHub if you see this happen.
			}
			break;
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
						if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal) == targetParameterGroupNumber)
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
								parentTC->set_state(StateMachineState::BeginSendingWorkingSetMaster);
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

} // namespace isobus
