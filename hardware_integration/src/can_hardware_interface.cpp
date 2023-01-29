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

std::thread *CANHardwareInterface::can_thread = nullptr;
std::thread *CANHardwareInterface::updateCANLibPeriodicThread = nullptr;
std::condition_variable CANHardwareInterface::threadConditionVariable;
std::vector<CANHardwareInterface::CanHardware *> CANHardwareInterface::hardwareChannels;
std::vector<CANHardwareInterface::RawCanMessageCallbackInfo> CANHardwareInterface::rxCallbacks;
std::vector<CANHardwareInterface::CanLibUpdateCallbackInfo> CANHardwareInterface::canLibUpdateCallbacks;
std::mutex CANHardwareInterface::hardwareChannelsMutex;
std::mutex CANHardwareInterface::threadMutex;
std::mutex CANHardwareInterface::rxCallbackMutex;
std::mutex CANHardwareInterface::canLibNeedsUpdateMutex;
std::mutex CANHardwareInterface::canLibUpdateCallbacksMutex;
bool CANHardwareInterface::threadsStarted = false;
bool CANHardwareInterface::canLibNeedsUpdate = false;
std::uint32_t CANHardwareInterface::canLibUpdatePeriod = CANLIB_UPDATE_RATE;
CANHardwareInterface CANHardwareInterface::CAN_HARDWARE_INTERFACE;

bool isobus::send_can_message_to_hardware(HardwareInterfaceCANFrame frame)
{
	return CANHardwareInterface::transmit_can_message(frame);
}

CANHardwareInterface::RawCanMessageCallbackInfo::RawCanMessageCallbackInfo() :
  callback(nullptr),
  parent(nullptr)
{
}

bool CANHardwareInterface::RawCanMessageCallbackInfo::operator==(const RawCanMessageCallbackInfo &obj)
{
	return ((obj.callback == this->callback) && (obj.parent == this->parent));
}

CANHardwareInterface::CanLibUpdateCallbackInfo::CanLibUpdateCallbackInfo() :
  callback(nullptr),
  parent(nullptr)
{
}

bool CANHardwareInterface::CanLibUpdateCallbackInfo::operator==(const CanLibUpdateCallbackInfo &obj)
{
	return ((obj.callback == this->callback) && (obj.parent == this->parent));
}

CANHardwareInterface::CANHardwareInterface()
{
}

CANHardwareInterface::~CANHardwareInterface()
{
	set_number_of_can_channels(0);
}

bool CANHardwareInterface::assign_can_channel_frame_handler(std::uint8_t aCANChannel, std::shared_ptr<CANHardwarePlugin> driver)
{
	bool retVal = false;

	if (hardwareChannelsMutex.try_lock())
	{
		if ((!threadsStarted) &&
		    (aCANChannel < hardwareChannels.size()))
		{
			if ((nullptr == hardwareChannels[aCANChannel]->frameHandler) ||
			    (driver == hardwareChannels[aCANChannel]->frameHandler))
			{
				retVal = true;
				hardwareChannels[aCANChannel]->frameHandler = driver;
			}
		}
		hardwareChannelsMutex.unlock();
	}
	return retVal;
}

uint8_t CANHardwareInterface::get_number_of_can_channels()
{
	return static_cast<uint8_t>(hardwareChannels.size() & std::numeric_limits<std::uint8_t>::max());
}

bool CANHardwareInterface::set_number_of_can_channels(uint8_t aValue)
{
	CanHardware *pCANHardware;
	bool retVal = false;

	if (hardwareChannelsMutex.try_lock())
	{
		if (!threadsStarted)
		{
			while (aValue > hardwareChannels.size())
			{
				pCANHardware = new CanHardware();
				pCANHardware->receiveMessageThread = nullptr;
				pCANHardware->frameHandler = nullptr;

				hardwareChannels.push_back(pCANHardware);
			}

			while (aValue < hardwareChannels.size())
			{
				pCANHardware = hardwareChannels.back();
				hardwareChannels.pop_back();
				delete pCANHardware;
			}
			retVal = true;
		}
		hardwareChannelsMutex.unlock();
	}
	return retVal;
}

