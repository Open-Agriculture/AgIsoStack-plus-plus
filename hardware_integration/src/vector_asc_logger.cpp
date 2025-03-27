/*******************************************************************************
** @file       vector_asc_logger.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "isobus/hardware_integration/vector_asc_logger.hpp"

#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <cmath>
#include <ctime>
#include <iomanip>

namespace isobus
{
	VectorASCLogger::VectorASCLogger()
	{
		std::time_t currentTime = std::time(0);
		std::tm *now = std::localtime(&currentTime);
		std::string defaultFileName = "AgISOStackLog_" +
		  isobus::to_string(now->tm_year + 1900) +
		  "_" +
		  isobus::to_string(now->tm_mon + 1) +
		  "_" +
		  isobus::to_string(now->tm_mday) +
		  "_" +
		  isobus::to_string(now->tm_hour) +
		  "_" +
		  isobus::to_string(now->tm_min) +
		  "_" +
		  isobus::to_string(now->tm_sec) +
		  ".asc";
		openFile(defaultFileName, constructHeaderTime(currentTime));
	}

	VectorASCLogger::VectorASCLogger(std::string filename)
	{
		std::time_t currentTime = std::time(0);

		openFile(filename, constructHeaderTime(currentTime));
	}

	VectorASCLogger::~VectorASCLogger()
	{
		if (logFileStream)
		{
			isobus::CANHardwareInterface::get_can_frame_received_event_dispatcher().remove_listener(canFrameReceivedListener);
			isobus::CANHardwareInterface::get_can_frame_received_event_dispatcher().remove_listener(canFrameSentListener);
			logFileStream.close();
		}
	}

	void VectorASCLogger::openFile(const std::string &filePath, const std::string &headerTime)
	{
		initialTimestamp = SystemTiming::get_timestamp_ms();

		logFileStream.open(filePath, std::ios::out | std::ios::trunc);

		// Write vector ascii header
		if (logFileStream)
		{
			logFileStream << "date " << headerTime << "\n";
			logFileStream << "base hex timestamps absolute\n";
			logFileStream << "no internal events logged\n";
			canFrameReceivedListener = isobus::CANHardwareInterface::get_can_frame_received_event_dispatcher().add_listener([this](const isobus::CANMessageFrame &canFrame) {
				logFileStream << "   ";
				auto currentTime = SystemTiming::get_timestamp_ms() - initialTimestamp;
				auto milliseconds = isobus::to_string(currentTime % 1000);

				while (milliseconds.length() < 3)
				{
					milliseconds = "0" + milliseconds;
				}

				logFileStream << isobus::to_string(std::floor(currentTime / 1000)) +
				    "." +
				    milliseconds +
				    "000 1  ";
				logFileStream << std::hex << std::uppercase
				              << std::setw(8) << std::setfill('0') << canFrame.identifier << "x       Rx   d " + isobus::to_string(static_cast<int>(canFrame.dataLength)) + " ";

				for (std::uint_fast8_t i = 0; i < canFrame.dataLength; i++)
				{
					logFileStream << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(canFrame.data[i]) << " ";
				}
				logFileStream << "\n";
			});

			canFrameSentListener = isobus::CANHardwareInterface::get_can_frame_transmitted_event_dispatcher().add_listener([this](const isobus::CANMessageFrame &canFrame) {
				logFileStream << "   ";
				auto currentTime = SystemTiming::get_timestamp_ms() - initialTimestamp;
				auto milliseconds = isobus::to_string(currentTime % 1000);

				while (milliseconds.length() < 3)
				{
					milliseconds = "0" + milliseconds;
				}

				logFileStream << isobus::to_string(std::floor(currentTime / 1000)) +
				    "." +
				    milliseconds +
				    "000 1  ";
				logFileStream << std::hex << std::uppercase
				              << std::setw(8) << std::setfill('0') << canFrame.identifier << "x       Tx   d " + isobus::to_string(static_cast<int>(canFrame.dataLength)) + " ";

				for (std::uint_fast8_t i = 0; i < canFrame.dataLength; i++)
				{
					logFileStream << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(canFrame.data[i]) << " ";
				}
				logFileStream << "\n";
			});
		}
		else
		{
			LOG_ERROR("[ASC Logger]: Failed to open log file for writing");
		}
	}

	std::string VectorASCLogger::constructHeaderTime(const std::time_t &currentTime)
	{
		const std::string MONTH_ABBREVIATIONS[] = {
			"Jan",
			"Feb",
			"Mar",
			"Apr",
			"May",
			"Jun",
			"Jul",
			"Aug",
			"Sep",
			"Oct",
			"Nov",
			"Dec"
		};
		const std::string DAYS_OF_THE_WEEK_ABBREVIATED[] = {
			"Mon",
			"Tue",
			"Wed",
			"Thu",
			"Fri",
			"Sat",
			"Sun"
		};
		std ::tm *now = std::localtime(&currentTime);
		std::string amOrPM = now->tm_hour > 11 ? std::string("pm") : std::string("am");
		std::string retVal = "date " +
		  DAYS_OF_THE_WEEK_ABBREVIATED[now->tm_wday] +
		  " " +
		  MONTH_ABBREVIATIONS[now->tm_mon] +
		  " " +
		  isobus::to_string(now->tm_mday) +
		  " " +
		  isobus::to_string(now->tm_hour) +
		  ":" +
		  isobus::to_string(now->tm_min) +
		  ":" +
		  isobus::to_string(now->tm_sec) +
		  " " +
		  amOrPM +
		  " " +
		  isobus::to_string(now->tm_year + 1900);
		return retVal;
	}
}
