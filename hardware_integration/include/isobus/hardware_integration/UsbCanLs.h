/****************************************************************************

  (c) SYS TEC electronic AG, D-08468 Heinsdorfergrund, Am Windrad 2
      www.systec-electronic.com

  Project:      USB-CANmodule

  Description:  Additional header file for the USBCAN32.DLL
                Functions for Low Speed CAN transceiver

  -------------------------------------------------------------------------

                $RCSfile:$
                
                $Author: R.Dietzsch $
                
                $Revision: 1.1 $  $Date: 2003/03/18 $
                Version: 2.18  Date: 27.04.2004
                
                $State: $
                
                Build Environment:
                MSVC 5.0 and MSVC 6.0

  -------------------------------------------------------------------------

  Revision History:

  2002/0?/0? r.d.:  First implementation

****************************************************************************/

// clang-format off
//! @cond Doxygen_Suppress

// Protection against mutliple including
#ifndef __USBCANLS_H__
#define __USBCANLS_H__


#ifndef __USBCAN32_H__

    #error 'ERROR: file "usbcan32.h" not included!'

#endif


// allow access to functions for C++ applications as well
#ifdef __cplusplus
extern "C"
{
#endif


//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#define UCAN_CANPORT_TRM            0x10    // termination resistor (not available for GW-001/002)
#define UCAN_CANPORT_ERR            0x20    // error signal of low speed CAN driver
#define UCAN_CANPORT_STB            0x40    // standby  of low speed CAN driver
#define UCAN_CANPORT_EN             0x80    // enable signal of low speed CAN driver

#define UCAN_CANPORT_OUTPUT         (UCAN_CANPORT_STB | UCAN_CANPORT_EN )
#define UCAN_CANPORT_INPUT          (UCAN_CANPORT_TRM | UCAN_CANPORT_ERR)


//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
// Function:    UcanWriteCanPort(), UcanWriteCanPortEx()
//
// Description: Writes a value to special CAN port of USB-CANmodule which is connected to a low
//              speed CAN transceiver. Only values STB and EN can be written (see defines below). Call
//              to this function has no effect when USB-CANmodule contains high speed CAN driver.
//
// Parameters:  UcanHandle_p = USB-CAN-Handle
//                  Handle, which is returned by the function UcanInitHardware()
//              bOutValue_p  = value to write to special CAN port
//
//              bChannel_p = CAN channel (0 or 1)
//
// Returns:     result of the function
//                  USBCAN_SUCCESSFUL
//                  USBCAN_ERR_ILLHANDLE
//                  USBCAN_ERR_MAXINSTANCES
//                  USBCAN_ERR_ILLHW
//                  USBCAN_ERR_IOFAILED
//                  USBCAN_ERR_DATA
//                  USBCAN_ERR_ABORT
//                  USBCAN_ERR_DISCONNECT
//                  USBCAN_ERR_BUSY
//                  USBCAN_ERR_TIMEOUT
//                  USBCAN_ERRCMD_...

//---------------------------------------------------------------------------

UCANRET PUBLIC UcanWriteCanPort (tUcanHandle UcanHandle_p, BYTE bOutValue_p);
UCANRET PUBLIC UcanWriteCanPortEx (tUcanHandle UcanHandle_p, BYTE bChannel_p, BYTE bOutValue_p);


//---------------------------------------------------------------------------
//
// Function:    UcanReadCanPort(), UcanReadCanPortEx()
//
// Description: Reads a value from special CAN port of USB-CANmodule which is connected to a low
//              speed CAN transceiver. Only values ERR can be read (see defines below). Call
//              to this function has no effect when USB-CANmodule contains high speed CAN driver.
//
// Parameters:  UcanHandle_p    = [in]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware()
//              pbInValue_p     = [out] pointer variable which receives the input value (see UCAN_CANPORT_...)
//                                      This parameter can not be NULL.
//
//              bChannel_p      = [in]  CAN channel (0 or 1)
//              pbLastOutVal_p  = [out] pointer to receive output data configuratiuon (see UCAN_CANPORT_...)
//                                      This parameter can be NULL.
//
// Returns:     result of the function
//                  USBCAN_SUCCESSFUL
//                  USBCAN_ERR_ILLHANDLE
//                  USBCAN_ERR_MAXINSTANCES
//                  USBCAN_ERR_ILLHW
//                  USBCAN_ERR_ILLPARAM
//                  USBCAN_ERR_IOFAILED
//                  USBCAN_ERR_DATA
//                  USBCAN_ERR_ABORT
//                  USBCAN_ERR_DISCONNECT
//                  USBCAN_ERR_BUSY
//                  USBCAN_ERR_TIMEOUT
//                  USBCAN_ERRCMD_...
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanReadCanPort (tUcanHandle UcanHandle_p, BYTE* pbInValue_p);
UCANRET PUBLIC UcanReadCanPortEx (tUcanHandle UcanHandle_p, BYTE bChannel_p, BYTE* pbInValue_p, BYTE* pbLastOutVal_p);


#ifdef __cplusplus
} // von extern "C"
#endif


#endif // __USBCANLS_H__

//! @endcond
// clang-format on
