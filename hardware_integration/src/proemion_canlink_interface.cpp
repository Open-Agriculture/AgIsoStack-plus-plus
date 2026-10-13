//================================================================================================
/// @file proemion_canlink_interface.cpp
///
/// @brief AgIsoStack++ CAN hardware interface for Proemion CANlink Wifi-CAN devices
//================================================================================================

#include "isobus/hardware_integration/proemion_canlink_interface.hpp"

#include "isobus/isobus/can_stack_logger.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <limits>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace
{
	// The small differences between Winsock and BSD sockets, so the driver itself can stay the same on both
#ifdef _WIN32
	using NativeSocket = SOCKET;

	constexpr int SHUTDOWN_BOTH = SD_BOTH;

	int poll_socket(pollfd *descriptor, int timeoutMs)
	{
		return WSAPoll(descriptor, 1, timeoutMs);
	}

	void close_socket(NativeSocket socketHandle)
	{
		closesocket(socketHandle);
	}

	std::string get_socket_error()
	{
		return "Winsock error " + std::to_string(WSAGetLastError());
	}

	std::string get_resolve_error(int status)
	{
		// The plain name maps to the wide character version when UNICODE is defined
		return gai_strerrorA(status);
	}
#else
	using NativeSocket = int;

	constexpr int SHUTDOWN_BOTH = SHUT_RDWR;

	int poll_socket(pollfd *descriptor, int timeoutMs)
	{
		return poll(descriptor, 1, timeoutMs);
	}

	void close_socket(NativeSocket socketHandle)
	{
		::close(socketHandle);
	}

	std::string get_socket_error()
	{
		return std::strerror(errno);
	}

	std::string get_resolve_error(int status)
	{
		return gai_strerror(status);
	}
#endif
}

namespace isobus
{
	ProemionCANLinkInterface::ProemionCANLinkInterface(const std::string &interface) :
	  interfaceString(interface)
	{
#ifdef _WIN32
		// Winsock counts these, so every instance can start and clean up its own use of it
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
		parse_interface();
	}

	ProemionCANLinkInterface::~ProemionCANLinkInterface()
	{
		close();
#ifdef _WIN32
		WSACleanup();
#endif
	}

	std::string ProemionCANLinkInterface::get_last_error() const
	{
		std::lock_guard<std::mutex> lock(lastErrorMutex);
		return lastError;
	}

	void ProemionCANLinkInterface::set_last_error(const std::string &error)
	{
		LOG_ERROR("[ProemionCANLink] " + error);
		std::lock_guard<std::mutex> lock(lastErrorMutex);
		lastError = error;
	}

	void ProemionCANLinkInterface::set_interface(const std::string &interface)
	{
		interfaceString = interface;
		parse_interface();
	}

	std::string ProemionCANLinkInterface::get_interface() const
	{
		return interfaceString;
	}

	std::string ProemionCANLinkInterface::get_name() const
	{
		return "Proemion CANlink";
	}

	bool ProemionCANLinkInterface::get_is_valid() const
	{
		return socketFd >= 0;
	}

