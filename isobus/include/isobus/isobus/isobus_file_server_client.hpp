//================================================================================================
/// @file isobus_file_server_client.hpp
///
/// @brief Defines an interface for an ISOBUS file server (client portion) (ISO 11783-13)
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_FILE_SERVER_CLIENT_HPP
#define ISOBUS_FILE_SERVER_CLIENT_HPP

#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/utility/processing_flags.hpp"

#include <map>
#include <memory>
#include <thread>

namespace isobus
{
	/// @brief A client interface for communicating with an ISOBUS file server
	/// @note Although one instance of this client interface can manage multiple files at a time, you can
	/// only write or read from one at a time. Part of the reason for this is to avoid trying to send two
	/// transport sessions at the same time with the same PGN to the same partner, which is not supported in general.
	class FileServerClient
	{
	public:
		/// @brief Enumerates the state machine states for talking to a file server
		enum class StateMachineState
		{
			Disconnected, ///< Waiting for a server status message
			SendGetFileServerProperties, ///< Transmitting the Get File Server Properties message
			WaitForGetFileServerPropertiesResponse, ///< Waiting for a response to the Get File Server Properties message
			ChangeToRootDirectory, ///< Navigate to the '/' directory so we can check for a manufacturer directory
			WaitForChangeToRootDirectory, ///< Waiting for the file server to respond to changing the directory to '/'
			CreateManufacturerDirectory, ///< Try and create the MCMC directory correlated to our ISO NAME manufacturer code
			WaitForCreateManufacturerDirectory, ///< Wait for response to our create directory command
			ChangeToManufacturerDirectory, ///< Attempting to change directory into "~\"
			WaitForChangeToManufacturerDirectoryResponse, ///< Waiting for the response to the change directory request for "~\"
			Connected, ///< FS is connected. You can use public functions on this class to interact further from this point!
			SendChangeDirectoryRequest, ///< Changing directory
			WaitForChangeDirectoryResponse ///< Waiting for a response to a directory change. Opening files is not allowed until this operation succeeds or fails.
		};

		/// @brief Enumerates the state a file can be in
		enum class FileState
		{
			Uninitialized, ///< The initial state
			WaitForConnection, ///< Ensures the FS client is connected before proceeding
			SendOpenFile, ///< The client is sending the open file message. File is not interactable yet
			WaitForOpenFileResponse, ///< Client is waiting for a response to the open file message. File is not interactable yet
			FileOpen, ///< The file is currently open and can be interacted with
			FileOpenFailed, ///< Could not open the file! Close the file, or try and re-open it
			SendWriteFile, ///< If the write file function is called, this state sends the appropriate message
			WaitForWriteFileResponse, ///< Waiting for a response to our last write file request
			SendReadFile, ///< If the read file function is called, this state sends the appropriate message
			WaitForReadFileResponse, ///< Waiting for a response to our last read file request
			SendCloseFile, ///< Try to close the file
			WaitForCloseFileResponse ///< Waiting for a response to our request to close a file
		};

		/// @brief Enumerates the different ways a file or directory can be opened
		enum class FileOpenMode : std::uint8_t
		{
			OpenFileForReadingOnly = 0, ///< Open a file in read only mode
			OpenFileForWritingOnly = 1, ///< Open a file in write only mode
			OpenFileForReadingAndWriting = 2, ///< Open a file for both read and write mode
			OpenDirectory = 3 ///< Opens a directory
		};

		/// @brief Enumerates options for where you want the file pointer set when opening a file
		enum class FilePointerMode : std::uint8_t
		{
			RandomAccess = 0, ///< File pointer set to the start of the file
			AppendMode = 1 ///< File pointer set to the end of the file
		};

		/// @brief The different file attribute bits that can be associated with a file handle
		enum class FileHandleAttributesBit : std::uint8_t
		{
			ReadOnly = 0,
			Hidden = 1,
			VolumeSupportsHiddenAttribute = 2,
			HandleSpecifiesAVolume = 3,
			HandleSpecifiesADirectory = 4,
			VolumeSupportsLongFileNames = 5,
			VolumeIsRemovable = 6,
			VolumeIsCaseSensitive = 7
		};

