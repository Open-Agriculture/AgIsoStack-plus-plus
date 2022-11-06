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

#include "can_protocol.hpp"
#include "can_network_manager.hpp"
#include "can_partnered_control_function.hpp"

#include <memory>

namespace isobus
{
	/// @brief A client interface for communicating with an ISOBUS file server
	class FileServerClient : public CANLibProtocol
	{
	public:
		/// @brief The constructor for a file server client
		/// @param[in] partner The file server control function to communicate with
		/// @param[in] clientSource The internal control function to use when communicating with the file server
		FileServerClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource);

		// @brief Destructor for a FileServerClient
		~FileServerClient();

	private:
		// @brief The number of the edition or version of ISO 11783-13 with which the FS or client is compliant
		enum class VersionNumber
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

		/// @brief Enumerates the different ways a file or directory can be opened
		enum class FileOpenMode
		{
			OpenFileForReadingOnly = 0, ///< Open a file in read only mode
			OpenFileForWritingOnly = 1, ///< Open a file in write only mode
			OpenFileForReadingAndWriting = 2, ///< Open a file for both read and write mode
			OpenDirectory = 3 ///< Opens a directory
		};

		/// @brief Enumerates options for where you want the file pointer set when opening a file
		enum class FilePointerMode
		{
			RandomAccess = 0, ///< File pointer set to the start of the file
			AppendMode = 1 ///< File pointer set to the end of the file
		};

		/// @brief The position mode specifies the location from which the offset value is used to determine the file pointer position.
		enum class PositionMode
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

		/// @brief The protocol's initializer function
		void initialize(CANLibBadge<CANNetworkManager>) override;

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		void process_message(CANMessage *const message) override;

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		/// @param[in] parent Provides the context to the actual TP manager object
		static void process_message(CANMessage *const message, void *parent);

		static constexpr std::uint8_t FILE_SERVER_BUSY_READING_BIT_MASK = 0x01; ///< A bitmask for reading the "busy reading" bit out of fileServerStatusBitfield
		static constexpr std::uint8_t FILE_SERVER_BUSY_WRITING_BIT_MASK = 0x02; ///< A bitmask for reading the "busy writing" bit out of fileServerStatusBitfield
		static constexpr std::uint8_t FILE_SERVER_CAPABILITIES_BIT_MASK = 0x01; ///< A bitmask for the multiple volume support bit in fileServerCapabilitiesBitfield
		static constexpr CANIdentifier::CANPriority FILE_SERVER_MESSAGE_PRIORITY = CANIdentifier::CANPriority::PriorityLowest7; ///< All FS messages are sent with lowest priority

		std::shared_ptr<PartneredControlFunction> partnerControlFunction; ///< The partner control function this client will send to
		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The internal control function the client uses to send from

		std::uint8_t fileServerStatusBitfield; ///< The current status of the FS. Can be 0, or have bits set for busy either reading or writing
		std::uint8_t numberFilesOpen; ///< The number of files that are currently open at the FS.
		std::uint8_t maxNumberSimultaneouslyOpenFiles; ///< The maximum number of files that can be opened simultaneously on the FS
		std::uint8_t fileServerCapabilitiesBitfield; ///< If the server supports only 1 volume or multiple volumes
	};
} // namespace isobus

#endif // ISOBUS_FILE_SERVER_CLIENT_HPP
