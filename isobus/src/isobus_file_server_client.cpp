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

namespace isobus
{
	FileServerClient::FileServerClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  fileServerStatusBitfield(0),
	  numberFilesOpen(0),
	  maxNumberSimultaneouslyOpenFiles(0),
	  fileServerCapabilitiesBitfield(0)
	{
	}

	FileServerClient::~FileServerClient()
	{
		if (nullptr != partnerControlFunction)
		{
			partnerControlFunction->remove_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::FileServerToClient), process_message, this);
		}
	}

	void FileServerClient::initialize(CANLibBadge<CANNetworkManager>)
	{
		if ((!initialized) && (nullptr != partnerControlFunction))
		{
			partnerControlFunction->add_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::FileServerToClient), process_message, this);
			initialized = true;
		}
	}

	void FileServerClient::process_message(CANMessage *const message)
	{
		if ((nullptr != message) &&
		    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::FileServerToClient) == message->get_identifier().get_parameter_group_number()))
		{
			auto messageData = message->get_data();

			switch (messageData[0])
			{
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::FileServerStatus):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::GetFileServerPropertiesResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::VolumeStatusResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::GetCurrentDirectoryResponse):
				case static_cast<std::uint8_t>(FileServerToClientMultiplexor::ChangeCurrentDirectoryResponse):
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

} // namespace isobus
