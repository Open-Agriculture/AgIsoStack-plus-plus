/*
  MIT License

  Copyright (c) 2018 Antonio Alexander Brewer (tonton81) - https://github.com/tonton81

  Designed and tested for PJRC Teensy 4.0.

  Forum link : https://forum.pjrc.com/threads/56035-FlexCAN_T4-FlexCAN-for-Teensy-4?highlight=flexcan_t4

  Thanks goes to skpang, mjs513, and collin for tech/testing support

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#if !defined(_FLEXCAN_T4_H_)
#define _FLEXCAN_T4_H_

#include "Arduino.h"
#include "circular_buffer.hpp"
#include "imxrt_flexcan.hpp"

typedef struct CAN_error_t
{
	char state[30] = "Idle";
	bool BIT1_ERR = 0;
	bool BIT0_ERR = 0;
	bool ACK_ERR = 0;
	bool CRC_ERR = 0;
	bool FRM_ERR = 0;
	bool STF_ERR = 0;
	bool TX_WRN = 0;
	bool RX_WRN = 0;
	char FLT_CONF[14] = { 0 };
	uint8_t RX_ERR_COUNTER = 0;
	uint8_t TX_ERR_COUNTER = 0;
	uint32_t ESR1 = 0;
	uint16_t ECR = 0;
} CAN_error_t;

typedef struct CAN_message_t
{
	uint32_t id = 0; // can identifier
	uint16_t timestamp = 0; // FlexCAN time when message arrived
	uint8_t idhit = 0; // filter that id came from
	struct
	{
		bool extended = 0; // identifier is extended (29-bit)
		bool remote = 0; // remote transmission request packet type
		bool overrun = 0; // message overrun
		bool reserved = 0;
	} flags;
	uint8_t len = 8; // length of data
	uint8_t buf[8] = { 0 }; // data
	int8_t mb = 0; // used to identify mailbox reception
	uint8_t bus = 0; // used to identify where the message came from when events() is used.
	bool seq = 0; // sequential frames
} CAN_message_t;

typedef struct CANFD_message_t
{
	uint32_t id = 0; // can identifier
	uint16_t timestamp = 0; // FlexCAN time when message arrived
	uint8_t idhit = 0; // filter that id came from
	bool brs = 1; // baud rate switching for data
	bool esi = 0; // error status indicator
	bool edl = 1; // extended data length (for RX, 0 == CAN2.0, 1 == FD)
	struct
	{
		bool extended = 0; // identifier is extended (29-bit)
		bool overrun = 0; // message overrun
		bool reserved = 0;
	} flags;
	uint8_t len = 8; // length of data
	uint8_t buf[64] = { 0 }; // data
	int8_t mb = 0; // used to identify mailbox reception
	uint8_t bus = 0; // used to identify where the message came from when events() is used.
	bool seq = 0; // sequential frames
} CANFD_message_t;

typedef void (*_MB_ptr)(const CAN_message_t &msg); /* mailbox / global callbacks */
typedef void (*_MBFD_ptr)(const CANFD_message_t &msg); /* mailbox / global callbacks */

typedef enum FLEXCAN_PINS
{
	ALT = 0,
	DEF = 1,
} FLEXCAN_PINS;

typedef enum FLEXCAN_MAILBOX
{
	MB0 = 0,
	MB1 = 1,
	MB2 = 2,
	MB3 = 3,
	MB4 = 4,
	MB5 = 5,
	MB6 = 6,
	MB7 = 7,
	MB8 = 8,
	MB9 = 9,
	MB10 = 10,
	MB11 = 11,
	MB12 = 12,
	MB13 = 13,
	MB14 = 14,
	MB15 = 15,
	MB16 = 16,
	MB17 = 17,
	MB18 = 18,
	MB19 = 19,
	MB20 = 20,
	MB21 = 21,
	MB22 = 22,
	MB23 = 23,
	MB24 = 24,
	MB25 = 25,
	MB26 = 26,
	MB27 = 27,
	MB28 = 28,
	MB29 = 29,
	MB30 = 30,
	MB31 = 31,
	MB32 = 32,
	MB33 = 33,
	MB34 = 34,
	MB35 = 35,
	MB36 = 36,
	MB37 = 37,
	MB38 = 38,
	MB39 = 39,
	MB40 = 40,
	MB41 = 41,
	MB42 = 42,
	MB43 = 43,
	MB44 = 44,
	MB45 = 45,
	MB46 = 46,
	MB47 = 47,
	MB48 = 48,
	MB49 = 49,
	MB50 = 50,
	MB51 = 51,
	MB52 = 52,
	MB53 = 53,
	MB54 = 54,
	MB55 = 55,
	MB56 = 56,
	MB57 = 57,
	MB58 = 58,
	MB59 = 59,
	MB60 = 60,
	MB61 = 61,
	MB62 = 62,
	MB63 = 63,
	FIFO = 99
} FLEXCAN_MAILBOX;

