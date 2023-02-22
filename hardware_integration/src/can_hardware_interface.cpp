//================================================================================================
/// @file can_hardware_interface.cpp
///
/// @brief An interface for using socket CAN on linux. Mostly for testing, but it could be
/// used in any application to get the stack hooked up to the bus.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>

std::unique_ptr<std::thread> CANHardwareInterface::canThread;
std::unique_ptr<std::thread> CANHardwareInterface::periodicUpdateThread;
std::condition_variable CANHardwareInterface::threadConditionVariable;
std::vector<std::unique_ptr<CANHardwareInterface::CANHardware>> CANHardwareInterface::hardwareChannels;
std::vector<CANHardwareInterface::RawCanMessageCallbackInfo> CANHardwareInterface::rxCallbacks;
std::vector<CANHardwareInterface::CanLibUpdateCallbackInfo> CANHardwareInterface::periodicUpdateCallbacks;
std::mutex CANHardwareInterface::hardwareChannelsMutex;
std::mutex CANHardwareInterface::threadMutex;
std::mutex CANHardwareInterface::rxCallbacksMutex;
std::mutex CANHardwareInterface::periodicUpdateCallbacksMutex;
std::atomic_bool CANHardwareInterface::threadsStarted = { false };
std::atomic_bool CANHardwareInterface::canLibNeedsUpdate = { false };
std::uint32_t CANHardwareInterface::canLibUpdatePeriod = PERIODIC_UPDATE_INTERVAL;

CANHardwareInterface CANHardwareInterface::SINGLETON;

CANHardwareInterface::~CANHardwareInterface()
{
	stop_threads();
}

bool isobus::send_can_message_to_hardware(HardwareInterfaceCANFrame frame)
{
	return CANHardwareInterface::transmit_can_message(frame);
}

bool CANHardwareInterface::RawCanMessageCallbackInfo::operator==(const RawCanMessageCallbackInfo &obj) const
{
	return ((obj.callback == this->callback) && (obj.parent == this->parent));
}

bool CANHardwareInterface::CanLibUpdateCallbackInfo::operator==(const CanLibUpdateCallbackInfo &obj) const
{
	return ((obj.callback == this->callback) && (obj.parent == this->parent));
}

bool CANHardwareInterface::set_number_of_can_channels(uint8_t value)
{
	std::lock_guard<std::mutex> lock(hardwareChannelsMutex);

	if (threadsStarted)
	{
		isobus::CANStackLogger::error("[HardwareInterface] Cannot set number of channels after interface is started.");
		return false;
	}

	while (value > hardwareChannels.size())
	{
		hardwareChannels.push_back(std::make_unique<CANHardware>());
		hardwareChannels.back()->receiveMessageThread = nullptr;
		hardwareChannels.back()->frameHandler = nullptr;
	}
	while (value < hardwareChannels.size())
	{
		hardwareChannels.pop_back();
	}
	return true;
}

bool CANHardwareInterface::assign_can_channel_frame_handler(std::uint8_t channelIndex, std::shared_ptr<CANHardwarePlugin> driver)
{
	std::lock_guard<std::mutex> lock(hardwareChannelsMutex);

	if (threadsStarted)
	{
		isobus::CANStackLogger::error("[HardwareInterface] Cannot assign frame handlers after interface is started.");
		return false;
	}

	if (channelIndex >= hardwareChannels.size())
	{
		isobus::CANStackLogger::error("[HardwareInterface] Unable to set frame handler at channel " + isobus::to_string(channelIndex) +
		                              ", because there are only " + isobus::to_string(hardwareChannels.size()) + " channels set. " +
		                              "Use set_number_of_can_channels() to increase the number of channels before assigning frame handlers.");
		return false;
	}

	if (nullptr != hardwareChannels[channelIndex]->frameHandler)
	{
		isobus::CANStackLogger::error("[HardwareInterface] Unable to set frame handler at channel " + isobus::to_string(channelIndex) + ", because it is already assigned.");
		return false;
	}

	hardwareChannels[channelIndex]->frameHandler = driver;
	return true;
}

std::uint8_t CANHardwareInterface::get_number_of_can_channels()
{
	return static_cast<std::uint8_t>(hardwareChannels.size() & std::numeric_limits<std::uint8_t>::max());
}

