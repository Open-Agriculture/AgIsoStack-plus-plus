//================================================================================================
/// @file candlelight_windows_plugin.cpp
///
/// @copyright 2026 Mauricio. Distributed under the MIT license.
///
/// @brief Windows/WinUSB implementation of the candleLight/gs_usb protocol (see header for the
/// protocol references).
///
/// Wire protocol summary (classic CAN only):
///  - Device is found via SetupAPI using its WinUSB device interface GUID
///    {c15b4308-04d3-11e6-b3ea-6057189e6443}.
///  - Setup is a handful of vendor control transfers: tell the device the host's byte order, read
///    back its bit-timing capabilities (clock rate), compute + send a bit-timing table for the
///    requested bitrate, then set the device mode to "start" with hardware timestamps enabled.
///  - Frames flow as 24-byte "host frame" structs over one bulk IN and one bulk OUT endpoint,
///    discovered dynamically rather than hardcoded (endpoint numbers aren't part of the protocol
///    contract, only their direction/type are).
//================================================================================================
#include "isobus/hardware_integration/candlelight_windows_plugin.hpp"
#include "candlelight_bittiming.hpp"

#include "isobus/isobus/can_stack_logger.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <setupapi.h>
#include <usb100.h>
#include <winusb.h>

#include <cstdio>
#include <cstring>

#include <vector>

/// @cond INTERNAL_CANDLELIGHT_TRANSPORT
namespace isobus
{
	// The device interface GUID used by candleLight firmware with WinUSB descriptors.
	static const GUID CANDLE_DEVICE_GUID = { 0xc15b4308, 0x04d3, 0x11e6, { 0xb3, 0xea, 0x60, 0x57, 0x18, 0x9e, 0x64, 0x43 } };

	static constexpr DWORD READ_TIMEOUT_MS = 20;
	static constexpr DWORD WRITE_TIMEOUT_MS = 100;
	static constexpr DWORD CONTROL_TIMEOUT_MS = 1000;
	static constexpr std::uint32_t RX_ECHO_ID = 0xFFFFFFFFu; ///< Marks a frame as "really received", not a TX echo

	// --- Vendor control request numbers (candle_api's candle_ctrl_req.c) ---
	enum : std::uint8_t
	{
		BREQ_HOST_FORMAT = 0,
		BREQ_BITTIMING = 1,
		BREQ_MODE = 2,
		BREQ_BT_CONST = 4,
	};
	static constexpr std::uint8_t BM_REQUEST_OUT = 0x41; // OUT | VENDOR | INTERFACE
	static constexpr std::uint8_t BM_REQUEST_IN = 0xC1; // IN  | VENDOR | INTERFACE

	enum : std::uint32_t
	{
		MODE_RESET = 0,
		MODE_START = 1
	};
	static constexpr std::uint32_t MODE_FLAG_HW_TIMESTAMP = (1u << 4);

#pragma pack(push, 1)
	struct HostConfig
	{
		std::uint32_t byteOrder;
	};
	struct DeviceMode
	{
		std::uint32_t mode;
		std::uint32_t flags;
	};
	using detail::BitTiming;
	using detail::BitTimingConst;
	/// The 24 byte frame exchanged on the bulk endpoints (20 byte base "host frame" + 4 byte hardware timestamp)
	struct HostFrame
	{
		std::uint32_t echoID;
		std::uint32_t canID;
		std::uint8_t canDLC;
		std::uint8_t channel;
		std::uint8_t flags;
		std::uint8_t reserved;
		std::uint8_t data[8];
		std::uint32_t timestampUs;
	};
#pragma pack(pop)
	static_assert(sizeof(HostFrame) == 24, "HostFrame must match the wire layout exactly");

	static constexpr std::uint32_t ID_FLAG_EXTENDED = 0x80000000u;
	static constexpr std::uint32_t ID_FLAG_RTR = 0x40000000u;
	static constexpr std::uint32_t ID_FLAG_ERR = 0x20000000u;

