//================================================================================================
/// @file socket_can_interface.cpp
///
/// @brief An CAN driver for socket CAN on linux.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#include "isobus/hardware_integration/socket_can_interface.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>

namespace isobus
{
	SocketCANInterface::SocketCANInterface(const std::string deviceName) :
	  pCANDevice(new sockaddr_can),
	  name(deviceName),
	  fileDescriptor(-1)
	{
		if (nullptr != pCANDevice)
		{
			std::memset(pCANDevice, 0, sizeof(struct sockaddr_can));
		}
	}

	std::string SocketCANInterface::get_name() const
	{
		return "SocketCAN";
	}

	SocketCANInterface::~SocketCANInterface()
	{
		close();

		if (nullptr != pCANDevice)
		{
			delete pCANDevice;
			pCANDevice = nullptr;
		}
	}

	bool SocketCANInterface::get_is_valid() const
	{
		return (-1 != fileDescriptor);
	}

	std::string SocketCANInterface::get_device_name() const
	{
		return name;
	}

	void SocketCANInterface::close()
	{
		::close(fileDescriptor);
		fileDescriptor = -1;
	}

	void SocketCANInterface::open()
	{
		set_last_error("");

		// strncpy into ifr_name would silently cut a longer name short, and the cut name can match a different interface
		if (name.size() >= IFNAMSIZ)
		{
			fail_open("the name is longer than " + to_string(IFNAMSIZ - 1) + " characters.");
		}
		else
		{
			fileDescriptor = socket(PF_CAN, SOCK_RAW, CAN_RAW);

			if (fileDescriptor >= 0)
			{
				struct ifreq interfaceRequestStructure;
				const int RECEIVE_OWN_MESSAGES = 0;
				const int DROP_MONITOR = 1;
				const int TIMESTAMPING = 0x58;
				const int TIMESTAMP = 1;
				memset(&interfaceRequestStructure, 0, sizeof(interfaceRequestStructure));
				strncpy(interfaceRequestStructure.ifr_name, name.c_str(), sizeof(interfaceRequestStructure.ifr_name));
				setsockopt(fileDescriptor, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &RECEIVE_OWN_MESSAGES, sizeof(RECEIVE_OWN_MESSAGES));
				setsockopt(fileDescriptor, SOL_SOCKET, SO_RXQ_OVFL, &DROP_MONITOR, sizeof(DROP_MONITOR));

				if (setsockopt(fileDescriptor, SOL_SOCKET, SO_TIMESTAMPING, &TIMESTAMPING, sizeof(TIMESTAMPING)) < 0)
				{
					setsockopt(fileDescriptor, SOL_SOCKET, SO_TIMESTAMP, &TIMESTAMP, sizeof(TIMESTAMP));
				}

				if (ioctl(fileDescriptor, SIOCGIFINDEX, &interfaceRequestStructure) >= 0)
				{
					memset(pCANDevice, 0, sizeof(sockaddr_can));
					pCANDevice->can_family = AF_CAN;
					pCANDevice->can_ifindex = interfaceRequestStructure.ifr_ifindex;

					// bind() succeeds on a down interface, and the socket would then only fail later with ENETDOWN
					if (ioctl(fileDescriptor, SIOCGIFFLAGS, &interfaceRequestStructure) < 0)
					{
						fail_open("ioctl(SIOCGIFFLAGS) failed: " + std::string(std::strerror(errno)));
					}
					else if (0 == (interfaceRequestStructure.ifr_flags & IFF_UP))
					{
						fail_open("the interface is down.");
					}
					else if (bind(fileDescriptor, (struct sockaddr *)pCANDevice, sizeof(struct sockaddr)) < 0)
					{
						fail_open("bind() failed: " + std::string(std::strerror(errno)));
					}
				}
				else
				{
					fail_open("ioctl(SIOCGIFINDEX) failed: " + std::string(std::strerror(errno)));
				}
			}
			else
			{
				fail_open("socket() failed: " + std::string(std::strerror(errno)));
			}
		}
	}

	void SocketCANInterface::fail_open(const std::string &reason)
	{
		close();
		const std::string error = "Unable to open interface " + name + ", " + reason;
		set_last_error(error);
		LOG_ERROR("[SocketCAN] " + error);
	}

	void SocketCANInterface::close_because_down()
	{
		const std::string error = name + " interface is down.";
		set_last_error(error);
		LOG_CRITICAL("[SocketCAN] " + error);
		close();
	}