bool CANHardwareInterface::unassign_can_channel_frame_handler(std::uint8_t channelIndex)
{
	std::lock_guard<std::mutex> lock(hardwareChannelsMutex);

	if (threadsStarted)
	{
		isobus::CANStackLogger::error("[HardwareInterface] Cannot remove frame handlers after interface is started.");
		return false;
	}

	if (channelIndex >= hardwareChannels.size())
	{
		isobus::CANStackLogger::error("[HardwareInterface] Unable to remove frame handler at channel " + isobus::to_string(channelIndex) +
		                              ", because there are only " + isobus::to_string(hardwareChannels.size()) + " channels set.");
		return false;
	}

	if (nullptr == hardwareChannels[channelIndex]->frameHandler)
	{
		isobus::CANStackLogger::error("[HardwareInterface] Unable to remove frame handler at channel " + isobus::to_string(channelIndex) + ", because it is not assigned.");
		return false;
	}

	hardwareChannels[channelIndex]->frameHandler = nullptr;
	return true;
}

bool CANHardwareInterface::start()
{
	std::lock_guard<std::mutex> lock(hardwareChannelsMutex);

	if (threadsStarted)
	{
		isobus::CANStackLogger::error("[HardwareInterface] Cannot start interface more than once.");
		return false;
	}

	canThread = std::make_unique<std::thread>(can_thread_function);
	periodicUpdateThread = std::make_unique<std::thread>(update_can_lib_periodic_function);

	threadsStarted = true;

	for (std::size_t i = 0; i < hardwareChannels.size(); i++)
	{
		if (nullptr != hardwareChannels[i]->frameHandler)
		{
			hardwareChannels[i]->frameHandler->open();

			if (hardwareChannels[i]->frameHandler->get_is_valid())
			{
				hardwareChannels[i]->receiveMessageThread = std::make_unique<std::thread>(receive_message_thread_function, static_cast<std::uint8_t>(i));
			}
		}
	}

	return true;
}

bool CANHardwareInterface::stop()
{
	if (!threadsStarted)
	{
		isobus::CANStackLogger::error("[HardwareInterface] Cannot stop interface before it is started.");
		return false;
	}
	stop_threads();

	std::unique_lock<std::mutex> channelsLock(hardwareChannelsMutex);
	std::for_each(hardwareChannels.begin(), hardwareChannels.end(), [](const std::unique_ptr<CANHardware> &channel) {
		if (nullptr != channel->frameHandler)
		{
			channel->frameHandler = nullptr;
		}
		std::unique_lock<std::mutex> transmittingLock(channel->messagesToBeTransmittedMutex);
		channel->messagesToBeTransmitted.clear();
		transmittingLock.unlock();

		std::unique_lock<std::mutex> receivingLock(channel->receivedMessagesMutex);
		channel->receivedMessages.clear();
		receivingLock.unlock();
	});
	channelsLock.unlock();

	std::unique_lock<std::mutex> rxCallbackLock(rxCallbacksMutex);
	rxCallbacks.clear();
	rxCallbackLock.unlock();

	std::unique_lock<std::mutex> periodicUpdateCallbackLock(periodicUpdateCallbacksMutex);
	periodicUpdateCallbacks.clear();
	periodicUpdateCallbackLock.unlock();
	return true;
}

bool CANHardwareInterface::transmit_can_message(const isobus::HardwareInterfaceCANFrame &packet)
{
	if (!threadsStarted)
	{
		isobus::CANStackLogger::error("[HardwareInterface] Cannot transmit message before interface is started.");
		return false;
	}

	if (packet.channel >= hardwareChannels.size())
	{
		isobus::CANStackLogger::error("[HardwareInterface] Cannot transmit message on channel " + isobus::to_string(packet.channel) +
		                              ", because there are only " + isobus::to_string(hardwareChannels.size()) + " channels set.");
		return false;
	}

	const std::unique_ptr<CANHardware> &channel = hardwareChannels[packet.channel];
	if (nullptr == channel->frameHandler)
	{
		isobus::CANStackLogger::error("[HardwareInterface] Cannot transmit message on channel " + isobus::to_string(packet.channel) + ", because it is not assigned.");
		return false;
	}

	if (channel->frameHandler->get_is_valid())
	{
		std::lock_guard<std::mutex> lock(channel->messagesToBeTransmittedMutex);
		channel->messagesToBeTransmitted.push_back(packet);

		threadConditionVariable.notify_all();
		return true;
	}
	return false;
}

