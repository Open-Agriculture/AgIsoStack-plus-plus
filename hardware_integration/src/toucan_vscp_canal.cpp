//================================================================================================
/// @file toucan_vscp_canal.cpp
///
/// @brief An interface for using a TouCAN USB probe, via the VSCP CANAL api.
/// @details The CANAL api is documented at https://docs.vscp.org/canal/1.0/#/
/// @note The driver library for this plugin is located at https://github.com/rusoku/
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#include "isobus/hardware_integration/toucan_vscp_canal.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#include <thread>

namespace isobus
{
	TouCANPlugin::TouCANPlugin(std::int16_t deviceID, std::uint32_t serialNumber, std::uint16_t baudRate)
	{
		generate_device_name(deviceID, serialNumber, baudRate);
	}

	TouCANPlugin::TouCANPlugin(std::string deviceName) :
	  name(deviceName)
	{
	}

	TouCANPlugin::~TouCANPlugin()
	{
	}

	std::string TouCANPlugin::get_name() const
	{
		return "Rusoku TouCAN";
	}

	bool TouCANPlugin::get_is_valid() const
	{
		return (CANAL_ERROR_SUCCESS == openResult);
	}

	void TouCANPlugin::close()
	{
		CanalClose(handle);
		openResult = CANAL_ERROR_NOT_OPEN;
	}

	void TouCANPlugin::open()
	{
		long tempHandle = CanalOpen(name.c_str(), 0);

		if (0 != tempHandle)
		{
			handle = tempHandle;
			openResult = CANAL_ERROR_SUCCESS;
		}
		else
		{
			LOG_CRITICAL("[TouCAN]: Error trying to connect to TouCAN probe. Check your device ID and serial number.");
		}
	}

	bool TouCANPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		long result = CANAL_ERROR_GENERIC;
		structCanalMsg CANMsg = { 0 };

		result = CanalReceive(handle, &CANMsg);

		if (CANAL_ERROR_SUCCESS == result)
		{
			canFrame.dataLength = CANMsg.sizeData;
			memcpy(canFrame.data, CANMsg.data, sizeof(canFrame.data));
			canFrame.identifier = CANMsg.id;
			canFrame.isExtendedFrame = (0 != (CANAL_IDFLAG_EXTENDED & CANMsg.flags));
			canFrame.timestamp_us = CANMsg.timestamp;
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return (CANAL_ERROR_SUCCESS == result);
	}

	bool TouCANPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		std::uint32_t result = CANAL_ERROR_SUCCESS;
		structCanalMsg msgCanMessage;

		msgCanMessage.id = canFrame.identifier;
		msgCanMessage.sizeData = canFrame.dataLength;
		msgCanMessage.flags = canFrame.isExtendedFrame ? CANAL_IDFLAG_EXTENDED : CANAL_IDFLAG_STANDARD;
		memcpy(msgCanMessage.data, canFrame.data, canFrame.dataLength);

		result = CanalSend(handle, &msgCanMessage);

		return (CANAL_ERROR_SUCCESS == result);
	}

	bool TouCANPlugin::reconfigure(std::int16_t deviceID, std::uint32_t serialNumber, std::uint16_t baudRate)
	{
		bool retVal = false;

		if (!get_is_valid())
		{
			generate_device_name(deviceID, serialNumber, baudRate);
			retVal = true;
		}
		return retVal;
	}

	std::uint32_t TouCANPlugin::get_serial_number() const
	{
		return currentlyConfiguredSerialNumber;
	}

	void TouCANPlugin::generate_device_name(std::int16_t deviceID, std::uint32_t serialNumber, std::uint16_t baudRate)
	{
		std::string deviceConfigString;
		std::string serialString;
		constexpr std::uint32_t MAX_SERIAL_LENGTH = 999999999;
		constexpr std::size_t SERIAL_NUMBER_CHARACTER_REQUIREMENT = 8;

		if (serialNumber > MAX_SERIAL_LENGTH)
		{
			LOG_CRITICAL("[TouCAN]: Invalid serial number. Must be 8 digits max.");
			serialNumber = 0;
		}
		currentlyConfiguredSerialNumber = serialNumber;
		serialString = isobus::to_string(serialNumber);

		if (SERIAL_NUMBER_CHARACTER_REQUIREMENT > serialString.length())
		{
			serialString.insert(0, SERIAL_NUMBER_CHARACTER_REQUIREMENT - serialString.length(), '0');
		}

		deviceConfigString += isobus::to_string(deviceID) + ';';
		deviceConfigString += serialString + ';';
		deviceConfigString += isobus::to_string(baudRate) + ';';
		name = deviceConfigString;
	}
}
