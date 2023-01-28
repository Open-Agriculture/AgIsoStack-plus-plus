#pragma once
#ifdef INNOMAKERUSB2CANLIB_EXPORTS
#define INNOMAKERUSB2CANLIB_EXP __declspec(dllexport)
#else
#define INNOMAKERUSB2CANLIB_EXP __declspec(dllimport)
#endif
#include <iostream>
#include "libusb.h"
#include <vector>
#include <atomic>

using  namespace  std;

class INNOMAKERUSB2CANLIB_EXP InnoMakerUsb2CanLib {

public:
	class InnoMakerDevice {
	public:
		libusb_device_handle *devHandle;
		libusb_device *device;
		bool isOpen;
	};

	struct innomaker_host_frame
	{
		UINT32 echo_id;
		UINT32 can_id;
		BYTE can_dlc;
		BYTE channel;
		BYTE flags;
		BYTE reserved;
		BYTE data[8];
		UINT32 timestamp_us;
	};

	struct Innomaker_device_bittming
	{
		UINT32 prop_seg;
		UINT32 phase_seg1;
		UINT32 phase_seg2;
		UINT32 sjw;
		UINT32 brp;
	};

	enum UsbCanMode
	{
		UsbCanModeNormal = 0,
		UsbCanModeLoopback,
		UsbCanModeListenOnly,
	};

	class innomaker_tx_context
	{
	public:
		UINT32 echo_id;
	};

	class spin_mutex {
		std::atomic<bool> flag = ATOMIC_VAR_INIT(false);
	public:
		spin_mutex() = default;
		spin_mutex(const spin_mutex&) = delete;
		spin_mutex& operator= (const spin_mutex&) = delete;
		void lock() {
			bool expected = false;
			while (!flag.compare_exchange_strong(expected, true))
				expected = false;
		}
		void unlock() {
			flag.store(false);
		}
	};

	class innomaker_can
	{
		/* This lock prevents a race condition between xmit and receive. */
	public:
		spin_mutex tx_ctx_lock;
		innomaker_tx_context tx_context[10];
	};
private:


	struct innomaker_identify_mode
	{
		UINT32 mode;
	};


	struct innomaker_device_mode
	{
		UINT32 mode;
		UINT32 flags;
	};


	/// 启动数据包格式
	struct UsbSetupPacket
	{
	public:
		BYTE RequestType;
		BYTE Request;
		short Value;
		short Index;
		short Length;
	};

	static int LIBUSB_CALL hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data);
	static int LIBUSB_CALL hotplug_callback_detach(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data);

public:

	/// Setup USBCAN, This must be called first when use lib
	bool setup();

	/// Setdown USBCAN, This must be called after when not use lib
	bool setdown();

	/// Scan device
	bool scanInnoMakerDevice();

	/// Get device count
	int getInnoMakerDeviceCount();

	/// Get Specify Device by Device Index
	InnoMakerDevice* getInnoMakerDevice(int devIndex);

	/// Open Device
	bool openInnoMakerDevice(InnoMakerDevice *device);

	/// Close Device 
	bool closeInnoMakerDevice(InnoMakerDevice *device);

	/// Send Buffer
	bool sendInnoMakerDeviceBuf(InnoMakerDevice *device, BYTE *buf, int size, unsigned int timeout);

	/// Recv Buffer
	bool recvInnoMakerDeviceBuf(InnoMakerDevice *device, BYTE *buf, int size, unsigned int timeout);

	/// Reset Device
	bool urbResetDevice(InnoMakerDevice *device);

	/// Setup Device
	bool urbSetupDevice(InnoMakerDevice *device, UsbCanMode canMode, Innomaker_device_bittming bittming);

	InnoMakerUsb2CanLib() {
		innoMakerDevices = new vector<InnoMakerDevice>();
	}

	~InnoMakerUsb2CanLib() {
		delete(innoMakerDevices);
	}

	/* Alloc a tx content */
	innomaker_tx_context *innomaker_alloc_tx_context(innomaker_can *dev);


	/* releases a tx context
	 */
	void innomaker_free_tx_context(innomaker_tx_context *txc);


	/* Get a tx context by id.
	 */
	innomaker_tx_context *innomaker_get_tx_context(innomaker_can *dev, UINT id);

private:
	/// Set Host format
	bool urbSetHostFormat(InnoMakerDevice *device);
	/// Set Bitrate
	bool urbSetBitrate(InnoMakerDevice *device, Innomaker_device_bittming bittming);
	/// Start device
	bool urbStartDevice(InnoMakerDevice *device, UsbCanMode mode);

private:
	/// Device List 
	vector<InnoMakerDevice> *innoMakerDevices;
	/// Endpoint Out 
	int endPointOut = 2;
	/// Endpoint In
	int endPointIn = 129;
	/// HotPlug CallHandle
	libusb_hotplug_callback_handle hp[2];
	/// PID
	UINT32 pid = 0x606f;
	/// VID
	UINT32 vid = 0x1d50;
	libusb_device **devs;

	/// Add Device Callback
	void(*addCallback)(InnoMakerDevice *device);
	/// Remove Device Callback
	void(*removeCallback)(InnoMakerDevice *device);
public:
	int innomaker_MAX_TX_URBS = 10;
};