typedef enum FLEXCAN_RXTX
{
	TX,
	RX,
	LISTEN_ONLY
} FLEXCAN_RXTX;

typedef enum FLEXCAN_FDRATES
{
	CAN_1M_2M,
	CAN_1M_4M,
	CAN_1M_6M,
	CAN_1M_8M
} FLEXCAN_FDRATES;

typedef enum FLEXCAN_CLOCK
{
	CLK_OFF,
	CLK_8MHz = 8,
	CLK_16MHz = 16,
	CLK_20MHz = 20,
	CLK_24MHz = 24,
	CLK_30MHz = 30,
	CLK_40MHz = 40,
	CLK_60MHz = 60,
	CLK_80MHz = 80
} FLEXCAN_CLOCK;

typedef struct CANFD_timings_t
{
	double baudrate = 1000000;
	double baudrateFD = 2000000;
	double propdelay = 190;
	double bus_length = 1;
	double sample = 75;
	FLEXCAN_CLOCK clock = CLK_24MHz;
} CANFD_timings_t;

typedef enum FLEXCAN_IDE
{
	NONE = 0,
	EXT = 1,
	RTR = 2,
	STD = 3,
	INACTIVE
} FLEXCAN_IDE;

typedef enum FLEXCAN_FLTEN
{
	ACCEPT_ALL = 0,
	REJECT_ALL = 1
} FLEXCAN_FLTEN;

typedef enum FLEXCAN_FILTER_TABLE
{
	FLEXCAN_MULTI = 1,
	FLEXCAN_RANGE = 2,
	FLEXCAN_TABLE_B_MULTI = 3,
	FLEXCAN_TABLE_B_RANGE = 4,
	FLEXCAN_USERMASK = 5
} FLEXCAN_FILTER_TABLE;

typedef enum FLEXCAN_FIFOTABLE
{
	A = 0,
	B = 1,
	C = 2
} FLEXCAN_FIFOTABLE;

typedef enum FLEXCAN_RXQUEUE_TABLE
{
	RX_SIZE_2 = (uint16_t)2,
	RX_SIZE_4 = (uint16_t)4,
	RX_SIZE_8 = (uint16_t)8,
	RX_SIZE_16 = (uint16_t)16,
	RX_SIZE_32 = (uint16_t)32,
	RX_SIZE_64 = (uint16_t)64,
	RX_SIZE_128 = (uint16_t)128,
	RX_SIZE_256 = (uint16_t)256,
	RX_SIZE_512 = (uint16_t)512,
	RX_SIZE_1024 = (uint16_t)1024
} FLEXCAN_RXQUEUE_TABLE;

typedef enum FLEXCAN_DLC_SIZE
{
	DLC_SIZE_8 = (uint16_t)8,
	DLC_SIZE_12 = (uint16_t)12,
	DLC_SIZE_16 = (uint16_t)16,
	DLC_SIZE_20 = (uint16_t)20,
	DLC_SIZE_24 = (uint16_t)24,
	DLC_SIZE_32 = (uint16_t)32,
	DLC_SIZE_48 = (uint16_t)48,
	DLC_SIZE_64 = (uint16_t)64
} FLEXCAN_DLC_SIZE;

