#include "create_can_driver.hpp"
#include <iostream>

#include "isobus/hardware_integration/available_can_drivers.hpp"

std::shared_ptr<isobus::CANHardwarePlugin> CANDriverFactory::create(const std::string &interfaceName, const std::string &driver)
{
	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;

	std::string driverToOpen = driver;
#if defined(_WIN32)
	if (driver.empty())
	{
#if defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
		driverToOpen = "peak";
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
		driverToOpen = "innomaker";
#elif defined(ISOBUS_SYS_TEC_AVAILABLE)
		driverToOpen = "systec";
#elif defined(ISOBUS_TOUCAN_AVAILABLE)
		driverToOpen = "toucan";
#endif
	}

	// Windows
	if ("peak" == driverToOpen)
	{
#if defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
		const int channel = interfaceName.empty() ? PCAN_USBBUS1 : (std::stoi(interfaceName) - 1 + PCAN_USBBUS1);
		canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(channel);
#endif
	}
	else if ("innomaker" == driverToOpen)
	{
#if defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
		const int channel = interfaceName.empty() ? 0 : std::stoi(interfaceName);
		canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(channel);
#endif
	}
	else if ("systec" == driverToOpen)
	{
#if defined(ISOBUS_SYS_TEC_AVAILABLE)
		canDriver = std::make_shared<isobus::SysTecWindowsPlugin>();
#endif
	}
	else if ("toucan" == driverToOpen)
	{
#if defined(ISOBUS_TOUCAN_AVAILABLE)
		canDriver = std::make_shared<isobus::TouCANPlugin>();
#endif
	}
#elif defined(__APPLE__)
	// OSX
#ifdef ISOBUS_MACCANPCAN_AVAILABLE
	if (driver.empty() || driver == "peak")
	{
		const int channel = interfaceName.empty() ? PCAN_USBBUS1 : (std::stoi(interfaceName) - 1 + PCAN_USBBUS1);
		canDriver = std::make_shared<isobus::MacCANPCANPlugin>(channel);
	}
#endif

#elif defined(__linux__)
	// Linux and BSDs
	if (driver.empty() || driver == "socketcan")
	{
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
		const std::string interfaceNameToOpen = interfaceName.empty() ? "vcan0" : interfaceName;
		canDriver = std::make_shared<isobus::SocketCANInterface>(interfaceNameToOpen);
#endif
	}
#endif

	if (driver == "virtual")
	{
#if defined(ISOBUS_VIRTUALCAN_AVAILABLE)
		canDriver = std::make_shared<isobus::VirtualCANPlugin>(interfaceName);
#endif
	}

	if (nullptr == canDriver)
	{
		if (!driver.empty())
		{
			std::cout << "The '" << driver << "' CAN driver is not supported on your platform." << std::endl;
			std::cout << "This is the list of possible drivers: " << std::endl;
			printAvailableDriverList();
			std::cout.flush();
		}
		else
		{
			std::string driverName = driver;
			if (driverName.empty())
			{
#if defined(_WIN32)
				driverName = "peak";
#elif defined(__APPLE__)
				// OSX
				driverName = "peak";
#else
				// Linux and BSDs
				driverName = "socketcan";
#endif
			}
			std::cout << "Unable to open the " << interfaceName << " with the " << driverName << " CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
			std::cout.flush();
		}
	}
	return canDriver;
}

void CANDriverFactory::printAvailableDriverList()
{
#if defined(_WIN32)
#if defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
	std::cout << " * peak      - driver based on the PEAK system's PCAN library" << std::endl;
#endif

#ifdef ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE
	std::cout << " * innomaker - driver for USB CAN devices manufactured by Innomaker" << std::endl;
#endif

#ifdef ISOBUS_SYS_TEC_AVAILABLE
	std::cout << " * systec    - driver for Systec CAN devices" << std::endl;
#endif

#ifdef ISOBUS_TOUCAN_AVAILABLE
	std::cout << " * toucan    - driver for Rusoku TouCAN device via the VSCP CANAL api" << std::endl;
#endif
#elif defined(__APPLE__)
#ifdef ISOBUS_MACCANPCAN_AVAILABLE
	std::cout << " * peak      - driver based on the MacCAN library" << std::endl;
#endif
#elif defined(__linux__)
#ifdef ISOBUS_SOCKETCAN_AVAILABLE
	std::cout << " * socketcan - SocketCAN driver" << std::endl;
#endif
#endif
#if defined(ISOBUS_VIRTUALCAN_AVAILABLE)
	std::cout << " * virtual   - virtual CAN driver for testing purposes" << std::endl;
#endif
}