	bool ProemionCANLinkInterface::parse_interface()
	{
		ipAddress.clear();
		tcpPort = DEFAULT_TCP_PORT;

		if (interfaceString.empty())
		{
			return false;
		}

		const auto colon = interfaceString.find(':');

		if (colon == std::string::npos)
		{
			ipAddress = interfaceString;
			return true;
		}

		// Only one colon is valid.
		// IPv6 addresses are intentionally not supported by this syntax.
		if (interfaceString.find(':', colon + 1) != std::string::npos)
		{
			return false;
		}

		ipAddress = interfaceString.substr(0, colon);

		const auto portString = interfaceString.substr(colon + 1);

		if (ipAddress.empty() || portString.empty())
		{
			return false;
		}

		try
		{
			std::size_t parsedCharacters = 0;

			const auto port = std::stoul(portString, &parsedCharacters, 10);

			if (parsedCharacters != portString.size() || ((0 == port) || (port > std::numeric_limits<std::uint16_t>::max())))
			{
				return false;
			}

			tcpPort = static_cast<std::uint16_t>(port);
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	bool ProemionCANLinkInterface::connect_tcp()
	{
		struct addrinfo hints = {};
		struct addrinfo *result = nullptr;

		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		const auto portString = std::to_string(tcpPort);

		const auto status = getaddrinfo(ipAddress.c_str(), portString.c_str(), &hints, &result);

		if (status != 0)
		{
			set_last_error("Unable to resolve " + ipAddress + ": " + get_resolve_error(status));
			return false;
		}

		std::string connectError = "no address found";

		for (auto address = result; address != nullptr; address = address->ai_next)
		{
			const NativeSocket candidate = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
			socketFd = static_cast<std::intptr_t>(candidate);

			if (socketFd < 0)
			{
				connectError = get_socket_error();
				socketFd = -1;
				continue;
			}

			if (connect(candidate, address->ai_addr, static_cast<int>(address->ai_addrlen)) == 0)
			{
				break;
			}

			// Read before closing the socket, which resets the error on Windows
			connectError = get_socket_error();
			close_socket(candidate);
			socketFd = -1;
		}

		freeaddrinfo(result);

		if (socketFd < 0)
		{
			set_last_error("Failed to connect to " + ipAddress + ":" + std::to_string(tcpPort) + " (" + connectError + ")");
		}
		return socketFd >= 0;
	}

	void ProemionCANLinkInterface::open()
	{
		close();

		{
			std::lock_guard<std::mutex> lock(lastErrorMutex);
			lastError.clear();
		}

		if (!parse_interface())
		{
			set_last_error("Invalid device address \"" + interfaceString + "\", expected an IP address with an optional :port");
			return;
		}

		if (!connect_tcp())
		{
			return;
		}

		receiveBuffer.clear();

		if (!configure_isobus_bitrate())
		{
			close();
			set_last_error("Connected to " + ipAddress + ":" + std::to_string(tcpPort) + ", but failed to configure the CAN bitrate to 250 kbit/s");
			return;
		}

		LOG_INFO("[ProemionCANLink] Connected to " + ipAddress + ":" + std::to_string(tcpPort));
	}

	void ProemionCANLinkInterface::close()
	{
		std::lock_guard<std::mutex> lock(transmitMutex);

		if (socketFd >= 0)
		{
			const auto socketHandle = static_cast<NativeSocket>(socketFd);
			shutdown(socketHandle, SHUTDOWN_BOTH);
			close_socket(socketHandle);
			socketFd = -1;
		}

		receiveBuffer.clear();
	}

	std::uint8_t ProemionCANLinkInterface::calculate_checksum(
	  const std::uint8_t *data,
	  std::size_t length)
	{
		std::uint8_t checksum = 0;

		for (std::size_t i = 0; i < length; ++i)
		{
			checksum ^= data[i];
		}

		return checksum;
	}

	std::vector<std::uint8_t>
	ProemionCANLinkInterface::create_pdu(
	  std::uint8_t command,
	  const std::uint8_t *data,
	  std::size_t dataLength) const
	{
		std::vector<std::uint8_t> pdu;
		const auto length = static_cast<std::uint8_t>(1 + dataLength);

		pdu.reserve(dataLength + 5);

		pdu.push_back(SOF_BYTE);
		pdu.push_back(length);
		pdu.push_back(command);

		if ((data != nullptr) && (dataLength > 0))
		{
			pdu.insert(pdu.end(), data, data + dataLength);
		}

		pdu.push_back(calculate_checksum(pdu.data(), pdu.size()));
		pdu.push_back(EOF_BYTE);

		return pdu;
	}

	bool ProemionCANLinkInterface::send_pdu(const std::vector<std::uint8_t> &pdu)
	{
		std::lock_guard<std::mutex> lock(transmitMutex);

		if (socketFd < 0)
		{
			return false;
		}

		std::size_t offset = 0;

		while (offset < pdu.size())
		{
			const auto result =
			  send(
			    static_cast<NativeSocket>(socketFd),
			    reinterpret_cast<const char *>(pdu.data() + offset),
#ifdef _WIN32
			    static_cast<int>(pdu.size() - offset),
#else
			    pdu.size() - offset,
#endif
#ifdef MSG_NOSIGNAL
			    MSG_NOSIGNAL
#else
			    0
#endif
			  );

			if (result <= 0)
			{
				return false;
			}

			offset +=
			  static_cast<std::size_t>(result);
		}

		return true;
	}

	bool ProemionCANLinkInterface::extract_pdu_from_receive_buffer(std::vector<std::uint8_t> &pdu)
	{
		while (!receiveBuffer.empty())
		{
			// Search for the beginning of a PDU.
			const auto sof = std::find(receiveBuffer.begin(), receiveBuffer.end(), SOF_BYTE);

			if (sof == receiveBuffer.end())
			{
				receiveBuffer.clear();
				return false;
			}

			if (sof != receiveBuffer.begin())
			{
				receiveBuffer.erase(receiveBuffer.begin(), sof);
			}

			// SOF + Length
			if (receiveBuffer.size() < 2)
			{
				return false;
			}

			const std::size_t commandAndDataLength = receiveBuffer[1];

			// Complete PDU:
			//
			// SOF
			// Length (command+data)
			// Command + Data
			// Checksum
			// EOF
			//

			const std::size_t totalLength = commandAndDataLength + 4;

			if (receiveBuffer.size() < totalLength)
			{
				return false;
			}

			if (receiveBuffer[totalLength - 1] != EOF_BYTE)
			{
				receiveBuffer.erase(receiveBuffer.begin());

				continue;
			}

			const auto expectedChecksum = calculate_checksum(receiveBuffer.data(), totalLength - 2);

			const auto receivedChecksum = receiveBuffer[totalLength - 2];

			if (expectedChecksum != receivedChecksum)
			{
				LOG_WARNING("[ProemionCANLink] Invalid PDU checksum");
				receiveBuffer.erase(receiveBuffer.begin());

				continue;
			}

			pdu.assign(receiveBuffer.begin(), receiveBuffer.begin() + totalLength);

			receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + totalLength);

			return true;
		}

		return false;
	}

	bool ProemionCANLinkInterface::receive_pdu(std::vector<std::uint8_t> &pdu, int timeoutMs)
	{
		if (socketFd < 0)
		{
			return false;
		}

		if (extract_pdu_from_receive_buffer(pdu))
		{
			return true;
		}

		pollfd descriptor = {};
		descriptor.fd = static_cast<NativeSocket>(socketFd);
		descriptor.events = POLLIN;

		const auto pollResult = poll_socket(&descriptor, timeoutMs);

		if (pollResult < 0)
		{
			const auto reason = get_socket_error();
			close();
			set_last_error("Lost the connection to " + ipAddress + ":" + std::to_string(tcpPort) + " (" + reason + ")");
			return false;
		}

		if (pollResult == 0)
		{
			return false;
		}

		if ((descriptor.revents & POLLIN) == 0)
		{
			// Only an error or hang up is pending, which poll would report straight away from now on
			close();
			set_last_error("Lost the connection to " + ipAddress + ":" + std::to_string(tcpPort));
			return false;
		}

		char buffer[1024];

		const auto received = recv(static_cast<NativeSocket>(socketFd), buffer, sizeof(buffer), 0);

		if (received <= 0)
		{
			// Zero means the device closed the connection, which recv would keep reporting from now on
			close();
			set_last_error("Lost the connection to " + ipAddress + ":" + std::to_string(tcpPort));
			return false;
		}

		receiveBuffer.insert(receiveBuffer.end(), buffer, buffer + received);

		return extract_pdu_from_receive_buffer(pdu);
	}

	bool ProemionCANLinkInterface::request_current_bitrate(BitrateKeys &bitrate)
	{
		const auto request = create_pdu(COMMAND_GET_BITRATE, nullptr, 0);

		if (!send_pdu(request))
		{
			return false;
		}

		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);

		while (std::chrono::steady_clock::now() < deadline)
		{
			std::vector<std::uint8_t> response;

			if (!receive_pdu(response, 100))
			{
				continue;
			}

			// Minimum bitrate response: SOF LEN 0x56 BITRATE CHECKSUM EOF
			if (response.size() < 6)
			{
				continue;
			}

			if ((response[2] == COMMAND_GET_BITRATE) && (response[1] == 2))
			{
				bitrate = static_cast<BitrateKeys>(response[3]);
				return true;
			}
		}

		return false;
	}

