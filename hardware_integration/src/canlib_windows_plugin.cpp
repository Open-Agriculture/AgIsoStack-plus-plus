//================================================================================================
/// @file canlib_windows_plugin.cpp
///
/// @brief An interface for using a Kvaser CANlib CAN driver.
/// @attention Use of the Kvaser driver is governed in part by their license, and requires you
/// to install their driver first, which in-turn requires you to agree to their terms and conditions.
/// Visit https://www.kvaser.com/ for the needed software.
/// @author Anderson Costa
///
/// @copyright 2025 The Open-Agriculture Developers
//================================================================================================
#include "isobus/hardware_integration/canlib_windows_plugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <thread>

namespace isobus
{
	CANLibWindowsPlugin::CANLibWindowsPlugin(int channel) :
	  handle(canINVALID_HANDLE),
	  channelIndex(channel),
	  openResult(canERR_NOTFOUND)
	{
		// Initialize CANlib
		canInitializeLibrary();
	}

	CANLibWindowsPlugin::~CANLibWindowsPlugin()
	{
		close();
	}

	std::string CANLibWindowsPlugin::get_name() const
	{
		return "Kvaser CANlib";
	}

	bool CANLibWindowsPlugin::get_is_valid() const
	{
		return (canOK == openResult) && (canINVALID_HANDLE != handle);
	}

	void CANLibWindowsPlugin::close()
	{
		if (canINVALID_HANDLE != handle)
		{
			canBusOff(handle);
			canClose(handle);
			handle = canINVALID_HANDLE;
			openResult = canERR_NOTFOUND;
		}
	}

	void CANLibWindowsPlugin::open()
	{
		if (canINVALID_HANDLE == handle)
		{
			// Open virtual channel using canOPEN_ACCEPT_VIRTUAL flag
			handle = canOpenChannel(channelIndex, canOPEN_ACCEPT_VIRTUAL);
			openResult = canOK; // Assume success initially

			if (canINVALID_HANDLE != handle)
			{
				// Set bus parameters for 250 kbps (typical for ISOBUS)
				canStatus result = canSetBusParams(handle, canBITRATE_250K, 0, 0, 0, 0, 0);
				if (canOK == result)
				{
					// Set bus on
					result = canBusOn(handle);
					if (canOK == result)
					{
						openResult = canOK;
						LOG_INFO("[Kvaser]: Successfully opened CAN channel %d", channelIndex);
					}
					else
					{
						LOG_CRITICAL("[Kvaser]: Failed to set bus on for channel %d. Error: %d", channelIndex, result);
						canClose(handle);
						handle = canINVALID_HANDLE;
						openResult = result;
					}
				}
				else
				{
					LOG_CRITICAL("[Kvaser]: Failed to set bus parameters for channel %d. Error: %d", channelIndex, result);
					canClose(handle);
					handle = canINVALID_HANDLE;
					openResult = result;
				}
			}
			else
			{
				LOG_CRITICAL("[Kvaser]: Failed to open CAN channel %d", channelIndex);
				openResult = canERR_NOTFOUND;
			}
		}
		else
		{
			LOG_WARNING("[Kvaser]: CAN channel is already open");
		}
	}

	bool CANLibWindowsPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		bool retVal = false;

		if (get_is_valid())
		{
			long id;
			unsigned char data[8];
			unsigned int dlc;
			unsigned int flags;
			unsigned long timestamp;

			canStatus result = canRead(handle, &id, data, &dlc, &flags, &timestamp);

			if (canOK == result)
			{
				canFrame.identifier = static_cast<std::uint32_t>(id);
				canFrame.dataLength = static_cast<std::uint8_t>(dlc);
				canFrame.isExtendedFrame = (flags & canMSG_EXT) != 0;
				canFrame.timestamp_us = timestamp * 1000ULL; // Kvaser timestamp is in milliseconds

				// Copy data
				memcpy(canFrame.data, data, dlc);

				// Clear remaining bytes if dlc < 8
				for (std::uint8_t i = dlc; i < 8; i++)
				{
					canFrame.data[i] = 0;
				}

				retVal = true;
			}
			else if (canERR_NOMSG == result)
			{
				// No message available, sleep briefly to avoid busy-waiting
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			else
			{
				LOG_CRITICAL("[Kvaser]: Failed to read CAN frame. Error: %d", result);
			}
		}
		return retVal;
	}

	bool CANLibWindowsPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		bool retVal = false;

		if (get_is_valid())
		{
			unsigned int flags = 0;
			if (canFrame.isExtendedFrame)
			{
				flags |= canMSG_EXT;
			}

			canStatus result = canWrite(handle,
			                            static_cast<long>(canFrame.identifier),
			                            const_cast<void *>(static_cast<const void *>(canFrame.data)),
			                            static_cast<unsigned int>(canFrame.dataLength),
			                            flags);

			retVal = (canOK == result);
			if (!retVal)
			{
				LOG_CRITICAL("[Kvaser]: Failed to write CAN frame. Error: %d", result);
			}
		}
		return retVal;
	}
}
