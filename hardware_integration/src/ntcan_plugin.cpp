//================================================================================================
/// @file ntcan_plugin.cpp
///
/// @brief An interface for using a ESD NTCAN driver.
/// @attention Use of the NTCAN driver is governed in part by their license, and requires you
/// to install their driver first, which in-turn requires you to agree to their terms and conditions.
/// @author Alex "Y_Less" Cole
/// @author Daan Steenbergen
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================

#include "isobus/hardware_integration/ntcan_plugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <chrono>
#include <cstring>
#include <thread>

namespace isobus
{
	NTCANPlugin::NTCANPlugin(int channel, int baudrate) :
	  net(channel),
	  baudrate(baudrate),
	  timestampFreq(1000000),
	  timestampOffset(0)
	{
	}

	std::string NTCANPlugin::get_name() const
	{
		return "ESD NTCAN";
	}

	bool NTCANPlugin::get_is_valid() const
	{
		return (NTCAN_SUCCESS == openResult) && (NTCAN_NO_HANDLE != handle);
	}

	void NTCANPlugin::close()
	{
		canClose(handle);
		handle = NTCAN_NO_HANDLE;
	}

	void NTCANPlugin::open()
	{
		if (NTCAN_NO_HANDLE != handle)
		{
			LOG_ERROR("[NTCAN]: Attempting to open a connection that is already open");
		}
		std::uint32_t mode = 0;
		std::int32_t txQueueSize = 256;
		std::int32_t rxQueueSize = 256;
		std::int32_t txTimeOut = 1000;
		std::int32_t rxTimeOut = 1000;

		openResult = canOpen(net, mode, txQueueSize, rxQueueSize, txTimeOut, rxTimeOut, &handle);

		if (NTCAN_SUCCESS != openResult)
		{
			LOG_ERROR("[NTCAN]: Error trying to open the connection");
			return;
		}

		CAN_IF_STATUS status{ 0 };

		openResult = canSetBaudrate(handle, baudrate);
		if (NTCAN_SUCCESS != openResult)
		{
			LOG_ERROR("[NTCAN]: Error trying to set the baudrate");
			close();
			return;
		}

		openResult = canStatus(handle, &status);
		if (NTCAN_SUCCESS != openResult)
		{
			LOG_ERROR("[NTCAN]: Error trying to get the status");
			close();
			return;
		}

		if (NTCAN_FEATURE_TIMESTAMP == (status.features & NTCAN_FEATURE_TIMESTAMP))
		{
			LOG_DEBUG("[NTCAN]: have timestamp feature");
			std::uint64_t timestamp = 0;
			openResult = canIoctl(handle, NTCAN_IOCTL_GET_TIMESTAMP_FREQ, &timestampFreq);
			if (NTCAN_SUCCESS == openResult)
			{
				openResult = canIoctl(handle, NTCAN_IOCTL_GET_TIMESTAMP, &timestamp);
				if (NTCAN_SUCCESS != openResult)
				{
					LOG_ERROR("[NTCAN]: Error NTCAN_IOCTL_GET_TIMESTAMP failed");
				}
			}
			else
			{
				LOG_ERROR("[NTCAN]: Error NTCAN_IOCTL_GET_TIMESTAMP_FREQ failed");
			}
			if (NTCAN_SUCCESS == openResult)
			{
				auto now = std::chrono::system_clock::now();
				auto unix = now.time_since_epoch();
				long long millis = std::chrono::duration_cast<std::chrono::microseconds>(unix).count();
				timestampOffset = millis - timestamp;
			}
		}

		if (NTCAN_FEATURE_SMART_ID_FILTER == (status.features & NTCAN_FEATURE_SMART_ID_FILTER))
		{
			std::int32_t ids = (1 << 11);
			openResult = canIdRegionAdd(handle, 0, &ids);
			if ((NTCAN_SUCCESS != openResult) || ((1 << 11) != ids))
			{
				openResult = NTCAN_INSUFFICIENT_RESOURCES;
				LOG_ERROR("[NTCAN]: Error trying to add the standard ID region");
				close();
				return;
			}

			ids = (1 << 29);
			openResult = canIdRegionAdd(handle, NTCAN_20B_BASE, &ids);
			if ((NTCAN_SUCCESS != openResult) || ((1 << 29) != ids))
			{
				openResult = NTCAN_INSUFFICIENT_RESOURCES;
				LOG_ERROR("[NTCAN]: Error trying to add the extended ID region");
				close();
				return;
			}
		}
		else
		{
			// Older devices such as the first generation CAN-USB don't support canIdRegionAdd()
			LOG_DEBUG("[NTCAN]: do not have Smart ID Filter feature");
			for (std::int32_t id = 0; id < (1 << 11); id++)
			{
				openResult = canIdAdd(handle, id);
				if (NTCAN_SUCCESS != openResult)
				{
					openResult = NTCAN_INSUFFICIENT_RESOURCES;
					LOG_ERROR("[NTCAN]: Error trying to add the standard ID region (no SmartId filter)");
					close();
					return;
				}
			}

			// Without the SmartId filter, enabling any one 29-bit ID lets all 29-bit IDs through,
			// because the AMR register defaults to 0x1FFFFFFF
			openResult = canIdAdd(handle, NTCAN_20B_BASE);
			if (NTCAN_SUCCESS != openResult)
			{
				openResult = NTCAN_INSUFFICIENT_RESOURCES;
				LOG_ERROR("[NTCAN]: Error trying to add the extended ID region (no SmartId filter)");
				close();
				return;
			}
		}

		// Makes canReadT() also return CAN error events, which read_frame() logs
		if (NTCAN_SUCCESS != canIdAdd(handle, NTCAN_EV_CAN_ERROR))
		{
			LOG_WARNING("[NTCAN]: failed to enable CAN error event reporting");
		}
	}

