//================================================================================================
/// @file innomaker_usb2can_windows_plugin.cpp
///
/// @brief An interface for using an INNO-Maker USB2CAN
/// @attention Use of this plugin may affect your license, as the LGPL-2.1 libusb will be linked to.
/// However, if you do not link to it, your executable should remain MIT.
/// This is not legal advice though, and you should be sure you understand the implications
/// of including this plugin.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#include "isobus/hardware_integration/innomaker_usb2can_windows_plugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#include <thread>

namespace isobus
{
	std::unique_ptr<InnoMakerUsb2CanLib> InnoMakerUSB2CANWindowsPlugin::driverInstance = nullptr;

	InnoMakerUSB2CANWindowsPlugin::InnoMakerUSB2CANWindowsPlugin(int channel, Baudrate baudrate) :
	  channel(channel),
	  baudrate(baudrate)
	{
		if (nullptr == driverInstance)
		{
			driverInstance = std::unique_ptr<InnoMakerUsb2CanLib>(new InnoMakerUsb2CanLib());
			driverInstance->setup();
			driverInstance->scanInnoMakerDevice();
		}
		txContexts = std::unique_ptr<InnoMakerUsb2CanLib::innomaker_can>(new InnoMakerUsb2CanLib::innomaker_can());
	}

	InnoMakerUSB2CANWindowsPlugin::~InnoMakerUSB2CANWindowsPlugin()
	{
		close();
	}

	std::string InnoMakerUSB2CANWindowsPlugin::get_name() const
	{
		return "INNO-Maker USB2CAN";
	}

	bool InnoMakerUSB2CANWindowsPlugin::get_is_valid() const
	{
		return nullptr != driverInstance->getInnoMakerDevice(channel) && driverInstance->getInnoMakerDevice(channel)->isOpen;
	}

	void InnoMakerUSB2CANWindowsPlugin::close()
	{
		InnoMakerUsb2CanLib::InnoMakerDevice *device = driverInstance->getInnoMakerDevice(channel);

		if (nullptr != device && device->isOpen)
		{
			driverInstance->urbResetDevice(device);
			driverInstance->closeInnoMakerDevice(device);

			// Check if all channels are closed and if so, close the driver instance
			bool allChannelsClosed = true;
			for (int i = 0; i < driverInstance->getInnoMakerDeviceCount(); i++)
			{
				if (nullptr != driverInstance->getInnoMakerDevice(i) &&
				    driverInstance->getInnoMakerDevice(i)->isOpen)
				{
					allChannelsClosed = false;
					break;
				}
			}
			if (allChannelsClosed)
			{
				LOG_INFO("[InnoMaker-Windows] All channels closed, closing driver instance");
				driverInstance->setdown();
				driverInstance = nullptr;
			}
		}
	}