typedef enum FLEXCAN_RFFN_TABLE
{
	RFFN_8 = (uint8_t)0,
	RFFN_16 = (uint8_t)1,
	RFFN_24 = (uint8_t)2,
	RFFN_32 = (uint8_t)3,
	RFFN_40 = (uint8_t)4,
	RFFN_48 = (uint8_t)5,
	RFFN_56 = (uint8_t)6,
	RFFN_64 = (uint8_t)7,
	RFFN_72 = (uint8_t)8,
	RFFN_80 = (uint8_t)9,
	RFFN_88 = (uint8_t)10,
	RFFN_96 = (uint8_t)11,
	RFFN_104 = (uint8_t)12,
	RFFN_112 = (uint8_t)13,
	RFFN_120 = (uint8_t)14,
	RFFN_128 = (uint8_t)15
} FLEXCAN_RFFN_TABLE;

typedef enum FLEXCAN_TXQUEUE_TABLE
{
	TX_SIZE_2 = (uint16_t)2,
	TX_SIZE_4 = (uint16_t)4,
	TX_SIZE_8 = (uint16_t)8,
	TX_SIZE_16 = (uint16_t)16,
	TX_SIZE_32 = (uint16_t)32,
	TX_SIZE_64 = (uint16_t)64,
	TX_SIZE_128 = (uint16_t)128,
	TX_SIZE_256 = (uint16_t)256,
	TX_SIZE_512 = (uint16_t)512,
	TX_SIZE_1024 = (uint16_t)1024
} FLEXCAN_TXQUEUE_TABLE;

typedef enum CAN_DEV_TABLE
{
#if defined(__IMXRT1062__)
	CAN0 = (uint32_t)0x0,
	CAN1 = (uint32_t)0x401D0000,
	CAN2 = (uint32_t)0x401D4000,
	CAN3 = (uint32_t)0x401D8000
#endif
#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
	CAN0 = (uint32_t)0x40024000,
	CAN1 = (uint32_t)0x400A4000,
	CAN2 = (uint32_t)0x0,
	CAN3 = (uint32_t)0x0
#endif
} CAN_DEV_TABLE;

#define FCTP_CLASS template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize = RX_SIZE_16, FLEXCAN_TXQUEUE_TABLE _txSize = TX_SIZE_16>
#define FCTP_FUNC template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize, FLEXCAN_TXQUEUE_TABLE _txSize>
#define FCTP_OPT FlexCAN_T4<_bus, _rxSize, _txSize>

#define FCTPFD_CLASS template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize = RX_SIZE_16, FLEXCAN_TXQUEUE_TABLE _txSize = TX_SIZE_16>
#define FCTPFD_FUNC template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize, FLEXCAN_TXQUEUE_TABLE _txSize>
#define FCTPFD_OPT FlexCAN_T4FD<_bus, _rxSize, _txSize>

#define SIZE_LISTENERS 4

class CANListener
{
public:
	CANListener()
	{
		callbacksActive = 0;
	}
	virtual bool frameHandler(CAN_message_t &frame, int mailbox, uint8_t controller)
	{
		return false;
	}
	virtual void txHandler(int mailbox, uint8_t controller)
	{
		;
	} /* unused in Collin's library */
	void attachMBHandler(uint64_t mailBox)
	{
		callbacksActive |= (1UL << mailBox);
	}
	void detachMBHandler(uint64_t mailBox)
	{
		callbacksActive &= ~(1UL << mailBox);
	}
	void attachGeneralHandler(void)
	{
		generalCallbackActive = 1;
	}
	void detachGeneralHandler(void)
	{
		generalCallbackActive = 0;
	}
	uint64_t callbacksActive; // bitfield indicating which callbacks are installed (for object oriented callbacks only)
	bool generalCallbackActive = 0;
};

class FlexCAN_T4_Base
{
public:
	virtual void flexcan_interrupt() = 0;
	virtual void setBaudRate(uint32_t baud = 1000000, FLEXCAN_RXTX listen_only = TX) = 0;
	virtual uint64_t events() = 0;
	virtual int write(const CANFD_message_t &msg) = 0;
	virtual int write(const CAN_message_t &msg) = 0;
	virtual bool isFD() = 0;
	virtual uint8_t getFirstTxBoxSize() = 0;
};

#if defined(__IMXRT1062__)
static FlexCAN_T4_Base *_CAN1 = nullptr;
static FlexCAN_T4_Base *_CAN2 = nullptr;
static FlexCAN_T4_Base *_CAN3 = nullptr;
#endif
#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
static FlexCAN_T4_Base *_CAN0 = nullptr;
static FlexCAN_T4_Base *_CAN1 = nullptr;
#endif