	struct CandleLightWindowsPlugin::Impl
	{
		HDEVINFO deviceInfoSet = INVALID_HANDLE_VALUE;
		HANDLE fileHandle = INVALID_HANDLE_VALUE;
		WINUSB_INTERFACE_HANDLE winusb = nullptr;
		HANDLE readEvent = nullptr;
		HANDLE writeEvent = nullptr;
		HANDLE controlEvent = nullptr;
		std::uint8_t pipeIn = 0;
		std::uint8_t pipeOut = 0;
		std::atomic<std::uint32_t> nextEchoId = { 0 };
		std::atomic_bool ioError = { false };
		std::string lastError;

		~Impl()
		{
			release();
		}

		void release()
		{
			if (nullptr != winusb)
			{
				WinUsb_Free(winusb);
				winusb = nullptr;
			}
			if (INVALID_HANDLE_VALUE != fileHandle)
			{
				CloseHandle(fileHandle);
				fileHandle = INVALID_HANDLE_VALUE;
			}
			if (INVALID_HANDLE_VALUE != deviceInfoSet)
			{
				SetupDiDestroyDeviceInfoList(deviceInfoSet);
				deviceInfoSet = INVALID_HANDLE_VALUE;
			}
			if (nullptr != readEvent)
			{
				CloseHandle(readEvent);
				readEvent = nullptr;
			}
			if (nullptr != writeEvent)
			{
				CloseHandle(writeEvent);
				writeEvent = nullptr;
			}
			if (nullptr != controlEvent)
			{
				CloseHandle(controlEvent);
				controlEvent = nullptr;
			}
		}

