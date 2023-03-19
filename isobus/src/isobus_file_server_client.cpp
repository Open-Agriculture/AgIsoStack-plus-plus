//================================================================================================
/// @file isobus_file_server_client.cpp
///
/// @brief Defines an interface for an ISOBUS file server client (ISO 11783-13)
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/isobus_file_server_client.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

#include <cassert>
#include <cstring>

namespace isobus
{
	const std::map<FileServerClient::ErrorCode, std::string> FileServerClient::ERROR_TO_STRING_MAP = {
		{ ErrorCode::Success, "Success" },
		{ ErrorCode::AccessDenied, "Access Denied" },
		{ ErrorCode::InvalidAccess, "Invalid Access" },
		{ ErrorCode::TooManyFilesOpen, "Too Many Files Open" },
		{ ErrorCode::FilePathOrVolumeNotFound, "File Path or Volume Not Found" },
		{ ErrorCode::InvalidHandle, "Invalid Handle" },
		{ ErrorCode::InvalidGivenSourceName, "Invalid Given Source Name" },
		{ ErrorCode::InvalidGivenDestinationName, "Invalid Given Destination Name" },
		{ ErrorCode::VolumeOutOfFreeSpace, "Volume Out of Free Space" },
		{ ErrorCode::FailureDuringAWriteOperation, "Failure During a Write Operation" },
		{ ErrorCode::MediaNotPresent, "Media not Present" },
		{ ErrorCode::FailureDuringAReadOperation, "Failure During a Read Operation" },
		{ ErrorCode::FunctionNotSupported, "Function not Supported" },
		{ ErrorCode::VolumeIsPossiblyNotInitialized, "Volume is Possibly not Initialized" },
		{ ErrorCode::InvalidRequestLength, "Invalid Request Length" },
		{ ErrorCode::OutOfMemory, "Out of Memory" },
		{ ErrorCode::AnyOtherError, "Any Other Error" },
		{ ErrorCode::FilePointerAtEndOfFile, "File Pointer at End of File" }
	};

