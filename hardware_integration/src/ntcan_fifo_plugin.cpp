//================================================================================================
/// @file ntcan_fifo_plugin.cpp
///
/// @brief An interface for using a ESD NTCAN FIFO driver.
/// @attention Use of the NTCAN driver is governed in part by their license, and requires you
/// to install their driver first, which in-turn requires you to agree to their terms and conditions.
/// @author Alex "Y_Less" Cole
///
/// @copyright 2023 Alex "Y_Less" Cole
//================================================================================================

#include "isobus/hardware_integration/ntcan_fifo_plugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <thread>
#include <chrono>

NTCANFIFOPlugin::NTCANFIFOPlugin(int channel) :
  net(channel),
  timestampFreq(1),
  timestamp(0),
  unix(std::chrono::system_clock::now()),
  openResult(NTCAN_SUCCESS)
{
}

NTCANFIFOPlugin::~NTCANFIFOPlugin()
{
}

bool NTCANFIFOPlugin::get_is_valid() const
{
	return (NTCAN_SUCCESS == openResult);
}

void NTCANFIFOPlugin::close()
{
	canClose(handle);
}

void NTCANFIFOPlugin::open()
{
	uint32_t mode = 0;
	int32_t txQueueSize = 8;
	int32_t rxQueueSize = 8;
	int32_t txTimeOut = 100;
	int32_t rxTimeOut = 1000;
	CAN_IF_STATUS status {0};
	uint64_t timestamp = 0;
	int ids;

	openResult = canOpen(net, mode, txQueueSize, rxQueueSize, txTimeOut, rxTimeOut, &handle);

	if (NTCAN_SUCCESS == openResult)
	{
		openResult = canSetBaudrate(handle, NTCAN_BAUD_250);

		if (NTCAN_SUCCESS == openResult)
		{
			openResult = canStatus(handle, NTCAN_FEATURE_TIMESTAMP, &status);
		}
		
		if (NTCAN_FEATURE_TIMESTAMP == status.features & NTCAN_FEATURE_TIMESTAMP)
		{
			if (NTCAN_SUCCESS == openResult)
			{
				openResult = canIoctl(handle, NTCAN_IOCTL_GET_TIMESTAMP_FREQ, &timestampFreq);
			}
			
			if (NTCAN_SUCCESS == openResult)
			{
				openResult = canIoctl(handle, NTCAN_IOCTL_GET_TIMESTAMP, &timestamp);
			}
			
			if (NTCAN_SUCCESS == openResult)
			{
				auto now = std::chrono::system_clock::now();
				auto unix = now.time_since_epoch();
				uint64_t millis = std::chrono::duration_cast<std::chrono::microseconds>(since_epoch);
				timestampOff = millis - timestamp;
			}
		}
			
		if (NTCAN_SUCCESS == openResult)
		{
			ids = 0x800;
			openResult = canIdRegionAdd(handle, 0, &ids);
			if (NTCAN_SUCCESS == openResult && ids != 0x800)
			{
				openResult = NTCAN_INSUFFICIENT_RESOURCES;
			}
		}
		
		if (NTCAN_SUCCESS == openResult)
		{
			ids = 0x20000000;
			openResult = canIdRegionAdd(handle, 0 | NTCAN_20B_BASE, &ids);
			if (NTCAN_SUCCESS == openResult && ids != 0x20000000)
			{
				openResult = NTCAN_INSUFFICIENT_RESOURCES;
			}
		}

		if (NTCAN_SUCCESS != openResult)
		{
			isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Critical, "[NTCAN]: Error trying to connect to NTCAN driver");
			canClose(handle);
		}
	}
	else
	{
		isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Critical, "[NTCAN]: Error trying to connect to NTCAN driver");
	}
}

bool NTCANFIFOPlugin::read_frame(isobus::HardwareInterfaceCANFrame &canFrame)
{
	NTCAN_RESULT result;
	CMSG_T msgCanMessage {0};
	bool retVal = false;
	int32_t count = 1;

	result = canReadT(handle, &msgCanMessage, &count, nullptr);

	if (NTCAN_SUCCESS == result && 1 == count)
	{
		canFrame.dataLength = msgCanMessage.len;
		memcpy(canFrame.data, msgCanMessage.data, msgCanMessage.len);
		canFrame.identifier = msgCanMessage.id & 0x1FFFFFFF;
		canFrame.isExtendedFrame = (NTCAN_20B_BASE == msgCanMessage.id & NTCAN_20B_BASE);
		canFrame.timestamp_us = msgCanMessage.timestamp * 1000000 / timestampFreq + timestampOff;
		retVal = true;
	}
	else
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return retVal;
}

bool NTCANFIFOPlugin::write_frame(const isobus::HardwareInterfaceCANFrame &canFrame)
{
	NTCAN_RESULT result;
	CMSG_T msgCanMessage {0};
	int32_t count = 1;

	msgCanMessage.id = canFrame.isExtendedFrame ? canFrame.identifier | NTCAN_20B_BASE : canFrame.identifier;
	msgCanMessage.len = canFrame.dataLength;
	memcpy(msgCanMessage.data, canFrame.data, canFrame.dataLength);

	result = canWriteT(handle, &msgCanMessage, &count, nullptr);

	return (NTCAN_SUCCESS == result && 1 == count);
}
