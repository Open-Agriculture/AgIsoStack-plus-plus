//================================================================================================
/// @file isobus_file_server_client.cpp
///
/// @brief Defines an interface for an ISOBUS file server client (ISO 11783-13)
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus_file_server_client.hpp"
#include "can_general_parameter_group_numbers.hpp"
#include "system_timing.hpp"
#include "can_warning_logger.hpp"

namespace isobus
{
	FileServerClient::FileServerClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  partnerControlFunction(partner),
	  myControlFunction(clientSource),
	  workerThread(nullptr),
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberFlags), process_flags, this),
	  stateMachineTimestamp_ms(0),
	  fileServerStatusBitfield(0),
	  numberFilesOpen(0),
	  maxNumberSimultaneouslyOpenFiles(0),
	  fileServerCapabilitiesBitfield(0),
	  fileServerVersion(0),
	  transactionNumber(0),
	  initialized(false)
	{
	}

	FileServerClient::~FileServerClient()
	{
		if (nullptr != partnerControlFunction)
		{
			partnerControlFunction->remove_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::FileServerToClient), process_message, this);
			CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::FileServerToClient), process_message, this);
		}
	}

	bool FileServerClient::initialize(bool spawnThread)
	{
		if ((!initialized) && (nullptr != partnerControlFunction))
		{
			partnerControlFunction->add_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::FileServerToClient), process_message, this);
			CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::FileServerToClient), process_message, this);
			initialized = true;

			if (spawnThread)
			{
				workerThread = new std::thread([this]() { worker_thread_function(); });
			}
		}
		return initialized;
	}

	void FileServerClient::terminate()
	{
		if (initialized)
		{
			shouldTerminate = true;

			if (nullptr != workerThread)
			{
				workerThread->join();
				delete workerThread;
				workerThread = nullptr;
			}
		}
	}

	FileServerClient::StateMachineState FileServerClient::get_state() const
	{
		return currentState;
	}

	bool FileServerClient::get_is_initialized() const
	{
		return initialized;
	}

	void FileServerClient::update()
	{
		if (nullptr != partnerControlFunction)
		{
			switch (currentState)
			{
				case StateMachineState::Disconnected:
				{
					// Waiting for a server status message
					if (0 != lastServerStatusTimestamp_ms)
					{
						// Got a status message. Now query the FS to see its capabilities
						set_state(StateMachineState::SendGetFileServerProperties);
					}
				}
				break;

				case StateMachineState::SendGetFileServerProperties:
				{
					// Send the request for the FS properties
					if (true == send_get_file_server_properties())
					{
						set_state(StateMachineState::WaitForGetFileServerPropertiesResponse);
					}
				}
				break;

				case StateMachineState::WaitForGetFileServerPropertiesResponse:
				{
					if ((SystemTiming::time_expired_ms(lastServerStatusTimestamp_ms, SERVER_STATUS_MESSAGE_TIMEOUT_MS)) || 
						(SystemTiming::time_expired_ms(stateMachineTimestamp_ms, GENERAL_OPERATION_TIMEOUT)))
					{
						set_state(StateMachineState::Disconnected);
						lastServerStatusTimestamp_ms = 0;
					}
				}
				break;

				case StateMachineState::ChangeToManufacturerDirectory:
				{
					if (true == send_change_current_directory_request("~\\"))
					{
						set_state(StateMachineState::WaitForChangeToManufaacturerDirectoryResponse);
					}
				}
				break;

				case StateMachineState::WaitForChangeToManufaacturerDirectoryResponse:
				{
				
				}
				break;

				case StateMachineState::Connected:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, CLIENT_STATUS_MESSAGE_REPETITION_RATE_MS))
					{
						txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::ClientToServerStatus));
						stateMachineTimestamp_ms = SystemTiming::get_timestamp_ms();
					}
					if (SystemTiming::time_expired_ms(lastServerStatusTimestamp_ms, SERVER_STATUS_MESSAGE_TIMEOUT_MS))
					{
						set_state(StateMachineState::Disconnected);
						lastServerStatusTimestamp_ms = 0;
					}
				}
				break;

				case StateMachineState::SendWriteFile:
				{

				}
				break;

				case StateMachineState::WaitForWriteFileResponse:
				{
					if (SystemTiming::time_expired_ms(lastServerStatusTimestamp_ms, SERVER_STATUS_MESSAGE_TIMEOUT_MS))
					{
						set_state(StateMachineState::Disconnected);
						lastServerStatusTimestamp_ms = 0;
					}
				}
				break;

				case StateMachineState::SendReadFile:
				{
				
				}
				break;

				case StateMachineState::WaitForReadFileResponse:
				{
					if (SystemTiming::time_expired_ms(lastServerStatusTimestamp_ms, SERVER_STATUS_MESSAGE_TIMEOUT_MS))
					{
						set_state(StateMachineState::Disconnected);
						lastServerStatusTimestamp_ms = 0;
					}
				}
				break;

				default:
				{
				}
				break;
			}
			txFlags.process_all_flags();
		}
	}

	std::string FileServerClient::error_code_to_string(ErrorCode errorCode) const
	{
		std::string retVal;

		switch (errorCode)
		{
			case ErrorCode::Success:
			{
				retVal = "Success";
			}
			break;

			case ErrorCode::AccessDenied:
			{
				retVal = "Access Denied";
			}
			break;

			case ErrorCode::InvalidAccess:
			{
				retVal = "Invalid Access";
			}
			break;

			case ErrorCode::TooManyFilesOpen:
			{
				retVal = "Too Many Files Open";
			}
			break;

			case ErrorCode::FilePathOrVolumeNotFound:
			{
				retVal = "File Path Or Volume Not Found";
			}
			break;

			case ErrorCode::InvalidHandle:
			{
				retVal = "Invalid Handle";
			}
			break;

			case ErrorCode::InvalidGivenSourceName:
			{
				retVal = "Invalid Given Source Name";
			}
			break;

			case ErrorCode::InvalidGivenDestinationName:
			{
				retVal = "Invalid Given Destination Name";
			}
			break;

			case ErrorCode::VolumeOutOfFreeSpace:
			{
				retVal = "Volume Out Of Free Space";
			}
			break;

			case ErrorCode::FailureDuringAWriteOperation:
			{
				retVal = "Failure During A Write Operation";
			}
			break;

			case ErrorCode::MediaNotPresent:
			{
				retVal = "Media Not Present";
			}
			break;

			case ErrorCode::FailureDuringAReadOperation:
			{
				retVal = "Failure During A Read Operation";
			}
			break;

			case ErrorCode::FunctionNotSupported:
			{
				retVal = "Function Not Supported";
			}
			break;

			case ErrorCode::VolumeIsPossiblyNotInitialized:
			{
				retVal = "Volume Is Possibly Not Initialized";
			}
			break;

			case ErrorCode::InvalidRequestLength:
			{
				retVal = "Invalid Request Length";
			}
			break;

			case ErrorCode::OutOfMemory:
			{
				retVal = "Out Of Memory";
			}
			break;

			case ErrorCode::AnyOtherError:
			{
				retVal = "Any Other Error";
			}
			break;

			case ErrorCode::FilePointerAtEndOfFile:
			{
				retVal = "File Pointer At End Of File";
			}
			break;

			default:
			{
				retVal = "Undefined";
			}
			break;
		}
		return retVal;
	}

	void FileServerClient::process_flags(std::uint32_t flag, void *parent)
	{
		if ((flag <= static_cast<std::uint32_t>(TransmitFlags::NumberFlags)) &&
		    (nullptr != parent))
		{
			TransmitFlags flagToProcess = static_cast<TransmitFlags>(flag);
			FileServerClient *fsClient = reinterpret_cast<FileServerClient *>(parent);
			bool transmitSuccessful = false;

			switch (flagToProcess)
			{
				case TransmitFlags::ClientToServerStatus:
				{
					transmitSuccessful = fsClient->send_client_connection_maintenance();
				}
				break;

				case TransmitFlags::NumberFlags:
				default:
				{
				}
				break;
			}

			if (false == transmitSuccessful)
			{
				fsClient->txFlags.set_flag(flag);
			}
		}
	}

	void FileServerClient::process_message(CANMessage *const message)
	{
		if ((nullptr != message) &&
		    (nullptr != partnerControlFunction) &&
		    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::FileServerToClient) == message->get_identifier().get_parameter_group_number()) &&
		    ((message->get_source_control_function()->get_address() == partnerControlFunction->get_address()) ||
		     (nullptr == message->get_destination_control_function())))
		{
			auto messageData = message->get_data();

			switch (messageData[0])
			{
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::FileServerStatus):
				{
					if (CAN_DATA_LENGTH == message->get_data_length())
					{
						if (0 == lastServerStatusTimestamp_ms)
						{
							CANStackLogger::CAN_stack_log("[FS]: New file server status detected. State machine started.");
						}

						fileServerStatusBitfield = messageData[1];
						numberFilesOpen = messageData[2];

						if (0 == lastServerStatusTimestamp_ms)
						{
							txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::ClientToServerStatus));
						}

						lastServerStatusTimestamp_ms = SystemTiming::get_timestamp_ms();
					}
					else
					{
						CANStackLogger::CAN_stack_log("[FS]: Detected malformed file server status message. DLC must be 8.");
					}
				}
				break;
				
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::GetFileServerPropertiesResponse):
				{
					if (CAN_DATA_LENGTH == message->get_data_length())
					{
						if (StateMachineState::WaitForGetFileServerPropertiesResponse == get_state())
						{
							fileServerVersion = messageData[1];
							maxNumberSimultaneouslyOpenFiles = messageData[2];
							fileServerCapabilitiesBitfield = messageData[3];
							set_state(StateMachineState::ChangeToManufacturerDirectory);
						}
						else
						{
							CANStackLogger::CAN_stack_log("[FS]: Received an unexpected response to get file server properties message");
						}
					}
					else
					{
						CANStackLogger::CAN_stack_log("[FS]: Detected malformed file server properties response message. DLC must be 8.");
					}
				}
				break;

				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::ChangeCurrentDirectoryResponse):
				{
					if (CAN_DATA_LENGTH == message->get_data_length())
					{
						if (StateMachineState::WaitForChangeToManufaacturerDirectoryResponse == get_state())
						{
							set_state(StateMachineState::Connected);
						}
					}
					else
					{
						CANStackLogger::CAN_stack_log("[FS]: Detected malformed file server change current directory response message. DLC must be 8.");
					}
				}
				break;

				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::VolumeStatusResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::GetCurrentDirectoryResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::OpenFileResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::SeekFileResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::ReadFileResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::WriteFileResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::CloseFileResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::MoveFileResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::DeleteFileResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::GetFileAttributesResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::SetFileAttributesResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::GetFileDateAndTimeResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::InitializeVolumeResponse):
				{
				}
				break;

				default:
				{
				}
				break;
			}
		}
	}

	void FileServerClient::process_message(CANMessage *const message, void *parent)
	{
		if (nullptr != parent)
		{
			reinterpret_cast<FileServerClient *>(parent)->process_message(message);
		}
	}

	bool FileServerClient::send_change_current_directory_request(std::string path)
	{
		std::vector<std::uint8_t> buffer;
		std::vector<std::uint8_t>::iterator it;
		bool retVal = false;

		if ((0 != path.size()) && (StateMachineState::Disconnected != get_state()))
		{
			buffer.reserve(4 + path.size());
			buffer.push_back(static_cast<std::uint8_t>(ClientToFileServerMultiplexor::ChangeCurrentDirectoryRequest));
			buffer.push_back(transactionNumber);
			buffer.push_back(path.size() & 0xFF);
			buffer.push_back((path.size() >> 8) & 0xFF);
			it = buffer.begin() + 4;

			for (std::size_t i = 0; i < path.size(); i++)
			{
				buffer.push_back(static_cast<std::uint8_t>(path[i]));
			}

			if (buffer.size() < CAN_DATA_LENGTH)
			{
				for (std::size_t i = buffer.size(); i < CAN_DATA_LENGTH; i++)
				{
					buffer.push_back(0xFF);		
				}
			}

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
			                                                        buffer.data(),
			                                                        buffer.size(),
			                                                        myControlFunction.get(),
			                                                        partnerControlFunction.get(),
			                                                        CANIdentifier::PriorityLowest7);
		}
		return retVal;
	}

	bool FileServerClient::send_client_connection_maintenance()
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;
		buffer.fill(0xFF);

		buffer[0] = static_cast<std::uint8_t>(ClientToFileServerMultiplexor::ClientConnectionMaintenance);
		buffer[1] = static_cast<std::uint8_t>(VersionNumber::SecondPublishedEdition);
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool FileServerClient::send_get_file_server_properties()
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;
		buffer.fill(0xFF);

		buffer[0] = static_cast<std::uint8_t>(ClientToFileServerMultiplexor::GetFileServerProperties);
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	void FileServerClient::set_state(StateMachineState state)
	{
		stateMachineTimestamp_ms = SystemTiming::get_timestamp_ms();
		currentState = state;
	}

	void FileServerClient::worker_thread_function()
	{
		for (;;)
		{
			if (shouldTerminate)
			{
				break;
			}
			update();
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

} // namespace isobus