	FileServerClient::FileServerClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  partnerControlFunction(partner),
	  myControlFunction(clientSource),
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberFlags), process_flags, this)
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

	bool FileServerClient::change_directory(std::string &path)
	{
		bool retVal = false;

		const std::unique_lock<std::mutex> lock(metadataMutex);

		/// @todo Change directory

		return retVal;
	}

	bool FileServerClient::get_file_attribute(std::uint8_t handle, FileHandleAttributesBit attributeToGet)
	{
		bool retVal = false;

		for (auto file : fileInfoList)
		{
			if (file->handle == handle)
			{
				retVal = (file->attributesBitField & (0x01 << static_cast<std::uint8_t>(attributeToGet)));
				break;
			}
		}
		return retVal;
	}

	bool FileServerClient::set_file_attribute(std::string filePath, bool hidden, ReadOnlyAttributeCommand readOnly)
	{
		bool retVal = false;

		if (StateMachineState::Connected == get_state())
		{
			/// @todo Set file attribute command
		}
		return retVal;
	}

	std::uint8_t FileServerClient::get_file_handle(std::string filePath)
	{
		std::uint8_t retVal = INVALID_FILE_HANDLE;

		const std::unique_lock<std::mutex> lock(metadataMutex);

		for (auto file : fileInfoList)
		{
			if (file->fileName == filePath)
			{
				retVal = file->handle;
				break;
			}
		}
		return retVal;
	}

	FileServerClient::FileState FileServerClient::get_file_state(std::uint8_t handle)
	{
		FileState retVal = FileState::Uninitialized;

		const std::unique_lock<std::mutex> lock(metadataMutex);

		for (auto file : fileInfoList)
		{
			if (file->handle == handle)
			{
				retVal = file->state;
				break;
			}
		}
		return retVal;
	}

	bool FileServerClient::open_file(std::string &fileName, bool createIfNotPresent, bool exclusiveAccess, FileOpenMode openMode, FilePointerMode pointerMode)
	{
		bool fileAlreadyInList = false;

		const std::unique_lock<std::mutex> lock(metadataMutex);

		for (auto file : fileInfoList)
		{
			if (file->fileName == fileName)
			{
				fileAlreadyInList = true;
				break;
			}
		}

		if (!fileAlreadyInList)
		{
			auto newFileMetadata = std::make_shared<FileInfo>();

			newFileMetadata->fileName = fileName;
			newFileMetadata->createIfNotPresent = createIfNotPresent;
			newFileMetadata->exclusiveAccess = exclusiveAccess;
			newFileMetadata->openMode = openMode;
			newFileMetadata->pointerMode = pointerMode;
			newFileMetadata->transactionNumberForRequest = transactionNumber;
			newFileMetadata->state = FileState::SendOpenFile;
			newFileMetadata->handle = INVALID_FILE_HANDLE;
			transactionNumber++;

			fileInfoList.push_back(newFileMetadata);
		}
		return !fileAlreadyInList;
	}

	bool FileServerClient::close_file(std::uint8_t handle)
	{
		bool retVal = false;

		const std::lock_guard<std::mutex> lock(metadataMutex);

		for (auto file : fileInfoList)
		{
			if (file->handle == handle)
			{
				file->state = FileState::SendCloseFile;
				file->transactionNumberForRequest = transactionNumber;
				transactionNumber++;
				retVal = true;
				break;
			}
		}
		return retVal;
	}

	bool FileServerClient::write_file(std::uint8_t handle, const std::uint8_t *data, std::uint8_t dataSize)
	{
		bool retVal = false;

		if ((INVALID_FILE_HANDLE == currentFileWriteHandle) &&
		    (nullptr != data) &&
		    (0 != dataSize))
		{
			for (auto file : fileInfoList)
			{
				if (file->handle == handle)
				{
					// Handle is valid for a file we're managing
					currentFileWriteHandle = handle;
					currentFileWriteData = data;
					currentFileWriteSize = dataSize;
					retVal = true;
					break;
				}
			}
		}
		return retVal;
	}

	bool FileServerClient::request_current_volume_status(std::string volumeName) const
	{
		constexpr std::size_t FIXED_HEADER_LENGTH_BYTES = 5;

		std::vector<std::uint8_t> buffer;

		assert(volumeName.size() < 0xFFFF); // Max length is 65535 since only 2 bytes are allocated for length
		buffer.resize(FIXED_HEADER_LENGTH_BYTES + volumeName.size());

		if (buffer.size() < CAN_DATA_LENGTH)
		{
			buffer.resize(CAN_DATA_LENGTH);
		}

		std::uint16_t pathLength = static_cast<std::uint16_t>(volumeName.size());

		buffer[0] = static_cast<std::uint8_t>(ClientToFileServerMultiplexor::VolumeStatusRequest);
		buffer[1] = 0x00; // Current Status request (B.30)
		buffer[2] = static_cast<std::uint8_t>(pathLength & 0xFF);
		buffer[3] = static_cast<std::uint8_t>(pathLength >> 8);

		for (std::size_t i = 0; i < volumeName.size(); i++)
		{
			buffer[4 + i] = volumeName[i];
		}

		if ((pathLength + FIXED_HEADER_LENGTH_BYTES) < CAN_DATA_LENGTH)
		{
			for (std::size_t i = (pathLength + FIXED_HEADER_LENGTH_BYTES) - 1; i < CAN_DATA_LENGTH; i++)
			{
				buffer[i] = 0xFF; // Reserved bytes
			}
		}
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
		                                                      buffer.data(),
		                                                      buffer.size(),
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
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
						set_state(StateMachineState::WaitForChangeToManufacturerDirectoryResponse);
					}
				}
				break;

				case StateMachineState::WaitForChangeToManufacturerDirectoryResponse:
				{
					if ((SystemTiming::time_expired_ms(lastServerStatusTimestamp_ms, SERVER_STATUS_MESSAGE_TIMEOUT_MS)) ||
					    (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, GENERAL_OPERATION_TIMEOUT)))
					{
						set_state(StateMachineState::Disconnected);
						lastServerStatusTimestamp_ms = 0;
					}
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

				case StateMachineState::SendChangeDirectoryRequest:
				{
				}
				break;

				case StateMachineState::WaitForChangeDirectoryResponse:
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
			update_open_files();
			txFlags.process_all_flags();
		}
	}

	void FileServerClient::clear_all_file_metadata()
	{
		const std::lock_guard<std::mutex> lock(metadataMutex);
		fileInfoList.clear();
	}

	std::string FileServerClient::error_code_to_string(ErrorCode errorCode) const
	{
		std::string retVal = "Undefined Error"; // Perhaps the file server version is newer than we support?

		auto errorText = ERROR_TO_STRING_MAP.find(errorCode);
		if (ERROR_TO_STRING_MAP.end() != errorText)
		{
			retVal = errorText->second;
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
			auto &messageData = message->get_data();

			switch (messageData[0])
			{
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::FileServerStatus):
				{
					if (CAN_DATA_LENGTH == message->get_data_length())
					{
						if (0 == lastServerStatusTimestamp_ms)
						{
							CANStackLogger::debug("[FS]: New file server status detected. State machine started.");
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
						CANStackLogger::warn("[FS]: Detected malformed file server status message. DLC must be 8.");
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
							CANStackLogger::warn("[FS]: Received an unexpected response to get file server properties message");
						}
					}
					else
					{
						CANStackLogger::warn("[FS]: Detected malformed file server properties response message. DLC must be 8.");
					}
				}
				break;

				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::ChangeCurrentDirectoryResponse):
				{
					if (CAN_DATA_LENGTH == message->get_data_length())
					{
						if (StateMachineState::WaitForChangeToManufacturerDirectoryResponse == get_state())
						{
							set_state(StateMachineState::Connected);
						}
					}
					else
					{
						CANStackLogger::warn("[FS]: Detected malformed file server change current directory response message. DLC must be 8.");
					}
				}
				break;

				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::OpenFileResponse):
				{
					if (CAN_DATA_LENGTH == message->get_data_length())
					{
						bool foundMatchingFileInList = false;

						std::unique_lock<std::mutex> lock(metadataMutex);

						for (auto file : fileInfoList)
						{
							if (file->transactionNumberForRequest == messageData[1])
							{
								foundMatchingFileInList = true;
								ErrorCode operationErrorState = static_cast<ErrorCode>(messageData[2]);

								if (ErrorCode::Success == operationErrorState)
								{
									file->handle = messageData[3];
									set_file_state(file, FileState::FileOpen);
								}
								else
								{
									CANStackLogger::error("[FS]: Open file failed for file " + file->fileName + " with error code: " + error_code_to_string(operationErrorState));
								}

								break;
							}
						}

						if (!foundMatchingFileInList)
						{
							CANStackLogger::error("[FS]: Open file response TAN could not be matched with any known file. The message will be ignored.");
						}
					}
					else
					{
						CANStackLogger::warn("[FS]: Detected malformed file server open file response message. DLC must be 8.");
					}
				}
				break;

				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::CloseFileResponse):
				{
					if (CAN_DATA_LENGTH == message->get_data_length())
					{
						bool foundMatchingFileInList = false;

						std::unique_lock<std::mutex> lock(metadataMutex);

						for (auto file = fileInfoList.begin(); file != fileInfoList.end(); file++)
						{
							if ((*file)->transactionNumberForRequest == messageData[1])
							{
								foundMatchingFileInList = true;

								ErrorCode operationErrorState = static_cast<ErrorCode>(messageData[2]);

								if (ErrorCode::Success == operationErrorState)
								{
									fileInfoList.erase(file);
								}
								else
								{
									CANStackLogger::error("[FS]: Close file failed for file " + (*file)->fileName + " with error code: " + error_code_to_string(operationErrorState));
								}
								break;
							}
						}

						if (!foundMatchingFileInList)
						{
							CANStackLogger::error("[FS]: Close file response TAN could not be matched with any known file. The message will be ignored.");
						}
					}
					else
					{
						CANStackLogger::warn("[FS]: Detected malformed close file response message. DLC must be 8.");
					}
				}
				break;

				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::WriteFileResponse):
				{
					if (CAN_DATA_LENGTH == message->get_data_length())
					{
						bool foundMatchingFileInList = false;

						std::unique_lock<std::mutex> lock(metadataMutex);

						for (auto file : fileInfoList)
						{
							if (file->transactionNumberForRequest == messageData[1])
							{
								foundMatchingFileInList = true;
								ErrorCode operationErrorState = static_cast<ErrorCode>(messageData[2]);

								if (ErrorCode::Success == operationErrorState)
								{
									if (file->state == FileState::WaitForWriteFileResponse)
									{
										file->state = FileState::FileOpen;
									}
									else
									{
										// This shouldn't be possible unless something very strange is happening... if you see this issue, consider filing an issue on GitHub
										CANStackLogger::error("[FS]: Write file transaction succeeded, but matching file is not in the correct state!");
									}
								}
								else
								{
									CANStackLogger::error("[FS]: Write file failed for file " + file->fileName + " with error code: " + error_code_to_string(operationErrorState));
								}
								break;
							}
						}

						if (!foundMatchingFileInList)
						{
							CANStackLogger::error("[FS]: Write file response TAN could not be matched with any known file. The message will be ignored.");
						}
					}
					else
					{
						CANStackLogger::warn("[FS]: Detected malformed write file response message. DLC must be 8.");
					}
				}
				break;

				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::VolumeStatusResponse):
				{
				}
				break;

				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::GetCurrentDirectoryResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::SeekFileResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::ReadFileResponse):
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

	bool FileServerClient::process_internal_file_write_callback(std::uint32_t,
	                                                            std::uint32_t bytesOffset,
	                                                            std::uint32_t numberOfBytesNeeded,
	                                                            std::uint8_t *chunkBuffer,
	                                                            void *parentPointer)
	{
		bool retVal = false;

		if ((nullptr != parentPointer) &&
		    (nullptr != chunkBuffer) &&
		    (0 != numberOfBytesNeeded))
		{
			FileServerClient *parentFSClient = reinterpret_cast<FileServerClient *>(parentPointer);

			if ((bytesOffset + numberOfBytesNeeded) < parentFSClient->currentFileWriteSize)
			{
				// We've got more data to transfer
				retVal = true;
				if (0 == bytesOffset)
				{
					chunkBuffer[0] = static_cast<std::uint8_t>(ClientToFileServerMultiplexor::WriteFileRequest);
					std::memcpy(&chunkBuffer[1], &parentFSClient->currentFileWriteData[bytesOffset], numberOfBytesNeeded - 1);
				}
				else
				{
					// Subtract off 1 to account for the mux in the first byte of the message
					std::memcpy(chunkBuffer, &parentFSClient->currentFileWriteData[bytesOffset - 1], numberOfBytesNeeded);
				}
			}
			else if ((bytesOffset + numberOfBytesNeeded) == parentFSClient->currentFileWriteSize + 1)
			{
				// We have a final non-aligned amount to transfer
				retVal = true;
				// Subtract off 1 to account for the mux in the first byte of the message
				std::memcpy(chunkBuffer, &parentFSClient->currentFileWriteData[bytesOffset - 1], numberOfBytesNeeded);
			}
		}
		return retVal;
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

	bool FileServerClient::send_client_connection_maintenance() const
	{
		constexpr std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(ClientToFileServerMultiplexor::ClientConnectionMaintenance),
			                                                             static_cast<std::uint8_t>(VersionNumber::SecondPublishedEdition),
			                                                             0xFF,
			                                                             0xFF,
			                                                             0xFF,
			                                                             0xFF,
			                                                             0xFF,
			                                                             0xFF };

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool FileServerClient::send_close_file(std::shared_ptr<FileInfo> fileMetadata) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(ClientToFileServerMultiplexor::CloseFileRequest),
			                                                   fileMetadata->transactionNumberForRequest,
			                                                   fileMetadata->handle,
			                                                   0xFF,
			                                                   0xFF,
			                                                   0xFF,
			                                                   0xFF,
			                                                   0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool FileServerClient::send_get_file_server_properties() const
	{
		constexpr std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(ClientToFileServerMultiplexor::GetFileServerProperties),
			                                                             0xFF,
			                                                             0xFF,
			                                                             0xFF,
			                                                             0xFF,
			                                                             0xFF,
			                                                             0xFF,
			                                                             0xFF };

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool FileServerClient::send_open_file(std::shared_ptr<FileInfo> fileMetadata) const
	{
		std::vector<std::uint8_t> buffer;
		bool retVal = false;

		if (StateMachineState::Connected == get_state())
		{
			buffer.resize(5 + fileMetadata->fileName.size());

			if (buffer.size() < CAN_DATA_LENGTH)
			{
				buffer.resize(CAN_DATA_LENGTH);
			}

			buffer[0] = static_cast<std::uint8_t>(ClientToFileServerMultiplexor::OpenFileRequest);
			buffer[1] = fileMetadata->transactionNumberForRequest;
			buffer[2] = ((static_cast<std::uint8_t>(fileMetadata->openMode) & 0x03) |
			             (fileMetadata->createIfNotPresent ? (0x01 << 2) : 0x00) |
			             (static_cast<std::uint8_t>(fileMetadata->pointerMode) << 3) |
			             (fileMetadata->exclusiveAccess ? (0x01 << 4) : 0x00));
			buffer[3] = (fileMetadata->fileName.size() & 0xFF);
			buffer[4] = ((fileMetadata->fileName.size() >> 8) & 0xFF);

			for (std::size_t i = 0; i < fileMetadata->fileName.size(); i++)
			{
				buffer[5 + i] = fileMetadata->fileName[i];
			}

			if (buffer.size() < CAN_DATA_LENGTH)
			{
				for (std::size_t i = buffer.size(); i < CAN_DATA_LENGTH; i++)
				{
					buffer[i] = 0xFF;
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

	void FileServerClient::set_state(StateMachineState state)
	{
		stateMachineTimestamp_ms = SystemTiming::get_timestamp_ms();
		currentState = state;
	}

	void FileServerClient::set_state(StateMachineState state, std::uint32_t timestamp_ms)
	{
		stateMachineTimestamp_ms = timestamp_ms;
		currentState = state;
	}

	void FileServerClient::set_file_state(std::shared_ptr<FileInfo> fileMetadata, FileState state)
	{
		if (nullptr != fileMetadata)
		{
			fileMetadata->state = state;
			fileMetadata->timstamp_ms = SystemTiming::get_timestamp_ms();
		}
	}

	void FileServerClient::update_open_files()
	{
		for (auto file : fileInfoList)
		{
			switch (file->state)
			{
				case FileState::SendOpenFile:
				{
					if (send_open_file(file))
					{
						set_file_state(file, FileState::WaitForOpenFileResponse);
					}
					else if (SystemTiming::time_expired_ms(file->timstamp_ms, GENERAL_OPERATION_TIMEOUT))
					{
						CANStackLogger::error("[FS]: Timeout trying to send an open file message.");
						set_file_state(file, FileState::FileOpenFailed);
					}
				}
				break;

				case FileState::WaitForOpenFileResponse:
				{
					if (SystemTiming::time_expired_ms(file->timstamp_ms, GENERAL_OPERATION_TIMEOUT))
					{
						CANStackLogger::error("[FS]: Timeout waiting to an open file response message.");
						set_file_state(file, FileState::FileOpenFailed);
					}
				}
				break;

				case FileState::SendCloseFile:
				{
					if (send_close_file(file))
					{
						set_file_state(file, FileState::WaitForCloseFileResponse);
					}
					else if (SystemTiming::time_expired_ms(file->timstamp_ms, GENERAL_OPERATION_TIMEOUT))
					{
						CANStackLogger::error("[FS]: Timeout trying to send a close file message.");
					}
				}
				break;

				case FileState::WaitForCloseFileResponse:
				{
					if (SystemTiming::time_expired_ms(file->timstamp_ms, GENERAL_OPERATION_TIMEOUT))
					{
						// todo
					}
				}
				break;

				default:
				case FileState::Uninitialized:
				case FileState::FileOpenFailed:
				{
				}
				break;
			}
		}
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
