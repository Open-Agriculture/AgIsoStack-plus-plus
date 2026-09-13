#include "create_can_driver.hpp"

#include "isobus/hardware_integration/available_can_drivers.hpp"

std::shared_ptr<isobus::CANHardwarePlugin> CANDriverFactory::create(const std::string &interfaceName)
{
	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;

#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	const std::string interfaceNameToOpen = interfaceName.empty() ? "vcan0" : interfaceName;
	canDriver = std::make_shared<isobus::SocketCANInterface>(interfaceNameToOpen);
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
	const int channel = interfaceName.empty() ? 0 : std::stoi(interfaceName);
	canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(channel);
#elif (defined(ISOBUS_MACCANPCAN_AVAILABLE) || defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE))
	const int channel = interfaceName.empty() ? PCAN_USBBUS1 : (std::stoi(interfaceName) - 1 + PCAN_USBBUS1);

#if defined(ISOBUS_MACCANPCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::MacCANPCANPlugin>(channel);
#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
	canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(channel);
#endif
#elif defined(ISOBUS_SYS_TEC_AVAILABLE)
	canDriver = std::make_shared<isobus::SysTecWindowsPlugin>();
#endif

	return canDriver;
}
