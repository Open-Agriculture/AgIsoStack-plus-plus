//================================================================================================
/// @file can_hardware_interface.hpp
///
/// @brief The hardware abstraction layer that separates the stack from the underlying CAN driver
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef CAN_HARDWARE_INTERFACE_HPP
#define CAN_HARDWARE_INTERFACE_HPP

#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "can_frame.hpp"
#include "can_hardware_abstraction.hpp"
#include "can_hardware_plugin.hpp"

class CANHardwareInterface
{
public:
	class CanLibUpdateCallbackInfo
	{
	public:
		CanLibUpdateCallbackInfo();

		bool operator==(const CanLibUpdateCallbackInfo &obj);

		void (*callback)();
		void *parent;
	};

	class RawCanMessageCallbackInfo
	{
	public:
		RawCanMessageCallbackInfo();

		bool operator==(const RawCanMessageCallbackInfo &obj);

		void (*callback)(isobus::HardwareInterfaceCANFrame &rxFrame, void *parentPointer);
		void *parent;
	};

	static CANHardwareInterface CAN_HARDWARE_INTERFACE;

	static std::uint8_t get_number_of_can_channels();
	static bool set_number_of_can_channels(std::uint8_t value);

	static bool assign_can_channel_frame_handler(std::uint8_t aCANChannel, CANHardwarePlugin *deviceName);

	static bool start();
	static bool stop();

	static bool transmit_can_message(isobus::HardwareInterfaceCANFrame &packet);
	static bool add_raw_can_message_rx_callback(void (*callback)(isobus::HardwareInterfaceCANFrame &rxFrame, void *parentPointer), void *parentPointer);
	static bool remove_raw_can_message_rx_callback(void (*callback)(isobus::HardwareInterfaceCANFrame &rxFrame, void *parentPointer), void *parentPointer);

	static bool add_can_lib_update_callback(void (*callback)(), void *parentPointer);
	static bool remove_can_lib_update_callback(void (*callback)(), void *parentPointer);

private:
	CANHardwareInterface();
	~CANHardwareInterface();

	struct CanHardware
	{
		std::mutex messagesToBeTransmittedMutex;
		std::deque<isobus::HardwareInterfaceCANFrame> messagesToBeTransmitted;

		std::mutex receivedMessagesMutex;
		std::deque<isobus::HardwareInterfaceCANFrame> receivedMessages;

		std::thread *receiveMessageThread;

		CANHardwarePlugin *frameHandler;
	};

	static const std::uint32_t CANLIB_UPDATE_RATE = 4;

	static void can_thread_function();
	static void receive_message_thread_function(std::uint8_t aCANChannel);
	static bool transmit_can_message_from_buffer(isobus::HardwareInterfaceCANFrame &packet);
	static void update_can_lib_periodic_function();
	static void set_can_lib_needs_update();
	static bool get_clear_can_lib_needs_update();

	static std::thread *can_thread;
	static std::thread *updateCANLibPeriodicThread;

	static std::vector<CanHardware *> hardwareChannels;
	static std::vector<RawCanMessageCallbackInfo> rxCallbacks;
	static std::vector<CanLibUpdateCallbackInfo> canLibUpdateCallbacks;

	static std::mutex hardwareChannelsMutex;
	static std::mutex threadMutex;
	static std::mutex rxCallbackMutex;
	static std::mutex canLibNeedsUpdateMutex;
	static std::mutex canLibUpdateCallbacksMutex;
	static std::condition_variable threadConditionVariable;
	static bool threadsStarted;
	static bool canLibNeedsUpdate;
};

#endif // CAN_HARDWARE_INTERFACE_HPP
