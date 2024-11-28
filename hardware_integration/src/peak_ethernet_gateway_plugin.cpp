#include "isobus/hardware_integration/peak_ethernet_gateway_plugin.hpp"

#include "isobus/isobus/can_stack_logger.hpp"

#include <cstring>

#ifndef _MSC_VER
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#endif

namespace isobus
{
#ifdef __GNUC__
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK(__Declaration__) __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#endif

	/// @brief This is the schema of the message payload the gateway uses
	/// @returns Packed version of _CAN2IP, as defined by Peak
	typedef PACK(struct _CAN2IP {
		std::uint16_t sz;
		std::uint16_t type;
		std::uint64_t tag;
		std::uint64_t timestamp;
		std::uint8_t channel;
		std::uint8_t dlc;
		std::uint16_t flag;
		std::uint32_t id;
		std::uint8_t data[8];
	}) S_CAN2IP;

	PeakEthernetGatewayPlugin::PeakEthernetGatewayPlugin(std::string targetIPAddress,
	                                                     std::uint16_t txPort,
	                                                     std::uint16_t rxPort,
	                                                     bool useUDP) :
	  ipAddress(targetIPAddress),
	  sendPort(txPort),
	  receivePort(rxPort),
	  udp(useUDP)
	{
	}

	PeakEthernetGatewayPlugin::~PeakEthernetGatewayPlugin()
	{
		PeakEthernetGatewayPlugin::close();
#ifdef WIN32
		WSACleanup();
#endif
	}

	bool PeakEthernetGatewayPlugin::get_is_valid() const
	{
		return isOpen;
	}

	void PeakEthernetGatewayPlugin::close()
	{
		if (0 != rxSocket)
		{
#ifdef WIN32
			closesocket(rxSocket);
#else
			::close(rxSocket);
#endif
		}
		if (0 != txSocket)
		{
#ifdef WIN32
			closesocket(txSocket);
#else
			::close(txSocket);
#endif
		}
	}

	void PeakEthernetGatewayPlugin::open()
	{
		if (!isOpen)
		{
			bool rxOpened = open_rx_socket();

			if (0 != rxSocket)
			{
				if (rxOpened)
				{
					bool txOpened = open_tx_socket();

					if (txOpened)
					{
						rxBuffer.reset(new std::uint8_t[RX_BUFFER_SIZE_BYTES]());
						isOpen = true;
					}
					else if (0 != txSocket)
					{
						close();
					}
				}
				else
				{
					close();
				}
			}
		}
	}

	bool PeakEthernetGatewayPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		bool retVal = false;

