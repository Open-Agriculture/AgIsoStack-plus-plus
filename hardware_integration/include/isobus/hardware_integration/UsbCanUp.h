/****************************************************************************

  (c) SYS TEC electronic AG, D-08468 Heinsdorfergrund, Am Windrad 2
      www.systec-electronic.com

  Project:      USB-CANmodule

  Description:  Additional header file for the USBCAN32.DLL
                Functions for User port at GW-002

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

  2002/03/18 r.d.:  First implementation

****************************************************************************/

// clang-format off
//! @cond Doxygen_Suppress

// Protection against mutliple including
#ifndef __USBCANUP_H__
#define __USBCANUP_H__


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


//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
// Function:    UcanConfigUserPort()
//
// Description: Configures the special User port of USB-CANmodule at the GW-002
//              for output or input.
//
// Parameters:  UcanHandle_p    = [in]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware()
//              bOutputEnable_p = [in]  8 bit configuration of user port
//                                      Each bit belongs to one pin of User port.
//                                      If the bit contains a 0 then the pin of User port is an input pin.
//                                      Otherwise it is an output pin.
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
//
//---------------------------------------------------------------------------

UCANRET PUBLIC UcanConfigUserPort (tUcanHandle UcanHandle_p, BYTE bOutputEnable_p);


//---------------------------------------------------------------------------
//
// Function:    UcanWriteUserPort()
//
// Description: Writes a value to special User port of USB-CANmodule at the GW-002.
//
// Parameters:  UcanHandle_p    = [in]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware()
//              bOutValue_p     = [in]  8 bit value to write to special User port.
//                                      Each bit belongs to one pin of User port.
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

UCANRET PUBLIC UcanWriteUserPort (tUcanHandle UcanHandle_p, BYTE bOutValue_p);


//---------------------------------------------------------------------------
//
// Function:    UcanReadUserPort()
//
// Description: Reads a value from special User port of USB-CANmodule at the GW-002.
//
//
// Parameters:  UcanHandle_p    = [in]  USB-CAN-Handle
//                                      Handle, which is returned by the function UcanInitHardware()
//              pbInValue_p     = [out] pointer 8 bit variable which receives the read value
//                                      Each bit belongs to one pin of User port.
//                                      This parameter can not be NULL.
//
//              pbLastOutEn_p   = [out] pointer to receive output enable configuratiuon
//                                      Each bit belongs to one pin of User port.
//                                      This parameter can be NULL.
//              pbLastOutVal_p  = [out] pointer to receive output data configuratiuon
//                                      Each bit belongs to one pin of User port.
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

UCANRET PUBLIC UcanReadUserPort (tUcanHandle UcanHandle_p, BYTE* pbValue_p);
UCANRET PUBLIC UcanReadUserPortEx (tUcanHandle UcanHandle_p, BYTE* pbInValue_p, BYTE* pbLastOutEn_p, BYTE* pbLastOutVal_p);


#ifdef __cplusplus
} // von extern "C"
#endif


#endif // __USBCANUP_H__

//! @endcond
// clang-format on
