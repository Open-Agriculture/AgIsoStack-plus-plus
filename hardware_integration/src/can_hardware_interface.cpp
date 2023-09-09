//================================================================================================
/// @file can_hardware_interface.cpp
///
/// @brief The hardware abstraction layer that separates the stack from the underlying CAN driver
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>

namespace isobus
{
	std::unique_ptr<std::thread> CANHardwareInterface::updateThread;
	std::unique_ptr<std::thread> CANHardwareInterface::wakeupThread;
	std::condition_variable CANHardwareInterface::updateThreadWakeupCondition;
	std::atomic_bool CANHardwareInterface::stackNeedsUpdate = { false };
	std::uint32_t CANHardwareInterface::periodicUpdateInterval = PERIODIC_UPDATE_INTERVAL;

	isobus::EventDispatcher<const isobus::CANMessageFrame &> CANHardwareInterface::frameReceivedEventDispatcher;
	isobus::EventDispatcher<const isobus::CANMessageFrame &> CANHardwareInterface::frameTransmittedEventDispatcher;
	isobus::EventDispatcher<> CANHardwareInterface::periodicUpdateEventDispatcher;

	std::vector<std::unique_ptr<CANHardwareInterface::CANHardware>> CANHardwareInterface::hardwareChannels;
	std::mutex CANHardwareInterface::hardwareChannelsMutex;
	std::mutex CANHardwareInterface::updateMutex;
	std::atomic_bool CANHardwareInterface::threadsStarted = { false };

	CANHardwareInterface CANHardwareInterface::SINGLETON;

	CANHardwareInterface::~CANHardwareInterface()
	{
		stop_threads();
	}