		/// @brief Finds the deviceIndex'th candleLight-GUID device path, or empty string if not found
		static std::string find_device_path(std::uint8_t deviceIndex, std::string &errorOut)
		{
			HDEVINFO infoSet = SetupDiGetClassDevsA(&CANDLE_DEVICE_GUID, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
			if (INVALID_HANDLE_VALUE == infoSet)
			{
				errorOut = "SetupDiGetClassDevs failed (error " + std::to_string(GetLastError()) + ")";
				return {};
			}

			SP_DEVICE_INTERFACE_DATA interfaceData = {};
			interfaceData.cbSize = sizeof(interfaceData);
			std::string path;
			if (SetupDiEnumDeviceInterfaces(infoSet, nullptr, &CANDLE_DEVICE_GUID, deviceIndex, &interfaceData))
			{
				DWORD requiredSize = 0;
				SetupDiGetDeviceInterfaceDetailA(infoSet, &interfaceData, nullptr, 0, &requiredSize, nullptr);
				if (requiredSize > 0)
				{
					std::vector<std::uint8_t> buffer(requiredSize);
					auto *detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_A>(buffer.data());
					detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
					if (SetupDiGetDeviceInterfaceDetailA(infoSet, &interfaceData, detail, requiredSize, nullptr, nullptr))
					{
						path = detail->DevicePath;
					}
					else
					{
						errorOut = "SetupDiGetDeviceInterfaceDetail failed (error " + std::to_string(GetLastError()) + ")";
					}
				}
				else
				{
					errorOut = "Could not determine the candleLight device path size (error " + std::to_string(GetLastError()) + ")";
				}
			}
			else if (ERROR_NO_MORE_ITEMS == GetLastError())
			{
				errorOut = "No candleLight/gs_usb WinUSB device found at index " + std::to_string(deviceIndex) +
				  ". Check Device Manager for the adapter under \"Universal Serial Bus devices\".";
			}
			else
			{
				errorOut = "SetupDiEnumDeviceInterfaces failed (error " + std::to_string(GetLastError()) + ")";
			}
			SetupDiDestroyDeviceInfoList(infoSet);
			return path;
		}

		bool find_bulk_pipes(std::string &errorOut)
		{
			USB_INTERFACE_DESCRIPTOR interfaceDescriptor = {};
			if (!WinUsb_QueryInterfaceSettings(winusb, 0, &interfaceDescriptor))
			{
				errorOut = "WinUsb_QueryInterfaceSettings failed (error " + std::to_string(GetLastError()) + ")";
				return false;
			}

			bool foundIn = false;
			bool foundOut = false;
			for (UCHAR i = 0; i < interfaceDescriptor.bNumEndpoints; i++)
			{
				WINUSB_PIPE_INFORMATION pipeInfo = {};
				if (!WinUsb_QueryPipe(winusb, 0, i, &pipeInfo))
				{
					continue;
				}
				if (UsbdPipeTypeBulk == pipeInfo.PipeType)
				{
					if (0 != (pipeInfo.PipeId & 0x80)) // USB_ENDPOINT_DIRECTION_MASK bit: IN
					{
						pipeIn = pipeInfo.PipeId;
						foundIn = true;
					}
					else
					{
						pipeOut = pipeInfo.PipeId;
						foundOut = true;
					}
				}
			}
			if (!foundIn || !foundOut)
			{
				errorOut = "Could not find both a bulk IN and bulk OUT endpoint on this device";
				return false;
			}
			return true;
		}

		/// @brief Synchronous vendor control transfer (the open()-time handshake is not perf sensitive)
		bool control_transfer(std::uint8_t bmRequestType, std::uint8_t bRequest, std::uint16_t value, std::uint16_t index, void *buffer, std::uint16_t length, std::string &errorOut)
		{
			WINUSB_SETUP_PACKET setup = {};
			setup.RequestType = bmRequestType;
			setup.Request = bRequest;
			setup.Value = value;
			setup.Index = index;
			setup.Length = length;

			OVERLAPPED overlapped = {};
			overlapped.hEvent = controlEvent;
			ResetEvent(controlEvent);

			DWORD transferred = 0;
			if (!WinUsb_ControlTransfer(winusb, setup, static_cast<PUCHAR>(buffer), length, &transferred, &overlapped))
			{
				if (ERROR_IO_PENDING != GetLastError())
				{
					errorOut = "Control transfer (request " + std::to_string(bRequest) + ") failed to start (error " + std::to_string(GetLastError()) + ")";
					return false;
				}
				if (WAIT_OBJECT_0 != WaitForSingleObject(controlEvent, CONTROL_TIMEOUT_MS))
				{
					CancelIoEx(fileHandle, &overlapped);
					GetOverlappedResult(fileHandle, &overlapped, &transferred, TRUE);
					errorOut = "Control transfer (request " + std::to_string(bRequest) + ") timed out";
					return false;
				}
				if (!GetOverlappedResult(fileHandle, &overlapped, &transferred, FALSE))
				{
					errorOut = "Control transfer (request " + std::to_string(bRequest) + ") failed (error " + std::to_string(GetLastError()) + ")";
					return false;
				}
			}
			if (transferred != length)
			{
				errorOut = "Control transfer returned an unexpected length";
				return false;
			}
			return true;
		}

		/// @brief Reads exactly one host frame. USB bulk transfers legitimately arrive as a
		/// "short packet": the device commonly sends only the base 20 byte frame (no hardware
		/// timestamp trailer) even though the full struct is 24 bytes, and that is NOT an error
		/// -- the real reference driver explicitly handles this (treats anything from 20 to 24
		/// bytes as valid, zeroing the timestamp field when the trailer wasn't present). Requiring
		/// an exact 24 byte read, as an earlier version of this function did, silently corrupts
		/// every frame once the device starts sending 20 byte packets.
		bool fill_rx(HostFrame &frame)
		{
			static constexpr std::size_t MIN_FRAME_SIZE = sizeof(HostFrame) - sizeof(frame.timestampUs);
			std::uint8_t rawBuffer[sizeof(HostFrame)];
			DWORD received = 0;
			OVERLAPPED overlapped = {};
			overlapped.hEvent = readEvent;
			ResetEvent(readEvent);

			if (!WinUsb_ReadPipe(winusb, pipeIn, rawBuffer, static_cast<ULONG>(sizeof(rawBuffer)), &received, &overlapped))
			{
				if (ERROR_IO_PENDING != GetLastError())
				{
					const DWORD error = GetLastError();
					LOG_ERROR("[candleLight] USB read failed to start (Windows error " + std::to_string(error) + ")");
					ioError = true;
					return false;
				}
				const DWORD waitResult = WaitForSingleObject(readEvent, READ_TIMEOUT_MS);
				if (WAIT_OBJECT_0 != waitResult)
				{
					WinUsb_AbortPipe(winusb, pipeIn);
					GetOverlappedResult(fileHandle, &overlapped, &received, TRUE); // Reap the cancelled request
					return false; // Just a timeout, not necessarily an error
				}
				if (!GetOverlappedResult(fileHandle, &overlapped, &received, FALSE))
				{
					const DWORD error = GetLastError();
					LOG_ERROR("[candleLight] USB read completion failed (Windows error " + std::to_string(error) + ")");
					ioError = true;
					return false;
				}
			}
			if (debugRawDump)
			{
				std::fprintf(stderr, "[candleLight RAW] received=%lu bytes:", static_cast<unsigned long>(received));
				for (DWORD i = 0; i < received; i++)
				{
					std::fprintf(stderr, " %02X", rawBuffer[i]);
				}
				std::fprintf(stderr, "\n");
				std::fflush(stderr);
			}

			if (received < MIN_FRAME_SIZE)
			{
				return false; // Too short to be a real frame; discard rather than read past what arrived
			}

			frame = HostFrame{}; // Zero first, so a missing timestamp trailer reads back as 0, not garbage
			std::memcpy(&frame, rawBuffer, (received < sizeof(HostFrame)) ? received : sizeof(HostFrame));
			return true;
		}

		/// Set to true (e.g. via an environment variable check in open()) to print every raw USB
		/// read to stderr before any interpretation -- a ground-truth diagnostic, not for normal use.
		bool debugRawDump = false;
	};