		/// @brief The different read-only states you can request for a file
		enum class ReadOnlyAttributeCommand : std::uint8_t
		{
			ClearReadOnly = 0, ///< Clears the read only attribute
			SetReadOnly = 1, ///< Sets the read only attribute
			DontCare = 3 ///< Leave the read only attribute alone as it is now
		};

		/// @brief Enumerates the statuses of the volume. (This parameter applies for Version 3 and later FS.)
		enum class VolumeStatus : std::uint8_t
		{
			Present = 0, ///< Volume is present
			InUse = 1, ///< Volume is in-use
			PreparingForRemoval = 2, ///< This state will be maintained for at least 2 seconds before a volume is removed/ejected.
			Removed = 3, ///< Volume is not present or has been ejected
			Reserved = 4 ///< Start of the reserved range of statuses
		};

		static constexpr std::uint8_t INVALID_FILE_HANDLE = 0xFF; ///< Used to represent an invalid file handle

		/// @brief A collection of volume data that can be provided to the user on-change or on-request
		class VolumeStatusInfo
		{
		public:
			std::string volumeName; ///< The name of the current volume on the file server
			VolumeStatus currentStatus = VolumeStatus::Reserved; ///< The current state of the volume on the file server
			std::uint8_t maximumTimeBeforeRemoval = 0; ///< the max time that the volume could be in the VolumeStatus::PreparingForRemoval state
		};

		/// @brief The constructor for a file server client
		/// @param[in] partner The file server control function to communicate with
		/// @param[in] clientSource The internal control function to use when communicating with the file server
		FileServerClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource);

		/// @brief Destructor for a FileServerClient
		~FileServerClient();

		/// @brief Requests to change the current directory
		/// @param[in] path The directory path to change to
		/// @returns true if the request was sent to the file server, otherwise false
		bool change_directory(std::string &path);

		/// @brief Returns the current directory that we're browsing on the server. Similar to running "pwd"
		/// @returns Out current working directory on the server
		std::string get_current_directory() const;

		/// @brief Returns the state of a single file attribute
		/// @param[in] handle File handle associated to the file whose attribute you want to check
		/// @param[in] attributeToGet The attribute to check
		/// @returns true if the selected attribute is set on the file
		bool get_file_attribute(std::uint8_t handle, FileHandleAttributesBit attributeToGet);

		/// @brief Sets a file's attributes
		/// @param[in] filePath Path to the file to change attributes on
		/// @param[in] hidden true to set the hidden attribute, false to show the file
		/// @param[in] readOnly true to mark the file as read only, false to mark the file as writeable
		bool set_file_attribute(std::string filePath, bool hidden, ReadOnlyAttributeCommand readOnly);

		/// @brief Returns the file handle (if any) is accociated with a file path
		/// @param[in] filePath The file path to try and match with a handle
		/// @returns The handle associated with the file path, or INVALID_FILE_HANDLE if no match was found
		std::uint8_t get_file_handle(std::string filePath);

		/// @brief Returns the state of a file that the client is managing
		/// @param[in] handle A file handle associated to the file whose status you want retrieved.
		/// @returns The state of the file associated to the handle you passed in
		FileState get_file_state(std::uint8_t handle);

		/// @brief Opens a file for interaction
		/// @param[in] fileName The file to open
		/// @param[in] createIfNotPresent If the server should create the file if the file specified does not exist
		/// @param[in] exclusiveAccess If you want an exclusive lock on the file
		/// @param[in] openMode The mode to open the file in (read only, write only, etc)
		/// @param[in] pointerMode Where you want the file pointer placed in the file (beginning or end of the file)
		/// @returns `true` If the command was accepted and the interface will attempt to open the file as specified
		bool open_file(std::string &fileName, bool createIfNotPresent, bool exclusiveAccess, FileOpenMode openMode, FilePointerMode pointerMode);

