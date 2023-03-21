/*
MIT License

Copyright (c) 2005-2023 Gediminas Simanskis, Rusoku technologijos UAB (gediminas@rusoku.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

// clang-format off
//! @cond Doxygen_Suppress

#ifndef ___CANAL_A_H___
#define ___CANAL_A_H___


#ifndef EXPORT
#define EXPORT
#endif



#ifdef WIN32
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#else
 
#endif

/* FILTER req type */
typedef enum {
  FILTER_ACCEPT_ALL   = 0,
  FILTER_REJECT_ALL,
  FILTER_VALUE,
}Filter_Type_TypeDef;


#ifdef __cplusplus
extern "C" {
#endif

/*!
	Set the 11bit filter  ID, Mask for a CANAL channel

	@param handle Handle to open physical CANAL channel.
	@return zero on success or error-code on failure.
*/
#ifdef WIN32
int WINAPI EXPORT CanalSetFilter11bit( long handle, Filter_Type_TypeDef type,  unsigned long list, unsigned long mask );
#else
int CanalSetFilter11bit( long handle, Filter_Type_TypeDef type, unsigned long id, unsigned long mask );
#endif

/*!
	Set the 29bit filter  ID, Mask for a CANAL channel

	@param handle Handle to open physical CANAL channel.
	@return zero on success or error-code on failure.
*/
#ifdef WIN32
int WINAPI EXPORT CanalSetFilter29bit( long handle, Filter_Type_TypeDef type,  unsigned long list, unsigned long mask );
#else
int CanalSetFilter29bit( long handle, Filter_Type_TypeDef type,  unsigned long id, unsigned long mask );
#endif

/*!
	Get bootloader ver

	@param handle Handle to open physical CANAL channel.
	@return zero on success or error-code on failure.
*/
#ifdef WIN32
int WINAPI EXPORT CanalGetBootloaderVersion(long handle, unsigned long *bootloader_version);
#else
int CanalGetBootloaderVersion(long handle, unsigned long *bootloader_version);
#endif


#ifdef WIN32
int WINAPI EXPORT CanalGetHardwareVersion(long handle, unsigned long *hardware_version);
#else
int CanalGetHardwareVersion(long handle, unsigned long *hardware_version);
#endif


#ifdef WIN32
int WINAPI EXPORT CanalGetFirmwareVersion(long handle, unsigned long *firmware_version);
#else
int CanalGetFirmwareVersion(long handle, unsigned long *firmware_version);
#endif

#ifdef WIN32
int WINAPI EXPORT CanalGetSerialNumber(long handle, unsigned long *serial);
#else
int CanalGetSerialNumber(long handle, unsigned long *serial);
#endif

#ifdef WIN32
int WINAPI EXPORT CanalGetVidPid(long handle, unsigned long *vidpid);
#else
int CanalGetVidPid(long handle, unsigned long *vidpid);
#endif

#ifdef WIN32
int WINAPI EXPORT CanalGetDeviceId(long handle, unsigned long *deviceid);
#else
int CanalGetDeviceId(long handle, unsigned long *deviceid);
#endif

#ifdef WIN32
int WINAPI EXPORT CanalGetVendor(long handle, unsigned int size, char *vendor);
#else
int CanalGetVendor(long handle, unsigned long *bootloader_version);
#endif

#ifdef WIN32
int WINAPI EXPORT CanalInterfaceStart(long handle);
#else
int CanalInterfaceStart(long handle);
#endif

#ifdef WIN32
int WINAPI EXPORT CanalInterfaceStop(long handle);
#else
int CanalInterfaceStop(long handle);
#endif

#ifdef __cplusplus
}
#endif

#endif //___CANAL_A_H___

// clang-format on
//! @endcond