	void InnoMakerUSB2CANWindowsPlugin::open()
	{
		InnoMakerUsb2CanLib::InnoMakerDevice *device = driverInstance->getInnoMakerDevice(channel);

		if (nullptr != device)
		{
			driverInstance->urbResetDevice(device);
			driverInstance->closeInnoMakerDevice(device);

			// Reset tx buffer according to the documentation
			for (int i = 0; i < driverInstance->innomaker_MAX_TX_URBS; i++)
			{
				txContexts->tx_context[i].echo_id = driverInstance->innomaker_MAX_TX_URBS;
			}

			InnoMakerUsb2CanLib::Innomaker_device_bittming bitTiming;
			switch (baudrate)
			{
				case B20k:
				{
					bitTiming.prop_seg = 6;
					bitTiming.phase_seg1 = 7;
					bitTiming.phase_seg2 = 2;
					bitTiming.sjw = 1;
					bitTiming.brp = 150;
				}
				break;
				case B33k3:
				{
					bitTiming.prop_seg = 3;
					bitTiming.phase_seg1 = 3;
					bitTiming.phase_seg2 = 1;
					bitTiming.sjw = 1;
					bitTiming.brp = 180;
				}
				break;
				case B40k:
				{
					bitTiming.prop_seg = 6;
					bitTiming.phase_seg1 = 7;
					bitTiming.phase_seg2 = 2;
					bitTiming.sjw = 1;
					bitTiming.brp = 75;
				}
				break;
				case B50k:
				{
					bitTiming.prop_seg = 6;
					bitTiming.phase_seg1 = 7;
					bitTiming.phase_seg2 = 2;
					bitTiming.sjw = 1;
					bitTiming.brp = 60;
				}
				break;
				case B66k6:
				{
					bitTiming.prop_seg = 3;
					bitTiming.phase_seg1 = 3;
					bitTiming.phase_seg2 = 1;
					bitTiming.sjw = 1;
					bitTiming.brp = 90;
				}
				break;
				case B80k:
				{
					bitTiming.prop_seg = 3;
					bitTiming.phase_seg1 = 3;
					bitTiming.phase_seg2 = 1;
					bitTiming.sjw = 1;
					bitTiming.brp = 75;
				}
				break;
				case B83k3:
				{
					bitTiming.prop_seg = 3;
					bitTiming.phase_seg1 = 3;
					bitTiming.phase_seg2 = 1;
					bitTiming.sjw = 1;
					bitTiming.brp = 72;
				}
				break;
				case B100k:
				{
					bitTiming.prop_seg = 6;
					bitTiming.phase_seg1 = 7;
					bitTiming.phase_seg2 = 2;
					bitTiming.sjw = 1;
					bitTiming.brp = 30;
				}
				break;
				case B125k:
				{
					bitTiming.prop_seg = 6;
					bitTiming.phase_seg1 = 7;
					bitTiming.phase_seg2 = 2;
					bitTiming.sjw = 1;
					bitTiming.brp = 24;
				}
				break;
				case B200k:
				{
					bitTiming.prop_seg = 6;
					bitTiming.phase_seg1 = 7;
					bitTiming.phase_seg2 = 2;
					bitTiming.sjw = 1;
					bitTiming.brp = 15;
				}
				break;
				case B250k:
				{
					bitTiming.prop_seg = 6;
					bitTiming.phase_seg1 = 7;
					bitTiming.phase_seg2 = 2;
					bitTiming.sjw = 1;
					bitTiming.brp = 12;
				}
				break;
				case B400k:
				{
					bitTiming.prop_seg = 3;
					bitTiming.phase_seg1 = 3;
					bitTiming.phase_seg2 = 1;
					bitTiming.sjw = 1;
					bitTiming.brp = 15;
				}
				break;
				case B500k:
				{
					bitTiming.prop_seg = 6;
					bitTiming.phase_seg1 = 7;
					bitTiming.phase_seg2 = 2;
					bitTiming.sjw = 1;
					bitTiming.brp = 6;
				}
				break;
				case B666k:
				{
					bitTiming.prop_seg = 3;
					bitTiming.phase_seg1 = 3;
					bitTiming.phase_seg2 = 2;
					bitTiming.sjw = 1;
					bitTiming.brp = 8;
				}
				break;
				case B800k:
				{
					bitTiming.prop_seg = 7;
					bitTiming.phase_seg1 = 8;
					bitTiming.phase_seg2 = 4;
					bitTiming.sjw = 1;
					bitTiming.brp = 3;
				}
				break;
				case B1000k:
				{
					bitTiming.prop_seg = 5;
					bitTiming.phase_seg1 = 6;
					bitTiming.phase_seg2 = 4;
					bitTiming.sjw = 1;
					bitTiming.brp = 3;
				}
				break;

				default:
				{
					LOG_ERROR("[InnoMaker-Windows] Unsupported baudrate with index " + isobus::to_string(baudrate) + " in InnoMakerUSB2CANWindowsPlugin::Baudrate enum.");
					return;
				}
				break;
			}
			driverInstance->urbSetupDevice(device, CAN_MODE, bitTiming);
			driverInstance->openInnoMakerDevice(device);
		}
		else
		{
			LOG_ERROR("[InnoMaker-Windows] No device found on channel " + isobus::to_string(channel));
		}
	}