		/// @brief Closes a file identified by a file handle
		/// @param[in] handle The file handle to close
		/// @returns `true` if the command was accepted and the interface will send the close file message
		bool close_file(std::uint8_t handle);

		/// @brief Writes data to a file associated with a handle
		/// @note File must be open to write, and your handle must be valid.
		/// @param[in] handle The file handle associated to the file you want to write to
		/// @param[in] data A pointer to some data to write
		/// @param[in] dataSize The amount of data to write in bytes
		bool write_file(std::uint8_t handle, const std::uint8_t *data, std::uint8_t dataSize);

		/// @brief Requests the volume status from the file server for a specific volume
		/// @param[in] volumeName The name of the volume to request the status of
		/// @returns true if the request was sent, otherwise false
		bool request_current_volume_status(std::string volumeName) const;

		// Setup Functions
		/// @brief This function starts the state machine. Call this once you have supplied 1 or more object pool and are ready to connect.
		/// @param[in] spawnThread The client will start a thread to manage itself if this parameter is true. Otherwise you must update it cyclically.
		bool initialize(bool spawnThread);

		/// @brief Returns if the client has been initialized
		/// @returns true if the client has been initialized
		bool get_is_initialized() const;

		/// @brief Terminates the client and joins the worker thread if applicable
		void terminate();

		/// @brief Returns the current state machine state
		/// @returns The current state machine state
		StateMachineState get_state() const;

		/// @brief Periodic Update Function (worker thread may call this)
		/// @details This class can spawn a thread, or you can supply your own to run this function.
		/// To configure that behavior, see the initialize function.
		void update();

	protected:
		/// @brief The number of the edition or version of ISO 11783-13 with which the FS or client is compliant
		enum class VersionNumber : std::uint8_t
		{
			DraftEdition = 0, ///< Draft edition of the International Standard
			FinalDraftEdition = 1, ///< Final draft edition of the International Standard
			FirstPublishedEdition = 2, ///< First published edition of the International Standard
			SecondPublishedEdition = 3, ///< Second published edition of the International Standard
			CompliantWithVersion2AndPrior = 255 ///< Compliant with Version 2 and prior (client only)
		};

		/// @brief Enumerates the different error codes for FS operations
		enum class ErrorCode : std::uint8_t
		{
			Success = 0,
			AccessDenied = 1,
			InvalidAccess = 2,
			TooManyFilesOpen = 3,
			FilePathOrVolumeNotFound = 4,
			InvalidHandle = 5,
			InvalidGivenSourceName = 6,
			InvalidGivenDestinationName = 7,
			VolumeOutOfFreeSpace = 8,
			FailureDuringAWriteOperation = 9,
			MediaNotPresent = 10, ///< formerly the code for error 13, below, in Version 2 FSs
			FailureDuringAReadOperation = 11,
			FunctionNotSupported = 12,
			VolumeIsPossiblyNotInitialized = 13,
			InvalidRequestLength = 42, ///< used when the file pointer hits the start/top of the file or on invalid space request of the volume
			OutOfMemory = 43, ///< used by FS to indicate out of resources at this time and cannot complete request
			AnyOtherError = 44,
			FilePointerAtEndOfFile = 45
		};

		/// @brief The position mode specifies the location from which the offset value is used to determine the file pointer position.
		enum class PositionMode : std::uint8_t
		{
			FromTheBeginningOfTheFile = 0, ///< From the beginning of the file
			FromTheCurrentPointerPosition = 1, ///< From the current pointer position
			FromTheEndOfTheFile = 2 ///< From the end of the file
		};