	CandleLightWindowsPlugin::CandleLightWindowsPlugin(std::uint8_t deviceIndex, std::uint32_t bitrate, std::uint8_t channel) :
	  deviceIndex(deviceIndex),
	  bitrate(bitrate),
	  channel(channel),
	  valid(false)
	{
	}

	CandleLightWindowsPlugin::~CandleLightWindowsPlugin()
	{
		close();
	}

	std::string CandleLightWindowsPlugin::get_name() const
	{
		return "candleLight/gs_usb (WinUSB)";
	}

	bool CandleLightWindowsPlugin::get_is_valid() const
	{
		return valid;
	}

	std::string CandleLightWindowsPlugin::get_last_error() const
	{
		return lastError;
	}

	void CandleLightWindowsPlugin::close()
	{
		if ((nullptr != impl) && valid && !impl->ioError)
		{
			DeviceMode stopMode = { MODE_RESET, 0 };
			std::string ignored;
			impl->control_transfer(BM_REQUEST_OUT, BREQ_MODE, channel, 0, &stopMode, sizeof(stopMode), ignored);
		}
		valid = false;
		impl.reset();
	}

	void CandleLightWindowsPlugin::open()
	{
		if (valid)
		{
			return;
		}
		impl.reset(new Impl());

		{
			char envValue[8] = {};
			DWORD envLen = GetEnvironmentVariableA("CANDLELIGHT_RAW_DUMP", envValue, sizeof(envValue));
			impl->debugRawDump = (envLen > 0) && ('0' != envValue[0]);
		}

		const std::string devicePath = Impl::find_device_path(deviceIndex, impl->lastError);
		if (devicePath.empty())
		{
			lastError = impl->lastError;
			LOG_ERROR("[candleLight] " + lastError);
			impl.reset();
			return;
		}

		impl->fileHandle = CreateFileA(devicePath.c_str(),
		                               GENERIC_READ | GENERIC_WRITE,
		                               0, // One process owns the adapter and CAN channel.
		                               nullptr,
		                               OPEN_EXISTING,
		                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		                               nullptr);
		if (INVALID_HANDLE_VALUE == impl->fileHandle)
		{
			lastError = "Could not open the device (error " + std::to_string(GetLastError()) + "). Is another program already using it?";
			LOG_ERROR("[candleLight] " + lastError);
			impl.reset();
			return;
		}

		if (!WinUsb_Initialize(impl->fileHandle, &impl->winusb))
		{
			lastError = "WinUsb_Initialize failed (error " + std::to_string(GetLastError()) + ")";
			LOG_ERROR("[candleLight] " + lastError);
			impl.reset();
			return;
		}

		impl->readEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		impl->writeEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		impl->controlEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (!impl->readEvent || !impl->writeEvent || !impl->controlEvent)
		{
			lastError = "Could not create USB I/O events (error " + std::to_string(GetLastError()) + ")";
			LOG_ERROR("[candleLight] " + lastError);
			impl.reset();
			return;
		}

		if (!impl->find_bulk_pipes(impl->lastError))
		{
			lastError = impl->lastError;
			LOG_ERROR("[candleLight] " + lastError);
			impl.reset();
			return;
		}

		// --- The setup handshake, matching candle_api's sequence ---
		HostConfig hostConfig = { 0x0000beefu };
		if (!impl->control_transfer(BM_REQUEST_OUT, BREQ_HOST_FORMAT, 1, 0, &hostConfig, sizeof(hostConfig), impl->lastError))
		{
			lastError = "Host format handshake failed: " + impl->lastError;
			LOG_ERROR("[candleLight] " + lastError);
			impl.reset();
			return;
		}

		BitTimingConst btConst = {};
		if (!impl->control_transfer(BM_REQUEST_IN, BREQ_BT_CONST, channel, 0, &btConst, sizeof(btConst), impl->lastError))
		{
			lastError = "Reading bit-timing capabilities failed: " + impl->lastError;
			LOG_ERROR("[candleLight] " + lastError);
			impl.reset();
			return;
		}
		if (0 == btConst.fclkCan)
		{
			lastError = "Device reported a 0 Hz CAN clock; refusing to compute a bit-timing table";
			LOG_ERROR("[candleLight] " + lastError);
			impl.reset();
			return;
		}

		BitTiming bitTiming = {};
		if (!detail::calculate_bit_timing(btConst, bitrate, bitTiming))
		{
			lastError = "No supported exact CAN bit timing for " + std::to_string(bitrate) + " bit/s";
			LOG_ERROR("[candleLight] " + lastError);
			impl.reset();
			return;
		}

		if (!impl->control_transfer(BM_REQUEST_OUT, BREQ_BITTIMING, channel, 0, &bitTiming, sizeof(bitTiming), impl->lastError))
		{
			lastError = "Setting bit timing failed: " + impl->lastError;
			LOG_ERROR("[candleLight] " + lastError);
			impl.reset();
			return;
		}

		const bool hasTimestamps = (0 != (btConst.feature & MODE_FLAG_HW_TIMESTAMP));
		DeviceMode startMode = { MODE_START, hasTimestamps ? MODE_FLAG_HW_TIMESTAMP : 0u };
		if (!impl->control_transfer(BM_REQUEST_OUT, BREQ_MODE, channel, 0, &startMode, sizeof(startMode), impl->lastError))
		{
			lastError = "Starting the CAN channel failed: " + impl->lastError;
			LOG_ERROR("[candleLight] " + lastError);
			impl.reset();
			return;
		}

		lastError.clear();
		valid = true;
		LOG_INFO("[candleLight] Opened device index " + std::to_string(static_cast<int>(deviceIndex)) + " at " + std::to_string(bitrate) + " bit/s (brp=" + std::to_string(bitTiming.brp) + ")");
	}