	bool ProemionCANLinkInterface::set_bitrate(BitrateKeys bitrate)
	{
		std::uint8_t tmpBitrate = bitrate;
		const auto request = create_pdu(COMMAND_SET_BITRATE, &tmpBitrate, 1);

		if (!send_pdu(request))
		{
			return false;
		}

		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);

		while (std::chrono::steady_clock::now() < deadline)
		{
			std::vector<std::uint8_t> response;

			if (!receive_pdu(response, 100))
			{
				continue;
			}

			// Expected response for 250 kbit/s: 43 02 57 04 XX 0D
			if ((response.size() >= 6) &&
			    (response[2] == COMMAND_SET_BITRATE) &&
			    (response[1] != 2) &&
			    (response[3] == bitrate))
			{
				return true;
			}
		}

		return false;
	}

	bool ProemionCANLinkInterface::configure_isobus_bitrate()
	{
		BitrateKeys currentBitrate = Bitrate_Custom;

		if (request_current_bitrate(currentBitrate))
		{
			if (currentBitrate == Bitrate_250kb)
			{
				LOG_DEBUG("[ProemionCANLink] CAN bitrate is already 250 kbit/s");
				return true;
			}
		}
		else
		{
			LOG_WARNING("[ProemionCANLink] Unable to read current CAN bitrate, attempting to set 250 kbit/s");
		}

		LOG_INFO("[ProemionCANLink] Changing CAN bitrate to 250 kbit/s");
		return set_bitrate(Bitrate_250kb);
	}

	std::uint64_t ProemionCANLinkInterface::get_timestamp_us()
	{
		return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
	}

	bool ProemionCANLinkInterface::decode_can_frame(const std::vector<std::uint8_t> &pdu, CANMessageFrame &canFrame) const
	{
		if (pdu.size() < 5)
		{
			return false;
		}

		const auto command = pdu[2];

		bool isExtendedFrame = false;
		bool hasTimestamp = false;
		std::size_t identifierLength = 0;

		switch (command)
		{
			case COMMAND_CAN_11_BIT:
			{
				identifierLength = 2;
				isExtendedFrame = false;
				hasTimestamp = false;
			}
			break;

			case COMMAND_CAN_11_BIT_TIMESTAMP:
			{
				identifierLength = 2;
				isExtendedFrame = false;
				hasTimestamp = true;
			}
			break;

			case COMMAND_CAN_29_BIT:
			{
				identifierLength = 4;
				isExtendedFrame = true;
				hasTimestamp = false;
			}
			break;

			case COMMAND_CAN_29_BIT_TIMESTAMP:
			{
				identifierLength = 4;
				isExtendedFrame = true;
				hasTimestamp = true;
			}
			break;

			default:
			{
				// Not a CAN process data message.
				return false;
			}
		}

		// Length contains:
		//
		// Command + ID + CAN data [+ timestamp]

		const std::size_t commandAndDataLength = pdu[1];

		const std::size_t timestampLength = hasTimestamp ? 4 : 0;

		const std::size_t minimumLength = 1 + identifierLength + timestampLength;

		if (commandAndDataLength < minimumLength)
		{
			return false;
		}

		const std::size_t canDataLength = commandAndDataLength - 1 - identifierLength - timestampLength;

		if (canDataLength > 8)
		{
			return false;
		}

		std::size_t offset = 3;

		std::uint32_t identifier = 0;

		for (std::size_t i = 0;
		     i < identifierLength;
		     ++i)
		{
			identifier <<= 8;
			identifier |= pdu[offset++];
		}

		// Mask unused identifier bits.
		if (isExtendedFrame)
		{
			identifier &= 0x1FFFFFFF;
		}
		else
		{
			identifier &= 0x7FF;
		}

		canFrame.identifier = identifier;
		canFrame.isExtendedFrame = isExtendedFrame;
		canFrame.channel = 0;
		canFrame.dataLength =
		  static_cast<std::uint8_t>(canDataLength);

		std::memset(
		  canFrame.data,
		  0,
		  sizeof(canFrame.data));

		if (canDataLength > 0)
		{
			std::memcpy(
			  canFrame.data,
			  pdu.data() + offset,
			  canDataLength);
		}

		// AgIsoStack++ gets a local monotonic timestamp.
		//
		// Timestamp-enabled Proemion frames (0x01 / 0x03) are
		// parsed correctly, but the gateway-specific 32-bit
		// timestamp is intentionally not exposed here.

		canFrame.timestamp_us = get_timestamp_us();

		return true;
	}

	bool ProemionCANLinkInterface::read_frame(
	  CANMessageFrame &canFrame)
	{
		if (!get_is_valid())
		{
			return false;
		}

		const auto deadline =
		  std::chrono::steady_clock::now() +
		  std::chrono::milliseconds(100);

		while (std::chrono::steady_clock::now() < deadline)
		{
			const auto remaining =
			  std::chrono::duration_cast<std::chrono::milliseconds>(
			    deadline -
			    std::chrono::steady_clock::now())
			    .count();

			if (remaining <= 0)
			{
				break;
			}

			std::vector<std::uint8_t> pdu;

			if (!receive_pdu(
			      pdu,
			      static_cast<int>(remaining)))
			{
				return false;
			}

			if (decode_can_frame(pdu, canFrame))
			{
				return true;
			}
		}

		return false;
	}

	bool ProemionCANLinkInterface::write_frame(
	  const CANMessageFrame &canFrame)
	{
		if (!get_is_valid())
		{
			return false;
		}

		if (canFrame.dataLength > 8)
		{
			return false;
		}

		std::vector<std::uint8_t> payload;

		std::uint8_t command;

		if (canFrame.isExtendedFrame)
		{
			// Section 2.3.2:
			//
			// Command 0x02
			// Byte 0-3: 29-bit CAN ID, MSB first
			// additional bytes: CAN data

			command = COMMAND_CAN_29_BIT;

			const auto identifier =
			  canFrame.identifier & 0x1FFFFFFF;

			payload.reserve(
			  4 + canFrame.dataLength);

			payload.push_back(
			  static_cast<std::uint8_t>(
			    (identifier >> 24) & 0xFF));

			payload.push_back(
			  static_cast<std::uint8_t>(
			    (identifier >> 16) & 0xFF));

			payload.push_back(
			  static_cast<std::uint8_t>(
			    (identifier >> 8) & 0xFF));

			payload.push_back(
			  static_cast<std::uint8_t>(
			    identifier & 0xFF));
		}
		else
		{
			// Section 2.3.2:
			//
			// Command 0x00
			// Byte 0-1: 11-bit CAN ID, MSB first
			// additional bytes: CAN data

			command = COMMAND_CAN_11_BIT;

			const auto identifier = canFrame.identifier & 0x7FF;

			payload.reserve(2 + canFrame.dataLength);

			payload.push_back(static_cast<std::uint8_t>((identifier >> 8) & 0xFF));

			payload.push_back(static_cast<std::uint8_t>(identifier & 0xFF));
		}

		payload.insert(payload.end(), canFrame.data, canFrame.data + canFrame.dataLength);

		const auto pdu = create_pdu(command, payload.data(), payload.size());

		return send_pdu(pdu);
	}
}