extern void ext_outputFD1(const CANFD_message_t &msg); // Interrupt data output, not filtered, for external libraries
extern void ext_outputFD2(const CANFD_message_t &msg);
extern void ext_outputFD3(const CANFD_message_t &msg);

extern void ext_output1(const CAN_message_t &msg); // Interrupt data output, not filtered, for external libraries
extern void ext_output2(const CAN_message_t &msg);
extern void ext_output3(const CAN_message_t &msg);

FCTPFD_CLASS class FlexCAN_T4FD : public FlexCAN_T4_Base
{
public:
	FlexCAN_T4FD();
	bool isFD()
	{
		return 1;
	}
	void begin();
	void setTX(FLEXCAN_PINS pin = DEF);
	void setRX(FLEXCAN_PINS pin = DEF);
	void enableFIFO(bool status = 1);
	void disableFIFO()
	{
		enableFIFO(0);
	}
	int read(CANFD_message_t &msg);
	int readMB(CANFD_message_t &msg);
	int write(const CANFD_message_t &msg); /* use any available mailbox for transmitting */
	int write(const CAN_message_t &msg)
	{
		return 0;
	} /* to satisfy base class for external pointers */
	int write(FLEXCAN_MAILBOX mb_num, const CANFD_message_t &msg); /* use a single mailbox for transmitting */
	bool setMB(const FLEXCAN_MAILBOX &mb_num, const FLEXCAN_RXTX &mb_rx_tx, const FLEXCAN_IDE &ide = STD);
	uint8_t setRegions(uint8_t size); /* 8, 16, 32 or 64 bytes */
	uint8_t setRegions(uint8_t mbdsr0, uint8_t mbdsr1); /* set both regions independantly */
	bool setBaudRateAdvanced(CANFD_timings_t config, uint8_t nominal_choice, uint8_t flexdata_choice, FLEXCAN_RXTX listen_only = TX);
	bool setBaudRate(CANFD_timings_t config, FLEXCAN_RXTX listen_only = TX);
	void setBaudRate(FLEXCAN_FDRATES input, FLEXCAN_RXTX listen_only = TX);
	void enableMBInterrupt(const FLEXCAN_MAILBOX &mb_num, bool status = 1);
	void disableMBInterrupt(const FLEXCAN_MAILBOX &mb_num)
	{
		enableMBInterrupt(mb_num, 0);
	}
	void enableMBInterrupts(bool status = 1);
	void disableMBInterrupts()
	{
		enableMBInterrupts(0);
	}
	uint64_t events();
	void onReceive(const FLEXCAN_MAILBOX &mb_num, _MBFD_ptr handler); /* individual mailbox callback function */
	void onReceive(_MBFD_ptr handler); /* global callback function */
	void setMBFilter(FLEXCAN_FLTEN input); /* enable/disable traffic for all MBs (for individual masking) */
	void setMBFilter(FLEXCAN_MAILBOX mb_num, FLEXCAN_FLTEN input); /* set specific MB to accept/deny traffic */
	bool setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1); /* input 1 ID to be filtered */
	bool setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2); /* input 2 ID's to be filtered */
	bool setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3); /* input 3 ID's to be filtered */
	bool setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4); /* input 4 ID's to be filtered */
	bool setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t id5); /* input 5 ID's to be filtered */
	bool setMBFilterRange(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2); /* filter a range of ids */
	void mailboxStatus();
	void enhanceFilter(FLEXCAN_MAILBOX mb_num);
	void distribute(bool state = 1)
	{
		distribution = state;
	}
	void enableDMA(bool state = 1);
	void disableDMA()
	{
		enableDMA(0);
	}
	uint8_t getFirstTxBoxSize();
	void setMBFilterProcessing(FLEXCAN_MAILBOX mb_num, uint32_t filter_id, uint32_t calculated_mask);

