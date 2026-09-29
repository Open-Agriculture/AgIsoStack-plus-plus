//================================================================================================
/// @file sys_tec_windows_plugin.cpp
///
/// @brief An interface for using a SYS TEC sysWORXX USB CAN device.
/// @attention Make sure you've installed the appropriate driver software for this device before
/// you use this plugin. Visit https://www.systec-electronic.com/ for the needed software.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#include "isobus/hardware_integration/sys_tec_windows_plugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <thread>

namespace isobus
{
	SysTecWindowsPlugin::SysTecWindowsPlugin(std::uint8_t channel, std::uint32_t baudrate) :
	  baudrateConstant(baudrate),
	  channelIndex(channel)
	{
		if (channel > USBCAN_CHANNEL_CH1)
		{
			LOG_CRITICAL("[SYSTEC]: Invalid channel");
		}
	}

	SysTecWindowsPlugin::SysTecWindowsPlugin(std::uint32_t serialNumber, std::uint32_t baudrate) :
	  serialNumber(serialNumber),
	  baudrateConstant(baudrate)
	{
	}

	SysTecWindowsPlugin::~SysTecWindowsPlugin()
	{
		close();
	}

	std::string SysTecWindowsPlugin::get_name() const
	{
		return "SYS TEC";
	}

	bool SysTecWindowsPlugin::get_is_valid() const
	{
		return (openResult && (USBCAN_INVALID_HANDLE != handle));
	}

	void SysTecWindowsPlugin::close()
	{
		if (get_is_valid())
		{
			openResult = false;
			UcanDeinitCan(handle);
			UcanDeinitHardware(handle);
			handle = USBCAN_INVALID_HANDLE;
		}
	}

	void SysTecWindowsPlugin::open()
	{
		if (!get_is_valid())
		{
			if (0 == serialNumber)
			{
				openResult = (USBCAN_SUCCESSFUL == UcanInitHardwareEx(&handle, channelIndex, NULL, this));
			}
			else
			{
				openResult = (USBCAN_SUCCESSFUL == UcanInitHardwareEx2(&handle, serialNumber, NULL, this));
			}

			if (openResult && (USBCAN_INVALID_HANDLE != handle))
			{
				tUcanInitCanParam initCANParameters = { 0 };
				tUcanChannelInfo channelInfo = { 0 };
				channelInfo.m_dwSize = sizeof(tUcanChannelInfo);

				initCANParameters.m_dwSize = sizeof(tUcanInitCanParam);
				initCANParameters.m_bMode = kUcanModeNormal;
				initCANParameters.m_bBTR0 = ((baudrateConstant >> 8) & 0xFF);
				initCANParameters.m_bBTR1 = (baudrateConstant & 0xFF);
				initCANParameters.m_bOCR = 0x1A;
				initCANParameters.m_dwAMR = USBCAN_AMR_ALL;
				initCANParameters.m_dwACR = USBCAN_ACR_ALL;
				initCANParameters.m_dwBaudrate = USBCAN_BAUDEX_USE_BTR01;
				initCANParameters.m_wNrOfRxBufferEntries = USBCAN_DEFAULT_BUFFER_ENTRIES;
				initCANParameters.m_wNrOfTxBufferEntries = USBCAN_DEFAULT_BUFFER_ENTRIES;

				openResult = (USBCAN_SUCCESSFUL == UcanInitCanEx2(this->handle, channelIndex, &initCANParameters));

				if (!openResult)
				{
					LOG_CRITICAL("[SYSTEC]: Error trying to configure a SYS TEC probe channel");
				}
			}
			else
			{
				LOG_CRITICAL("[SYSTEC]: Error trying to connect to SYS TEC probe");
			}
		}
		else
		{
			LOG_WARNING("[SYSTEC]: CAN Adapter already initialized.");
		}
	}

	bool SysTecWindowsPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		bool retVal = false;

		if (get_is_valid())
		{
			tCanMsgStruct CANMessage = { 0 };

			retVal = (USBCAN_SUCCESSFUL == UcanReadCanMsgEx(handle, &channelIndex, &CANMessage, NULL));

			if (retVal)
			{
				canFrame.dataLength = CANMessage.m_bDLC;
				canFrame.identifier = CANMessage.m_dwID;
				canFrame.isExtendedFrame = (USBCAN_MSG_FF_EXT == CANMessage.m_bFF);
				canFrame.data[0] = CANMessage.m_bData[0];
				canFrame.data[1] = CANMessage.m_bData[1];
				canFrame.data[2] = CANMessage.m_bData[2];
				canFrame.data[3] = CANMessage.m_bData[3];
				canFrame.data[4] = CANMessage.m_bData[4];
				canFrame.data[5] = CANMessage.m_bData[5];
				canFrame.data[6] = CANMessage.m_bData[6];
				canFrame.data[7] = CANMessage.m_bData[7];
			}
			else
			{
				//! @todo Can probably optimize this better somehow by using the RX callback
				//! from the driver and a condition variable?
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // This is long enough for 2 messages to be received at max bus load, which should be fine.
			}
		}
		return retVal;
	}

	bool SysTecWindowsPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		bool retVal = false;

		if (get_is_valid())
		{
			tCanMsgStruct CANMessage;

			CANMessage.m_dwID = canFrame.identifier;
			CANMessage.m_bDLC = canFrame.dataLength;
			CANMessage.m_bFF = USBCAN_MSG_FF_EXT;
			CANMessage.m_bData[0] = canFrame.data[0];
			CANMessage.m_bData[1] = canFrame.data[1];
			CANMessage.m_bData[2] = canFrame.data[2];
			CANMessage.m_bData[3] = canFrame.data[3];
			CANMessage.m_bData[4] = canFrame.data[4];
			CANMessage.m_bData[5] = canFrame.data[5];
			CANMessage.m_bData[6] = canFrame.data[6];
			CANMessage.m_bData[7] = canFrame.data[7];

			retVal = (USBCAN_SUCCESSFUL == UcanWriteCanMsgEx(handle, channelIndex, &CANMessage, nullptr));
		}
		return retVal;
	}
}