bool CANHardwareInterface::start()
{
	bool retVal = false;

	if (hardwareChannelsMutex.try_lock())
	{
		if (!threadsStarted)
		{
			threadsStarted = true;
			retVal = true;
			can_thread = new std::thread(can_thread_function);
			updateCANLibPeriodicThread = new std::thread(update_can_lib_periodic_function);

			for (std::uint32_t i = 0; i < hardwareChannels.size(); i++)
			{
				if (nullptr != hardwareChannels[i]->frameHandler)
				{
					hardwareChannels[i]->frameHandler->open();

					if (hardwareChannels[i]->frameHandler->get_is_valid())
					{
						hardwareChannels[i]->receiveMessageThread = new std::thread(receive_message_thread_function, i);
					}
				}
			}
		}
		hardwareChannelsMutex.unlock();
	}
	return retVal;
}

bool CANHardwareInterface::stop()
{
	bool retVal = false;

	if (hardwareChannelsMutex.try_lock())
	{
		if (threadsStarted)
		{
			threadsStarted = false;
			retVal = true;

			if (nullptr != can_thread)
			{
				if (can_thread->joinable())
				{
					hardwareChannelsMutex.unlock();
					threadConditionVariable.notify_all();
					can_thread->join();
					hardwareChannelsMutex.lock();
				}
				delete can_thread;
				can_thread = nullptr;
			}

			if (nullptr != updateCANLibPeriodicThread)
			{
				if (updateCANLibPeriodicThread->joinable())
				{
					updateCANLibPeriodicThread->join();
				}
				delete updateCANLibPeriodicThread;
				updateCANLibPeriodicThread = nullptr;
			}

			for (std::uint32_t i = 0; i < hardwareChannels.size(); i++)
			{
				if (nullptr != hardwareChannels[i]->frameHandler)
				{
					hardwareChannels[i]->frameHandler->close();
				}
			}

			for (std::uint32_t i = 0; i < hardwareChannels.size(); i++)
			{
				if (nullptr != hardwareChannels[i]->receiveMessageThread)
				{
					if (hardwareChannels[i]->receiveMessageThread->joinable())
					{
						hardwareChannels[i]->receiveMessageThread->join();
					}
					delete hardwareChannels[i]->receiveMessageThread;
					hardwareChannels[i]->receiveMessageThread = nullptr;
				}
				hardwareChannels[i]->messagesToBeTransmittedMutex.lock();

				while (0 != hardwareChannels[i]->messagesToBeTransmitted.size())
				{
					hardwareChannels[i]->messagesToBeTransmitted.erase(hardwareChannels[i]->messagesToBeTransmitted.begin());
				}
				hardwareChannels[i]->messagesToBeTransmittedMutex.unlock();

				hardwareChannels[i]->receivedMessagesMutex.lock();
				hardwareChannels[i]->receivedMessages.clear();
				hardwareChannels[i]->receivedMessagesMutex.unlock();
			}
		}
		hardwareChannelsMutex.unlock();
	}

	if (rxCallbackMutex.try_lock())
	{
		for (std::uint32_t i = 0; i < rxCallbacks.size(); i++)
		{
			rxCallbacks.pop_back();
		}
		rxCallbackMutex.unlock();
	}

	if (canLibUpdateCallbacksMutex.try_lock())
	{
		for (std::uint32_t i = 0; i < canLibUpdateCallbacks.size(); i++)
		{
			canLibUpdateCallbacks.pop_back();
		}
		canLibUpdateCallbacksMutex.unlock();
	}
	return retVal;
}

bool CANHardwareInterface::transmit_can_message(isobus::HardwareInterfaceCANFrame &packet)
{
	std::uint8_t lChannel = packet.channel;
	bool retVal = false;

	if ((lChannel < hardwareChannels.size()) &&
	    (threadsStarted) &&
	    (nullptr != hardwareChannels[lChannel]->frameHandler) &&
	    (hardwareChannels[lChannel]->frameHandler->get_is_valid()))
	{
		hardwareChannels[lChannel]->messagesToBeTransmittedMutex.lock();
		hardwareChannels[lChannel]->messagesToBeTransmitted.push_back(packet);
		hardwareChannels[lChannel]->messagesToBeTransmittedMutex.unlock();

		threadConditionVariable.notify_all();

		retVal = true;
	}
	return retVal;
}