private:
	volatile bool isEventsUsed = 0;
	uint64_t readIFLAG()
	{
		return (((uint64_t)FLEXCANb_IFLAG2(_bus) << 32) | FLEXCANb_IFLAG1(_bus));
	}
	uint32_t mailbox_offset(uint8_t mailbox, uint8_t &maxsize);
	void writeTxMailbox(uint8_t mb_num, const CANFD_message_t &msg);
	bool setBaudRate(CANFD_timings_t config, uint8_t nominal_choice, uint8_t flexdata_choice, FLEXCAN_RXTX listen_only = TX, bool advanced = 0);
	int getFirstTxBox();
	uint32_t mb_filter_table[64][7];
	Circular_Buffer<uint8_t, (uint32_t)_rxSize, sizeof(CANFD_message_t)> rxBuffer;
	Circular_Buffer<uint8_t, (uint32_t)_txSize, sizeof(CANFD_message_t)> txBuffer;
	void FLEXCAN_ExitFreezeMode();
	void FLEXCAN_EnterFreezeMode();
	void reset()
	{
		softReset();
	} /* reset flexcan controller (needs register restore capabilities...) */
	void flexcan_interrupt();
	void writeIFLAG(uint64_t value);
	void writeIFLAGBit(uint8_t mb_num);
	uint8_t max_mailboxes();
	uint64_t readIMASK()
	{
		return (((uint64_t)FLEXCANb_IMASK2(_bus) << 32) | FLEXCANb_IMASK1(_bus));
	}
	void frame_distribution(CANFD_message_t &msg);
	void setBaudRate(uint32_t baud = 1000000, FLEXCAN_RXTX listen_only = TX)
	{
		;
	} // unused, CAN2.0 only (needed for base class existance)
	void setClock(FLEXCAN_CLOCK clock = CLK_24MHz);
	uint32_t getClock();
	void softReset();
	void writeIMASK(uint64_t value);
	void writeIMASKBit(uint8_t mb_num, bool set = 1);
	uint8_t busNumber;
	uint8_t mailbox_reader_increment = 0;
	uint32_t nvicIrq = 0;
	uint8_t dlc_to_len(uint8_t val);
	uint8_t len_to_dlc(uint8_t val);
	uint32_t setBaudRateFD(CANFD_timings_t config, uint32_t flexdata_choice, bool advanced); /* internally used */
	void mbCallbacks(const FLEXCAN_MAILBOX &mb_num, const CANFD_message_t &msg);
	_MBFD_ptr _mbHandlers[64]; /* individual mailbox handlers */
	_MBFD_ptr _mainHandler; /* global mailbox handler */
	void filter_store(FLEXCAN_FILTER_TABLE type, FLEXCAN_MAILBOX mb_num, uint32_t id_count, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t id5, uint32_t mask);
	bool filter_match(FLEXCAN_MAILBOX mb_num, uint32_t id);
	void struct2queueTx(const CANFD_message_t &msg);
	void struct2queueRx(const CANFD_message_t &msg);
	bool distribution = 0;
};

