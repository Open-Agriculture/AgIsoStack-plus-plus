//================================================================================================
/// @file virtual_can_plugin.cpp
///
/// @brief A driver for a virtual CAN bus that can be used for (automated) testing.
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/hardware_integration/virtual_can_plugin.hpp"

namespace isobus
{
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

	VirtualCANPlugin::~VirtualCANPlugin()
	{
		// Prevent a deadlock in the read_frame() function
		running = false;
		ourDevice->condition.notify_one();
	}

	std::string VirtualCANPlugin::get_name() const
	{
		return "Open-Agriculture Virtual CAN";
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

	bool VirtualCANPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
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

	void VirtualCANPlugin::write_frame_as_if_received(const isobus::CANMessageFrame &canFrame) const
	{
		const std::lock_guard<std::mutex> lock(mutex);
		ourDevice->queue.push_back(canFrame);
		ourDevice->condition.notify_one();
	}

	bool VirtualCANPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		return read_frame(canFrame, 1000);
	}

	bool VirtualCANPlugin::read_frame(isobus::CANMessageFrame &canFrame, std::uint32_t timeout) const
	{
		std::unique_lock<std::mutex> lock(mutex);
		ourDevice->condition.wait_for(lock, std::chrono::milliseconds(timeout), [this] { return !ourDevice->queue.empty() || !running; });
		if (!ourDevice->queue.empty())
		{
			canFrame = ourDevice->queue.front();
			ourDevice->queue.pop_front();
			return true;
		}
		return false;
	}

	bool VirtualCANPlugin::get_queue_empty() const
	{
		const std::lock_guard<std::mutex> lock(mutex);
		return ourDevice->queue.empty();
	}

	void VirtualCANPlugin::clear_queue() const
	{
		const std::lock_guard<std::mutex> lock(mutex);
		ourDevice->queue.clear();
	}
}
