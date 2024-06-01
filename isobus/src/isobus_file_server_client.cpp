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
#include "isobus/utility/to_string.hpp"

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
	  myControlFunction(clientSource)
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

		const std::lock_guard<std::mutex> lock(metadataMutex);

		if (path != currentDirectory)
		{
			retVal = send_change_current_directory_request(path);
		}
		return retVal;
	}

	std::string FileServerClient::get_current_directory() const
	{
		return currentDirectory;
	}

	bool FileServerClient::get_file_attribute(std::uint8_t handle, FileHandleAttributesBit attributeToGet)
	{
		bool retVal = false;

		const std::lock_guard<std::mutex> lock(metadataMutex);

		for (const auto &file : fileInfoList)
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

		const std::lock_guard<std::mutex> lock(metadataMutex);

		for (const auto &file : fileInfoList)
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

		for (const auto &file : fileInfoList)
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

		const std::lock_guard<std::mutex> lock(metadataMutex);

		for (const auto &file : fileInfoList)
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
			newFileMetadata->state = FileState::WaitForConnection;
			newFileMetadata->handle = INVALID_FILE_HANDLE;
			transactionNumber++;

			fileInfoList.push_back(newFileMetadata);
		}
		return !fileAlreadyInList;
	}

	bool FileServerClient::create_manufacturer_directory(std::string &fileName)
	{
		std::vector<std::uint8_t> buffer;
		bool retVal = false;

		if (false == waitingOnOperation)
		{
			buffer.resize(5 + fileName.size());

			if (buffer.size() < CAN_DATA_LENGTH)
			{
				buffer.resize(CAN_DATA_LENGTH);
			}

			buffer[0] = static_cast<std::uint8_t>(ClientToFileServerMultiplexor::OpenFileRequest);
			buffer[1] = transactionNumber;
			transactionNumber++;
			buffer[2] = ((static_cast<std::uint8_t>(FileOpenMode::OpenDirectory) & 0x03) |
			             (0x01 << 2) |
			             (static_cast<std::uint8_t>(FilePointerMode::AppendMode) << 3));
			buffer[3] = (fileName.size() & 0xFF);
			buffer[4] = ((fileName.size() >> 8) & 0xFF);

			for (std::size_t i = 0; i < fileName.size(); i++)
			{
				buffer[5 + i] = fileName[i];
			}

			while (buffer.size() < CAN_DATA_LENGTH)
			{
				buffer.push_back(0xFF);
			}
			waitingOnOperation = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
			                                                                    buffer.data(),
			                                                                    buffer.size(),
			                                                                    myControlFunction,
			                                                                    partnerControlFunction,
			                                                                    CANIdentifier::CANPriority::PriorityLowest7);

			retVal = waitingOnOperation;
		}
		return retVal;
	}

	bool FileServerClient::close_file(std::uint8_t handle)
	{
		bool retVal = false;

		const std::lock_guard<std::mutex> lock(metadataMutex);

		for (auto &file : fileInfoList)
		{
			if (file->handle == handle)
			{
				set_file_state(file, FileState::SendCloseFile);
				file->transactionNumberForRequest = transactionNumber;
				transactionNumber++;
				retVal = true;
				break;
			}
		}
		return retVal;
	}

	bool FileServerClient::write_file(std::uint8_t handle, const std::uint8_t *data, std::uint8_t dataSize, std::function<void(std::uint8_t handle, bool success)> writeFileCallback)
	{
		bool retVal = false;

		const std::lock_guard<std::mutex> lock(metadataMutex);

		if ((INVALID_FILE_HANDLE == currentFileOperationHandle) &&
		    (nullptr != data) &&
		    (0 != dataSize))
		{
			for (auto &file : fileInfoList)
			{
				if (file->handle == handle)
				{
					// Handle is valid for a file we're managing
					currentFileOperationHandle = handle;
					currentFileWriteData = data;
					currentFileOperationSize = dataSize;
					file->writeCallback = writeFileCallback;
					file->state = FileState::SendWriteFile;
					retVal = true;
					break;
				}
			}
		}
		return retVal;
	}

	bool FileServerClient::read_file(std::uint8_t handle, std::uint16_t numberOfBytesToRead, std::function<void(std::uint8_t handle, bool success, const std::vector<std::uint8_t> data)> readDataCallback)
	{
		bool retVal = false;

		const std::lock_guard<std::mutex> lock(metadataMutex);

		if (INVALID_FILE_HANDLE == currentFileOperationHandle)
		{
			for (auto &file : fileInfoList)
			{
				if (file->handle == handle)
				{
					currentFileOperationHandle = handle;
					currentFileOperationSize = numberOfBytesToRead;
					file->readCallback = readDataCallback;
					set_file_state(file, FileState::SendReadFile);
					break;
				}
			}
		}
		return retVal;
	}

	bool FileServerClient::request_current_volume_status(std::string volumeName)
	{
		bool retVal = false;

		if (get_is_ready_for_new_command())
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
			waitingOnOperation = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
			                                                                    buffer.data(),
			                                                                    buffer.size(),
			                                                                    myControlFunction,
			                                                                    partnerControlFunction,
			                                                                    CANIdentifier::CANPriority::PriorityLowest7);
			retVal = waitingOnOperation;
		}
		return retVal;
	}

	EventDispatcher<FileServerClient::VolumeStatusInfo> &FileServerClient::get_volume_status_event_dispatcher()
	{
		return volumeStatusEventDispatcher;
	}

	bool FileServerClient::initialize(bool spawnThread)
	{
		if ((!initialized) &&
		    (nullptr != partnerControlFunction))
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

	FileServerClient::FileServerProperties FileServerClient::get_file_server_properties() const
	{
		FileServerProperties retVal;

		switch (get_state())
		{
			case StateMachineState::Disconnected:
			case StateMachineState::SendGetFileServerProperties:
			case StateMachineState::WaitForGetFileServerPropertiesResponse:
			{
				// Not received yet
			}
			break;

			default:
			{
				retVal.fileServerVersion = fileServerVersion;
				retVal.maxNumberSimultaneouslyOpenFiles = maxNumberSimultaneouslyOpenFiles;
				retVal.supportsMultipleVolumes = (0 != (fileServerCapabilitiesBitfield & (1 << static_cast<std::uint8_t>(FileServerCapabilities::SupportsMultipleVolumes))));
				retVal.supportsRemovableVolumes = (0 != (fileServerCapabilitiesBitfield & (1 << static_cast<std::uint8_t>(FileServerCapabilities::SupportsRemovableVolumes))));
			}
			break;
		}
		return retVal;
	}

	FileServerClient::StateMachineState FileServerClient::get_state() const
	{
		return currentState;
	}

	bool FileServerClient::get_is_initialized() const
	{
		return initialized;
	}

	bool FileServerClient::get_is_ready_for_new_command() const
	{
		return (initialized && (StateMachineState::Connected == get_state()) && !waitingOnOperation);
	}

	void FileServerClient::update()
	{
		if (nullptr != partnerControlFunction)
		{
			const std::lock_guard<std::mutex> lock(metadataMutex);

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

				case StateMachineState::ChangeToRootDirectory:
				{
					if (send_change_current_directory_request("\\"))
					{
						set_state(StateMachineState::WaitForChangeToRootDirectory);
					}
					else if ((SystemTiming::time_expired_ms(lastServerStatusTimestamp_ms, SERVER_STATUS_MESSAGE_TIMEOUT_MS)) ||
					         (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, GENERAL_OPERATION_TIMEOUT)))
					{
						set_state(StateMachineState::Disconnected);
						lastServerStatusTimestamp_ms = 0;
					}
				}
				break;

				case StateMachineState::WaitForChangeToRootDirectory:
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
					if ((SystemTiming::time_expired_ms(lastServerStatusTimestamp_ms, SERVER_STATUS_MESSAGE_TIMEOUT_MS)) ||
					    (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, GENERAL_OPERATION_TIMEOUT)))
					{
						CANStackLogger::error("[FS]: Timeout trying to change directory to ~\\");
						set_state(StateMachineState::Disconnected);
						lastServerStatusTimestamp_ms = 0;
					}
				}
				break;

				case StateMachineState::WaitForChangeToManufacturerDirectoryResponse:
				{
					if ((SystemTiming::time_expired_ms(lastServerStatusTimestamp_ms, SERVER_STATUS_MESSAGE_TIMEOUT_MS)) ||
					    (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, GENERAL_OPERATION_TIMEOUT)))
					{
						CANStackLogger::error("[FS]: Timeout waiting to change to manufacturer directory.");
						set_state(StateMachineState::Disconnected);
						lastServerStatusTimestamp_ms = 0;
					}
				}
				break;

				case StateMachineState::CreateManufacturerDirectory:
				{
					// This "MCMC" format for your home directory is defined in ISO11783-13 and is required
					if (true == create_manufacturer_directory(".\\MCMC" + isobus::to_string(static_cast<int>(myControlFunction->get_NAME().get_manufacturer_code())) + "\\"))
					{
						set_state(StateMachineState::WaitForCreateManufacturerDirectory);
					}
					else if ((SystemTiming::time_expired_ms(lastServerStatusTimestamp_ms, SERVER_STATUS_MESSAGE_TIMEOUT_MS)) ||
					         (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, GENERAL_OPERATION_TIMEOUT)))
					{
						CANStackLogger::error("[FS]: Timeout creating manufacturer directory.");
						set_state(StateMachineState::Disconnected);
						lastServerStatusTimestamp_ms = 0;
					}
				}
				break;

				case StateMachineState::WaitForCreateManufacturerDirectory:
				{
					if ((SystemTiming::time_expired_ms(lastServerStatusTimestamp_ms, SERVER_STATUS_MESSAGE_TIMEOUT_MS)) ||
					    (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, GENERAL_OPERATION_TIMEOUT)))
					{
						CANStackLogger::error("[FS]: Timeout waiting for creation of manufacturer directory to complete.");
						set_state(StateMachineState::Disconnected);
						lastServerStatusTimestamp_ms = 0;
					}
				}
				break;

				case StateMachineState::Connected:
				{
					if ((SystemTiming::time_expired_ms(stateMachineTimestamp_ms, CLIENT_STATUS_MESSAGE_REPETITION_RATE_MS)) &&
					    (send_client_connection_maintenance()))
					{
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

	void FileServerClient::process_message(const CANMessage &message)
	{
		if ((nullptr != partnerControlFunction) &&
		    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::FileServerToClient) == message.get_identifier().get_parameter_group_number()) &&
		    ((message.get_source_control_function()->get_address() == partnerControlFunction->get_address()) ||
		     (nullptr == message.get_destination_control_function())))
		{
			auto &messageData = message.get_data();
			std::unique_lock<std::mutex> lock(metadataMutex);

			switch (messageData[0])
			{
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::FileServerStatus):
				{
					if (CAN_DATA_LENGTH == message.get_data_length())
					{
						if (0 == lastServerStatusTimestamp_ms)
						{
							CANStackLogger::debug("[FS]: New file server status detected. State machine started.");
						}

						fileServerStatusBitfield = messageData[1];
						numberFilesOpen = messageData[2];

						if (0 == lastServerStatusTimestamp_ms)
						{
							send_client_connection_maintenance();
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
					if (CAN_DATA_LENGTH == message.get_data_length())
					{
						if (StateMachineState::WaitForGetFileServerPropertiesResponse == get_state())
						{
							fileServerVersion = messageData[1];
							maxNumberSimultaneouslyOpenFiles = messageData[2];
							fileServerCapabilitiesBitfield = messageData[3];
							CANStackLogger::info("[FS]: File server is version " +
							                     isobus::to_string(static_cast<int>(fileServerVersion)) +
							                     ", supports up to " +
							                     isobus::to_string(static_cast<int>(maxNumberSimultaneouslyOpenFiles)) +
							                     " open files, and supports " + (0 == fileServerCapabilitiesBitfield ? "1 volume." : "multiple volumes."));
							fileServerPropertiesEventDispatcher.invoke(get_file_server_properties());
							set_state(StateMachineState::ChangeToRootDirectory);
							waitingOnOperation = false;
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
					if (CAN_DATA_LENGTH == message.get_data_length())
					{
						if (StateMachineState::WaitForChangeToManufacturerDirectoryResponse == get_state())
						{
							if (0 == messageData[2])
							{
								set_state(StateMachineState::Connected);
							}
							else
							{
								CANStackLogger::error("[FS]: Error changing to manufacturer directory: " +
								                      error_code_to_string(static_cast<ErrorCode>(messageData[2])) +
								                      ". Directory will be created.");
								set_state(StateMachineState::CreateManufacturerDirectory);
							}
						}
						else if (StateMachineState::WaitForChangeToRootDirectory == get_state())
						{
							if (0 == messageData[2])
							{
								CANStackLogger::debug("[FS]: Changed to root directory.");
								set_state(StateMachineState::ChangeToManufacturerDirectory);
							}
							else
							{
								CANStackLogger::error("[FS]: Error changing to root directory of the file server : " +
								                      error_code_to_string(static_cast<ErrorCode>(messageData[2])) +
								                      ". Connection will not be recovered.");
								set_state(StateMachineState::Disconnected);
								terminate();
							}
						}
						waitingOnOperation = false;
					}
					else
					{
						CANStackLogger::warn("[FS]: Detected malformed file server change current directory response message. DLC must be 8.");
					}
				}
				break;

				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::OpenFileResponse):
				{
					if (CAN_DATA_LENGTH == message.get_data_length())
					{
						if (StateMachineState::WaitForCreateManufacturerDirectory == get_state())
						{
							waitingOnOperation = false;
							CANStackLogger::info("[FS]: Created new manufacturer directory.");
							set_state(StateMachineState::ChangeToManufacturerDirectory);
						}
						else
						{
							bool foundMatchingFileInList = false;

							for (auto &file : fileInfoList)
							{
								if (file->transactionNumberForRequest == messageData[1])
								{
									foundMatchingFileInList = true;
									ErrorCode operationErrorState = static_cast<ErrorCode>(messageData[2]);

									if (ErrorCode::Success == operationErrorState)
									{
										file->handle = messageData[3];
										set_file_state(file, FileState::FileOpen);
										CANStackLogger::debug("[FS]: File opened.");
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
							else
							{
								waitingOnOperation = false;
							}
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
					if (CAN_DATA_LENGTH == message.get_data_length())
					{
						bool foundMatchingFileInList = false;

						for (auto file = fileInfoList.begin(); file != fileInfoList.end(); file++)
						{
							if ((*file)->transactionNumberForRequest == messageData[1])
							{
								foundMatchingFileInList = true;

								ErrorCode operationErrorState = static_cast<ErrorCode>(messageData[2]);

								if (ErrorCode::Success == operationErrorState)
								{
									fileInfoList.erase(file);
									CANStackLogger::debug("[FS]: Closed file OK.");
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
						else
						{
							waitingOnOperation = false;
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
					if (CAN_DATA_LENGTH == message.get_data_length())
					{
						bool foundMatchingFileInList = false;

						for (auto &file : fileInfoList)
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
										CANStackLogger::debug("[FS]: Write file transaction succeeded");
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
								waitingOnOperation = false;
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
					if (message.get_data_length() >= 7)
					{
						VolumeStatusInfo currentVolumeStatus;
						std::string statusString = "[FS]: Volume ";

						auto volumeNameLength = message.get_uint16_at(4);

						if (message.get_data_length() < (6 + volumeNameLength))
						{
							CANStackLogger::warn("[FS]: Detected malformed volume status response message. Message is shorter than the indicated length.");
						}

						for (std::uint32_t i = 6; i < message.get_data_length(); i++)
						{
							currentVolumeStatus.volumeName.push_back(static_cast<char>(messageData[i]));
						}
						statusString.append(currentVolumeStatus.volumeName);
						statusString.append(" status: ");

						if (0 != ((1 << static_cast<std::uint8_t>(VolumeStatus::Present)) & (messageData[1])))
						{
							currentVolumeStatus.currentStatuses.push_back(VolumeStatus::Present);
							statusString.append(" [Present] ");
						}
						else
						{
							statusString.append(" [Not Present] ");
						}
						if (0 != ((1 << static_cast<std::uint8_t>(VolumeStatus::InUse)) & (messageData[1])))
						{
							currentVolumeStatus.currentStatuses.push_back(VolumeStatus::InUse);
							statusString.append(" [In Use] ");
						}
						else
						{
							statusString.append(" [Not In Use] ");
						}
						if (0 != ((1 << static_cast<std::uint8_t>(VolumeStatus::PreparingForRemoval)) & (messageData[1])))
						{
							currentVolumeStatus.currentStatuses.push_back(VolumeStatus::PreparingForRemoval);
							statusString.append(" [Preparing for Removal] ");
						}
						else
						{
							statusString.append(" [Not Preparing for Removal] ");
						}
						if (0 != ((1 << static_cast<std::uint8_t>(VolumeStatus::Removed)) & (messageData[1])))
						{
							currentVolumeStatus.currentStatuses.push_back(VolumeStatus::Removed);
							statusString.append(" [Removed] ");
						}
						else
						{
							statusString.append(" [Not Removed] ");
						}
						currentVolumeStatus.maximumTimeBeforeRemoval = messageData[2];
						statusString.append(". Max time before removal: ");
						statusString.append(isobus::to_string(static_cast<int>(currentVolumeStatus.maximumTimeBeforeRemoval)));

						CANStackLogger::debug(statusString);
						volumeStatusEventDispatcher.invoke(std::move(currentVolumeStatus));
						waitingOnOperation = false;
					}
					else
					{
						CANStackLogger::warn("[FS]: Detected malformed volume status response message. DLC must be >= 7.");
					}
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

	void FileServerClient::process_message(const CANMessage &message, void *parent)
	{
		if (nullptr != parent)
		{
			static_cast<FileServerClient *>(parent)->process_message(message);
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

			if ((bytesOffset + numberOfBytesNeeded) < parentFSClient->currentFileOperationSize)
			{
				// We've got more data to transfer
				retVal = true;
				if (0 == bytesOffset)
				{
					assert(parentFSClient->currentFileOperationSize <= 65530); // You can only write 65530 bytes or less at a time!
					const std::lock_guard<std::mutex> lock(parentFSClient->metadataMutex); // Locking to make sure we are the only ones messing with the TAN, since we're on the stack thread in here
					chunkBuffer[0] = static_cast<std::uint8_t>(ClientToFileServerMultiplexor::WriteFileRequest);
					chunkBuffer[1] = parentFSClient->transactionNumber;
					parentFSClient->transactionNumber++;
					chunkBuffer[2] = parentFSClient->currentFileOperationHandle;
					chunkBuffer[3] = static_cast<std::uint8_t>(parentFSClient->currentFileOperationSize);
					chunkBuffer[4] = static_cast<std::uint8_t>(parentFSClient->currentFileOperationSize >> 8);
					std::memcpy(&chunkBuffer[5], &parentFSClient->currentFileWriteData[bytesOffset], numberOfBytesNeeded - 5);
				}
				else
				{
					// Subtract off 1 to account for the mux in the first byte of the message
					std::memcpy(chunkBuffer, &parentFSClient->currentFileWriteData[bytesOffset - 1], numberOfBytesNeeded);
				}
			}
			else if ((bytesOffset + numberOfBytesNeeded) == parentFSClient->currentFileOperationSize + 1)
			{
				// We have a final non-aligned amount to transfer
				retVal = true;
				// Subtract off 1 to account for the mux in the first byte of the message
				std::memcpy(chunkBuffer, &parentFSClient->currentFileWriteData[bytesOffset - 1], numberOfBytesNeeded);
			}
		}
		return retVal;
	}

	void FileServerClient::write_file_tx_callback(std::uint32_t parameterGroupNumber,
	                                              std::uint32_t dataLength,
	                                              std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                                              std::shared_ptr<ControlFunction> destinationControlFunction,
	                                              bool successful,
	                                              void *parentPointer)
	{
		auto targetInterface = static_cast<FileServerClient *>(parentPointer);

		if (nullptr != targetInterface)
		{
			for (auto &file : targetInterface->fileInfoList)
			{
				if (file->handle == targetInterface->currentFileOperationHandle)
				{
					targetInterface->waitingOnOperation = false;
					targetInterface->set_file_state(file, FileState::WaitForWriteFileResponse);

					if (false == successful)
					{
						file->writeCallback(file->handle, false);
					}
					break;
				}
			}
		}
	}

	bool FileServerClient::send_change_current_directory_request(std::string path)
	{
		std::vector<std::uint8_t> buffer;
		std::vector<std::uint8_t>::iterator it;
		bool retVal = false;

		if ((0 != path.size()) &&
		    (StateMachineState::Disconnected != get_state()) &&
		    !waitingOnOperation)
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

			waitingOnOperation = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
			                                                                    buffer.data(),
			                                                                    buffer.size(),
			                                                                    myControlFunction,
			                                                                    partnerControlFunction,
			                                                                    CANIdentifier::CANPriority::PriorityLowest7);
			retVal = waitingOnOperation;

			if (retVal)
			{
				transactionNumber++;
			}
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
		                                                      myControlFunction,
		                                                      partnerControlFunction,
		                                                      CANIdentifier::CANPriority::PriorityLowest7);
	}

	bool FileServerClient::send_close_file(std::shared_ptr<FileInfo> fileMetadata)
	{
		bool retVal = false;

		if (!waitingOnOperation)
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(ClientToFileServerMultiplexor::CloseFileRequest),
				                                                         fileMetadata->transactionNumberForRequest,
				                                                         fileMetadata->handle,
				                                                         0xFF,
				                                                         0xFF,
				                                                         0xFF,
				                                                         0xFF,
				                                                         0xFF };

			waitingOnOperation = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
			                                                                    buffer.data(),
			                                                                    CAN_DATA_LENGTH,
			                                                                    myControlFunction,
			                                                                    partnerControlFunction,
			                                                                    CANIdentifier::CANPriority::PriorityLowest7);
			retVal = waitingOnOperation;
		}
		return retVal;
	}

	bool FileServerClient::send_get_file_server_properties()
	{
		bool retVal = false;

		if (!waitingOnOperation)
		{
			constexpr std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(ClientToFileServerMultiplexor::GetFileServerProperties),
				                                                             0xFF,
				                                                             0xFF,
				                                                             0xFF,
				                                                             0xFF,
				                                                             0xFF,
				                                                             0xFF,
				                                                             0xFF };

			waitingOnOperation = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
			                                                                    buffer.data(),
			                                                                    CAN_DATA_LENGTH,
			                                                                    myControlFunction,
			                                                                    partnerControlFunction,
			                                                                    CANIdentifier::CANPriority::PriorityLowest7);
			retVal = waitingOnOperation;
		}
		return retVal;
	}

	bool FileServerClient::send_open_file(std::shared_ptr<FileInfo> fileMetadata)
	{
		std::vector<std::uint8_t> buffer;
		bool retVal = false;

		if ((StateMachineState::Connected == get_state()) &&
		    (false == waitingOnOperation))
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

			while (buffer.size() < CAN_DATA_LENGTH)
			{
				buffer.push_back(0xFF);
			}
			waitingOnOperation = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
			                                                                    buffer.data(),
			                                                                    buffer.size(),
			                                                                    myControlFunction,
			                                                                    partnerControlFunction,
			                                                                    CANIdentifier::CANPriority::PriorityLowest7);

			retVal = waitingOnOperation;
		}
		return retVal;
	}

	bool FileServerClient::send_write_request(std::shared_ptr<FileInfo> fileToWriteTo)
	{
		bool retVal = false;

		if (!waitingOnOperation)
		{
			waitingOnOperation = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
			                                                                    nullptr,
			                                                                    currentFileOperationSize + 5, // The data, plus TAN, handle, mux, and size
			                                                                    myControlFunction,
			                                                                    partnerControlFunction,
			                                                                    CANIdentifier::CANPriority::PriorityLowest7,
			                                                                    write_file_tx_callback,
			                                                                    this,
			                                                                    process_internal_file_write_callback);
			fileToWriteTo->transactionNumberForRequest = transactionNumber;
			retVal = waitingOnOperation;
		}
		return retVal;
	}

	bool FileServerClient::send_read_request(std::shared_ptr<FileInfo> fileToReadFrom, std::uint16_t numberBytesToRead)
	{
		bool retVal = false;

		if (!waitingOnOperation)
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(ClientToFileServerMultiplexor::ReadFileRequest),
				                                                         transactionNumber,
				                                                         fileToReadFrom->handle,
				                                                         static_cast<std::uint8_t>(numberBytesToRead),
				                                                         static_cast<std::uint8_t>(numberBytesToRead >> 8),
				                                                         0xFF, // In version 3 and prior, this would control if hidden files were viewable in a directory. Now Reserved.
				                                                         0xFF,
				                                                         0xFF };
			fileToReadFrom->transactionNumberForRequest = transactionNumber;

			waitingOnOperation = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ClientToFileServer),
			                                                                    buffer.data(),
			                                                                    buffer.size(),
			                                                                    myControlFunction,
			                                                                    partnerControlFunction,
			                                                                    CANIdentifier::CANPriority::PriorityLowest7);
			if (waitingOnOperation)
			{
				transactionNumber++;
			}
			retVal = waitingOnOperation;
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
			fileMetadata->timestamp_ms = SystemTiming::get_timestamp_ms();
		}
	}

	void FileServerClient::update_open_files()
	{
		for (auto &file : fileInfoList)
		{
			switch (file->state)
			{
				case FileState::WaitForConnection:
				{
					// We're waiting for whatever is in-progress to wrap up, like changing a directory
					if (StateMachineState::Connected == get_state())
					{
						set_file_state(file, FileState::SendOpenFile);
					}
				}
				break;

				case FileState::SendOpenFile:
				{
					if (send_open_file(file))
					{
						set_file_state(file, FileState::WaitForOpenFileResponse);
					}
					else if (SystemTiming::time_expired_ms(file->timestamp_ms, GENERAL_OPERATION_TIMEOUT))
					{
						CANStackLogger::error("[FS]: Timeout trying to send an open file message.");
						set_file_state(file, FileState::FileOpenFailed);
					}
				}
				break;

				case FileState::WaitForOpenFileResponse:
				{
					if (SystemTiming::time_expired_ms(file->timestamp_ms, GENERAL_OPERATION_TIMEOUT))
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
					else if (SystemTiming::time_expired_ms(file->timestamp_ms, GENERAL_OPERATION_TIMEOUT))
					{
						CANStackLogger::error("[FS]: Timeout trying to send a close file message.");
						set_file_state(file, FileState::FileDead);
					}
				}
				break;

				case FileState::WaitForCloseFileResponse:
				{
					if (SystemTiming::time_expired_ms(file->timestamp_ms, GENERAL_OPERATION_TIMEOUT))
					{
						CANStackLogger::error("[FS]: Expected close file response was never received.");
						set_file_state(file, FileState::FileDead);
					}
				}
				break;

				case FileState::SendWriteFile:
				{
					if (send_write_request(file))
					{
						set_file_state(file, FileState::WaitForWriteTransport);
					}
					else if (SystemTiming::time_expired_ms(file->timestamp_ms, GENERAL_OPERATION_TIMEOUT))
					{
						CANStackLogger::error("[FS]: Can't send write file message.");
						set_file_state(file, FileState::SendCloseFile);
					}
				}
				break;

				case FileState::WaitForWriteTransport:
				{
					// This could take a very long time... no timeout
				}
				break;

				case FileState::WaitForWriteFileResponse:
				{
					if (SystemTiming::time_expired_ms(file->timestamp_ms, GENERAL_OPERATION_TIMEOUT))
					{
						CANStackLogger::error("[FS]: No write file response from server.");
						set_file_state(file, FileState::FileOpen);
					}
				}
				break;

				case FileState::SendReadFile:
				{
					if (send_read_request(file, currentFileOperationSize))
					{
						set_file_state(file, FileState::WaitForReadFileResponse);
					}
					else if (SystemTiming::time_expired_ms(file->timestamp_ms, GENERAL_OPERATION_TIMEOUT))
					{
						CANStackLogger::error("[FS]: Can't send read file message.");
						set_file_state(file, FileState::SendCloseFile);
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
