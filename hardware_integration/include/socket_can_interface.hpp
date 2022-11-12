//================================================================================================
/// @file socket_can_interface.hpp
///
/// @brief An interface for using socket CAN on linux. Mostly for testing, but it could be
/// used in any application to get the stack hooked up to the bus.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef SOCKET_CAN_INTERFACE_HPP
#define SOCKET_CAN_INTERFACE_HPP

#include <cstdint>
#include <string>

#include "can_frame.hpp"
#include "can_hardware_abstraction.hpp"
#include "can_hardware_plugin.hpp"

class SocketCANInterface : public CANHardwarePlugin
{
public:
	explicit SocketCANInterface(const std::string deviceName);
	~SocketCANInterface();

	bool get_is_valid() const override;

	std::string get_device_name() const;

	void close() override;
	void open() override;
	bool read_frame(isobus::HardwareInterfaceCANFrame &canFrame) override;
	bool write_frame(const isobus::HardwareInterfaceCANFrame &canFrame) override;

private:
	struct sockaddr_can *pCANDevice;
	const std::string name;
	int fileDescriptor;
};

#endif // SOCKET_CAN_INTERFACE_HPP
