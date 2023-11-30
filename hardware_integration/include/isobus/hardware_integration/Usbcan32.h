/****************************************************************************

  (c) SYS TEC electronic AG, D-08468 Heinsdorfergrund, Am Windrad 2
      www.systec-electronic.com

  Project:      USB-CANmodul

  Description:  Standard header file for the USBCAN32.DLL

  -------------------------------------------------------------------------

                $RCSfile:$

                $Author: R.Dietzsch $

                $Revision: 1.1 $  $Date: 2003/03/18 $

                $State: $

                Build Environment:
                    MSVC 5.0 and MSVC 6.0

  -------------------------------------------------------------------------

  Revision History:

  2003/03/17 r.d.:  Parameter m_dwSerialNr in tUcanHardwareInfo included.
  2003/03/18 r.d.:  New Function UcanInitCanEx() for additional init parameters.
                    UcanGetVersion() returns standard version format.
  2005/10/12 r.d.:  Several new functions included for Multiport CAN-to-USB 3004006
  2006/06/06 r.d.:  - Support of USB-CANmodul1 "Basic" 3204000 or USB-CANmodul2 "Advanced" 3204002
                    - New member variable m_dwProductCode in struct tUcanHardwareInfoEx to find out the hardware type.
                    - New member variables m_wNrOfRxBufferEntries and m_wNrOfTxBufferEntries in struct tUcanInitCanParam to change number of buffer entries.

****************************************************************************/

// clang-format off
//! @cond Doxygen_Suppress

// Protection against mutliple including
#ifndef __USBCAN32_H__
#define __USBCAN32_H__