	bool InnoMakerUSB2CANWindowsPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		InnoMakerUsb2CanLib::InnoMakerDevice *device = driverInstance->getInnoMakerDevice(channel);

		if (nullptr == device)
		{
			return false;
		}
		BYTE receiveBuffer[sizeof(InnoMakerUsb2CanLib::innomaker_host_frame)];
		bool success = driverInstance->recvInnoMakerDeviceBuf(device, receiveBuffer, sizeof(InnoMakerUsb2CanLib::innomaker_host_frame), 0xFFFFFFFF);
		if (!success)
		{
			// Most likely a timeout
			return false;
		}
		InnoMakerUsb2CanLib::innomaker_host_frame frame;
		memcpy(&frame, receiveBuffer, sizeof(InnoMakerUsb2CanLib::innomaker_host_frame));

		if (0xFFFFFFFF != frame.echo_id)
		{
			InnoMakerUsb2CanLib::innomaker_tx_context *txc = driverInstance->innomaker_get_tx_context(txContexts.get(), frame.echo_id);
			if (nullptr == txc)
			{
				LOG_WARNING("[InnoMaker-Windows] Received frame with bad echo ID: " + isobus::to_string(static_cast<int>(frame.echo_id)));
				return false;
			}
			driverInstance->innomaker_free_tx_context(txc);

			/// @todo error frame handling
			return false;
		}

		canFrame.dataLength = frame.can_dlc;
		canFrame.isExtendedFrame = frame.can_id & CAN_EFF_FLAG;
		if (canFrame.isExtendedFrame)
		{
			canFrame.identifier = frame.can_id & CAN_EFF_MASK;
		}
		else
		{
			canFrame.identifier = frame.can_id & CAN_SFF_MASK;
		}
		canFrame.timestamp_us = frame.timestamp_us;
		memcpy(canFrame.data, frame.data, canFrame.dataLength);
		return true;
	}

	bool InnoMakerUSB2CANWindowsPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		InnoMakerUsb2CanLib::InnoMakerDevice *device = driverInstance->getInnoMakerDevice(channel);
		if (nullptr == device)
		{
			return false;
		}

		InnoMakerUsb2CanLib::innomaker_tx_context *txc = driverInstance->innomaker_alloc_tx_context(txContexts.get());
		if (0xFF == txc->echo_id)
		{
			LOG_DEBUG("[InnoMaker-Windows] No free transmission context");
			return false;
		}

		InnoMakerUsb2CanLib::innomaker_host_frame frame;
		memset(&frame, 0, sizeof(InnoMakerUsb2CanLib::innomaker_host_frame));

		frame.echo_id = txc->echo_id;
		frame.can_dlc = canFrame.dataLength;
		frame.can_id = canFrame.identifier;
		memcpy(frame.data, canFrame.data, canFrame.dataLength);

		if (canFrame.isExtendedFrame)
		{
			frame.can_id |= CAN_EFF_FLAG;
		}

		BYTE sendBuffer[sizeof(InnoMakerUsb2CanLib::innomaker_host_frame)];
		memcpy(sendBuffer, &frame, sizeof(InnoMakerUsb2CanLib::innomaker_host_frame));

		bool success = driverInstance->sendInnoMakerDeviceBuf(device, sendBuffer, sizeof(InnoMakerUsb2CanLib::innomaker_host_frame), 10);
		if (!success)
		{
			LOG_WARNING("[InnoMaker-Windows] Failed to send frame");
			driverInstance->innomaker_free_tx_context(txc);
		}
		return success;
	}
}
