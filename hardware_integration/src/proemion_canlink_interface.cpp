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
#include <thread>

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
#include <fcntl.h>
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

	std::string describe_socket_error(int code)
	{
		return "Winsock error " + std::to_string(code);
	}

	std::string get_socket_error()
	{
		return describe_socket_error(WSAGetLastError());
	}

	void set_socket_blocking(NativeSocket socketHandle, bool blocking)
	{
		u_long nonBlocking = blocking ? 0 : 1;
		ioctlsocket(socketHandle, FIONBIO, &nonBlocking);
	}

	bool is_connect_in_progress()
	{
		return WSAGetLastError() == WSAEWOULDBLOCK;
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

	std::string describe_socket_error(int code)
	{
		return std::strerror(code);
	}

	std::string get_socket_error()
	{
		return describe_socket_error(errno);
	}

	void set_socket_blocking(NativeSocket socketHandle, bool blocking)
	{
		const int flags = fcntl(socketHandle, F_GETFL, 0);
		fcntl(socketHandle, F_SETFL, blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK));
	}

	bool is_connect_in_progress()
	{
		return errno == EINPROGRESS;
	}

	std::string get_resolve_error(int status)
	{
		return gai_strerror(status);
	}
#endif

	std::string to_hex(const std::vector<std::uint8_t> &bytes)
	{
		static const char digits[] = "0123456789ABCDEF";
		std::string text;
		for (const auto byte : bytes)
		{
			if (!text.empty())
			{
				text += ' ';
			}
			text += digits[byte >> 4];
			text += digits[byte & 0x0F];
		}
		return text;
	}
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

	void ProemionCANLinkInterface::report_lost_connection(const std::string &reason)
	{
		const auto now = std::chrono::steady_clock::now();
		const auto connectedSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - connectedAt).count();
		const auto lastDataMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDataReceivedAt).count();

		disconnect();
		set_last_error("Lost the connection to " + ipAddress + ":" + std::to_string(tcpPort) + " (" + reason + ")");
		LOG_INFO("[ProemionCANLink] The connection lasted " + std::to_string(connectedSeconds) + " s, " +
		         std::to_string(framesReceived) + " frames received, " + std::to_string(framesSent.load()) + " sent, " +
		         "longest pause in reading " + std::to_string(longestReadPauseMs) + " ms, " +
		         "last data from the device " + std::to_string(lastDataMs) + " ms before the loss");

		// Only a connection which worked is recovered, so a wrong address is still reported when opening
		if (keepConnected)
		{
			if (!reconnecting)
			{
				reconnectDelayMs = FIRST_RECONNECT_DELAY_MS;
				reconnecting = true;
			}
			nextReconnectAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(reconnectDelayMs);
			LOG_INFO("[ProemionCANLink] Reconnecting in " + std::to_string(reconnectDelayMs / 1000) + " s");
		}
	}

	void ProemionCANLinkInterface::on_connected()
	{
		connectedAt = std::chrono::steady_clock::now();
		lastDataReceivedAt = connectedAt;
		lastReadCallAt = {};
		longestReadPauseMs = 0;
		framesReceived = 0;
		framesSent = 0;
	}

	void ProemionCANLinkInterface::try_reconnect()
	{
		const auto now = std::chrono::steady_clock::now();
		if (now < nextReconnectAt)
		{
			std::this_thread::sleep_for(std::min<std::chrono::steady_clock::duration>(nextReconnectAt - now, std::chrono::milliseconds(100)));
			return;
		}

		LOG_INFO("[ProemionCANLink] Reconnecting to " + ipAddress + ":" + std::to_string(tcpPort));
		if (connect_tcp())
		{
			on_connected();
			if (configure_isobus_bitrate())
			{
				reconnecting = false;
				LOG_INFO("[ProemionCANLink] Reconnected to " + ipAddress + ":" + std::to_string(tcpPort));
				return;
			}
			disconnect();
			set_last_error("Connected to " + ipAddress + ":" + std::to_string(tcpPort) + ", but failed to configure the CAN bitrate to 250 kbit/s");
		}

		if (reconnecting)
		{
			reconnectDelayMs = std::min(reconnectDelayMs * 2, MAX_RECONNECT_DELAY_MS);
			nextReconnectAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(reconnectDelayMs);
			LOG_WARNING("[ProemionCANLink] Reconnecting failed, trying again in " + std::to_string(reconnectDelayMs / 1000) + " s");
		}
	}

	std::string ProemionCANLinkInterface::get_name() const
	{
		return "Proemion CANlink";
	}

	bool ProemionCANLinkInterface::get_is_valid() const
	{
		return (socketFd >= 0) || reconnecting;
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
		lastConnectTimedOut = false;

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

			// Connect without blocking, so an unreachable device fails after CONNECT_TIMEOUT_MS instead of the OS default
			set_socket_blocking(candidate, false);
			bool connected = (connect(candidate, address->ai_addr, static_cast<int>(address->ai_addrlen)) == 0);

			if (!connected && is_connect_in_progress())
			{
				pollfd descriptor = {};
				descriptor.fd = candidate;
				descriptor.events = POLLOUT;
				const auto pollResult = poll_socket(&descriptor, CONNECT_TIMEOUT_MS);

				if (pollResult > 0)
				{
					int socketError = 0;
					socklen_t socketErrorLength = sizeof(socketError);
					getsockopt(candidate, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&socketError), &socketErrorLength);
					connected = (0 == socketError);
					if (!connected)
					{
						connectError = describe_socket_error(socketError);
					}
				}
				else if (0 == pollResult)
				{
					connectError = "no answer within " + std::to_string(CONNECT_TIMEOUT_MS / 1000) + " s";
					lastConnectTimedOut = true;
				}
				else
				{
					connectError = get_socket_error();
				}
			}
			else if (!connected)
			{
				connectError = get_socket_error();
			}

			if (connected)
			{
				set_socket_blocking(candidate, true);
				break;
			}

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
			// No answer usually means the device is not on the network yet, like when the computer's WiFi is still
			// connecting after it starts, so keep trying. A refused connection or an unknown name is reported straight away.
			if (lastConnectTimedOut)
			{
				keepConnected = true;
				reconnecting = true;
				reconnectDelayMs = FIRST_RECONNECT_DELAY_MS;
				nextReconnectAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(reconnectDelayMs);
				LOG_WARNING("[ProemionCANLink] " + ipAddress + ":" + std::to_string(tcpPort) + " did not answer, it may not be on the network yet. Retrying in the background.");
			}
			return;
		}

		receiveBuffer.clear();

		on_connected();

		if (!configure_isobus_bitrate())
		{
			disconnect();
			set_last_error("Connected to " + ipAddress + ":" + std::to_string(tcpPort) + ", but failed to configure the CAN bitrate to 250 kbit/s");
			return;
		}

		keepConnected = true;
		LOG_INFO("[ProemionCANLink] Connected to " + ipAddress + ":" + std::to_string(tcpPort));
	}

	void ProemionCANLinkInterface::close()
	{
		keepConnected = false;
		reconnecting = false;
		disconnect();
	}

	void ProemionCANLinkInterface::disconnect()
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
			report_lost_connection(reason);
			return false;
		}

		if (pollResult == 0)
		{
			return false;
		}

		if ((descriptor.revents & POLLIN) == 0)
		{
			// Only an error or hang up is pending, which poll would report straight away from now on
			int socketError = 0;
			socklen_t socketErrorLength = sizeof(socketError);
			getsockopt(static_cast<NativeSocket>(socketFd), SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&socketError), &socketErrorLength);
			const std::string reason = (0 != socketError) ? describe_socket_error(socketError) : ((descriptor.revents & POLLHUP) ? "the device hung up" : "poll flags " + std::to_string(descriptor.revents));
			report_lost_connection(reason);
			return false;
		}

		char buffer[1024];

		const auto received = recv(static_cast<NativeSocket>(socketFd), buffer, sizeof(buffer), 0);

		if (received <= 0)
		{
			// Zero means the device closed the connection, which recv would keep reporting from now on
			const std::string reason = (0 == received) ? "the device closed the connection" : get_socket_error();
			report_lost_connection(reason);
			return false;
		}

		lastDataReceivedAt = std::chrono::steady_clock::now();
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

		return wait_for_bitrate_reply(bitrate);
	}

	bool ProemionCANLinkInterface::wait_for_bitrate_reply(BitrateKeys &bitrate)
	{
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(BITRATE_REPLY_TIMEOUT_MS);
		int otherMessagesLogged = 0;

		while (get_is_valid() && (std::chrono::steady_clock::now() < deadline))
		{
			std::vector<std::uint8_t> response;

			if (!receive_pdu(response, 100))
			{
				continue;
			}

			CANMessageFrame frame = {};
			if (decode_can_frame(response, frame))
			{
				canFramesSeenWhileConfiguring++;
				continue;
			}

			// The manual says a settings reply carries the same command byte as the request and the new
			// settings, the rate key first. Either bitrate command is accepted, as both report the rate the same way.
			if ((response.size() >= 6) &&
			    ((response[2] == COMMAND_GET_BITRATE) || (response[2] == COMMAND_SET_BITRATE)))
			{
				bitrate = static_cast<BitrateKeys>(response[3]);
				return true;
			}

			if (otherMessagesLogged < 10)
			{
				otherMessagesLogged++;
				LOG_INFO("[ProemionCANLink] Unexpected message while configuring the bitrate: " + to_hex(response));
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

		// Expected response for 250 kbit/s: 43 02 57 04 XX 0D
		BitrateKeys reportedBitrate = Bitrate_Custom;
		if (!wait_for_bitrate_reply(reportedBitrate))
		{
			return false;
		}
		if (reportedBitrate != bitrate)
		{
			LOG_WARNING("[ProemionCANLink] The device reported bitrate key " + std::to_string(static_cast<int>(reportedBitrate)) + " after being set to " + std::to_string(static_cast<int>(bitrate)));
			return false;
		}
		return true;
	}

	bool ProemionCANLinkInterface::configure_isobus_bitrate()
	{
		BitrateKeys currentBitrate = Bitrate_Custom;
		canFramesSeenWhileConfiguring = 0;

		if (request_current_bitrate(currentBitrate))
		{
			if (currentBitrate == Bitrate_250kb)
			{
				LOG_INFO("[ProemionCANLink] CAN bitrate is already 250 kbit/s");
				return true;
			}
			LOG_INFO("[ProemionCANLink] CAN bitrate key is " + std::to_string(static_cast<int>(currentBitrate)) + ", changing it to 250 kbit/s");
		}
		else
		{
			LOG_WARNING("[ProemionCANLink] No reply to the bitrate request, attempting to set 250 kbit/s");
		}

		if (set_bitrate(Bitrate_250kb))
		{
			LOG_INFO("[ProemionCANLink] CAN bitrate set to 250 kbit/s");
			return true;
		}

		// A CAN controller only receives valid frames at the bus's own bitrate, so if they arrive the device is
		// usable as it is, even though it did not confirm the setting
		if (get_is_valid() && (canFramesSeenWhileConfiguring > 0))
		{
			LOG_WARNING("[ProemionCANLink] The device did not confirm the bitrate, but " + std::to_string(canFramesSeenWhileConfiguring) + " CAN frames arrived meanwhile, so it is listening at the bus's bitrate. Continuing.");
			return true;
		}
		return false;
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
		if (socketFd < 0)
		{
			if (reconnecting)
			{
				try_reconnect();
			}
			return false;
		}

		// The stack stops calling this while its receive queue is full, and meanwhile the device's buffer fills up
		const auto callTime = std::chrono::steady_clock::now();
		if (lastReadCallAt != std::chrono::steady_clock::time_point{})
		{
			const auto pauseMs = std::chrono::duration_cast<std::chrono::milliseconds>(callTime - lastReadCallAt).count();
			longestReadPauseMs = std::max<std::int64_t>(longestReadPauseMs, pauseMs);
			if (pauseMs >= 1000)
			{
				LOG_WARNING("[ProemionCANLink] No frames were read for " + std::to_string(pauseMs) + " ms, so they queued up in the device");
			}
		}
		lastReadCallAt = callTime;

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
				framesReceived++;
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

		if (!send_pdu(pdu))
		{
			return false;
		}
		framesSent++;
		return true;
	}
}