	bool CandleLightWindowsPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		if (!valid || (nullptr == impl))
		{
			return false;
		}

		HostFrame frame;
		const auto readStarted = GetTickCount64();
		while (GetTickCount64() - readStarted < READ_TIMEOUT_MS)
		{
			if (!impl->fill_rx(frame))
			{
				if (impl->ioError)
				{
					LOG_ERROR("[candleLight] Lost connection to the adapter");
					valid = false;
				}
				return false;
			}

			if (RX_ECHO_ID != frame.echoID)
			{
				if (impl->debugRawDump)
				{
					std::fprintf(stderr, "[candleLight RAW] discarded as TX echo: echoID=%u canID=%08X canDLC=%u channel=%u\n", frame.echoID, frame.canID, frame.canDLC, frame.channel);
					std::fflush(stderr);
				}
				continue; // This is a TX-confirmation echo of our own earlier write_frame(), not incoming data
			}

			if (0 != (frame.canID & ID_FLAG_ERR))
			{
				// This is a CAN controller error report (bus-off, no-ACK, arbitration loss, etc.),
				// not a real received message -- AgIsoStack has no concept of an "error frame", so
				// passing this through as if it were real data with identifier 0 would corrupt the
				// stack's view of the bus. The low bits of canID are SocketCAN-style error flags
				// (e.g. 0x20 = no ACK received -- expected if nothing else is transmitting on the
				// bus yet) and are logged for visibility, then the frame is discarded.
				if (impl->debugRawDump)
				{
					std::fprintf(stderr, "[candleLight RAW] discarded as CAN error frame: errorBits=%08X data=%02X %02X %02X %02X %02X %02X %02X %02X\n", frame.canID & ~ID_FLAG_ERR, frame.data[0], frame.data[1], frame.data[2], frame.data[3], frame.data[4], frame.data[5], frame.data[6], frame.data[7]);
					std::fflush(stderr);
				}
				continue;
			}

			// The stack accepts classic data frames only, from this adapter channel.
			if ((frame.channel != channel) || (frame.canID & ID_FLAG_RTR) ||
			    (frame.flags & (1u << 1)) || (frame.canDLC > 8))
			{
				continue;
			}
			canFrame = {};
			canFrame.identifier = frame.canID & 0x1FFFFFFFu;
			canFrame.isExtendedFrame = (0 != (frame.canID & ID_FLAG_EXTENDED));
			canFrame.dataLength = frame.canDLC;
			std::memcpy(canFrame.data, frame.data, canFrame.dataLength);
			canFrame.channel = frame.channel;
			canFrame.timestamp_us = frame.timestampUs;
			return true;
		}
		return false;
	}