bool CANHardwareInterface::add_raw_can_message_rx_callback(void (*callback)(isobus::HardwareInterfaceCANFrame &rxFrame, void *parentPointer), void *parent)
{
	std::lock_guard<std::mutex> lock(rxCallbacksMutex);

	RawCanMessageCallbackInfo callbackInfo;
	callbackInfo.callback = callback;
	callbackInfo.parent = parent;

	if ((nullptr != callback) && (rxCallbacks.end() == std::find(rxCallbacks.begin(), rxCallbacks.end(), callbackInfo)))
	{
		rxCallbacks.push_back(callbackInfo);
		return true;
	}
	return false;
}

bool CANHardwareInterface::remove_raw_can_message_rx_callback(void (*callback)(isobus::HardwareInterfaceCANFrame &rxFrame, void *parentPointer), void *parent)
{
	std::lock_guard<std::mutex> lock(rxCallbacksMutex);

	RawCanMessageCallbackInfo callbackInfo;
	callbackInfo.callback = callback;
	callbackInfo.parent = parent;

	if (nullptr != callback)
	{
		auto callbackLocation = std::find(rxCallbacks.begin(), rxCallbacks.end(), callbackInfo);

		if (rxCallbacks.end() != callbackLocation)
		{
			rxCallbacks.erase(callbackLocation);
			return true;
		}
	}

	return false;
}

void CANHardwareInterface::set_can_driver_update_period(std::uint32_t value)
{
	canLibUpdatePeriod = value;
}

bool CANHardwareInterface::add_can_lib_update_callback(void (*callback)(void *parentPointer), void *parentPointer)
{
	std::lock_guard<std::mutex> lock(periodicUpdateCallbacksMutex);

	CanLibUpdateCallbackInfo callbackInfo;
	callbackInfo.callback = callback;
	callbackInfo.parent = parentPointer;

	if ((nullptr != callback) && (periodicUpdateCallbacks.end() == std::find(periodicUpdateCallbacks.begin(), periodicUpdateCallbacks.end(), callbackInfo)))
	{
		periodicUpdateCallbacks.push_back(callbackInfo);
		return true;
	}

	return false;
}

bool CANHardwareInterface::remove_can_lib_update_callback(void (*callback)(void *parentPointer), void *parentPointer)
{
	std::lock_guard<std::mutex> lock(periodicUpdateCallbacksMutex);

	CanLibUpdateCallbackInfo callbackInfo;
	callbackInfo.callback = callback;
	callbackInfo.parent = parentPointer;

	if (nullptr != callback)
	{
		auto callbackLocation = std::find(periodicUpdateCallbacks.begin(), periodicUpdateCallbacks.end(), callbackInfo);

		if (periodicUpdateCallbacks.end() != callbackLocation)
		{
			periodicUpdateCallbacks.erase(callbackLocation);
			return true;
		}
	}
	return false;
}

void CANHardwareInterface::can_thread_function()
{
	hardwareChannelsMutex.lock();
	// Wait until everything is running
	hardwareChannelsMutex.unlock();

	while (threadsStarted)
	{
		std::unique_lock<std::mutex> threadLock(threadMutex);
		threadConditionVariable.wait_for(threadLock, std::chrono::seconds(1)); // Timeout after 1 second

		if (threadsStarted)
		{
			for (std::size_t i = 0; i < hardwareChannels.size(); i++)
			{
				hardwareChannels[i]->receivedMessagesMutex.lock();
				bool processNextMessage = (!hardwareChannels[i]->receivedMessages.empty());
				hardwareChannels[i]->receivedMessagesMutex.unlock();

				while (processNextMessage)
				{
					isobus::HardwareInterfaceCANFrame tempCanFrame;

					hardwareChannels[i]->receivedMessagesMutex.lock();
					tempCanFrame = hardwareChannels[i]->receivedMessages.front();
					hardwareChannels[i]->receivedMessages.pop_front();
					processNextMessage = (!hardwareChannels[i]->receivedMessages.empty());
					hardwareChannels[i]->receivedMessagesMutex.unlock();

					rxCallbacksMutex.lock();
					for (std::size_t j = 0; j < rxCallbacks.size(); j++)
					{
						if (nullptr != rxCallbacks[j].callback)
						{
							rxCallbacks[j].callback(tempCanFrame, rxCallbacks[j].parent);
						}
					}
					rxCallbacksMutex.unlock();
				}
			}

			if (canLibNeedsUpdate)
			{
				canLibNeedsUpdate = false;
				periodicUpdateCallbacksMutex.lock();
				for (std::size_t j = 0; j < periodicUpdateCallbacks.size(); j++)
				{
					if (nullptr != periodicUpdateCallbacks[j].callback)
					{
						periodicUpdateCallbacks[j].callback(periodicUpdateCallbacks[j].parent);
					}
				}
				periodicUpdateCallbacksMutex.unlock();
			}

			for (std::size_t i = 0; i < hardwareChannels.size(); i++)
			{
				hardwareChannels[i]->messagesToBeTransmittedMutex.lock();
				isobus::HardwareInterfaceCANFrame packet;
				bool sendPacket = false;

				for (std::size_t j = 0; j < hardwareChannels[i]->messagesToBeTransmitted.size(); j++)
				{
					sendPacket = false;

					if (0 != hardwareChannels[i]->messagesToBeTransmitted.size())
					{
						packet = hardwareChannels[i]->messagesToBeTransmitted.front();
						sendPacket = true;
					}

					if (sendPacket)
					{
						if (transmit_can_message_from_buffer(packet))
						{
							hardwareChannels[i]->messagesToBeTransmitted.pop_front();
						}
						else
						{
							break;
						}
						// Todo, notify CAN lib that we sent, or did not send, each packet
					}
				}
				hardwareChannels[i]->messagesToBeTransmittedMutex.unlock();
			}
		}
	}
}

