//================================================================================================
/// @file virtual_can_plugin.cpp
///
/// @brief A driver for a virtual CAN bus that can be used for (automated) testing.
/// @author Daan Steenbergen
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "isobus/hardware_integration/virtual_can_plugin.hpp"

std::mutex VirtualCANPlugin::mutex;
std::map<std::string, std::vector<std::shared_ptr<VirtualCANPlugin::VirtualDevice>>> VirtualCANPlugin::channels;

VirtualCANPlugin::VirtualCANPlugin(const std::string channel, const bool receiveOwnMessages) :
  channel(channel),
  receiveOwnMessages(receiveOwnMessages)
{
	const std::lock_guard<std::mutex> lock(mutex);
	ourDevice = std::make_shared<VirtualDevice>();
	channels[channel].push_back(ourDevice);
}

bool VirtualCANPlugin::get_is_valid() const
{
	return running;
}

std::string VirtualCANPlugin::get_channel_name() const
{
	return channel;
}

void VirtualCANPlugin::open()
{
	running = true;
}

void VirtualCANPlugin::close()
{
	running = false;
	ourDevice->condition.notify_one();
}

bool VirtualCANPlugin::write_frame(const isobus::HardwareInterfaceCANFrame &canFrame)
{
	bool retVal = false;
	const std::lock_guard<std::mutex> lock(mutex);
	for (std::shared_ptr<VirtualDevice> device : channels[channel])
	{
		if (device->queue.size() < MAX_QUEUE_SIZE)
		{
			if (receiveOwnMessages || device != ourDevice)
			{
				device->queue.push_back(canFrame);
				device->condition.notify_one();
				retVal = true;
			}
		}
	}
	return retVal;
}

bool VirtualCANPlugin::read_frame(isobus::HardwareInterfaceCANFrame &canFrame)
{
	std::unique_lock<std::mutex> lock(mutex);
	ourDevice->condition.wait(lock, [this] { return !running || !ourDevice->queue.empty(); });
	if (!ourDevice->queue.empty())
	{
		canFrame = ourDevice->queue.front();
		ourDevice->queue.pop_front();
		return true;
	}
	return false;
}