FCTP_CLASS class FlexCAN_T4 : public FlexCAN_T4_Base
{
public:
	FlexCAN_T4();
	CANListener *listener[SIZE_LISTENERS];
	bool attachObj(CANListener *listener);
	bool detachObj(CANListener *listener);
	bool isFD()
	{
		return 0;
	}
	void begin();
	uint32_t getBaudRate()
	{
		return currentBitrate;
	}
	void setTX(FLEXCAN_PINS pin = DEF);
	void setRX(FLEXCAN_PINS pin = DEF);
	void setBaudRate(uint32_t baud = 1000000, FLEXCAN_RXTX listen_only = TX);
	void reset()
	{
		softReset();
	} /* reset flexcan controller (needs register restore capabilities...) */
	void setMaxMB(uint8_t last);
	void enableLoopBack(bool yes = 1);
	void enableFIFO(bool status = 1);
	void disableFIFO()
	{
		enableFIFO(0);
	}
	void enableFIFOInterrupt(bool status = 1);
	void disableFIFOInterrupt()
	{
		enableFIFOInterrupt(0);
	}
	void mailboxStatus();
	int read(CAN_message_t &msg);
	int readMB(CAN_message_t &msg);
	int readFIFO(CAN_message_t &msg);
	bool setMB(const FLEXCAN_MAILBOX &mb_num, const FLEXCAN_RXTX &mb_rx_tx, const FLEXCAN_IDE &ide = STD);
	void enableMBInterrupt(const FLEXCAN_MAILBOX &mb_num, bool status = 1);
	void disableMBInterrupt(const FLEXCAN_MAILBOX &mb_num)
	{
		enableMBInterrupt(mb_num, 0);
	}
	void enableMBInterrupts(bool status = 1);
	void disableMBInterrupts()
	{
		enableMBInterrupts(0);
	}
	void setMRP(bool mrp = 1); /* mailbox(1)/fifo(0) priority */
	void setRRS(bool rrs = 1); /* store remote frames */
	void onReceive(const FLEXCAN_MAILBOX &mb_num, _MB_ptr handler); /* individual mailbox callback function */
	void onReceive(_MB_ptr handler); /* global callback function */
	void onTransmit(const FLEXCAN_MAILBOX &mb_num, _MB_ptr handler); /* individual mailbox callback function */
	void onTransmit(_MB_ptr handler); /* global callback function */
	bool setMBUserFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t mask);
	bool setMBUserFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t mask);
	bool setMBUserFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t mask);
	bool setMBUserFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t mask);
	bool setMBManualFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t mask);
	void setMBFilter(FLEXCAN_FLTEN input); /* enable/disable traffic for all MBs (for individual masking) */
	void setMBFilter(FLEXCAN_MAILBOX mb_num, FLEXCAN_FLTEN input); /* set specific MB to accept/deny traffic */
	bool setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1); /* input 1 ID to be filtered */
	bool setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2); /* input 2 ID's to be filtered */
	bool setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3); /* input 3 ID's to be filtered */
	bool setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4); /* input 4 ID's to be filtered */
	bool setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t id5); /* input 5 ID's to be filtered */
	bool setMBFilterRange(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2); /* filter a range of ids */
	int write(const CAN_message_t &msg); /* use any available mailbox for transmitting */
	int write(const CANFD_message_t &msg)
	{
		return 0;
	} /* to satisfy base class for external pointers */
	int write(FLEXCAN_MAILBOX mb_num, const CAN_message_t &msg); /* use a single mailbox for transmitting */
	uint64_t events();
	uint8_t setRFFN(FLEXCAN_RFFN_TABLE rffn = RFFN_8); /* Number Of Rx FIFO Filters (0 == 8 filters, 1 == 16 filters, etc.. */
	uint8_t setRFFN(uint8_t rffn)
	{
		return setRFFN((FLEXCAN_RFFN_TABLE)constrain(rffn, 0, 15));
	}
	void setFIFOFilterTable(FLEXCAN_FIFOTABLE letter);
	bool setFIFOUserFilter(uint8_t filter, uint32_t id1, uint32_t mask, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote = NONE);
	bool setFIFOUserFilter(uint8_t filter, uint32_t id1, uint32_t id2, uint32_t mask, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote = NONE);
	bool setFIFOUserFilter(uint8_t filter, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t mask, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote = NONE);
	bool setFIFOUserFilter(uint8_t filter, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t mask, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote = NONE);
	void setFIFOFilter(const FLEXCAN_FLTEN &input);
	bool setFIFOManualFilter(uint8_t filter, uint32_t id1, uint32_t mask, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote = NONE);
	bool setFIFOFilter(uint8_t filter, uint32_t id1, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote = NONE); /* single ID per filter */
	bool setFIFOFilter(uint8_t filter, uint32_t id1, uint32_t id2, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote = NONE); /* 2 ID's per filter */
	bool setFIFOFilterRange(uint8_t filter, uint32_t id1, uint32_t id2, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote = NONE); /* ID range per filter */
	bool setFIFOFilter(uint8_t filter, uint32_t id1, const FLEXCAN_IDE &ide1, const FLEXCAN_IDE &remote1, uint32_t id2, const FLEXCAN_IDE &ide2, const FLEXCAN_IDE &remote2); /* TableB 2 ID / filter */
	bool setFIFOFilter(uint8_t filter, uint32_t id1, uint32_t id2, const FLEXCAN_IDE &ide1, const FLEXCAN_IDE &remote1, uint32_t id3, uint32_t id4, const FLEXCAN_IDE &ide2, const FLEXCAN_IDE &remote2); /* TableB 4 minimum ID / filter */
	bool setFIFOFilterRange(uint8_t filter, uint32_t id1, uint32_t id2, const FLEXCAN_IDE &ide1, const FLEXCAN_IDE &remote1, uint32_t id3, uint32_t id4, const FLEXCAN_IDE &ide2, const FLEXCAN_IDE &remote2); /* TableB dual range based IDs */
	bool struct2queueTx(const CAN_message_t &msg);
	void struct2queueRx(const CAN_message_t &msg);