	void SocketCANInterface::set_last_error(const std::string &error)
	{
		LOCK_GUARD(Mutex, lastErrorMutex);
		lastError = error;
	}

	std::string SocketCANInterface::get_last_error() const
	{
		LOCK_GUARD(Mutex, lastErrorMutex);
		return lastError;
	}

	bool SocketCANInterface::read_frame(isobus::CANMessageFrame &canFrame)
	{
		struct pollfd pollingFileDescriptor;
		bool retVal = false;

		pollingFileDescriptor.fd = fileDescriptor;
		pollingFileDescriptor.events = POLLIN;
		pollingFileDescriptor.revents = 0;

		if (1 == poll(&pollingFileDescriptor, 1, 100))
		{
			canFrame.timestamp_us = std::numeric_limits<std::uint64_t>::max();
			struct can_frame txFrame;
			struct msghdr message;
			struct iovec segment;

			char lControlMessage[CMSG_SPACE(sizeof(struct timeval) + (3 * sizeof(struct timespec)) + sizeof(std::uint32_t))];

			segment.iov_base = &txFrame;
			segment.iov_len = sizeof(struct can_frame);
			message.msg_iov = &segment;
			message.msg_iovlen = 1;
			message.msg_control = &lControlMessage;
			message.msg_controllen = sizeof(lControlMessage);
			message.msg_name = pCANDevice;
			message.msg_namelen = sizeof(struct sockaddr_can);
			message.msg_flags = 0;

			if (recvmsg(fileDescriptor, &message, 0) > 0)
			{
				if (0 == (txFrame.can_id & CAN_ERR_FLAG))
				{
					if (0 != (txFrame.can_id & CAN_EFF_FLAG))
					{
						canFrame.identifier = (txFrame.can_id & CAN_EFF_MASK);
						canFrame.isExtendedFrame = true;
					}
					else
					{
						canFrame.identifier = (txFrame.can_id & CAN_SFF_MASK);
						canFrame.isExtendedFrame = false;
					}
					canFrame.dataLength = txFrame.can_dlc;
					memset(canFrame.data, 0, sizeof(canFrame.data));
					memcpy(canFrame.data, txFrame.data, canFrame.dataLength);

					for (struct cmsghdr *pControlMessage = CMSG_FIRSTHDR(&message); (nullptr != pControlMessage) && (SOL_SOCKET == pControlMessage->cmsg_level); pControlMessage = CMSG_NXTHDR(&message, pControlMessage))
					{
						switch (pControlMessage->cmsg_type)
						{
							case SO_TIMESTAMP:
							{
								struct timeval *time = (struct timeval *)CMSG_DATA(pControlMessage);

								if (std::numeric_limits<std::uint64_t>::max() == canFrame.timestamp_us)
								{
									canFrame.timestamp_us = static_cast<std::uint64_t>(time->tv_usec) + (static_cast<std::uint64_t>(time->tv_sec) * 1000000);
								}
							}
							break;

							case SO_TIMESTAMPING:
							{
								struct timespec *time = (struct timespec *)(CMSG_DATA(pControlMessage));
								canFrame.timestamp_us = (static_cast<std::uint64_t>(time[2].tv_nsec) / 1000) + (static_cast<std::uint64_t>(time[2].tv_sec) * 1000000);
							}
							break;
						}
					}
					retVal = true;
				}
			}
			else if (errno == ENETDOWN)
			{
				close_because_down();
			}
		}
		else if (pollingFileDescriptor.revents & (POLLERR | POLLHUP))
		{
			close();
		}
		return retVal;
	}

	bool SocketCANInterface::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		struct can_frame txFrame;
		bool retVal = false;

		txFrame.can_id = canFrame.identifier;
		txFrame.can_dlc = canFrame.dataLength;
		memcpy(txFrame.data, canFrame.data, canFrame.dataLength);

		if (canFrame.isExtendedFrame)
		{
			txFrame.can_id |= CAN_EFF_FLAG;
		}

		if (write(fileDescriptor, &txFrame, sizeof(struct can_frame)) > 0)
		{
			retVal = true;
		}
		else if (errno == ENETDOWN)
		{
			close_because_down();
		}
		return retVal;
	}

	bool SocketCANInterface::set_name(const std::string &newName)
	{
		bool retVal = false;

		if (!get_is_valid())
		{
			name = newName;
			retVal = true;
		}
		return retVal;
	}
}