	bool NTCANPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		NTCAN_RESULT result;
		CMSG_T msgCanMessage{ 0 };
		bool retVal = false;
		std::int32_t count = 1;

		result = canReadT(handle, &msgCanMessage, &count, nullptr);

		if ((NTCAN_SUCCESS != result) || (1 != count))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		else if (NTCAN_IS_EVENT(msgCanMessage.id))
		{
			NTCAN_FORMATEVENT_PARAMS par = { 0 };
			par.timestamp = msgCanMessage.timestamp;
			par.timestamp_freq = timestampFreq;
			char eventText[128] = { 0 };
			// canReadT() returns events in a CMSG_T, whose leading fields have the EVMSG layout
			if (NTCAN_SUCCESS == canFormatEvent(reinterpret_cast<EVMSG *>(&msgCanMessage), &par, eventText, sizeof(eventText)))
			{
				eventText[sizeof(eventText) - 1] = '\0';
				LOG_WARNING("[NTCAN EVT]: %s", eventText);
			}
		}
		else
		{
			canFrame.dataLength = NTCAN_LEN_TO_DATASIZE(msgCanMessage.len);
			std::memcpy(canFrame.data, msgCanMessage.data, canFrame.dataLength);
			canFrame.identifier = NTCAN_ID(msgCanMessage.id);
			canFrame.isExtendedFrame = NTCAN_IS_EFF(msgCanMessage.id) ? 1 : 0;
			canFrame.timestamp_us = msgCanMessage.timestamp * 1000000 / timestampFreq + timestampOffset;
			if (msgCanMessage.msg_lost > 0)
			{
				numLostMsgs += msgCanMessage.msg_lost;
			}
			retVal = true;
		}
		return retVal;
	}

	bool NTCANPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		NTCAN_RESULT result;
		CMSG msgCanMessage{ 0 };
		std::int32_t count = 1;

		msgCanMessage.id = canFrame.isExtendedFrame ? (canFrame.identifier | NTCAN_20B_BASE) : canFrame.identifier;
		msgCanMessage.len = canFrame.dataLength;
		std::memcpy(msgCanMessage.data, canFrame.data, canFrame.dataLength);

		// we won't use canWriteT() here bcs. we do not need scheduled transmits: AND THERE IS A BUG IN
		// current NTCAN driver: 'count' is returned 0 always while the CAN message has been send out
		// successfully ==> this leads to 100% bus load bcs. the message will NOT be messagesToBeTransmittedQueue.pop()'ed
		result = canWrite(handle, &msgCanMessage, &count, nullptr);

		return (NTCAN_SUCCESS == result && 1 == count);
	}

	bool NTCANPlugin::reconfigure(int channel, int baudrate)
	{
		bool retVal = false;

		if (!get_is_valid())
		{
			net = channel;
			this->baudrate = baudrate;
			retVal = true;
		}
		return retVal;
	}
} // namespace isobus