	bool send_can_message_frame_to_hardware(const CANMessageFrame &frame)
	{
		return CANHardwareInterface::transmit_can_frame(frame);
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

		updateThread = std::make_unique<std::thread>(update_thread_function);
		wakeupThread = std::make_unique<std::thread>(periodic_update_function);

		threadsStarted = true;

		for (std::size_t i = 0; i < hardwareChannels.size(); i++)
		{
			if (nullptr != hardwareChannels[i]->frameHandler)
			{
				hardwareChannels[i]->frameHandler->open();

				if (hardwareChannels[i]->frameHandler->get_is_valid())
				{
					hardwareChannels[i]->receiveMessageThread = std::make_unique<std::thread>(receive_can_frame_thread_function, static_cast<std::uint8_t>(i));
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

		std::lock_guard<std::mutex> channelsLock(hardwareChannelsMutex);
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
		return true;
	}
	bool CANHardwareInterface::is_running()
	{
		return threadsStarted;
	}

	bool CANHardwareInterface::transmit_can_frame(const isobus::CANMessageFrame &frame)
	{
		if (!threadsStarted)
		{
			isobus::CANStackLogger::error("[HardwareInterface] Cannot transmit message before interface is started.");
			return false;
		}

		if (frame.channel >= hardwareChannels.size())
		{
			isobus::CANStackLogger::error("[HardwareInterface] Cannot transmit message on channel " + isobus::to_string(frame.channel) +
			                              ", because there are only " + isobus::to_string(hardwareChannels.size()) + " channels set.");
			return false;
		}

		const std::unique_ptr<CANHardware> &channel = hardwareChannels[frame.channel];
		if (nullptr == channel->frameHandler)
		{
			isobus::CANStackLogger::error("[HardwareInterface] Cannot transmit message on channel " + isobus::to_string(frame.channel) + ", because it is not assigned.");
			return false;
		}

		if (channel->frameHandler->get_is_valid())
		{
			std::lock_guard<std::mutex> lock(channel->messagesToBeTransmittedMutex);
			channel->messagesToBeTransmitted.push_back(frame);

			updateThreadWakeupCondition.notify_all();
			return true;
		}
		return false;
	}

	isobus::EventDispatcher<const isobus::CANMessageFrame &> &CANHardwareInterface::get_can_frame_received_event_dispatcher()
	{
		return frameReceivedEventDispatcher;
	}

	isobus::EventDispatcher<const isobus::CANMessageFrame &> &CANHardwareInterface::get_can_frame_transmitted_event_dispatcher()
	{
		return frameTransmittedEventDispatcher;
	}

	isobus::EventDispatcher<> &CANHardwareInterface::get_periodic_update_event_dispatcher()
	{
		return periodicUpdateEventDispatcher;
	}

	void CANHardwareInterface::set_periodic_update_interval(std::uint32_t value)
	{
		periodicUpdateInterval = value;
	}

	std::uint32_t CANHardwareInterface::get_periodic_update_interval()
	{
		return periodicUpdateInterval;
	}

	void CANHardwareInterface::update_thread_function()
	{
		std::unique_lock<std::mutex> channelsLock(hardwareChannelsMutex);
		// Wait until everything is running
		channelsLock.unlock();

		while (threadsStarted)
		{
			std::unique_lock<std::mutex> threadLock(updateMutex);
			updateThreadWakeupCondition.wait_for(threadLock, std::chrono::seconds(1)); // Timeout after 1 second

			if (threadsStarted)
			{
				// Stage 1 - Receiving messages from hardware
				channelsLock.lock();
				std::for_each(hardwareChannels.begin(), hardwareChannels.end(), [](const std::unique_ptr<CANHardware> &channel) {
					std::lock_guard<std::mutex> lock(channel->receivedMessagesMutex);
					while (!channel->receivedMessages.empty())
					{
						const auto &frame = channel->receivedMessages.front();

						frameReceivedEventDispatcher.invoke(frame);
						isobus::receive_can_message_frame_from_hardware(frame);

						channel->receivedMessages.pop_front();
					}
				});
				channelsLock.unlock();

				// Stage 2 - Sending messages
				if (stackNeedsUpdate)
				{
					stackNeedsUpdate = false;
					periodicUpdateEventDispatcher.invoke();
					isobus::periodic_update_from_hardware();
				}

				// Stage 3 - Transmitting messages to hardware
				channelsLock.lock();
				std::for_each(hardwareChannels.begin(), hardwareChannels.end(), [](const std::unique_ptr<CANHardware> &channel) {
					std::lock_guard<std::mutex> lock(channel->messagesToBeTransmittedMutex);
					while (!channel->messagesToBeTransmitted.empty())
					{
						const auto &frame = channel->messagesToBeTransmitted.front();

						if (transmit_can_frame_from_buffer(frame))
						{
							frameTransmittedEventDispatcher.invoke(frame);
							isobus::on_transmit_can_message_frame_from_hardware(frame);
							channel->messagesToBeTransmitted.pop_front();
						}
						else
						{
							break;
						}
					}
				});
				channelsLock.unlock();
			}
		}
	}

	void CANHardwareInterface::receive_can_frame_thread_function(std::uint8_t channelIndex)
	{
		std::unique_lock<std::mutex> channelsLock(hardwareChannelsMutex);
		// Wait until everything is running
		channelsLock.unlock();

		isobus::CANMessageFrame frame;
		while ((threadsStarted) &&
		       (nullptr != hardwareChannels[channelIndex]->frameHandler))
		{
			if (hardwareChannels[channelIndex]->frameHandler->get_is_valid())
			{
				// Socket or other hardware still open
				if (hardwareChannels[channelIndex]->frameHandler->read_frame(frame))
				{
					frame.channel = channelIndex;
					std::unique_lock<std::mutex> receiveLock(hardwareChannels[channelIndex]->receivedMessagesMutex);
					hardwareChannels[channelIndex]->receivedMessages.push_back(frame);
					receiveLock.unlock();
					updateThreadWakeupCondition.notify_all();
				}
			}
			else
			{
				isobus::CANStackLogger::CAN_stack_log(isobus::CANStackLogger::LoggingLevel::Critical, "[CAN Rx Thread]: CAN Channel " + isobus::to_string(channelIndex) + " appears to be invalid.");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Arbitrary, but don't want to infinite loop on the validity check.
			}
		}
	}

	bool CANHardwareInterface::transmit_can_frame_from_buffer(const isobus::CANMessageFrame &frame)
	{
		bool retVal = false;
		if (frame.channel < hardwareChannels.size())
		{
			retVal = ((nullptr != hardwareChannels[frame.channel]->frameHandler) &&
			          (hardwareChannels[frame.channel]->frameHandler->write_frame(frame)));
		}
		return retVal;
	}

	void CANHardwareInterface::periodic_update_function()
	{
		std::unique_lock<std::mutex> channelsLock(hardwareChannelsMutex);
		// Wait until everything is running
		channelsLock.unlock();

		while (threadsStarted)
		{
			stackNeedsUpdate = true;
			updateThreadWakeupCondition.notify_all();
			std::this_thread::sleep_for(std::chrono::milliseconds(periodicUpdateInterval));
		}
	}

	void CANHardwareInterface::stop_threads()
	{
		threadsStarted = false;
		if (nullptr != updateThread)
		{
			if (updateThread->joinable())
			{
				updateThreadWakeupCondition.notify_all();
				updateThread->join();
			}
			updateThread = nullptr;
		}

		if (nullptr != wakeupThread)
		{
			if (wakeupThread->joinable())
			{
				wakeupThread->join();
			}
			wakeupThread = nullptr;
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
}