		/// @brief The multiplexor byte options for the file server to client PGN
		enum class FileServerToClientMultiplexor : std::uint8_t
		{
			FileServerStatus = 0x00, ///< The File Server Status message is sent by the FS to provide file server status information
			GetFileServerPropertiesResponse = 0x01, ///< The Get File Server Properties Response message is sent by the FS to a client in response to the Get File Server Properties message.
			VolumeStatusResponse = 0x02, ///< sent by the file server to a client in response to the Volume Status Request or on volume status change.
			GetCurrentDirectoryResponse = 0x10, ///< Sent in response to Get Current Directory Request message
			ChangeCurrentDirectoryResponse = 0x11, ///< Sent in response to Change Current Directory Request message
			OpenFileResponse = 0x20, ///< Sent in response to Open File Request message
			SeekFileResponse = 0x21, ///< Sent in response to Seek File Request message
			ReadFileResponse = 0x22, ///< The Read File Response message contains the data read from a file referred to by the Handle
			WriteFileResponse = 0x23, ///< Sent in response to Write File Request message
			CloseFileResponse = 0x24, ///< Sent in response to Close File Request message
			MoveFileResponse = 0x30, ///< Sent in response to Move File Request message
			DeleteFileResponse = 0x31, ///< Sent in response to Delete File Request message
			GetFileAttributesResponse = 0x32, ///< Sent in response to Get File Attributes Request message
			SetFileAttributesResponse = 0x33, ///< Sent in response to Set File Attributes Request message
			GetFileDateAndTimeResponse = 0x34, ///< Sent in response to Get File Date & Time Request message
			InitializeVolumeResponse = 0x40 ///< Sent in response to Initialize Volume Request message
		};

		/// @brief The multiplexor byte options for the client to file server PGN
		enum class ClientToFileServerMultiplexor : std::uint8_t
		{
			ClientConnectionMaintenance = 0x00, ///< The Client Connection Maintenance message is sent by a client in order to maintain a connection with the FS
			GetFileServerProperties = 0x01, ///< The Get File Server Properties message is sent by the client to request the FS properties.
			VolumeStatusRequest = 0x02, ///< The Volume Status Request message is sent by the client to command the file server volume status or request the current volume status.
			GetCurrentDirectoryRequest = 0x10, ///< Get Current Directory returns the current directory as a pathname
			ChangeCurrentDirectoryRequest = 0x11, ///< Change Current Directory selects the current directory.
			OpenFileRequest = 0x20, ///< Open File opens the file specified by the Path
			SeekFileRequest = 0x21, ///< Seek File sets the file pointer for the next access within a file
			ReadFileRequest = 0x22, ///< Read File reads data from the file referenced by a Handle
			WriteFileRequest = 0x23, ///< Write File writes data to an open file that is addressed by a Handle
			CloseFileRequest = 0x24, ///< Close File closes the file specified by the Handle
			MoveFileRequest = 0x30, ///< Move File moves or copies a file from its current location to a new location
			DeleteFileRequest = 0x31, ///< Delete File deletes a file from its current location
			GetFileAttributesRequest = 0x32, ///< Get File Attributes returns the attributes of the file or the directory specified by Volume, Path and Filename
			SetFileAttributesRequest = 0x33, ///< Set File Attributes sets or resets the attribute bits of the file or directory specified by Volume, Path, File and wildcard Name
			GetFileTimeAndDateRequest = 0x34, ///< Get File Date & Time returns the date and time of the file or directory specified by Volume, Path and Filename
			InitializeVolumeRequest = 0x40 ///< Prepare the volume to accept files and directories. All data is lost upon completion
		};

		/// @brief Enumerates the transmit flags (CAN messages that support non-state-machine-driven retries)
		enum class TransmitFlags
		{
			ClientToServerStatus = 0, ///< Flag to send the maintenance message to the file server

			NumberFlags ///< The number of flags in this enumeration
		};

		/// @brief A storage class to keep track of file metadata that the interface is managing
		class FileInfo
		{
		public:
			/// @brief Constructor for file info, sets default values
			FileInfo() = default;