	bool CandleLightWindowsPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		if (!valid || (nullptr == impl))
		{
			return false;
		}

		if ((canFrame.dataLength > 8) ||
		    (canFrame.identifier > (canFrame.isExtendedFrame ? 0x1FFFFFFFu : 0x7FFu)))
		{
			return false;
		}
		HostFrame frame = {};
		frame.echoID = impl->nextEchoId.fetch_add(1);
		if (RX_ECHO_ID == frame.echoID)
		{
			frame.echoID = impl->nextEchoId.fetch_add(1); // Skip the one reserved value, extremely rare
		}
		frame.canID = (canFrame.identifier & 0x1FFFFFFFu) | (canFrame.isExtendedFrame ? ID_FLAG_EXTENDED : 0u);
		frame.canDLC = canFrame.dataLength;
		frame.channel = channel;
		std::memcpy(frame.data, canFrame.data, frame.canDLC);

		DWORD written = 0;
		OVERLAPPED overlapped = {};
		overlapped.hEvent = impl->writeEvent;
		ResetEvent(impl->writeEvent);

		if (!WinUsb_WritePipe(impl->winusb, impl->pipeOut, reinterpret_cast<PUCHAR>(&frame), sizeof(frame) - sizeof(frame.timestampUs), &written, &overlapped))
		{
			if (ERROR_IO_PENDING != GetLastError())
			{
				const DWORD error = GetLastError();
				LOG_ERROR("[candleLight] USB write failed to start (Windows error " + std::to_string(error) + ")");
				impl->ioError = true;
				valid = false;
				return false;
			}
			if (WAIT_OBJECT_0 != WaitForSingleObject(impl->writeEvent, WRITE_TIMEOUT_MS))
			{
				WinUsb_AbortPipe(impl->winusb, impl->pipeOut);
				GetOverlappedResult(impl->fileHandle, &overlapped, &written, TRUE);
				return false; // Timed out, but not necessarily a dead connection
			}
			if (!GetOverlappedResult(impl->fileHandle, &overlapped, &written, FALSE))
			{
				const DWORD error = GetLastError();
				LOG_ERROR("[candleLight] USB write completion failed (Windows error " + std::to_string(error) + ")");
				impl->ioError = true;
				valid = false;
				return false;
			}
		}
		return written == sizeof(frame) - sizeof(frame.timestampUs);
	}
}
/// @endcond