		if (!rxQueue.empty())
		{
			canFrame = rxQueue.front();
			rxQueue.pop_front();
			retVal = true;
		}
		else if (nullptr != rxBuffer)
		{
			memset(rxBuffer.get(), 0, RX_BUFFER_SIZE_BYTES);
			int bytesRead = recv(rxSocket, (char *)rxBuffer.get(), RX_BUFFER_SIZE_BYTES, 0);

			if (bytesRead > 0)
			{
				std::uint32_t numberOfFrames = bytesRead / sizeof(S_CAN2IP);

				for (std::uint32_t i = 0; i < numberOfFrames; i++)
				{
					// I know this is all very cursed, but I copied it from PEAK's own example code
					auto size = ntohs(*((std::uint16_t *)&rxBuffer.get()[0 + (i * sizeof(S_CAN2IP))]));

					if (sizeof(S_CAN2IP) == size)
					{
						// Currently unused in the PEAK driver? I hope peak adds support for this eventually so that > 1 channel will work.
						// std::uint8_t channel = rxBuffer.get()[20];

						canFrame.timestamp_us = ntohl(*((std::uint64_t *)&rxBuffer.get()[12]));
						canFrame.dataLength = rxBuffer.get()[21];
						canFrame.isExtendedFrame = (0x02 == ntohs(*((std::uint16_t *)&rxBuffer.get()[22])));
						canFrame.identifier = ntohl(*((std::uint32_t *)&rxBuffer.get()[24]));
						memset(&(canFrame.data[0]), 0, 8);
						memcpy(&(canFrame.data[0]), &rxBuffer.get()[28], canFrame.dataLength > 8 ? 8 : canFrame.dataLength);
						rxQueue.push_back(canFrame);
					}
					else
					{
						// Bad frame?
					}
				}

				if (!rxQueue.empty())
				{
					canFrame = rxQueue.front();
					rxQueue.pop_front();
					retVal = true;
				}
			}
			else
			{
				LOG_ERROR("[PEAK]: Error reading from rx socket. Remote disconnected?");
			}
		}
		return retVal;
	}

	bool PeakEthernetGatewayPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		S_CAN2IP frame = {
			htons(0x24),
			htons(0x80),
			0,
			0,
			0,
			canFrame.dataLength,
			htons(0x02),
			htonl(canFrame.identifier),
			{ canFrame.data[0],
			  canFrame.data[1],
			  canFrame.data[2],
			  canFrame.data[3],
			  canFrame.data[4],
			  canFrame.data[5],
			  canFrame.data[6],
			  canFrame.data[7] }
		};
		return send(txSocket, (char *)&frame, sizeof(frame), 0);
	}

	bool PeakEthernetGatewayPlugin::open_rx_socket()
	{
		struct sockaddr_in sockname;
#ifdef WIN32
		char optval = 1;
		SOCKET sockID = 0;
#else
		std::int32_t optval = 1;
		std::int32_t sockID = 0;
#endif
		int setsockoptRetVal = 0;
		int socketType = udp ? SOCK_DGRAM : SOCK_STREAM;

#ifdef WIN32
		WSAData data;
		WSAStartup(MAKEWORD(2, 2), &data);
#endif

		sockID = socket(AF_INET, socketType, udp ? IPPROTO_UDP : IPPROTO_TCP);

		if (sockID < 0)
		{
			LOG_ERROR("[PEAK]: Unable to open rx socket");
			return false;
		}

		setsockoptRetVal = setsockopt(sockID, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
		if (setsockoptRetVal < 0)
		{
			LOG_ERROR("[PEAK]: Error using rx setsockopt (SOL_SOCKET, SO_REUSEADDR)");
			return false;
		}

		// Enable keepalive packets
		if (!udp)
		{
			setsockoptRetVal = setsockopt(sockID, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
			if (setsockoptRetVal < 0)
			{
				LOG_ERROR("[PEAK]: Error using rx setsockopt (SOL_SOCKET, SO_KEEPALIVE)");
				return false;
			}
		}

#ifdef __linux__
		setsockoptRetVal = setsockopt(sockID, SOL_IP, IP_RECVERR, &optval, sizeof(int));
		if (setsockoptRetVal < 0)
		{
			LOG_ERROR("[PEAK]: Error using rx setsockopt (SOL_IP, IP_RECVERR)");
			return false;
		}
#endif

		//Config Socket
		memset((char *)&sockname, 0, sizeof(struct sockaddr_in));
		sockname.sin_family = AF_INET;
		sockname.sin_port = htons(receivePort);
		sockname.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(sockID, (const struct sockaddr *)&sockname, sizeof(sockname)) < 0)
		{
			LOG_ERROR("[PEAK]: Unable to bind rx socket");
			return false;
		}

		if (!udp)
		{
			struct sockaddr peer_addr;
#if defined __linux__ || defined __APPLE__
			socklen_t addr_len;
#else
			int addr_len;
#endif

			if (listen(sockID, 10) < 0) // Hardcoded queue length of 10, is that enough?
			{
				LOG_ERROR("[PEAK]: Unable to listen to clients");
				return false;
			}

			sockID = accept(sockID, &peer_addr, &addr_len);
			rxSocket = sockID;
			LOG_DEBUG("[PEAK]: Accepted TCP connection from client. Ready to receive TCP CAN frames");
		}
		else
		{
			LOG_DEBUG("[PEAK]: Ready to receive UDP CAN frames");
			rxSocket = sockID;
		}
		return (0 != sockID);
	}

	bool PeakEthernetGatewayPlugin::open_tx_socket()
	{
		struct sockaddr_in sockname;
#ifdef WIN32
		char optval = 1;
#else
		std::int32_t optval = 1;
#endif
		int sockID = 0;
		int setsockoptRetVal = 0;
		int socketType = udp ? SOCK_DGRAM : SOCK_STREAM;

		if ((ipAddress.length() < 7) && ("*" != ipAddress))
		{
			LOG_ERROR("[PEAK]: Invalid IP address");
			return false;
		}

		//New Socket Instance
		if (socketType == SOCK_STREAM)
		{
			if ((sockID = socket(PF_INET, SOCK_STREAM, 0)) < 0)
			{
				LOG_ERROR("[PEAK]: Unable to open tx socket");
				return false;
			}
			setsockoptRetVal = setsockopt(sockID, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int));
			if (setsockoptRetVal < 0)
			{
				LOG_ERROR("[PEAK]: Error using tx setsockopt");
				return false;
			}
		}
		else if (socketType == SOCK_DGRAM)
		{
			if ((sockID = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
			{
				LOG_ERROR("[PEAK]: Unable to open tx socket");
				return false;
			}
		}

		setsockoptRetVal = setsockopt(sockID, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
		if (setsockoptRetVal < 0)
		{
			LOG_ERROR("[PEAK]: Error using tx setsockopt (SOL_SOCKET, SO_REUSEADDR)");
			return false;
		}

		if (!udp)
		{
			setsockoptRetVal = setsockopt(sockID, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int));
			if (setsockoptRetVal < 0)
			{
				LOG_ERROR("[PEAK]: Error using tx setsockopt (SOL_SOCKET, SO_KEEPALIVE)");
				return false;
			}
		}

#ifdef __linux__
		setsockoptRetVal = setsockopt(sockID, SOL_IP, IP_RECVERR, &optval, sizeof(int));
		if (setsockoptRetVal < 0)
		{
			LOG_ERROR("[PEAK]: Error using tx setsockopt (SOL_IP, IP_RECVERR)");
			return false;
		}
#endif

		//Config Socket
		memset((char *)&sockname, 0, sizeof(struct sockaddr_in));
		sockname.sin_family = AF_INET;
		sockname.sin_port = htons(sendPort);
		if ("*" != ipAddress)
		{
			sockname.sin_addr.s_addr = inet_addr(ipAddress.c_str());
			if (sockname.sin_addr.s_addr == INADDR_NONE)
			{
				LOG_ERROR("[PEAK]: tx socket invalid IP address");
				return false;
			}
		}
		else
		{
			sockname.sin_addr.s_addr = htonl(INADDR_ANY); /* wildcard */
		}

		int connectResult = connect(sockID, (struct sockaddr *)&sockname, sizeof(struct sockaddr_in));
		if (0 == connectResult)
		{
			txSocket = sockID;
			LOG_DEBUG("[PEAK]: Transmit socket connected");
			return true;
		}
		else
		{
			LOG_ERROR("[PEAK]: tx connect failed (%s)\n", strerror(errno));
#ifdef WIN32
			closesocket(sockID);
#else
			::close(sockID);
#endif
			return false;
		}
	}
}