			std::string fileName; ///< The file name/path of this file
			FileState state = FileState::Uninitialized; ///< A sub-state-machine state for the file
			FileOpenMode openMode = FileOpenMode::OpenFileForReadingOnly; ///< The file open mode (read only, write only, etc)
			FilePointerMode pointerMode = FilePointerMode::AppendMode; ///< Where the file pointer should be set for this file
			std::uint32_t timestamp_ms = 0; ///< A timestamp to track when file operations take too long
			std::uint8_t attributesBitField = 0; ///< The reported file attributes
			std::uint8_t transactionNumberForRequest = 0; ///< The TAN for the latest request corresponding to this file
			std::uint8_t handle = INVALID_FILE_HANDLE; ///< The file handle associated with this file
			bool createIfNotPresent = false; ///< If the interface should create the file when opening it, if it does not exist
			bool exclusiveAccess = true; ///< If exclusive access was requested for the file
		};

		/// @brief Cleans up all open file's metadata, used when disconnected from the server
		void clear_all_file_metadata();

		/// @brief Takes an error code and converts it to a human readable string for logging
		/// @param[in] errorCode The error code to convert to string
		/// @returns The human readable error code, or "Undefined" if some other value is passed in
		std::string error_code_to_string(ErrorCode errorCode) const;

		/// @brief Processes the internal Tx flags
		/// @param[in] flag The flag to process
		/// @param[in] parent A context variable to find the relevant VT client class
		static void process_flags(std::uint32_t flag, void *parent);

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		void process_message(const CANMessage &message);

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		/// @param[in] parent Provides the context to the actual TP manager object
		static void process_message(const CANMessage &message, void *parent);

		/// @brief The data callback passed to the network manger's send function for the transport layer messages
		/// @details We upload the data with callbacks to avoid making a complete copy of the data to
		/// accommodate the multiplexor that needs to get passed to the transport layer message's first byte.
		/// @param[in] callbackIndex The number of times the callback has been called
		/// @param[in] bytesOffset The byte offset at which to get pool data
		/// @param[in] numberOfBytesNeeded The number of bytes the protocol needs to send another frame (usually 7)
		/// @param[out] chunkBuffer A pointer through which the data should be returned to the protocol
		/// @param[in] parentPointer A context variable that is passed back through the callback
		/// @returns true if the data was successfully returned via the callback
		static bool process_internal_file_write_callback(std::uint32_t callbackIndex,
		                                                 std::uint32_t bytesOffset,
		                                                 std::uint32_t numberOfBytesNeeded,
		                                                 std::uint8_t *chunkBuffer,
		                                                 void *parentPointer);

		/// @brief Sends the change current directory request message
		/// @param[in] path The new path to change to
		/// @returns `true` if the message was sent, otherwise false
		bool send_change_current_directory_request(std::string path);

		// @brief sends the Client Connection Maintenance message
		/// @details The Client Connection Maintenance message is sent by a client in order to maintain a connection with the FS.
		/// The client sends this message when actively interacting with the FS.
		/// When this message is no longer received by the FS for 6 s, the open files are closed and all Handles for that client become invalid.
		/// The client's working directory is also lost and set back to the default.
		/// @returns `true` if the message was sent, otherwise `false`
		bool send_client_connection_maintenance() const;

		/// @brief Sends the close file request message
		/// @param[in] fileMetadata The file meta data structure used to send the message
		/// @returns `true` if the message was sent, otherwise `false`
		bool send_close_file(std::shared_ptr<FileInfo> fileMetadata) const;

		/// @brief Sends the get file server properties request message
		/// @returns `true` if the message was sent, otherwise `false`
		bool send_get_file_server_properties() const;

		/// @brief Sends the open file request message
		/// @param[in] fileMetadata The file meta data structure used to send the message
		/// @returns `true` if the message is sent, otherwise `false`
		bool send_open_file(std::shared_ptr<FileInfo> fileMetadata) const;

		/// @brief Sets the current file state and a transition timestamp
		/// @param[in] state The new state
		void set_state(StateMachineState state);

		/// @brief Changes the internal state machine state and updates the associated timestamp to the specified one
		/// @note This is intended for testing purposes only
		/// @param[in] newState The new state for the state machine
		/// @param[in] timestamp The new value for the state machine timestamp (in milliseconds)
		void set_state(StateMachineState state, std::uint32_t timestamp_ms);