void CANHardwareInterface::receive_message_thread_function(std::uint8_t channelIndex)
{
	isobus::HardwareInterfaceCANFrame tempCanFrame;

	hardwareChannelsMutex.lock();
	// Wait until everything is running
	hardwareChannelsMutex.unlock();

	if (channelIndex < hardwareChannels.size())
	{
		while ((threadsStarted) &&
		       (nullptr != hardwareChannels[channelIndex]->frameHandler))
		{
			if (hardwareChannels[channelIndex]->frameHandler->get_is_valid())
			{
				// Socket or other hardware still open
				if (hardwareChannels[channelIndex]->frameHandler->read_frame(tempCanFrame))
				{
					tempCanFrame.channel = channelIndex;
					hardwareChannels[channelIndex]->receivedMessagesMutex.lock();
					hardwareChannels[channelIndex]->receivedMessages.push_back(tempCanFrame);
					hardwareChannels[channelIndex]->receivedMessagesMutex.unlock();
					threadConditionVariable.notify_all();
				}
			}
			else
			{
				isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Critical, "[CAN Rx Thread]: CAN Channel " + isobus::to_string(channelIndex) + " appears to be invalid.");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Arbitrary, but don't want to infinite loop on the validity check.
			}
		}
	}
}

bool CANHardwareInterface::transmit_can_message_from_buffer(isobus::HardwareInterfaceCANFrame &packet)
{
	bool retVal = false;
	std::uint8_t lChannel = packet.channel;

	if (lChannel < hardwareChannels.size())
	{
		retVal = ((nullptr != hardwareChannels[lChannel]->frameHandler) &&
		          (hardwareChannels[lChannel]->frameHandler->write_frame(packet)));
	}
	return retVal;
}

void CANHardwareInterface::update_can_lib_periodic_function()
{
	hardwareChannelsMutex.lock();
	// Wait until everything is running
	hardwareChannelsMutex.unlock();

	while (threadsStarted)
	{
		canLibNeedsUpdate = true;
		threadConditionVariable.notify_all();
		std::this_thread::sleep_for(std::chrono::milliseconds(canLibUpdatePeriod));
	}
}

void CANHardwareInterface::stop_threads()
{
	threadsStarted = false;
	if (nullptr != canThread)
	{
		if (canThread->joinable())
		{
			threadConditionVariable.notify_all();
			canThread->join();
		}
		canThread = nullptr;
	}

	if (nullptr != periodicUpdateThread)
	{
		if (periodicUpdateThread->joinable())
		{
			periodicUpdateThread->join();
		}
		periodicUpdateThread = nullptr;
	}

	std::for_each(hardwareChannels.begin(), hardwareChannels.end(), [](const std::unique_ptr<CANHardware> &channel) {
		if (nullptr != channel->frameHandler)
		{
			channel->frameHandler->close();
		}
		if (nullptr != channel->receiveMessageThread)
		{
			if (channel->receiveMessageThread->joinable())
			{
				channel->receiveMessageThread->join();
			}
			channel->receiveMessageThread = nullptr;
		}
	});
}