// allow access to functions for C++ applications as well
#ifdef __cplusplus
extern "C"
{
#endif

#if defined (WIN32) || defined (LINUX) || defined (__linux__)

    // obsolate define
    #ifndef STDCALL
    #define STDCALL __stdcall
    #endif

    // new define
    #ifndef PUBLIC
    #define PUBLIC __stdcall
    #endif

#else

    // obsolate define
    #ifndef STDCALL
    #define STDCALL far pascal
    #endif

    // new define
    #ifndef PUBLIC
    #define PUBLIC far pascal
    #endif

#endif

#if !defined (_TCHAR_DEFINED)
    #if defined (__linux__)
        #define _TCHAR_DEFINED
        #ifdef UNICODE
            typedef wchar_t     _TCHAR;
            #define _T(x)       L##x
        #else
            typedef char        _TCHAR;
            #define _T(x)       x
        #endif
    #else
        #include <tchar.h>
    #endif
#endif


//---------------------------------------------------------------------------
// macro definition
//---------------------------------------------------------------------------

// Filter calculation:

// Extended Frame:
// 33222222 22221111 11111100 00000000 \ (Bits 0-31 of AMR and ACR)
// 10987654 32109876 54321098 76543210 /
// -------- -------- -------- -----    > 29-Bit ID
//                                 -   > RTR-Bit        > GW-002 only
//                                 .-- > unused

// Standard Frame:
// 33222222 22221111 11111100 00000000 \ (Bits 0-31 of AMR and ACR)
// 10987654 32109876 54321098 76543210 /
// -------- ---                        > 11-Bit ID
//             -                       > RTR-Bit        > GW-002 only
//             .---- ........ ........ > unused
//                   --------          > Data byte 0    \ GW-002 only
//                            -------- > Data byte 1    /

// Macros for calculation of vallues for acceptance filter
#define USBCAN_SET_AMR(extended, can_id, rtr) \
    (extended) ?    ((((DWORD)can_id)<<3 )|((rtr)?0x000004:0)|0x00003) : \
                    ((((DWORD)can_id)<<21)|((rtr)?0x100000:0)|0xfffff)
#define USBCAN_SET_ACR(extended, can_id, rtr) \
    (extended) ?    ((((DWORD)can_id)<<3 )|((rtr)?0x000004:0)) : \
                    ((((DWORD)can_id)<<21)|((rtr)?0x100000:0))


//---------------------------------------------------------------------------
//
// Macro:       USBCAN_CALCULATE_AMR(), USBCAN_CALCULATE_ACR()
//
// Description: macros are calculating AMR and ACR using CAN-ID as parameter
//
//              NOTE:   This macros only are working if both from-id and
//                      to-id results a successive order of bits.
//              Example:
//                      from-id: 0x000  to-id:  0x01F   --> macros are working
//                      from-id: 0x400  to-id:  0x4FF   --> macros are working
//                      from-id: 0x101  to-id:  0x202   --> macros are NOT working
//
// A received CAN messages (with CAN-ID = RxID) will be accepted if the following
// condition is fulfilled:
//      ((~(RxID ^ ACR) | AMR) == 0xFFFFFFFF)
//
// Parameters:  extended    = [IN] if TRUE parameters from_id and to_id contains 29-bit CAN-ID
//              from_id     = [IN] first CAN-ID which should be received
//              to_id       = [IN] last CAN-ID which should be received
//              rtr_only    = [IN] if TRUE only RTR-Messages should be received, and rtr_too will be ignored
//              rtr_too     = [IN] if TRUE CAN data frames and RTR-Messages should be received
//
//              NOTE: The parameters rtr_only and rtr_too will be ignored with all the modules of
//                    3rd adn 4th generation.
//
// Return:      DWORD       = AMR or ACR
//
//---------------------------------------------------------------------------

#define USBCAN_CALCULATE_AMR(extended,from_id,to_id,rtr_only,rtr_too) \
    (extended) ?    ((((DWORD)(from_id)^(to_id))<<3 )|((rtr_too&!rtr_only)?0x000004:0)|0x00003) : \
                    ((((DWORD)(from_id)^(to_id))<<21)|((rtr_too&!rtr_only)?0x100000:0)|0xfffff)
#define USBCAN_CALCULATE_ACR(extended,from_id,to_id,rtr_only,rtr_too) \
    (extended) ?    ((((DWORD)(from_id)&(to_id))<<3 )|((rtr_only)?0x000004:0)) : \
                    ((((DWORD)(from_id)&(to_id))<<21)|((rtr_only)?0x100000:0))


//---------------------------------------------------------------------------
//
// Macro:       USBCAN_MAJOR_VER(), USBCAN_MINOR_VER(), USBCAN_RELEASE_VER()
//
// Description: macros to convert the version iformation from functions UcanGetVersionEx() and UcanGetFwVersion()
//
// Parameters:  ver     = [IN] extended version information as unsigned long (32 bit)
//
// Return:      major, minor or release version
//
//---------------------------------------------------------------------------

#define USBCAN_MAJOR_VER(ver)               ( (ver) & 0x000000FF)
#define USBCAN_MINOR_VER(ver)               (((ver) & 0x0000FF00) >> 8)
#define USBCAN_RELEASE_VER(ver)             (((ver) & 0xFFFF0000) >> 16)


//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#if !defined (_WIN32_WCE)

    // maximum number of modules, that are supported (can not be changed!)
    #define USBCAN_MAX_MODULES                  64

    // maximum number of applications, that can make use of this DLL (can not be changed!)
    #define USBCAN_MAX_INSTANCES                64

#else

    // maximum number of modules, that are supported (can not be changed!)
    #define USBCAN_MAX_MODULES                  9

    // maximum number of applications, that can make use of this DLL (can not be changed!)
    #define USBCAN_MAX_INSTANCES                9

#endif

// With the function UcanInitHardware() or UcanInitHardwareEx() the module is used, which
// is detected first. This define should only be used, in case only one module is connected
// to the computer.
#define USBCAN_ANY_MODULE                   255

// no valid USB-CAN Handle
#define USBCAN_INVALID_HANDLE               0xff

// pre-defined baudrate values for GW-001 and GW-002 (use function UcanInitCan() or UcanSetBaudrate())
// fCAN = 8MHz
// --- bit rate values for 1st and 2nd generation (G1/G2) of USB-CANmodul ---
#define USBCAN_BAUD_1MBit                   0x0014              // = 1000 kBit/s
#define USBCAN_BAUD_800kBit                 0x0016              // =  800 kBit/s
#define USBCAN_BAUD_500kBit                 0x001c              // =  500 kBit/s
#define USBCAN_BAUD_250kBit                 0x011c              // =  250 kBit/s
#define USBCAN_BAUD_125kBit                 0x031c              // =  125 kBit/s
#define USBCAN_BAUD_100kBit                 0x432f              // =  100 kBit/s
#define USBCAN_BAUD_50kBit                  0x472f              // =   50 kBit/s
#define USBCAN_BAUD_20kBit                  0x532f              // =   20 kBit/s
#define USBCAN_BAUD_10kBit                  0x672f              // =   10 kBit/s
//
// other bitrates:
// -  83,33 bit/sec, sample point 83,33% =  0x6f03
// - 307,69 bit/sec, sample point 84,62% =  0x1901

#define USBCAN_BAUD_USE_BTREX               0x0000              // uses predefined extended values of baudrate for
                                                                // Multiport 3004006, USB-CANmodul1 3204000 or USB-CANmodul2 3204002
                                                                // (do not use for GW-001/002)
#define USBCAN_BAUD_AUTO                    0xFFFF              // automatic baudrate detection (not implemented in this version)

// pre-defined baudrate values for Multiport 3004006, USB-CANmodul1 3204000 or USB-CANmodul2 3204002 (use function UcanInitCanEx or UcanSetBaudrateEx)
// fCAN = 48MHz (for 10kBit/sec fCAN = 24MHz)
// --- bit rate values for 3rd generation (G3) of USB-CANmodul ---
#define USBCAN_BAUDEX_1MBit                 0x00020354          // = 1000 kBit/s    Sample Point: 68,75%
#define USBCAN_BAUDEX_800kBit               0x00030254          // =  800 kBit/s    Sample Point: 66,67%
#define USBCAN_BAUDEX_500kBit               0x00050354          // =  500 kBit/s    Sample Point: 68,75%
#define USBCAN_BAUDEX_250kBit               0x000B0354          // =  250 kBit/s    Sample Point: 68,75%
#define USBCAN_BAUDEX_125kBit               0x00170354          // =  125 kBit/s    Sample Point: 68,75%
#define USBCAN_BAUDEX_100kBit               0x00171466          // =  100 kBit/s    Sample Point: 65,00%
#define USBCAN_BAUDEX_50kBit                0x002F1466          // =   50 kBit/s    Sample Point: 65,00%
#define USBCAN_BAUDEX_20kBit                0x00771466          // =   20 kBit/s    Sample Point: 65,00%
#define USBCAN_BAUDEX_10kBit                0x80771466          // =   10 kBit/s    Sample Point: 65,00% (CLK = 1, see L-487 since version 15)

// pre-defined baudrate values for Multiport 3004006, USB-CANmodul1 3204000 or USB-CANmodul2 3204002 (use function UcanInitCanEx or UcanSetBaudrateEx)
// fCAN = 48MHz (for 10kBit/sec fCAN = 24MHz)
// --- bit rate values for 3rd generation (G3) of USB-CANmodul ---
#define USBCAN_BAUDEX_SP2_1MBit             0x00020741          // = 1000 kBit/s    Sample Point: 87,50%
#define USBCAN_BAUDEX_SP2_800kBit           0x00030731          // =  800 kBit/s    Sample Point: 86,67%
#define USBCAN_BAUDEX_SP2_500kBit           0x00050741          // =  500 kBit/s    Sample Point: 87,50%
#define USBCAN_BAUDEX_SP2_250kBit           0x000B0741          // =  250 kBit/s    Sample Point: 87,50%
#define USBCAN_BAUDEX_SP2_125kBit           0x00170741          // =  125 kBit/s    Sample Point: 87,50%
#define USBCAN_BAUDEX_SP2_100kBit           0x001D1741          // =  100 kBit/s    Sample Point: 87,50%
#define USBCAN_BAUDEX_SP2_50kBit            0x003B1741          // =   50 kBit/s    Sample Point: 87,50%
#define USBCAN_BAUDEX_SP2_20kBit            0x00771772          // =   20 kBit/s    Sample Point: 85,00%
#define USBCAN_BAUDEX_SP2_10kBit            0x80771772          // =   10 kBit/s    Sample Point: 85,00% (CLK = 1, see L-487 since version 15)
//
// other bitrates:
// -  33,333 kBit/sec, sample point 88,89% =  0x004F1671
// -  83,333 kBit/sec, sample point 88,89% =  0x001F1671
// - 307,692 kBit/sec, sample point 84,62% =  0x000B0441
// - 615,384 kBit/sec, sample point 84,62% =  0x00050441
// - 666,667 kBit/sec, sample point 88,89% =  0x00030671

// pre-defined baudrate values for 4th generation of USB-CANmodul series (use function UcanInitCanEx or UcanSetBaudrateEx)
// fCAN = 24MHz (used when 25% higher performance is deactivated)
// --- bit rate values for 4th generation (G4) of USB-CANmodul ---
#define USBCAN_BAUDEX_G4_1MBit              0x40180001          // = 1000 kBit/s    Sample Point: 83,33%
#define USBCAN_BAUDEX_G4_800kBit            0x401B0001          // =  800 kBit/s    Sample Point: 86,67%
#define USBCAN_BAUDEX_G4_500kBit            0x401C0002          // =  500 kBit/s    Sample Point: 87,50%
#define USBCAN_BAUDEX_G4_250kBit            0x401C0005          // =  250 kBit/s    Sample Point: 87,50%
#define USBCAN_BAUDEX_G4_125kBit            0x401C000B          // =  125 kBit/s    Sample Point: 87,50%
#define USBCAN_BAUDEX_G4_100kBit            0x412F000B          // =  100 kBit/s    Sample Point: 85,00%
#define USBCAN_BAUDEX_G4_50kBit             0x412F0017          // =   50 kBit/s    Sample Point: 85,00%
#define USBCAN_BAUDEX_G4_20kBit             0x412F003B          // =   20 kBit/s    Sample Point: 85,00%
#define USBCAN_BAUDEX_G4_10kBit             0x412F0077          // =   10 kBit/s    Sample Point: 85,00%
//
// other bitrates:
// -  33,333 kBit/sec, sample point 85,00% =  0x412F0023
// -  83,333 kBit/sec, sample point 88,9%  =  0x411E000F
// - 307,692 kBit/sec, sample point 84,62% =  0x40190005
// - 615,384 kBit/sec, sample point 84,62% =  0x40190002
// - 666,667 kBit/sec, sample point 83,33% =  0x402D0001
//
// fCAN = 30MHz (used when 25% higher performance is activated)
// --- bit rate values for 4th generation (G4) of USB-CANmodul ---
#define USBCAN_BAUDEX_G4X_1MBit             0xC01B0001          // = 1000 kBit/s    Sample Point: 86,67%
//#define USBCAN_BAUDEX_G4X_800kBit     /* not supported */     // =  800 kBit/s
#define USBCAN_BAUDEX_G4X_500kBit           0xC02F0002          // =  500 kBit/s    Sample Point: 85,00%
#define USBCAN_BAUDEX_G4X_250kBit           0xC02F0005          // =  250 kBit/s    Sample Point: 85,00%
#define USBCAN_BAUDEX_G4X_125kBit           0xC02F000B          // =  125 kBit/s    Sample Point: 85,00%
#define USBCAN_BAUDEX_G4X_100kBit           0xC12F000E          // =  100 kBit/s    Sample Point: 85,00%
#define USBCAN_BAUDEX_G4X_50kBit            0xC12F001D          // =   50 kBit/s    Sample Point: 85,00%
#define USBCAN_BAUDEX_G4X_20kBit            0xC12F004A          // =   20 kBit/s    Sample Point: 85,00%
#define USBCAN_BAUDEX_G4X_10kBit            0xC12F0095          // =   10 kBit/s    Sample Point: 85,00%
//
// other bitrates:
// -  33,333 kBit/sec, sample point 85,00% =  0xC02F002C
// -  83,333 kBit/sec, sample point 85,00% =  0xC02F0011
// - 307,692 kBit/sec,    not supported
// - 615,384 kBit/sec,    not supported
// - 666,667 kBit/sec, sample point 86,67% =  0xC01B0002

#define USBCAN_BAUDEX_USE_BTR01             0x00000000          // uses predefined values of BTR0/BTR1 for GW-001/002
#define USBCAN_BAUDEX_AUTO                  0xFFFFFFFF          // automatic baudrate detection (not implemented in this version)

// Frame format for a CAN message (bit oriented)
#define USBCAN_MSG_FF_STD                   0x00                // Standard Frame (11-Bit-ID)
#define USBCAN_MSG_FF_ECHO                  0x20                // Tx echo (message received from UcanReadCanMsg.. was previously sent by UcanWriteCanMsg..)
#define USBCAN_MSG_FF_RTR                   0x40                // Remote Transmition Request Frame
#define USBCAN_MSG_FF_EXT                   0x80                // Extended Frame (29-Bit-ID)

// Function return codes (encoding)
#define USBCAN_SUCCESSFUL                   0x00                // no error
#define USBCAN_ERR                          0x01                // error in library; function has not been executed
#define USBCAN_ERRCMD                       0x40                // error in module; function has not been executed
#define USBCAN_WARNING                      0x80                // Warning; function has been executed anyway
#define USBCAN_RESERVED                     0xc0                // reserved return codes (up to 255)

// Error messages, that can occur in the library
#define USBCAN_ERR_RESOURCE                 0x01                // could not created a resource (memory, Handle, ...)
#define USBCAN_ERR_MAXMODULES               0x02                // the maximum number of open modules is exceeded
#define USBCAN_ERR_HWINUSE                  0x03                // a module is already in use
#define USBCAN_ERR_ILLVERSION               0x04                // the software versions of the module and library are incompatible
#define USBCAN_ERR_ILLHW                    0x05                // the module with the corresponding device number is not connected
#define USBCAN_ERR_ILLHANDLE                0x06                // wrong USB-CAN-Handle handed over to the function
#define USBCAN_ERR_ILLPARAM                 0x07                // wrong parameter handed over to the function
#define USBCAN_ERR_BUSY                     0x08                // instruction can not be processed at this time
#define USBCAN_ERR_TIMEOUT                  0x09                // no answer from the module
#define USBCAN_ERR_IOFAILED                 0x0a                // a request for the driver failed
#define USBCAN_ERR_DLL_TXFULL               0x0b                // the message did not fit into the transmission queue
#define USBCAN_ERR_MAXINSTANCES             0x0c                // maximum number of applications is reached
#define USBCAN_ERR_CANNOTINIT               0x0d                // CAN-interface is not yet initialized
#define USBCAN_ERR_DISCONNECT               0x0e                // USB-CANmodul was disconnected
#define USBCAN_ERR_DISCONECT            USBCAN_ERR_DISCONNECT   // renamed (still defined for compatibility reason)
#define USBCAN_ERR_NOHWCLASS                0x0f                // the needed device class does not exist
#define USBCAN_ERR_ILLCHANNEL               0x10                // illegal CAN channel for GW-001/GW-002
#define USBCAN_ERR_RESERVED1                0x11
#define USBCAN_ERR_ILLHWTYPE                0x12                // the API function can not be used with this hardware
#define USBCAN_ERR_SERVER_TIMEOUT           0x13                // the command server does not send an reply of an command

// Error messages, that the module returns during the command sequence
#define USBCAN_ERRCMD_NOTEQU                0x40                // the received response does not match with the transmitted command
#define USBCAN_ERRCMD_REGTST                0x41                // no access to the CAN controler possible
#define USBCAN_ERRCMD_ILLCMD                0x42                // the module could not interpret the command
#define USBCAN_ERRCMD_EEPROM                0x43                // error while reading the EEPROM occured
#define USBCAN_ERRCMD_RESERVED1             0x44
#define USBCAN_ERRCMD_RESERVED2             0x45
#define USBCAN_ERRCMD_RESERVED3             0x46
#define USBCAN_ERRCMD_ILLBDR                0x47                // illegal baudrate values for Multiport 3004006, USB-CANmodul1 3204000 or USB-CANmodul2 3204002 in BTR0/BTR1
#define USBCAN_ERRCMD_NOTINIT               0x48                // CAN channel was not initialized
#define USBCAN_ERRCMD_ALREADYINIT           0x49                // CAN channel was already initialized
#define USBCAN_ERRCMD_ILLSUBCMD             0x4A                // illegal sub-command specified
#define USBCAN_ERRCMD_ILLIDX                0x4B                // illegal index specified (e.g. index for cyclic CAN message)
#define USBCAN_ERRCMD_RUNNING               0x4C                // cyclic CAN message(s) can not be defined because transmission of cyclic CAN messages is already running

// Warning messages, that can occur in library
// NOTE: These messages are only warnings. The function has been executed anyway.
#define USBCAN_WARN_NODATA                  0x80                // no CAN messages received
#define USBCAN_WARN_SYS_RXOVERRUN           0x81                // overrun in the receive queue of the driver (but this CAN message is successfuly read)
#define USBCAN_WARN_DLL_RXOVERRUN           0x82                // overrun in the receive queue of the library (but this CAN message is successfuly read)
#define USBCAN_WARN_RESERVED1               0x83
#define USBCAN_WARN_RESERVED2               0x84
#define USBCAN_WARN_FW_TXOVERRUN            0x85                // overrun in the transmit queue of the firmware (but this CAN message was successfully stored in buffer)
#define USBCAN_WARN_FW_RXOVERRUN            0x86                // overrun in the receive queue of the firmware (but this CAN message was successfully read)
#define USBCAN_WARN_FW_TXMSGLOST            0x87                // (not implemented yet)
#define USBCAN_WARN_NULL_PTR                0x90                // pointer to address is NULL (function will not work correctly)
#define USBCAN_WARN_TXLIMIT                 0x91                // function UcanWriteCanMsgEx() was called for sending more CAN messages than one
                                                                //      But not all of them could be sent because the buffer is full.
//                                                              //      Variable pointed by pdwCount_p received the number of succeddfully sent CAN messages.
#define USBCAN_WARN_BUSY                    0x92                // function cannot be processed because the DLL is busy
#define USBCAN_WARN_CONFIG                  0x93                // function not processed because configuration

// system errors
//#define USBCAN_ERR_ABORT                    0xC0
//#define USBCAN_ERR_DATA                     0xC1

// macros to check the error code
#define USBCAN_CHECK_VALID_RXCANMSG(ret) \
    ((ret == USBCAN_SUCCESSFUL) || (ret > USBCAN_WARNING))      // checks if function UcanReadCanMsg..() returns a valid CAN message

#define USBCAN_CHECK_TX_OK(ret) \
    ((ret == USBCAN_SUCCESSFUL) || (ret > USBCAN_WARNING))      // checks if function UcanWriteCanMsg..() successfuly wrote CAN message(s)
                                                                //      While using UcanWriteCanMsgEx() the number of sent CAN messages can be less than
                                                                //      the numer of CAN messages which shold be sent. (see also USBCAN_WARN_TXLIMIT)
#define USBCAN_CHECK_TX_SUCCESS(ret) \
    (ret == USBCAN_SUCCESSFUL)                                  // checks if function UcanWriteCanMsgEx() successfuly wrote all CAN message(s)

#define USBCAN_CHECK_TX_NOTALL(ret) \
    (ret == USBCAN_WARN_TXLIMIT)                                // checks if function UcanWriteCanMsgEx() did not sent all CAN messages

#define USBCAN_CHECK_WARNING(ret) \
    (ret >= USBCAN_WARNING)                                     // checks if any function returns a warning

#define USBCAN_CHECK_ERROR(ret) \
    ((ret != USBCAN_SUCCESSFUL) && (ret < USBCAN_WARNING))      // checks if any function returns an error

#define USBCAN_CHECK_ERROR_CMD(ret) \
    ((ret >= USBCAN_ERRCMD) && (ret < USBCAN_WARNING))          // checks if any function returns an error from firmware in USB-CANmodul


// The Callback function is called, if certain events did occur.
// These Defines specify the event.
#define USBCAN_EVENT_INITHW                 0                   // the USB-CANmodul has been initialized
#define USBCAN_EVENT_INITCAN                1                   // the CAN interface has been initialized
#define USBCAN_EVENT_RECIEVE                2                   // a new CAN message has been received (for compatibility reason)
#define USBCAN_EVENT_RECEIVE                2                   // a new CAN message has been received
#define USBCAN_EVENT_STATUS                 3                   // the error state in the module has changed
#define USBCAN_EVENT_DEINITCAN              4                   // the CAN interface has been deinitialized (UcanDeinitCan() was called)
#define USBCAN_EVENT_DEINITHW               5                   // the USB-CANmodul has been deinitialized (UcanDeinitHardware() was called)
#define USBCAN_EVENT_CONNECT                6                   // a new USB-CANmodul has been connected
#define USBCAN_EVENT_DISCONNECT             7                   // a USB-CANmodul has been disconnected
#define USBCAN_EVENT_FATALDISCON            8                   // a USB-CANmodul has been disconnected during operation
#define USBCAN_EVENT_USBBUS_ERROR           16                  // An USB bus error has occurred (e.g. STALL or XACT)
#define USBCAN_EVENT_RECONNECT              17                  // The USB-CANmodul was automatically reconnected
#define USBCAN_EVENT_RESERVED1              0x80

// CAN status flags (is returned with function UcanGetStatus() or UcanGetStatusEx() )
#define USBCAN_CANERR_OK                    0x0000              // no error
#define USBCAN_CANERR_XMTFULL               0x0001              // Tx-buffer of the CAN controller is full
#define USBCAN_CANERR_OVERRUN               0x0002              // Rx-buffer of the CAN controller is full
#define USBCAN_CANERR_BUSLIGHT              0x0004              // Bus error: Error Limit 1 exceeded (refer to SJA1000 manual)
#define USBCAN_CANERR_BUSHEAVY              0x0008              // Bus error: Error Limit 2 exceeded (refer to SJA1000 manual)
#define USBCAN_CANERR_BUSOFF                0x0010              // Bus error: CAN controllerhas gone into Bus-Off state
#define USBCAN_CANERR_QRCVEMPTY             0x0020              // RcvQueue is empty
#define USBCAN_CANERR_QOVERRUN              0x0040              // RcvQueue overrun
#define USBCAN_CANERR_QXMTFULL              0x0080              // transmit queue is full
#define USBCAN_CANERR_REGTEST               0x0100              // Register test of the SJA1000 failed
#define USBCAN_CANERR_MEMTEST               0x0200              // Memory test failed
#define USBCAN_CANERR_TXMSGLOST             0x0400              // transmit CAN message was automatically deleted by firmware
                                                                // (because transmit timeout run over)

// USB error messages (is returned with UcanGetStatus..() )
#define USBCAN_USBERR_OK                    0x0000              // no error
#define USBCAN_USBERR_STATUS_TIMEOUT        0x2000              // restet because status timeout (device reconnected via USB)
#define USBCAN_USBERR_WATCHDOG_TIMEOUT      0x4000              // restet because watchdog timeout (device reconnected via USB)

// ABR and ACR for mode "receive all CAN messages"
#define USBCAN_AMR_ALL                      (DWORD) 0xffffffff
#define USBCAN_ACR_ALL                      (DWORD) 0x00000000

#define USBCAN_OCR_DEFAULT                  0x1A                // default OCR for standard GW-002
#define USBCAN_OCR_RS485_ISOLATED           0x1E                // OCR for RS485 interface and galvanic isolation
#define USBCAN_OCR_RS485_NOT_ISOLATED       0x0A                // OCR for RS485 interface without galvanic isolation
#define USBCAN_DEFAULT_BUFFER_ENTRIES       4096

// definitions for CAN channels
#define USBCAN_CHANNEL_CH0                  0
#define USBCAN_CHANNEL_CH1                  1
#define USBCAN_CHANNEL_ANY                  255                 // only available for functions UcanCallbackFktEx, UcanReadCanMsgEx
#define USBCAN_CHANNEL_ALL                  254                 // reserved for futer use
#define USBCAN_CHANNEL_NO                   253                 // reserved for futer use
#define USBCAN_CHANNEL_CAN1                 USBCAN_CHANNEL_CH0  // differences between software and label at hardware
#define USBCAN_CHANNEL_CAN2                 USBCAN_CHANNEL_CH1  // differences between software and label at hardware
#define USBCAN_CHANNEL_LIN                  USBCAN_CHANNEL_CH1  // reserved for futer use

// definitions for function UcanResetCanEx()    (for compatibility reason these bits are inverted)
#define USBCAN_RESET_ALL                    0x00000000          // reset everything
#define USBCAN_RESET_NO_STATUS              0x00000001          // no CAN status reset  (only supported in new devices - not GW-001/002)
#define USBCAN_RESET_NO_CANCTRL             0x00000002          // no CAN controller reset
#define USBCAN_RESET_NO_TXCOUNTER           0x00000004          // no TX message counter reset
#define USBCAN_RESET_NO_RXCOUNTER           0x00000008          // no RX message counter reset
#define USBCAN_RESET_NO_TXBUFFER_CH         0x00000010          // no TX message buffer reset at channel level
#define USBCAN_RESET_NO_TXBUFFER_DLL        0x00000020          // no TX message buffer reset at USBCAN32.DLL level
#define USBCAN_RESET_NO_TXBUFFER_FW         0x00000080          // no TX message buffer reset at firmware level
#define USBCAN_RESET_NO_RXBUFFER_CH         0x00000100          // no RX message buffer reset at channel level
#define USBCAN_RESET_NO_RXBUFFER_DLL        0x00000200          // no RX message buffer reset at USBCAN32.DLL level
#define USBCAN_RESET_NO_RXBUFFER_SYS        0x00000400          // no RX message buffer reset at kernel driver level
#define USBCAN_RESET_NO_RXBUFFER_FW         0x00000800          // no RX message buffer reset at firmware level
#define USBCAN_RESET_FIRMWARE               0xFFFFFFFF          // buffers, counters and CANCRTL will be reseted indirectly during firmware reset
                                                                //      (means automatically disconnect from USB port in 500ms)

// combinations of flags for UcanResetCanEx()
//      NOTE: for combinations use OR (example: USBCAN_RESET_NO_COUNTER_ALL | USBCAN_RESET_NO_BUFFER_ALL)
#define USBCAN_RESET_NO_COUNTER_ALL         (USBCAN_RESET_NO_TXCOUNTER     | USBCAN_RESET_NO_RXCOUNTER)
#define USBCAN_RESET_NO_TXBUFFER_COMM       (USBCAN_RESET_NO_TXBUFFER_DLL  | 0x00000040                    | USBCAN_RESET_NO_TXBUFFER_FW)
#define USBCAN_RESET_NO_RXBUFFER_COMM       (USBCAN_RESET_NO_RXBUFFER_DLL  | USBCAN_RESET_NO_RXBUFFER_SYS  | USBCAN_RESET_NO_RXBUFFER_FW)
#define USBCAN_RESET_NO_TXBUFFER_ALL        (USBCAN_RESET_NO_TXBUFFER_CH   | USBCAN_RESET_NO_TXBUFFER_COMM)
#define USBCAN_RESET_NO_RXBUFFER_ALL        (USBCAN_RESET_NO_RXBUFFER_CH   | USBCAN_RESET_NO_RXBUFFER_COMM)
#define USBCAN_RESET_NO_BUFFER_COMM         (USBCAN_RESET_NO_TXBUFFER_COMM | USBCAN_RESET_NO_RXBUFFER_COMM)
#define USBCAN_RESET_NO_BUFFER_ALL          (USBCAN_RESET_NO_TXBUFFER_ALL  | USBCAN_RESET_NO_RXBUFFER_ALL)
//      NOTE: for combinations use AND instead of OR (example: USBCAN_RESET_ONLY_RX_BUFF & USBCAN_RESET_ONLY_STATUS)
#define USBCAN_RESET_ONLY_STATUS            (0x0000FFFF & ~(USBCAN_RESET_NO_STATUS))
#define USBCAN_RESET_ONLY_CANCTRL           (0x0000FFFF & ~(USBCAN_RESET_NO_CANCTRL))
#define USBCAN_RESET_ONLY_TXBUFFER_FW       (0x0000FFFF & ~(USBCAN_RESET_NO_TXBUFFER_FW))
#define USBCAN_RESET_ONLY_RXBUFFER_FW       (0x0000FFFF & ~(USBCAN_RESET_NO_RXBUFFER_FW))
#define USBCAN_RESET_ONLY_RXCHANNEL_BUFF    (0x0000FFFF & ~(USBCAN_RESET_NO_RXBUFFER_CH))
#define USBCAN_RESET_ONLY_TXCHANNEL_BUFF    (0x0000FFFF & ~(USBCAN_RESET_NO_TXBUFFER_CH))
#define USBCAN_RESET_ONLY_RX_BUFF           (0x0000FFFF & ~(USBCAN_RESET_NO_RXBUFFER_ALL | USBCAN_RESET_NO_RXCOUNTER))
#define USBCAN_RESET_ONLY_RX_BUFF_GW002     (0x0000FFFF & ~(USBCAN_RESET_NO_RXBUFFER_ALL | USBCAN_RESET_NO_RXCOUNTER | USBCAN_RESET_NO_TXBUFFER_FW))
#define USBCAN_RESET_ONLY_TX_BUFF           (0x0000FFFF & ~(USBCAN_RESET_NO_TXBUFFER_ALL | USBCAN_RESET_NO_TXCOUNTER))
#define USBCAN_RESET_ONLY_ALL_BUFF          (USBCAN_RESET_ONLY_RX_BUFF & USBCAN_RESET_ONLY_TX_BUFF)
#define USBCAN_RESET_ONLY_ALL_COUNTER       (0x0000FFFF & ~(USBCAN_RESET_NO_COUNTER_ALL))


// Meaning of suffixes of defines USBCAN_RESET_NO_..BUFFER_.. (used for function UcanResetCanEx() to reset buffers or not)
//
//              CAN-            Firmware-       Kernel-         USBCAN32.DLL    USBCAN32.DLL
//              Controller:     Buffer          Buffer          Buffer          Channel-Buffer
//----------------------------------------------------------------------------------------------------------------------------
// Suffix:                      .._FW           .._SYS          .._DLL         .._CH
//   .._COMM =                 {~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~}
//   .._ALL  =                 {~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~}
//--------------RX buffers----------------------------------------------------------------------------------------------------
// devices:
//
// GW-001:      +-----+                                                         +---------+
// GW-002:      | CH0 |                                                         |||||||||||     +--------------------+
// sysWORXX:    +-----+ ----->  +---------+     +---------+     +---------+ --> +---------+ --> | UcanReadCanMsg..() |
//                              ||||||||||| --> ||||||||||| --> |||||||||||                     |                    |
//              ....... ----->  +---------+     +---------+     +---------+ --> +---------+ --> | UcanReadCanMsgEx() |
// sysWORXX:    . CH1 .                                                         |||||||||||     +--------------------+
//              .......                                                         +---------+
//
// Nr. of messages in buffer:   768             4096            4096            (tUcanInitCanParam.m_wNrOfRxBufferEntries)
//--------------TX buffers----------------------------------------------------------------------------------------------------
// devices:
//
// GW-001:      +-----+
// GW-002:      | CH0 |                                                                         +--------------------+
// sysWORXX:    +-----+ <-----  +---------+                     +---------+ <------------------ | UcanWriteCanMsg..()|
//                              ||||||||||| <------------------ |||||||||||                     |                    |
//              ....... <-----  +---------+                     +---------+ <------------------ | UcanWriteCanMsgEx()|
// sysWORXX:    . CH1 .                                                                         +--------------------+
//              .......
//
// Nr. of messages in buffer:   768                             (tUcanInitCanParam.m_wNrOfRxBufferEntries)
//----------------------------------------------------------------------------------------------------------------------------
//
// NOTE:
// If Rx-Buffers of Firmware, Kernel-Mode Driver and/or Rx-Buffer of DLL (suffix .._DLL) will be reseted,
// then CAN messages of both CAN-Channels (CH0 and CH1) may be deleted (lost for application).
//
// If UcanResetCanEx() is called with dwResetFlags_p = USBCAN_RESET_NO_RXBUFFER_COMM (for example), then
// only Rx-Buffer of CAN-Channel 0 or 1 will be deleted (as defined in parameter bChannel_p).

// definitions for product code in structure tUcanHardwareInfoEx
#define USBCAN_PRODCODE_MASK_DID            0xFFFF0000L
#define USBCAN_PRODCODE_MASK_MFU            0x00008000L
#define USBCAN_PRODCODE_PID_TWO_CHA         0x00000001L
#define USBCAN_PRODCODE_PID_TERM            0x00000001L
#define USBCAN_PRODCODE_PID_RBUSER          0x00000001L
#define USBCAN_PRODCODE_PID_RBCAN           0x00000001L
#define USBCAN_PRODCODE_PID_G4              0x00000020L
#define USBCAN_PRODCODE_PID_RESVD           0x00000040L
#define USBCAN_PRODCODE_MASK_PID            0x00007FFFL
#define USBCAN_PRODCODE_MASK_PIDG3          (USBCAN_PRODCODE_MASK_PID & ~USBCAN_PRODCODE_PID_RESVD)
#define USBCAN_PRODCODE_MASK_PIDG4          (USBCAN_PRODCODE_MASK_PID & ~USBCAN_PRODCODE_PID_RESVD)

#define USBCAN_PRODCODE_PID_GW001           0x00001100L     // order code GW-001 "USB-CANmodul" outdated
#define USBCAN_PRODCODE_PID_GW002           0x00001102L     // order code GW-002 "USB-CANmodul" outdated
#define USBCAN_PRODCODE_PID_MULTIPORT       0x00001103L     // order code 3004006/3404000/3404001 "Multiport CAN-to-USB"
#define USBCAN_PRODCODE_PID_BASIC           0x00001104L     // order code 3204000/3204001 "USB-CANmodul1"
#define USBCAN_PRODCODE_PID_ADVANCED        0x00001105L     // order code 3204002/3204003 "USB-CANmodul2"
#define USBCAN_PRODCODE_PID_USBCAN8         0x00001107L     // order code 3404000 "USB-CANmodul8"
#define USBCAN_PRODCODE_PID_USBCAN16        0x00001109L     // order code 3404001 "USB-CANmodul16"
#define USBCAN_PRODCODE_PID_RESERVED3       0x00001110L
#define USBCAN_PRODCODE_PID_ADVANCED_G4     0x00001121L     // order code ------- "USB-CANmodul2" 4th generation
#define USBCAN_PRODCODE_PID_BASIC_G4        0x00001122L     // order code 3204000 "USB-CANmodul1" 4th generation
#define USBCAN_PRODCODE_PID_USBCAN8_G4      0x00001123L     // order code 3404000 "USB-CANmodul8"
#define USBCAN_PRODCODE_PID_USBCAN16_G4     0x00001125L     // order code 3404001 "USB-CANmodul16"
#define USBCAN_PRODCODE_PID_RESERVED1       0x00001144L
#define USBCAN_PRODCODE_PID_RESERVED2       0x00001145L
#define USBCAN_PRODCODE_PID_RESERVED4       0x00001162L


//---------------------------------------------------------------------------
//
// Macro:       USBCAN_CHECK_SUPPORT_CYCLIC_MSG()
//              USBCAN_CHECK_SUPPORT_TWO_CHANNEL()
//              USBCAN_CHECK_SUPPORT_TERM_RESISTOR()
//              USBCAN_CHECK_SUPPORT_USER_PORT()
//              USBCAN_CHECK_SUPPORT_RBUSER_PORT()
//              USBCAN_CHECK_SUPPORT_RBCAN_PORT()
//              USBCAN_CHECK_IS_SYSWORXX()
//              USBCAN_CHECK_SUPPORT_UCANNET()
//
// Description: macros to check if an USB-CANmodul does support different features
//
// Parameters:  pHwInfoEx   = [IN] pointer to the extended hardware information structure
//                                  (returned by UcanGetHardwareInfoEx2()
//
// Return:      BOOL        = TRUE if hardware does support the feature
//
//---------------------------------------------------------------------------

// checks if the module is a sysWORXX USB-CANmodul
#define USBCAN_CHECK_IS_SYSWORXX(pHwInfoEx) \
    (((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_MASK_PID) >= USBCAN_PRODCODE_PID_MULTIPORT) && \
    (((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_MASK_PID) != USBCAN_PRODCODE_PID_RESERVED3)

// checks if the module is a USB-CANmodul G4
#define USBCAN_CHECK_IS_G4(pHwInfoEx) \
    (((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_PID_G4) != 0)

// checks if the module is a USB-CANmodul G3
#define USBCAN_CHECK_IS_G3(pHwInfoEx) \
    (USBCAN_CHECK_IS_SYSWORXX(pHwInfoEx) && !USBCAN_CHECK_IS_G4(pHwInfoEx))

// checks if the module is a USB-CANmodul G2 (GW-002)
#define USBCAN_CHECK_IS_G2(pHwInfoEx) \
    ((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_MASK_PID) == USBCAN_PRODCODE_PID_GW002)

// checks if the module is a USB-CANmodul G1 (GW-001)
#define USBCAN_CHECK_IS_G1(pHwInfoEx) \
    ((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_MASK_PID) == USBCAN_PRODCODE_PID_GW001)

// checks if the module supports automatically transmission of cyclic CAN messages
#define USBCAN_CHECK_SUPPORT_CYCLIC_MSG(pHwInfoEx) \
    (USBCAN_CHECK_IS_SYSWORXX(pHwInfoEx) && \
     (USBCAN_MAJOR_VER((pHwInfoEx)->m_dwFwVersionEx) >  3) || (                                      \
     (USBCAN_MAJOR_VER((pHwInfoEx)->m_dwFwVersionEx) == 3)                                        && \
     (USBCAN_MINOR_VER((pHwInfoEx)->m_dwFwVersionEx) >= 6)    )                                      )

// checks if the module supports two CAN channels (at logical device)
#define USBCAN_CHECK_SUPPORT_TWO_CHANNEL(pHwInfoEx) \
    ((((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_MASK_PID) >= USBCAN_PRODCODE_PID_MULTIPORT) && \
     (((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_PID_TWO_CHA) != 0)                             )

// checks if the module supports a termination resistor
#define USBCAN_CHECK_SUPPORT_TERM_RESISTOR(pHwInfoEx) \
    ((((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_PID_TERM) != 0))

// checks if the module supports a user I/O port
#define USBCAN_CHECK_SUPPORT_USER_PORT(pHwInfoEx) \
    ((((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_MASK_PID) != USBCAN_PRODCODE_PID_GW001) && \
     (((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_MASK_PID) != USBCAN_PRODCODE_PID_BASIC) && \
     (((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_MASK_PID) != USBCAN_PRODCODE_PID_RESERVED3) && \
     (((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_MASK_PID) != USBCAN_PRODCODE_PID_BASIC_G4) && \
     (((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_MASK_PID) != USBCAN_PRODCODE_PID_RESERVED1) && \
     (((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_MASK_PID) != USBCAN_PRODCODE_PID_RESERVED4) && \
     ( (USBCAN_MAJOR_VER ((pHwInfoEx)->m_dwFwVersionEx) >  2)   || \
      ((USBCAN_MAJOR_VER ((pHwInfoEx)->m_dwFwVersionEx) == 2)   && \
       (USBCAN_MINOR_VER ((pHwInfoEx)->m_dwFwVersionEx) >= 16))    )   )

// checks if the module supports a user I/O port including read back feature
#define USBCAN_CHECK_SUPPORT_RBUSER_PORT(pHwInfoEx) \
    ((((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_PID_RBUSER) != 0))

// checks if the module supports a CAN I/O port including read back feature
#define USBCAN_CHECK_SUPPORT_RBCAN_PORT(pHwInfoEx) \
    ((((pHwInfoEx)->m_dwProductCode & USBCAN_PRODCODE_PID_RBCAN) != 0))

// checks if the module supports the usage of USB-CANnetwork driver
#define USBCAN_CHECK_SUPPORT_UCANNET(pHwInfoEx) \
    (USBCAN_CHECK_IS_SYSWORXX(pHwInfoEx)                       && \
    (USBCAN_MAJOR_VER((pHwInfoEx)->m_dwFwVersionEx) >  3) || (    \
    (USBCAN_MAJOR_VER((pHwInfoEx)->m_dwFwVersionEx) == 3)      && \
    (USBCAN_MINOR_VER((pHwInfoEx)->m_dwFwVersionEx) >= 8)    )    )


//---------------------------------------------------------------------------
// definitions for cyclic CAN messages
#define USBCAN_MAX_CYCLIC_CANMSG            16

// stopps the transmission of cyclic CAN messages (use instead of USBCAN_CYCLIC_FLAG_START)
#define USBCAN_CYCLIC_FLAG_STOPP            0x00000000L

// the following flags can be cobined
#define USBCAN_CYCLIC_FLAG_START            0x80000000L     // global enable of transmission of cyclic CAN messages
#define USBCAN_CYCLIC_FLAG_SEQUMODE         0x40000000L     // list of cyclcic CAN messages will be processed in
                                                            // sequential mode (otherwise in parallel mode)
#define USBCAN_CYCLIC_FLAG_NOECHO           0x00010000L     // each sent CAN message of the list will be sent back
                                                            // to the host

// each CAN message from the list can be enabled or disabled separatly
#define USBCAN_CYCLIC_FLAG_LOCK_0           0x00000001L     // if some of these bits are set, then the according
#define USBCAN_CYCLIC_FLAG_LOCK_1           0x00000002L     // CAN message from the list is disabled
#define USBCAN_CYCLIC_FLAG_LOCK_2           0x00000004L
#define USBCAN_CYCLIC_FLAG_LOCK_3           0x00000008L
#define USBCAN_CYCLIC_FLAG_LOCK_4           0x00000010L
#define USBCAN_CYCLIC_FLAG_LOCK_5           0x00000020L
#define USBCAN_CYCLIC_FLAG_LOCK_6           0x00000040L
#define USBCAN_CYCLIC_FLAG_LOCK_7           0x00000080L
#define USBCAN_CYCLIC_FLAG_LOCK_8           0x00000100L
#define USBCAN_CYCLIC_FLAG_LOCK_9           0x00000200L
#define USBCAN_CYCLIC_FLAG_LOCK_10          0x00000400L
#define USBCAN_CYCLIC_FLAG_LOCK_11          0x00000800L
#define USBCAN_CYCLIC_FLAG_LOCK_12          0x00001000L
#define USBCAN_CYCLIC_FLAG_LOCK_13          0x00002000L
#define USBCAN_CYCLIC_FLAG_LOCK_14          0x00004000L
#define USBCAN_CYCLIC_FLAG_LOCK_15          0x00008000L

//---------------------------------------------------------------------------
// definitions for function UcanGetMsgPending()
#define USBCAN_PENDING_FLAG_RX_DLL          0x00000001L     // returns number of pending RX CAN messages in USBCAN32.DLL
#define USBCAN_PENDING_FLAG_RX_SYS          0x00000002L     // (not implemented yet) returns number of pending RX CAN messages in USBCAN.SYS or UCANNET.SYS
#define USBCAN_PENDING_FLAG_RX_FW           0x00000004L     // returns number of pending RX CAN messages in firmware
#define USBCAN_PENDING_FLAG_TX_DLL          0x00000010L     // returns number of pending TX CAN messages in USBCAN32.DLL
#define USBCAN_PENDING_FLAG_TX_SYS          0x00000020L     // place holder - there is no TX buffer in USBCAN.SYS or UCANNET.SYS
#define USBCAN_PENDING_FLAG_TX_FW           0x00000040L     // returns number of pending TX CAN messages in firmware
// These bits can be combined. In these case the function UcanGetMsgPending() returns the summary of the counters.

#define USBCAN_PENDING_FLAG_RX_ALL          (USBCAN_PENDING_FLAG_RX_DLL | USBCAN_PENDING_FLAG_RX_SYS | USBCAN_PENDING_FLAG_RX_FW)
#define USBCAN_PENDING_FLAG_TX_ALL          (USBCAN_PENDING_FLAG_TX_DLL | USBCAN_PENDING_FLAG_TX_SYS | USBCAN_PENDING_FLAG_TX_FW)
#define USBCAN_PENDING_FLAG_ALL             (USBCAN_PENDING_FLAG_RX_ALL | USBCAN_PENDING_FLAG_TX_ALL)

#define USBCAN_HWEX_FLAG_UCANNET            0x00000001L     // if enabled, then the USB-CANmodul is configured for UCANNET.SYS driver
#define USBCAN_HWEX_FLAG_HWCTRL             0x00000002L     // if enabled, then application hat exclusive hw control
#define USBCAN_HWEX_FLAG_USBBUS_ERROR       0x00000004L     // if enabled, then USB bus error occured (e.g. STALL or XACT)
#define USBCAN_HWEX_FLAG_USBBUS_AUTO        0x00000100L     // if enabled, then the DLL processes a reconnect automatically


//---------------------------------------------------------------------------
// types
//---------------------------------------------------------------------------

// set structure alignement to 1 byte
#if !defined (_WIN32_WCE)
#pragma pack(push,1)
#endif

// Typedef for the USB-CAN Handle
#ifndef UCHANDLE_DEFINED
    #define UCHANDLE_DEFINED
    typedef BYTE tUcanHandle;
#endif

// for further use
#define UCANRET         BYTE
#define UCANBYTE        BYTE
#define UCANWORD        WORD
#define UCANDWORD       DWORD

typedef BYTE tUcanMode;                     // definitions of CAN modes

#define kUcanModeNormal         0x00        // normal mode (send and reciceive)
#define kUcanModeListenOnly     0x01        // listen only mode (only reciceive)
#define kUcanModeTxEcho         0x02        // CAN messages which was sent will be received at UcanReadCanMsg..
#define kUcanModeRxOrderCh      0x04        // reserved (not implemented in this version)
#define kUcanModeHighResTimer   0x08        // high resolution time stamps in received CAN messages (only avaylable with STM derivates)
#define kUcanModeReserved       0x10        // only used for SocketCAN


// version types for function UcanGetVersionEx()
typedef DWORD tUcanVersionType;

#define kVerTypeUserLib         0x00000001L // version of the library
#define kVerTypeUserDll         0x00000001L // version of USBCAN32.DLL (for Win2k, WinXP or higher) USBCANCE.DLL (for WinCE)
#define kVerTypeSysDrv          0x00000002L // version of USBCAN.SYS (for Win2k, WinXP or higher) USBCANDRV.DLL (for WinCE)
#define kVerTypeFirmware        0x00000003L // reserved (not supported, use function UcanGetFwVersion)
#define kVerTypeNetDrv          0x00000004L // version of UCANNET.SYS  (not supported for WinCE)
#define kVerTypeSysLd           0x00000005L // version of USBCANLD.SYS (not supported for WinCE) (loader for USB-CANmodul GW-001)
#define kVerTypeSysL2           0x00000006L // version of USBCANL2.SYS                           (loader for USB-CANmodul GW-002)
#define kVerTypeSysL3           0x00000007L // version of USBCANL3.SYS (not supported for WinCE) (loader for Multiport CAN-to-USB 340400x or 3004006)
#define kVerTypeSysL4           0x00000008L // version of USBCANL4.SYS                           (loader for USB-CANmodul1 3204000 or 3204001)
#define kVerTypeSysL5           0x00000009L // version of USBCANL5.SYS                           (loader for USB-CANmodul1 3204002 or 3204003)
#define kVerTypeCpl             0x0000000AL // version of USBCANCP.CPL (not supported for WinCE)
#define kVerTypeSysL21          0x0000000BL // version of USBCANL21.SYS                          (loader for USB-CANmodul2 G4)
#define kVerTypeSysL22          0x0000000CL // version of USBCANL22.SYS                          (loader for USB-CANmodul1 G4)
#define kVerTypeSysL23          0x0000000DL // version of USBCANL23.SYS                          (loader for USB-CANmodul8/16 G4)
#define kVerTypeSysLex          0x0000000EL // version of USBCANLEX.SYS                          (loader for all USB-CANmodul types of G4)


// Typedef for the Callback function
typedef void (PUBLIC *tCallbackFkt) (tUcanHandle UcanHandle_p, BYTE bEvent_p);
typedef void (PUBLIC *tCallbackFktEx) (tUcanHandle UcanHandle_p, DWORD dwEvent_p, BYTE bChannel_p, void* pArg_p);

// Typedef for the Connection-Control function
typedef void (PUBLIC *tConnectControlFkt) (BYTE bEvent_p, DWORD dwParam_p);
typedef void (PUBLIC *tConnectControlFktEx) (DWORD dwEvent_p, DWORD dwParam_p, void* pArg_p);

// Structure for the CAN message (suitable for CAN messages according to spec. CAN2.0B)
typedef struct _tCanMsgStruct
{
    DWORD   m_dwID;                         // CAN Identifier
    BYTE    m_bFF;                          // CAN Frame format (BIT7=1: 29BitID / BIT6=1: RTR-Frame / BIT5=1: Tx echo)
    BYTE    m_bDLC;                         // CAN Data Length Code
    BYTE    m_bData[8];                     // CAN Data
    DWORD   m_dwTime;                       // Time in ms

    // NOTE:
    //
    // Value of m_dwTime only is valid for received CAN messages. It is ignored for
    // CAN messages to send.
    //
    // Value m_dwTime will be received from Hardware. Thus it is
    // important to know that only 24 bits of the value are valid.
    // It means that the timer value will be reset to zero each
    // 2^24 milliseconds = 4,66 hours. If you need to calculate
    // a time difference you should use the macro USBCAN_CALC_TIMEDIFF().
    //
    // Since driver version V4.11 the time stamp parameter is corrected to 32 bits.
    // ( but the marco USBCAN_CALC_TIMEDIFF() does work too in this version )

} tCanMsgStruct;

//#define USBCAN_CALC_TIMEDIFF(old_time,new_time) \
//    ((new_time) >= (old_time)) ?                \
//            ((new_time) - (old_time)) :         \
//        -1* ((old_time) - (new_time))
#define USBCAN_CALC_TIMEDIFF(old_time,new_time)     ((new_time) - (old_time))

// Structure with the stati (Function: UcanGetStatus() or UcanGetStatusEx() )
typedef struct _tStatusStruct
{
    WORD        m_wCanStatus;                   // current CAN status
    WORD        m_wUsbStatus;                   // current USB status

} tStatusStruct;

// Structure with init parameters for function UcanInitCanEx() and UcanInitCanEx2()
typedef struct _tUcanInitCanParam
{
    DWORD       m_dwSize;                       // [IN] size of this structure
    BYTE        m_bMode;                        // [IN] slecets the mode of CAN controller (see kUcanMode...)

    // Baudrate Registers for GW-001 or GW-002
    BYTE        m_bBTR0;                        // [IN] Bus Timing Register 0 (SJA1000 - use high byte USBCAN_BAUD_...)
    BYTE        m_bBTR1;                        // [IN] Bus Timing Register 1 (SJA1000 - use low  byte USBCAN_BAUD_...)

    BYTE        m_bOCR;                         // [IN] Output Controll Register of SJA1000 (should be 0x1A)
    DWORD       m_dwAMR;                        // [IN] Acceptance Mask Register (SJA1000)
    DWORD       m_dwACR;                        // [IN] Acceptance Code Register (SJA1000)

    // since version V3.00 - is ignored from function UcanInitCanEx() and until m_dwSize < 20
    DWORD       m_dwBaudrate;                   // [IN] Baudrate Register for Multiport 3004006, USB-CANmodul1 3204000 or USB-CANmodul2 3204002
                                                //      (use USBCAN_BAUDEX_...)

    // since version V3.05 - is ignored unltil m_dwSize < 24
    WORD        m_wNrOfRxBufferEntries;         // [IN] number of receive buffer entries (default is 4096)
    WORD        m_wNrOfTxBufferEntries;         // [IN] number of transmit buffer entries (default is 4096)

} tUcanInitCanParam;

// Structure with the hardware properties of a USB-CANmodul (Function: UcanGetHardwareInf())
typedef struct _tUcanHardwareInfo
{
    BYTE        m_bDeviceNr;                    // [OUT] device number of the USB-CANmodul
    tUcanHandle m_UcanHandle;                   // [OUT] USB-CAN-Handle assigned by the library
    DWORD       m_dwReserved;                   // [OUT] reserved

    // values only for CAN channel 0
    BYTE        m_bBTR0;                        // [OUT] Bus Timing Register 0 (SJA1000)
    BYTE        m_bBTR1;                        // [OUT] Bus Timing Register 1 (SJA1000)
    BYTE        m_bOCR;                         // [OUT] Output Control Register (SJA1000)
    DWORD       m_dwAMR;                        // [OUT] Acceptance Mask Register (SJA1000)
    DWORD       m_dwACR;                        // [OUT] Acceptance Code Register (SJA1000)

    // new values since 17.03.03 Version V2.16
    BYTE        m_bMode;                        // [OUT] mode of CAN controller (see kUcanMode...)
    DWORD       m_dwSerialNr;                   // [OUT] serial number from USB-CANmodul

} tUcanHardwareInfo;

typedef struct _tUcanHardwareInfoEx
{
    DWORD       m_dwSize;                       // [IN]  size of this structure
    tUcanHandle m_UcanHandle;                   // [OUT] USB-CAN-Handle assigned by the DLL
    BYTE        m_bDeviceNr;                    // [OUT] device number of the USB-CANmodul
    DWORD       m_dwSerialNr;                   // [OUT] serial number from USB-CANmodul
    DWORD       m_dwFwVersionEx;                // [OUT] version of firmware
    DWORD       m_dwProductCode;                // [OUT] product code (for differentiate between different hardware modules)
                                                //       see constants USBCAN_PRODCODE_...

    DWORD       m_adwUniqueId[4];               // [OUT] unique ID (available since V5.01) !!! m_dwSize must be >= USBCAN_HWINFO_SIZE_V2
    DWORD       m_dwFlags;                      // [OUT] additional flags
}
tUcanHardwareInfoEx;
#define USBCAN_HWINFO_SIZE_V1           0x12    // size without m_adwDeviceId[]
#define USBCAN_HWINFO_SIZE_V2           0x22    // size with m_adwDeviceId[]
#define USBCAN_HWINFO_SIZE_V3           0x26    // size with m_adwDeviceId[] and m_dwFlags

typedef struct _tUcanHardwareInitInfo
{
    DWORD           m_dwSize;                   // [IN]  size of this structure
    BOOL            m_fDoInitialize;            // [IN]  specifies if the found module should be initialized by the DLL
    tUcanHandle*    m_pUcanHandle;              // [IN]  pointer to variable receiving the USB-CAN-Handle
    tCallbackFktEx  m_fpCallbackFktEx;          // [IN]  pointer to callback function
    void*           m_pCallbackArg;             // [IN]  pointer to user defined parameter for callback function
    BOOL            m_fTryNext;                 // [IN]  specifies if a further module should be found
}
tUcanHardwareInitInfo;

typedef void (PUBLIC *tUcanEnumCallback) (
    DWORD                  dwIndex_p,           // [IN]  gives a sequential number of the enumerated module
    BOOL                   fIsUsed_p,           // [IN]  set to TRUE if the module is used by another application
    tUcanHardwareInfoEx*   pHwInfoEx_p,         // [IN]  pointer to the hardware info structure identifying the enumerated module
    tUcanHardwareInitInfo* pInitInfo_p,         // [IN]  pointer to an init structure for initializing the module
    void*                  pArg_p);             // [IN]  user argument which was overhand with UcanEnumerateHardware()

typedef struct _tUcanChannelInfo
{
    DWORD       m_dwSize;                       // [IN]  size of this structure
    BYTE        m_bMode;                        // [OUT] slecets the mode of CAN controller (see kUcanMode...)
    BYTE        m_bBTR0;                        // [OUT] Bus Timing Register 0 (SJA1000 - use high byte USBCAN_BAUD_...)
    BYTE        m_bBTR1;                        // [OUT] Bus Timing Register 1 (SJA1000 - use low  byte USBCAN_BAUD_...)
    BYTE        m_bOCR;                         // [OUT] Output Controll Register of SJA1000 (should be 0x1A)
    DWORD       m_dwAMR;                        // [OUT] Acceptance Mask Register (SJA1000)
    DWORD       m_dwACR;                        // [OUT] Acceptance Code Register (SJA1000)
    DWORD       m_dwBaudrate;                   // [OUT] Baudrate Register for Multiport 3004006, USB-CANmodul1 3204000 or USB-CANmodul2 3204002 (use USBCAN_BAUDEX_...)
    BOOL        m_fCanIsInit;                   // [OUT] is TRUE if CAN interface was initialized, otherwise FALSE
    WORD        m_wCanStatus;                   // [OUT] CAN status (same as received from function UcanGetStatus..())

} tUcanChannelInfo;

// structure with transfered packet information
typedef struct _tUcanMsgCountInfo
{
    WORD        m_wSentMsgCount;                // counter of sent CAN messages
    WORD        m_wRecvdMsgCount;               // counter of received CAN messages

} tUcanMsgCountInfo;

typedef struct _tUcanMsgCountInfoEx
{
    DWORD       m_dwSentMsgCount;               // counter of sent CAN messages
    DWORD       m_dwRecvdMsgCount;              // counter of received CAN messages

} tUcanMsgCountInfoEx;

typedef struct _tUcanRtcStatus
{
    DWORD       m_dwSize;                       // [IN]  size of this structure
    BYTE        m_bSeconds;                     // [OUT]
    BYTE        m_bMinutes;                     // [OUT]
    BYTE        m_bHours;                       // [OUT]
    BYTE        m_bDays;                        // [OUT]
    BYTE        m_bWeekdays;                    // [OUT]
    BYTE        m_bMonthsCentury;               // [OUT]
    BYTE        m_bYears;                       // [OUT]
}
tUcanRtcStatus;

typedef struct _tUcanSdCardStatus
{
    DWORD       m_dwSize;                       // [IN]  size of this structure
    DWORD       m_dwFlags;                      // [OUT] (SD card set or not set)
    DWORD       m_dwTotalSize;                  // [OUT] (in 1024 bytes)
    DWORD       m_dwFreeSize;                   // [OUT] (in 1024 bytes)
}
tUcanSdCardStatus;

#if !defined (_WIN32_WCE)
#pragma pack(pop)
#endif


//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
//
// Function:    UcanSetDebugMode()
//
// Description: sets a new debug mode
//
// Parameters:  dwDbgLevel_p        = debug level (bit format)
//              pszFilePathName_p   = file path to debug log file
//              dwFlags_p           = 0x00000000 no file append mode
//                                    0x00000001 file append mode
//
// Return:      BOOL    = FALSE if logfile not created otherwise TRUE
//
//---------------------------------------------------------------------------

BOOL PUBLIC UcanSetDebugMode (DWORD dwDbgLevel_p, _TCHAR* pszFilePathName_p, DWORD dwFlags_p);


//---------------------------------------------------------------------------
//
// Function:    UcanGetVersion()
//
// Description: returns software version of USBCAN32.DLL
//
// NOTE: This function is obsolete. Pleas use UcanGetVersionEx() instead of.
//
// Parameters:  none
//
// Returns:     software version
//                  format: Bits 0-7:   least significant version number (binary)
//                          Bits 8-15:  most significant version number (binary)
//                          Bits 16-30: reserved
//                          Bit 31:     1 = customer specific version
//
//---------------------------------------------------------------------------

DWORD PUBLIC UcanGetVersion (void);


//---------------------------------------------------------------------------
//
// Function:    UcanGetVersionEx()
//
// Description: returns software version of different software modules
//
// Parameters:  VerType_p   = [IN] which version should be returned
//                                 kVerTypeUserDll returnes Version of USBCAN32.DLL
//                                 (see tUcanVersionType for more information)
//
// Returns:     software version
//
//                  format: Bit 0-7:    Version     (use USBCAN_MAJOR_VER() )
//                          Bit 8-15:   Revision    (use USBCAN_MINOR_VER() )
//                          Bit 16-31:  Release     (use USBCAN_RELEASE_VER() )
//
// NOTE: If returned version is zero, then value of VerType_p
//       is unknown or not supported or the file version could not be read.
//
//---------------------------------------------------------------------------

DWORD PUBLIC UcanGetVersionEx (tUcanVersionType VerType_p);


//---------------------------------------------------------------------------
//
// Function:    UcanGetFwVersion()
//
// Description: returns version of the firmware within USB-CANmodul
//
// Parameters:  UcanHandle_p    = [IN] USBCAN handle
//
// Return:      DWORD           = version in extended format (see UcanGetVersionEx)
//
//---------------------------------------------------------------------------

DWORD PUBLIC UcanGetFwVersion (tUcanHandle UcanHandle_p);


//---------------------------------------------------------------------------
//
// Function:    UcanInitHwConnectControl(), UcanInitHwConnectControlEx
//
// Description: Initializes the Hardware-Connection-Control function
//
// Parameters:  fpConnectControlFkt_p   = [IN] address to Hardware-Connection-Control function
//
//              fpConnectControlFktEx_p = [IN] address of the new Hardware-Connection-Control function
//                                             NULL: no Callback function is used
//              pCallbackArg_p          = [IN] additional user defined parameter for callback function
//
// NOTE:        Do not set function pointer of type tConnectControlFkt to fpConnectControlFktEx_p and
//              do not set function pointer of type tConnectControlFktEx to fpConnectControlFkt_p !
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanInitHwConnectControl (tConnectControlFkt fpConnectControlFkt_p);
UCANRET PUBLIC UcanInitHwConnectControlEx (tConnectControlFktEx fpConnectControlFktEx_p, void* pCallbackArg_p);


//---------------------------------------------------------------------------
//
// Function:    UcanDeinitHwConnectControl()
//
// Description: Deinitializes the Hardware-Connection-Control function
//
// Parameters:  none
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanDeinitHwConnectControl (void);


//---------------------------------------------------------------------------
//
// Function:    UcanEnumerateHardware()
//
// Description: Enumerates connected USB-CANmoduls
//
// Parameters:  fpCallback_p        = [IN]  callback handler which is called for each found module
//              pCallbackArg_p      = [IN]  user argument which is overhand to the callback handler with each call
//              fEnumUsedDevs_p     = [IN]  if set to TRUE modules are enumerated too which are used by other applications
//              bDeviceNrLow_p      = [IN]  lower value of the device number which should be enumerated
//              bDeviceNrHigh_p     = [IN]  higher value of the device number which should be enumerated
//              dwSerialNrLow_p     = [IN]  lower value of the serial number which should be enumerated
//              dwSerialNrHigh_p    = [IN]  higher value of the serial number which should be enumerated
//              dwProductCodeLow_p  = [IN]  lower value of the product code which should be enumerated
//              dwProductCodeHigh_p = [IN]  higher value of the product code which should be enumerated
//
// Return:      DWORD               = number of enumerated modules
//
//---------------------------------------------------------------------------

DWORD PUBLIC UcanEnumerateHardware (tUcanEnumCallback fpCallback_p, void* pCallbackArg_p,
    BOOL  fEnumUsedDevs_p,                                  // if TRUE used modules are enumerated too
    BYTE  bDeviceNrLow_p,     BYTE  bDeviceNrHigh_p,        // filter parameter for device number
    DWORD dwSerialNrLow_p,    DWORD dwSerialNrHigh_p,       // filter parameter for serial number
    DWORD dwProductCodeLow_p, DWORD dwProductCodeHigh_p);   // filter parameter for product code


//---------------------------------------------------------------------------
//
// Function:    UcanInitHardware(), UcanInitHardwareEx(), UcanInitHardwareEx2()
//
// Description: Initializes a USB-CANmodul with the device number X or serial number Y
//
// Parameters:  pUcanHandle_p       = [OUT] address pointing to the variable for the USB-CAN-Handle
//                                          should not be NULL!
//              bDeviceNr_p         = [IN]  device number of the USB-CANmodul
//                                          valid values: 0 through 254
//                                          USBCAN_ANY_MODULE: the first module that is found will be used
//              dwSerialNr_p        = [IN]  serial number of the USB-CANmodul
//              fpCallbackFkt_p     = [IN]  address of the Callback function
//                                          NULL: no Callback function is used
//              fpCallbackFktEx_p   = [IN]  address of the new callback function
//                                          NULL: no Callback function is used
//              pCallbackArg_p      = [IN]  additional user defined parameter for callback function
//
// NOTE:        Do not set function pointer of type tCallbackFkt to fpCallbackFktEx_p and
//              do not set function pointer of type tCallbackFktEx to fpCallbackFkt_p !
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanInitHardware (tUcanHandle* pUcanHandle_p, BYTE bDeviceNr_p, tCallbackFkt fpCallbackFkt_p);
UCANRET PUBLIC UcanInitHardwareEx (tUcanHandle* pUcanHandle_p, BYTE bDeviceNr_p, tCallbackFktEx fpCallbackFktEx_p, void* pCallbackArg_p);
UCANRET PUBLIC UcanInitHardwareEx2 (tUcanHandle* pUcanHandle_p, DWORD dwSerialNr_p, tCallbackFktEx fpCallbackFktEx_p, void* pCallbackArg_p);


//---------------------------------------------------------------------------
//
// Function:    UcanSetDeviceNr()
//
// Description: Sets a new device number to the USB-CANmodul.
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              bDeviceNr_p     = [IN]  new device nr
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanSetDeviceNr (tUcanHandle UcanHandle_p, BYTE bDeviceNr_p);


//---------------------------------------------------------------------------
//
// Function:    UcanGetModuleTime()
//
// Description: Returns the current time stamp of USB-CANmodul.
//              NOTE: Some milliseconds are pased if function returnes.
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              pdwTime_p       = [OUT] pointer to DWORD receiving time stamp
//                                      can not be NULL
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanGetModuleTime (tUcanHandle UcanHandle_p, DWORD* pdwTime_p);


//---------------------------------------------------------------------------
//
// Function:    UcanGetHardwareInfo(), UcanGetHardwareInfoEx2()
//
// Description: Returns the hardware information of an initialized USB-CANmodul
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              pHwInfo_p       = [OUT] pointer to hardware info structure
//                                      can not be NULL
//
//              pCanInfoCh0_p   = [OUT] pointer to CAN channel 0 info structure
//              pCanInfoCh1_p   = [OUT] pointer to CAN channel 1 info structure
//                                      pCanInfoCh0_p and pCanInfoCh1_p can be NULL
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanGetHardwareInfo (tUcanHandle UcanHandle_p, tUcanHardwareInfo* pHwInfo_p);
UCANRET PUBLIC UcanGetHardwareInfoEx2 (tUcanHandle UcanHandle_p, tUcanHardwareInfoEx* pHwInfo_p, tUcanChannelInfo* pCanInfoCh0_p, tUcanChannelInfo* pCanInfoCh1_p);


//---------------------------------------------------------------------------
//
// Function:    UcanInitCan(), UcanInitCanEx(), UcanInitCanEx2()
//
// Description: Initializes the CAN interface on the USB-CANmodul
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              bBTR0_p         = [IN]  Baudrate Register 0 (SJA1000)
//              bBTR1_p         = [IN]  Baudrate Register 1 (SJA1000)
//              dwAMR_p         = [IN]  Acceptance Filter Mask (SJA1000)
//              dwACR_p         = [IN]  Acceptance Filter Code (SJA1000)
//
//              pInitCanParam_p = [IN]  pointer to init parameter structure
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanInitCan (tUcanHandle UcanHandle_p, BYTE bBTR0_p, BYTE bBTR1_p, DWORD dwAMR_p, DWORD dwACR_p);
UCANRET PUBLIC UcanInitCanEx (tUcanHandle UcanHandle_p, tUcanInitCanParam* pInitCanParam_p);
UCANRET PUBLIC UcanInitCanEx2 (tUcanHandle UcanHandle_p, BYTE bChannel_p, tUcanInitCanParam* pInitCanParam_p);


//---------------------------------------------------------------------------
//
// Function:    UcanSetBaudrate(), UcanSetBaudrateEx()
//
// Description: Modifies the baudrate settings of the USB-CANmodul
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              bBTR0_p         = [IN]  Baudrate Register 0 (GW-001/002 - Multiport 3004006,
//                                      USB-CANmodul1 3204000 or USB-CANmodul2 3204002 only standard values)
//              bBTR1_p         = [IN]  Baudrate Register 1 (GW-001/002 - Multiport 3004006,
//                                      USB-CANmodul1 3204000 or USB-CANmodul2 3204002 only standard values)
//
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//              dwBaudrate_p    = [IN]  Baudrate Register of Multiport 3004006, USB-CANmodul1 3204000 or USB-CANmodul2 3204002
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanSetBaudrate (tUcanHandle UcanHandle_p, BYTE bBTR0_p, BYTE bBTR1_p);
UCANRET PUBLIC UcanSetBaudrateEx (tUcanHandle UcanHandle_p, BYTE bChannel_p, BYTE bBTR0_p, BYTE bBTR1_p, DWORD dwBaudrate_p);


//---------------------------------------------------------------------------
//
// Function:    UcanSetAcceptance(), UcanSetAcceptanceEx()
//
// Description: Modifies the Acceptance Filter settings of the USB-CANmodul
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              dwAMR_p         = [IN]  Acceptance Filter Mask (SJA1000)
//              dwACR_p         = [IN]  Acceptance Filter Code (SJA1000)
//
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanSetAcceptance (tUcanHandle UcanHandle_p, DWORD dwAMR_p, DWORD dwACR_p);
UCANRET PUBLIC UcanSetAcceptanceEx (tUcanHandle UcanHandle_p, BYTE bChannel_p, DWORD dwAMR_p, DWORD dwACR_p);


//---------------------------------------------------------------------------
//
// Function:    UcanResetCan(), UcanResetCanEx()
//
// Description: Resets the CAN interface (Hardware-Reset, empty buffer, ...)
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//              dwResetFlags_p  = [IN]  flags defines what should be reseted
//                                      (see USBCAN_RESET_...)
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanResetCan (tUcanHandle UcanHandle_p);
UCANRET PUBLIC UcanResetCanEx (tUcanHandle UcanHandle_p, BYTE bChannel_p, DWORD dwResetFlags_p);


//---------------------------------------------------------------------------
//
// Function:    UcanReadCanMsg(), UcanReadCanMsgEx()
//
// Description: Reads one or more CAN messages
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              pCanMsg_p       = [OUT] pointer to the CAN message structure
//
//              pbChannel_p     = [IN/OUT] pointer CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1, USBCAN_CHANNEL_ANY)
//                                         pointer must not be NULL
//                                         if INPUT is USBCAN_CHANNEL_ANY the OUTPUT will be the read CAN channel
//              pdwCount_p      = [IN/OUT] pointer to number of received CAN messages
//                                         if NULL only one CAN message will be read
//                                         INPUT:  *pdwCount_p contains max number of CAN messages which should be read
//                                         OUTPUT: *pdwCount_p contains real number of CAN messages was read
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanReadCanMsg (tUcanHandle UcanHandle_p, tCanMsgStruct* pCanMsg_p);
UCANRET PUBLIC UcanReadCanMsgEx (tUcanHandle UcanHandle_p, BYTE* pbChannel_p, tCanMsgStruct* pCanMsg_p, DWORD* pdwCount_p);


//---------------------------------------------------------------------------
//
// Function:    UcanWriteCanMsg(), UcanWriteCanMsgEx()
//
// Description: Sends one or more CAN messages
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              pCanMsg_p       = [IN]  pointer to the CAN message structure
//
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//              pdwCount_p      = [IN/OUT] pointer to number of CAN messages to write
//                                         if NULL only one CAN message will be written
//                                         INPUT:  *pdwCount_p contains number of CAN messages which should be written
//                                         OUTPUT: *pdwCount_p contains real number of CAN messages was written
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanWriteCanMsg (tUcanHandle UcanHandle_p, tCanMsgStruct* pCanMsg_p);
UCANRET PUBLIC UcanWriteCanMsgEx (tUcanHandle UcanHandle_p, BYTE bChannel_p, tCanMsgStruct* pCanMsg_p, DWORD* pdwCount_p);


//---------------------------------------------------------------------------
//
// Function:    UcanGetStatus(), UcanGetStatusEx()
//
// Description: Returns the state of the USB-CANmodul
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              pStatus_p       = [OUT] pointer to Status structure
//
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanGetStatus (tUcanHandle UcanHandle_p, tStatusStruct* pStatus_p);
UCANRET PUBLIC UcanGetStatusEx (tUcanHandle UcanHandle_p, BYTE bChannel_p, tStatusStruct* pStatus_p);


//---------------------------------------------------------------------------
//
// Function:    UcanGetMsgCountInfo(), UcanGetMsgCountInfoEx()
//
// Description: Reads the packet information from USB-CANmodul (counter of
//              received and sent CAN messages).
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              pMsgCountInfo_p = [OUT] pointer to message counter information structure
//
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanGetMsgCountInfo (tUcanHandle UcanHandle_p, tUcanMsgCountInfo* pMsgCountInfo_p);
UCANRET PUBLIC UcanGetMsgCountInfoEx (tUcanHandle UcanHandle_p, BYTE bChannel_p, tUcanMsgCountInfo* pMsgCountInfo_p);
UCANRET PUBLIC UcanGetMsgCountInfoEx2 (tUcanHandle UcanHandle_p, BYTE bChannel_p, tUcanMsgCountInfoEx* pMsgCountInfo_p);


//---------------------------------------------------------------------------
//
// Function:    UcanDeinitCan(), UcanDeinitCanEx()
//
// Description: Shuts down the CAN interface on the USB-CANmodul
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanDeinitCan (tUcanHandle UcanHandle_p);
UCANRET PUBLIC UcanDeinitCanEx (tUcanHandle UcanHandle_p, BYTE bChannel_p);


//---------------------------------------------------------------------------
//
// Function:    UcanDeinitHardware()
//
// Description: Deinitializes a USB-CANmodul
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanDeinitHardware (tUcanHandle UcanHandle_p);


//---------------------------------------------------------------------------
//
// Callback functions
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
// Function:    UcanCallbackFkt(), UcanCallbackFktEx()
//
// Description: Is called from the USBCAN32.DLL if a working event occured.
//
// Parameters:  UcanHandle_p        = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              bEvent_p/dwEvent_p  = [IN]  event
//                  USBCAN_EVENT_INITHW
//                  USBCAN_EVENT_INITCAN
//                  USBCAN_EVENT_RECEIVE
//                  USBCAN_EVENT_STATUS
//                  USBCAN_EVENT_DEINITCAN
//                  USBCAN_EVENT_DEINITHW
//
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1, USBCAN_CHANNEL_ANY)
//              pArg_p          = [IN]  additional parameter
//                                      Parameter which was defined with UcanInitHardwareEx()
//
// Returns:     none
//
//---------------------------------------------------------------------------

void PUBLIC UcanCallbackFkt (tUcanHandle UcanHandle_p, BYTE bEvent_p);
void PUBLIC UcanCallbackFktEx (tUcanHandle UcanHandle_p, DWORD dwEvent_p, BYTE bChannel_p, void* pArg_p);


//---------------------------------------------------------------------------
//
// Function:    UcanConnectControlFkt(), UcanConnectControlFktEx()
//
// Description: Is called from the USBCAN32.DLL if a plug & play event occured.
//
// Parameters:  bEvent_p/dwEvent_p    = [IN]  event
//                  USBCAN_EVENT_CONNECT
//                  USBCAN_EVENT_DISCONNECT
//                  USBCAN_EVENT_FATALDISCON
//              dwParam_p   = [IN]  additional parameter (depends on bEvent_p)
//                  USBCAN_EVENT_CONNECT:       always 0
//                  USBCAN_EVENT_DISCONNECT.    always 0
//                  USBCAN_EVENT_FATALDISCON:   USB-CAN-Handle of the disconnected module
//
//              pArg_p      = [IN]  additional parameter
//                                  Parameter which was defined with UcanInitHardwareEx()
//
// Returns:     none
//
//---------------------------------------------------------------------------

void PUBLIC UcanConnectControlFkt (BYTE bEvent_p, DWORD dwParam_p);
void PUBLIC UcanConnectControlFktEx (DWORD dwEvent_p, DWORD dwParam_p, void* pArg_p);


//---------------------------------------------------------------------------
//
// automatic transmission of cyclic CAN messages by the device firmware
//
//              Only available for Multiport CAN-to-USB, USB-CANmodulX
//              (NOT for GW-001 and GW-002 !!!).
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
// Function:    UcanDefineCyclicCanMsg()
//
// Description: defines a list of CAN messages for automatic transmission
// NOTE:        when this function is called an older list will be deleted
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//              pCanMsgList_p   = [IN]  pointer to the CAN message list (must be an array)
//              dwCount_p       = [IN]  number of CAN messages within the list
//                                          if zero an older list will only be deleted
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanDefineCyclicCanMsg (tUcanHandle UcanHandle_p,
    BYTE bChannel_p, tCanMsgStruct* pCanMsgList_p, DWORD dwCount_p);


//---------------------------------------------------------------------------
//
// Function:    UcanReadCyclicCanMsg()
//
// Description: reads the list of CAN messages for automatically sending back
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//              pCanMsgList_p   = [OUT] pointer to receive the CAN message list (must be an array)
//              pdwCount_p      = [OUT] pointer to a 32 bit variables for receiving the number of
//                                      CAN messages within the list
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanReadCyclicCanMsg (tUcanHandle UcanHandle_p,
    BYTE bChannel_p, tCanMsgStruct* pCanMsgList_p, DWORD* pdwCount_p);


//---------------------------------------------------------------------------
//
// Function:    UcanEnableCyclicCanMsg()
//
// Description: enables or disables the automatically sending
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware..()
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//              dwFlags_p       = [IN]  this flags specifies which CAN messages should be
//                                      activated, specifies the processing mode oth the list
//                                      (sequential or parallel), enables or disables the TxEcho
//                                      for this CAN messages
//                                      (see constants USBCAN_CYCLIC_FLAG_...)
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanEnableCyclicCanMsg (tUcanHandle UcanHandle_p,
    BYTE bChannel_p, DWORD dwFlags_p);


// for the future:
//UCANRET PUBLIC UcanSaveCyclicCanMsg (tUcanHandle UcanHandle_p,
//    BYTE bChannel_p, tUcanInitCanParam* pInitCanParam_p, DWORD dwFlags_p);


//---------------------------------------------------------------------------
//
// Function:    UcanGetMsgPending()
//
// Description: returns the number of pending CAN messages
//
// Parameters:  UcanHandle_p        = [IN]  USB-CAN-Handle
//                                          Handle, which is returned by the function UcanInitHardware..()
//              bChannel_p          = [IN]  CAN channel (USBCAN_CHANNEL_CH0, USBCAN_CHANNEL_CH1 or USBCAN_CHANNEL_ANY)
//                                          If USBCAN_CHANNEL_ANY is set then the number of borth channels will be
//                                          added as long as they are initialized.
//              dwFlags_p           = [IN]  this flags specifies which buffers shoulb be checked
//                                          (see constants USBCAN_PENDING_FLAG_...)
//              pdwPendingCount_p   = [OUT] pointer to a 32 bit variable which receives the number of pending messages
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanGetMsgPending (tUcanHandle UcanHandle_p,
    BYTE bChannel_p, DWORD dwFlags_p, DWORD* pdwPendingCount_p);



//---------------------------------------------------------------------------
//
// Function:    UcanGetCanErrorCounter()
//
// Description: reads the current value of the error counters within the CAN controller
//
//              Only available for Multiport CAN-to-USB, USB-CANmodulX
//              (NOT for GW-001 and GW-002 !!!).
//
// Parameters:  UcanHandle_p        = [IN]  USB-CAN-Handle
//                                          Handle, which is returned by the function UcanInitHardware..()
//              bChannel_p          = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//              pdwTxErrorCounter_p = [OUT] pointer to a 32 bit variable which receives the TX error counter
//              pdwRxErrorCounter_p = [OUT] pointer to a 32 bit variable which receives the RX error counter
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanGetCanErrorCounter (tUcanHandle UcanHandle_p,
    BYTE bChannel_p, DWORD* pdwTxErrorCounter_p, DWORD* pdwRxErrorCounter_p);


//---------------------------------------------------------------------------
//
// Function:    UcanSetTxTimeout()
//
// Description: sets the transmission timeout
//
// Note: When a transmission timeout is set the firmware tries to send
// a message within this timeout. If it could not be sent the firmware sets
// the "auto delete" state. Within this state all transmit CAN messages for
// this channel will be deleted automatically for not blocking the other
// channel. When firmware does delete a transmit CAN message then a new
// error status will be set: USBCAN_CANERR_TXMSGLOST (red LED is blinking).
//
// This function can also be used for USB-CANmodul2, 8 or 16 (multiport).
//
// Parameters:  UcanHandle_p    = [IN]  USB-CAN-Handle
//              bChannel_p      = [IN]  CAN channel (USBCAN_CHANNEL_CH0 or USBCAN_CHANNEL_CH1)
//              dwTxTimeout_p   = [IN]  transmit timeout in milleseconds
//                                      (value 0 swithes off the "auto delete" featuere = default setting)
//
// Return:      result of the function (see USBCAN_ERR_...)
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanSetTxTimeout (tUcanHandle UcanHandle_p,
    BYTE bChannel_p, DWORD dwTxTimeout_p);


// future functions
UCANRET PUBLIC UcanGetRtcStatus (tUcanHandle UcanHandle_p, tUcanRtcStatus* pRtcStatus_p);
UCANRET PUBLIC UcanGetSdCardStatus (tUcanHandle UcanHandle_p, tUcanSdCardStatus* pSdCardStatus_p);


#ifdef __cplusplus
} // von extern "C"
#endif


#endif //__USBCAN32_H__

//! @endcond
// clang-format on