bool CANHardwareInterface::add_raw_can_message_rx_callback(void (*callback)(isobus::HardwareInterfaceCANFrame &rxFrame, void *parentPointer), void *parent)
{
	bool retVal = false;
	RawCanMessageCallbackInfo callbackInfo;

	callbackInfo.callback = callback;
	callbackInfo.parent = parent;

	rxCallbackMutex.lock();

	if ((nullptr != callback) && (rxCallbacks.end() == find(rxCallbacks.begin(), rxCallbacks.end(), callbackInfo)))
	{
		rxCallbacks.push_back(callbackInfo);
		retVal = true;
	}

	rxCallbackMutex.unlock();

	return retVal;
}

bool CANHardwareInterface::remove_raw_can_message_rx_callback(void (*callback)(isobus::HardwareInterfaceCANFrame &rxFrame, void *parentPointer), void *parent)
{
	bool retVal = false;
	RawCanMessageCallbackInfo callbackInfo;

	callbackInfo.callback = callback;
	callbackInfo.parent = parent;

	rxCallbackMutex.lock();

	if (nullptr != callback)
	{
		std::vector<RawCanMessageCallbackInfo>::iterator callbackLocation;
		callbackLocation = std::find(rxCallbacks.begin(), rxCallbacks.end(), callbackInfo);

		if (rxCallbacks.end() != callbackLocation)
		{
			rxCallbacks.erase(callbackLocation);
			retVal = true;
		}
	}

	rxCallbackMutex.unlock();

	return retVal;
}
void CANHardwareInterface::set_can_driver_update_period(std::uint32_t value)
{
	canLibUpdatePeriod = value;
}

bool CANHardwareInterface::add_can_lib_update_callback(void (*callback)(), void *parentPointer)
{
	bool retVal = false;
	CanLibUpdateCallbackInfo callbackInfo;

	callbackInfo.callback = callback;
	callbackInfo.parent = parentPointer;

	canLibUpdateCallbacksMutex.lock();

	if ((nullptr != callback) && (canLibUpdateCallbacks.end() == find(canLibUpdateCallbacks.begin(), canLibUpdateCallbacks.end(), callbackInfo)))
	{
		canLibUpdateCallbacks.push_back(callbackInfo);
		retVal = true;
	}

	canLibUpdateCallbacksMutex.unlock();

	return retVal;
}

bool CANHardwareInterface::remove_can_lib_update_callback(void (*callback)(), void *parentPointer)
{
	bool retVal = false;
	CanLibUpdateCallbackInfo callbackInfo;

	callbackInfo.callback = callback;
	callbackInfo.parent = parentPointer;

	canLibUpdateCallbacksMutex.lock();

	if (nullptr != callback)
	{
		std::vector<CanLibUpdateCallbackInfo>::iterator callbackLocation;
		callbackLocation = std::find(canLibUpdateCallbacks.begin(), canLibUpdateCallbacks.end(), callbackInfo);

		if (canLibUpdateCallbacks.end() != callbackLocation)
		{
			canLibUpdateCallbacks.erase(callbackLocation);
			retVal = true;
		}
	}

	canLibUpdateCallbacksMutex.unlock();

	return retVal;
}