		/// @brief Sets the current state machine state and a transition timestamp
		/// @param[in] state The new state
		void set_file_state(std::shared_ptr<FileInfo> fileMetadata, FileState state);

		/// @brief Updates the sub-state-machines of each managed file
		void update_open_files();

		/// @brief The worker thread will execute this function when it runs, if applicable
		void worker_thread_function();

	private:
		static constexpr std::uint32_t SERVER_STATUS_MESSAGE_TIMEOUT_MS = 6000; ///< The max time to wait for a server status message. After this time, we can assume it has shutdown.
		static constexpr std::uint32_t CLIENT_STATUS_MESSAGE_REPETITION_RATE_MS = 2000; ///< The time interval to use when sending the client maintenance message
		static constexpr std::uint32_t GENERAL_OPERATION_TIMEOUT = 1250; ///< The standard says that the timouts should be "reasonable" and to use the timeouts from TP and ETP, so I selected the t2/t3 timeout
		static constexpr std::uint8_t FILE_SERVER_BUSY_READING_BIT_MASK = 0x01; ///< A bitmask for reading the "busy reading" bit out of fileServerStatusBitfield
		static constexpr std::uint8_t FILE_SERVER_BUSY_WRITING_BIT_MASK = 0x02; ///< A bitmask for reading the "busy writing" bit out of fileServerStatusBitfield
		static constexpr std::uint8_t FILE_SERVER_CAPABILITIES_BIT_MASK = 0x01; ///< A bitmask for the multiple volume support bit in fileServerCapabilitiesBitfield
		static constexpr CANIdentifier::CANPriority FILE_SERVER_MESSAGE_PRIORITY = CANIdentifier::CANPriority::PriorityLowest7; ///< All FS messages are sent with lowest priority
		static const std::map<ErrorCode, std::string> ERROR_TO_STRING_MAP; ///< A map between error code and a string description

		std::shared_ptr<PartneredControlFunction> partnerControlFunction; ///< The partner control function this client will send to
		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The internal control function the client uses to send from

		std::thread *workerThread = nullptr; ///< The worker thread that updates this interface
		std::mutex metadataMutex; ///< Protects the TAN and file metadata list
		std::list<std::shared_ptr<FileInfo>> fileInfoList; ///< List of files the client interface knows about and is managing
		ProcessingFlags txFlags; ///< A retry mechanism for internal Tx messages
		StateMachineState currentState = StateMachineState::Disconnected; ///< The current state machine state
		std::string currentDirectory; ///< Maintatains our current working directory location
		const std::uint8_t *currentFileWriteData = nullptr; ///< A pointer to the data for any in-progress file write

		std::uint32_t currentFileWriteSize = 0; ///< The size of any in-progress file write
		std::uint32_t stateMachineTimestamp_ms = 0; ///< The timestamp for when the state machine state was last updated
		std::uint32_t lastServerStatusTimestamp_ms = 0; ///< The timstamp when we last got a status message from the server
		std::uint8_t fileServerStatusBitfield = 0; ///< The current status of the FS. Can be 0, or have bits set for busy either reading or writing
		std::uint8_t numberFilesOpen = 0; ///< The number of files that are currently open at the FS.
		std::uint8_t maxNumberSimultaneouslyOpenFiles = 0; ///< The maximum number of files that can be opened simultaneously on the FS
		std::uint8_t fileServerCapabilitiesBitfield = 0; ///< If the server supports only 1 volume or multiple volumes
		std::uint8_t fileServerVersion = 0; ///< The version of the standard that the file server complies to
		std::uint8_t transactionNumber = 0; ///< The TAN as specified in ISO 11783-13
		std::uint8_t currentFileWriteHandle = INVALID_FILE_HANDLE; ///< Used to keep track of if we're currently writing a file, and which file is being written.
		bool initialized = false; ///< Stores the client initialization state
		bool shouldTerminate = false; ///< Used to determine if the client should exit and join the worker thread
	};
} // namespace isobus

#endif // ISOBUS_FILE_SERVER_CLIENT_HPP