#if defined(__IMXRT1062__)
	void setClock(FLEXCAN_CLOCK clock = CLK_24MHz);
#endif
	void enhanceFilter(FLEXCAN_MAILBOX mb_num);
	void distribute(bool state = 1)
	{
		distribution = state;
	}
	void enableDMA(bool state = 1);
	void disableDMA()
	{
		enableDMA(0);
	}
	uint8_t getFirstTxBoxSize()
	{
		return 8;
	}
	void FLEXCAN_ExitFreezeMode();
	void FLEXCAN_EnterFreezeMode();
	bool error(CAN_error_t &error, bool printDetails);
	uint32_t getRXQueueCount()
	{
		return rxBuffer.size();
	}
	uint32_t getTXQueueCount()
	{
		return txBuffer.size();
	}

private:
	void setMBFilterProcessing(FLEXCAN_MAILBOX mb_num, uint32_t filter_id, uint32_t calculated_mask);
	void writeTxMailbox(uint8_t mb_num, const CAN_message_t &msg);
	uint64_t readIMASK(); // { return (((uint64_t)FLEXCANb_IMASK2(_bus) << 32) | FLEXCANb_IMASK1(_bus)); }
	void flexcan_interrupt();
	void flexcanFD_interrupt()
	{
		;
	} // dummy placeholder to satisfy base class
	Circular_Buffer<uint8_t, (uint32_t)_rxSize, sizeof(CAN_message_t)> rxBuffer;
	Circular_Buffer<uint8_t, (uint32_t)_txSize, sizeof(CAN_message_t)> txBuffer;
	Circular_Buffer<uint32_t, 16> busESR1;
	Circular_Buffer<uint16_t, 16> busECR;
	void printErrors(const CAN_error_t &error);
#if defined(__IMXRT1062__)
	uint32_t getClock();
#endif
	volatile uint32_t fifo_filter_table[32][6];
	volatile uint32_t mb_filter_table[64][6];
	volatile bool fifo_filter_match(uint32_t id);
	volatile bool isEventsUsed = 0;
	volatile void frame_distribution(CAN_message_t &msg);
	void filter_store(FLEXCAN_FILTER_TABLE type, FLEXCAN_MAILBOX mb_num, uint32_t id_count, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t id5);
	void fifo_filter_store(FLEXCAN_FILTER_TABLE type, uint8_t filter, uint32_t id_count, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t id5);
	volatile bool filter_match(FLEXCAN_MAILBOX mb_num, uint32_t id);
	volatile bool distribution = 0;
	uint8_t getNumMailBoxes()
	{
		return FLEXCANb_MAXMB_SIZE(_bus);
	}
	uint8_t mailboxOffset();
	void softReset();
	int getFirstTxBox();
	_MB_ptr _mbHandlers[64]; /* individual mailbox handlers */
	_MB_ptr _mainHandler; /* global mailbox handler */
	_MB_ptr _mbTxHandlers[64]; /* individual mailbox tx handlers */
	_MB_ptr _mainTxHandler; /* global mailbox handler */
	uint64_t readIFLAG(); // { return (((uint64_t)FLEXCANb_IFLAG2(_bus) << 32) | FLEXCANb_IFLAG1(_bus)); }
	void writeIFLAG(uint64_t value);
	void writeIFLAGBit(uint8_t mb_num);
	void writeIMASK(uint64_t value);
	void writeIMASKBit(uint8_t mb_num, bool set = 1);
	uint32_t nvicIrq = 0;
	uint32_t currentBitrate = 0UL;
	uint8_t mailbox_reader_increment = 0;
	uint8_t busNumber;
	void mbCallbacks(const FLEXCAN_MAILBOX &mb_num, const CAN_message_t &msg);
};

#include "FlexCAN_T4.tpp"

#if defined(__IMXRT1062__)
#include "FlexCAN_T4FD.tpp"
#include "FlexCAN_T4FDTimings.tpp"
#endif

#endif