void CANHardwareInterface::can_thread_function()
{
	hardwareChannelsMutex.lock();
	// Wait until everything is running
	hardwareChannelsMutex.unlock();

	while (threadsStarted)
	{
		std::unique_lock<std::mutex> lMutex(threadMutex);
		CanHardware *pCANHardware;

		if (threadsStarted)
		{
			threadConditionVariable.wait(lMutex, [] { return true; }); // Always wake up

			for (std::uint32_t i = 0; i < hardwareChannels.size(); i++)
			{
				pCANHardware = hardwareChannels[i];

				pCANHardware->receivedMessagesMutex.lock();
				bool processNextMessage = (!pCANHardware->receivedMessages.empty());
				pCANHardware->receivedMessagesMutex.unlock();

				while (processNextMessage)
				{
					isobus::HardwareInterfaceCANFrame tempCanFrame;

					pCANHardware->receivedMessagesMutex.lock();
					tempCanFrame = pCANHardware->receivedMessages.front();
					pCANHardware->receivedMessages.pop_front();
					processNextMessage = (!pCANHardware->receivedMessages.empty());
					pCANHardware->receivedMessagesMutex.unlock();

					rxCallbackMutex.lock();
					for (std::uint32_t j = 0; j < rxCallbacks.size(); j++)
					{
						if (nullptr != rxCallbacks[j].callback)
						{
							rxCallbacks[j].callback(tempCanFrame, rxCallbacks[j].parent);
						}
					}
					rxCallbackMutex.unlock();
				}
			}

			if (get_clear_can_lib_needs_update())
			{
				canLibUpdateCallbacksMutex.lock();
				for (std::uint32_t j = 0; j < canLibUpdateCallbacks.size(); j++)
				{
					if (nullptr != canLibUpdateCallbacks[j].callback)
					{
						canLibUpdateCallbacks[j].callback();
					}
				}
				canLibUpdateCallbacksMutex.unlock();
			}

			for (std::uint32_t i = 0; i < hardwareChannels.size(); i++)
			{
				pCANHardware = hardwareChannels[i];
				pCANHardware->messagesToBeTransmittedMutex.lock();
				isobus::HardwareInterfaceCANFrame packet;
				bool sendPacket = false;

				for (std::uint32_t j = 0; j < pCANHardware->messagesToBeTransmitted.size(); j++)
				{
					sendPacket = false;

					if (0 != pCANHardware->messagesToBeTransmitted.size())
					{
						packet = pCANHardware->messagesToBeTransmitted.front();
						sendPacket = true;
					}

					if (sendPacket)
					{
						if (transmit_can_message_from_buffer(packet))
						{
							pCANHardware->messagesToBeTransmitted.pop_front();
						}
						else
						{
							break;
						}
						// Todo, notify CAN lib that we sent, or did not send, each packet
					}
				}
				pCANHardware->messagesToBeTransmittedMutex.unlock();
			}
		}
	}
}

void CANHardwareInterface::receive_message_thread_function(uint8_t aCANChannel)
{
	CanHardware *pCANHardware;
	isobus::HardwareInterfaceCANFrame tempCanFrame;

	hardwareChannelsMutex.lock();
	hardwareChannelsMutex.unlock();

	if (aCANChannel < hardwareChannels.size())
	{
		pCANHardware = hardwareChannels[aCANChannel];

		while ((threadsStarted) &&
		       (nullptr != pCANHardware->frameHandler))
		{
			if (pCANHardware->frameHandler->get_is_valid())
			{
				// Socket or other hardware still open
				if (pCANHardware->frameHandler->read_frame(tempCanFrame))
				{
					tempCanFrame.channel = aCANChannel;
					pCANHardware->receivedMessagesMutex.lock();
					pCANHardware->receivedMessages.push_back(tempCanFrame);
					pCANHardware->receivedMessagesMutex.unlock();
					threadConditionVariable.notify_all();
				}
			}
			else
			{
				isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Critical, "[CAN Rx Thread]: CAN Channel " + isobus::to_string(aCANChannel) + " appears to be invalid.");
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
	const std::uint32_t UPDATE_RATE = canLibUpdatePeriod;
	hardwareChannelsMutex.lock();
	hardwareChannelsMutex.unlock();

	while (threadsStarted)
	{
		set_can_lib_needs_update();
		threadConditionVariable.notify_all();
		std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_RATE));
	}
}

void CANHardwareInterface::set_can_lib_needs_update()
{
	canLibNeedsUpdateMutex.lock();
	canLibNeedsUpdate = true;
	canLibNeedsUpdateMutex.unlock();
}

bool CANHardwareInterface::get_clear_can_lib_needs_update()
{
	bool retVal = false;

	canLibNeedsUpdateMutex.lock();
	retVal = canLibNeedsUpdate;
	canLibNeedsUpdate = false;
	canLibNeedsUpdateMutex.unlock();

	return retVal;
}
