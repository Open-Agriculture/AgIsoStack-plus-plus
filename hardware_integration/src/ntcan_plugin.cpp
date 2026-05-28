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
			isobus::CANStackLogger::error("[NTCAN]: Attempting to open a connection that is already open");
		}
		std::uint32_t mode = 0;
		std::int32_t txQueueSize = min( NTCAN_MAX_TX_QUEUESIZE, 256 );
		std::int32_t rxQueueSize = min( NTCAN_MAX_RX_QUEUESIZE, 256 );
		std::int32_t txTimeOut = 1000;
		std::int32_t rxTimeOut = 1000;

		openResult = canOpen(net, mode, txQueueSize, rxQueueSize, txTimeOut, rxTimeOut, &handle);

		if (NTCAN_SUCCESS != openResult)
		{
			isobus::CANStackLogger::error("[NTCAN]: Error trying to open the connection");
			return;
		}

		CAN_IF_STATUS status{ 0 };

		openResult = canSetBaudrate(handle, baudrate);
		if (NTCAN_SUCCESS != openResult)
		{
			isobus::CANStackLogger::error("[NTCAN]: Error trying to set the baudrate");
			close();
			return;
		}

		openResult = canStatus(handle, &status);
		if (NTCAN_SUCCESS != openResult)
		{
			isobus::CANStackLogger::error("[NTCAN]: Error trying to get the status");
			close();
			return;
		}

		if (NTCAN_FEATURE_TIMESTAMP == (status.features & NTCAN_FEATURE_TIMESTAMP))
		{
			isobus::CANStackLogger::debug("[NTCAN]: have timestamp feature");
			std::uint64_t timestamp = 0;
			openResult = canIoctl(handle, NTCAN_IOCTL_GET_TIMESTAMP_FREQ, &timestampFreq);
			if (NTCAN_SUCCESS == openResult)
			{
				openResult = canIoctl(handle, NTCAN_IOCTL_GET_TIMESTAMP, &timestamp);
				if (NTCAN_SUCCESS != openResult) {
					isobus::CANStackLogger::error("[NTCAN]: Error NTCAN_IOCTL_GET_TIMESTAMP failed");
				}
			} else {
				isobus::CANStackLogger::error("[NTCAN]: Error NTCAN_IOCTL_GET_TIMESTAMP_FREQ failed");
			}
			if (NTCAN_SUCCESS == openResult)
			{
				auto now = std::chrono::system_clock::now();
				auto unix = now.time_since_epoch();
				long long millis = std::chrono::duration_cast<std::chrono::microseconds>(unix).count();
				timestampOffset = millis - timestamp;
			}
		}

		bool smart_filt = true;
		if (NTCAN_FEATURE_SMART_ID_FILTER != (status.features & NTCAN_FEATURE_SMART_ID_FILTER))
		{
			isobus::CANStackLogger::debug("[NTCAN]: do not have Smart ID Filter feature");
			smart_filt = false;
		}
		 
		if (smart_filt) {
			std::int32_t ids = (1 << 11);
			openResult = canIdRegionAdd(handle, 0, &ids);
			if (NTCAN_SUCCESS != openResult || (NTCAN_SUCCESS == openResult && ids != (1 << 11)))
			{
				openResult = NTCAN_INSUFFICIENT_RESOURCES;
				isobus::CANStackLogger::error("[NTCAN]: Error trying to add the standard ID region");
				close();
				return;
			}
			ids = (1 << 29);
			openResult = canIdRegionAdd(handle, NTCAN_20B_BASE, &ids);
			if (NTCAN_SUCCESS != openResult || (NTCAN_SUCCESS == openResult && ids != (1 << 29)))
			{
				openResult = NTCAN_INSUFFICIENT_RESOURCES;
				isobus::CANStackLogger::error("[NTCAN]: Error trying to add the extended ID region");
				close();
				return;
			}

		} else {
			// cannot use canIdRegionAdd() with old esd devices/drivers like "CAN-USB" first gen
			std::int32_t id;
			for (id = 0; id < 0x7FF; id++) {
				openResult = canIdAdd(handle, id);
				if(openResult != NTCAN_SUCCESS) {
					openResult = NTCAN_INSUFFICIENT_RESOURCES;
					isobus::CANStackLogger::error("[NTCAN]: Error trying to add the standard ID region (no SmartId filter)");
					close();
					return;
				}
			}
			// Without SmartId filter: As soon as an arbitrary 29-bit CAN-ID is enabled with canIdAdd()
			// all 29-bit CAN-IDs will pass the first filter stage, bcs. AMR register is default initialized 
			// to 0x1FFFFFFF.
			id = NTCAN_20B_BASE;
			openResult = canIdAdd(handle, id);
			if(openResult != NTCAN_SUCCESS) {
				openResult = NTCAN_INSUFFICIENT_RESOURCES;
				isobus::CANStackLogger::error("[NTCAN]: Error trying to add the extended ID region (no SmartId filter)");
				close();
				return;
			}
		}
		
		if(1) {
			std::int32_t id = NTCAN_EV_CAN_ERROR;	// canReadT() will report error events
			NTCAN_RESULT res = canIdAdd(handle, id);
			if(res != NTCAN_SUCCESS) {
				isobus::CANStackLogger::warn("[NTCAN]: failed to enable CAN error event reporting");
			}
		}
		//#define NTCAN_IS_EVENT(id)

	}

	bool NTCANPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		NTCAN_RESULT result;
		CMSG_T msgCanMessage{ 0 };
		bool retVal = false;
		std::int32_t count = 1;

		result = canReadT(handle, &msgCanMessage, &count, nullptr);

		if (NTCAN_SUCCESS != result || 1 != count) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			return false;
		}
		
		if ( NTCAN_IS_EVENT(msgCanMessage.id) ) {
			// got a an event frame
			EVMSG_T *msgCanEvent = (EVMSG_T*)&msgCanMessage;
			std::uint32_t evt_len = msgCanEvent->len & 0x0F;
			switch (msgCanEvent->evid) {
			case NTCAN_EV_CAN_ERROR:
				{
					const EV_CAN_ERROR &err(msgCanEvent->evdata.error);
					// uint8_t err.can_status;     // CAN controller status
					// uint8_t err.dma_stall;      // DMA stall counter (HW dependent)
					// uint8_t err.ctrl_overrun;   // Controller overruns
					// uint8_t err.fifo_overrun;   // Driver FIFO overruns
				}
				break;
			default:
				;  // ignore
			}
			if (1) {	// print EVT message
				NTCAN_FORMATEVENT_PARAMS par = { 0 };
				par.timestamp = msgCanEvent->timestamp;
				par.timestamp_freq = timestampFreq;
				par.num_baudrate = baudrate; // ???
				char evt_msg[128];
				evt_msg[0] = '\0';
				result = canFormatEvent( (EVMSG*)msgCanEvent, &par, evt_msg, sizeof evt_msg);
				if (NTCAN_SUCCESS == result) {
					evt_msg[sizeof evt_msg -1] ='\0';
					isobus::CANStackLogger::warn("[NTCAN EVT]: %s", evt_msg);
				}
			}
			// no sleep here!
			return false;
		} else {
			// got a data frame, might be CC or FD
			canFrame.dataLength = NTCAN_LEN_TO_DATASIZE(msgCanMessage.len);
			memcpy(canFrame.data, msgCanMessage.data, canFrame.dataLength);
			canFrame.identifier = NTCAN_ID(msgCanMessage.id);  // lower 29 bits
			canFrame.isExtendedFrame = NTCAN_IS_EFF(msgCanMessage.id) ? 1 : 0;
			canFrame.timestamp_us = msgCanMessage.timestamp * 1000000 / timestampFreq + timestampOffset;
			if (msgCanMessage.msg_lost > 0) {
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
		memcpy(msgCanMessage.data, canFrame.data, canFrame.dataLength);

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
