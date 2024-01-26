//================================================================================================
/// @file can_hardware_interface_single_thread.cpp
///
/// @brief The hardware abstraction layer that separates the stack from the underlying CAN driver
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "isobus/hardware_integration/can_hardware_interface_single_thread.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>
#include <limits>

namespace isobus
{
	isobus::EventDispatcher<const isobus::CANMessageFrame &> CANHardwareInterface::frameReceivedEventDispatcher;
	isobus::EventDispatcher<const isobus::CANMessageFrame &> CANHardwareInterface::frameTransmittedEventDispatcher;

	std::vector<std::unique_ptr<CANHardwareInterface::CANHardware>> CANHardwareInterface::hardwareChannels;
	bool CANHardwareInterface::started = false;

	CANHardwareInterface CANHardwareInterface::SINGLETON;

	bool send_can_message_frame_to_hardware(const CANMessageFrame &frame)
	{
		return CANHardwareInterface::transmit_can_frame(frame);
	}

	bool CANHardwareInterface::set_number_of_can_channels(std::uint8_t value)
	{
		if (started)
		{
			isobus::CANStackLogger::error("[HardwareInterface] Cannot set number of channels after interface is started.");
			return false;
		}

		while (value > hardwareChannels.size())
		{
			hardwareChannels.emplace_back(new CANHardware());
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
		if (started)
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
		if (started)
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
		if (started)
		{
			isobus::CANStackLogger::error("[HardwareInterface] Cannot start interface more than once.");
			return false;
		}

		started = true;

		for (std::size_t i = 0; i < hardwareChannels.size(); i++)
		{
			if (nullptr != hardwareChannels[i]->frameHandler)
			{
				hardwareChannels[i]->frameHandler->open();
			}
		}

		return true;
	}

	bool CANHardwareInterface::stop()
	{
		if (!started)
		{
			isobus::CANStackLogger::error("[HardwareInterface] Cannot stop interface before it is started.");
			return false;
		}

		std::for_each(hardwareChannels.begin(), hardwareChannels.end(), [](const std::unique_ptr<CANHardware> &channel) {
			if (nullptr != channel->frameHandler)
			{
				channel->frameHandler = nullptr;
			}
			channel->messagesToBeTransmitted.clear();
			channel->receivedMessages.clear();
		});
		return true;
	}
	bool CANHardwareInterface::is_running()
	{
		return started;
	}

	bool CANHardwareInterface::transmit_can_frame(const isobus::CANMessageFrame &frame)
	{
		if (!started)
		{
			isobus::CANStackLogger::error("[HardwareInterface] Cannot transmit message before interface is started.");
			return false;
		}

		if (frame.channel >= hardwareChannels.size())
		{
			isobus::CANStackLogger::error("[HardwareInterface] Cannot transmit message on channel %u, because there are only %u channels set.", frame.channel, hardwareChannels.size());
			return false;
		}

		const std::unique_ptr<CANHardware> &channel = hardwareChannels[frame.channel];
		if (nullptr == channel->frameHandler)
		{
			isobus::CANStackLogger::error("[HardwareInterface] Cannot transmit message on channel %u, because it is not assigned.", frame.channel);
			return false;
		}

		if (channel->frameHandler->get_is_valid())
		{
			channel->messagesToBeTransmitted.push_back(frame);
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

	void CANHardwareInterface::update()
	{
		if (started)
		{
			// Stage 1 - Receiving messages from hardware
			for (std::size_t i = 0; i < hardwareChannels.size(); i++)
			{
				receive_can_frame(i);
				while (!hardwareChannels[i]->receivedMessages.empty())
				{
					const auto &frame = hardwareChannels[i]->receivedMessages.front();

					frameReceivedEventDispatcher.invoke(frame);
					isobus::receive_can_message_frame_from_hardware(frame);

					hardwareChannels[i]->receivedMessages.pop_front();
				}
			}

			// Stage 2 - Sending messages
			isobus::periodic_update_from_hardware();

			// Stage 3 - Transmitting messages to hardware
			std::for_each(hardwareChannels.begin(), hardwareChannels.end(), [](const std::unique_ptr<CANHardware> &channel) {
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
		}
	}

	void CANHardwareInterface::receive_can_frame(std::uint8_t channelIndex)
	{
		isobus::CANMessageFrame frame;
		if (started &&
		    (nullptr != hardwareChannels[channelIndex]->frameHandler))
		{
			if (hardwareChannels[channelIndex]->frameHandler->get_is_valid())
			{
				// Socket or other hardware still open
				if (hardwareChannels[channelIndex]->frameHandler->read_frame(frame))
				{
					frame.channel = channelIndex;
					hardwareChannels[channelIndex]->receivedMessages.push_back(frame);
				}
			}
			else
			{
				isobus::CANStackLogger::critical("[CAN Rx Thread]: CAN Channel " + isobus::to_string(channelIndex) + " appears to be invalid.");
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
} // namespace isobus
