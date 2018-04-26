/****************************************************************************

	Thorlabs SPX_Drv - SP1-USB/SP2-USB SPxyz-USB spectrometer VISA instrument driver

 	Copyright: 	Copyright(c) 2005, Thorlabs GmbH (www.thorlabs.com)
 	Author:		Lutz Hoerl (lhoerl@thorlabs.com)

	Disclaimer:

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


 	Header file

 	Date:       	Apr-11-2006
 	Software-Nr:   09.167.130
 	Version:    	2.4

	Changelog:		see 'SPX_Drv.c'

****************************************************************************/


#ifndef __SPX_HEADER
#define __SPX_HEADER

///#include <vpptype.h>
#include "vitypes.h"


#if defined(__cplusplus) || defined(__cplusplus__)
extern "C"
{
#endif


/*---------------------------------------------------------------------------
 Buffers
---------------------------------------------------------------------------*/
#define SPX_BUFFER_SIZE						256             		// General buffer size
#define SPX_ERR_DESCR_BUFFER_SIZE		512						// Buffer size for error messages
#define SPX_TEXT_BUFFER_SIZE				SPX_BUFFER_SIZE		// Buffer size for texts from the SPX
#define SPX_NUM_PIXELS						3000						// this is the number of effective pixels of CCD
#define SPX_MAX_USER_NAME_SIZE			32							// this includes the trailing '\0'

#define SPX_MIN_NUM_USR_ADJ				4							// minimum number of user adjustment data points
#define SPX_MAX_NUM_USR_ADJ				10							// maximum number of user adjustment data points

// Driver Error Codes

/*---------------------------------------------------------------------------
 Error/Warning Codes
---------------------------------------------------------------------------*/
// Offsets
#define VI_INSTR_WARNING_OFFSET                   	(0x3FFC0900L)
#define VI_INSTR_ERROR_OFFSET          (_VI_ERROR + 0x3FFC0900L)	//0xBFFC0900

// Driver WarningCodes
#define VI_WARN_SPX_DATA_NOT_READY		(VI_INSTR_WARNING_OFFSET + 1)
// Driver Error Codes
#define VI_ERROR_READ_INCOMPLETE			(VI_INSTR_ERROR_OFFSET + 1)
#define VI_ERROR_NO_USER_DATA				(VI_INSTR_ERROR_OFFSET + 10)	//	there is no wavelength adjustment data available at the instruments nonvolatile memory
#define VI_ERROR_INV_USER_DATA			(VI_INSTR_ERROR_OFFSET + 11)	//	the given wavelength adjustment data results in negative wavelength values.

/*---------------------------------------------------------------------------
 VISA strings
---------------------------------------------------------------------------*/
#define SPX_VI_FIND_RSC_PATTERN			"USB?*?{VI_ATTR_MANF_ID==0x1313 && ((VI_ATTR_MODEL_CODE==0x0111) || (VI_ATTR_MODEL_CODE==0x0112))}"


/*---------------------------------------------------------------------------
 Communication timeout
---------------------------------------------------------------------------*/
#define SPX_TIMEOUT_MIN						1000
#define SPX_TIMEOUT_MAX						60000

/*---------------------------------------------------------------------------
 SPX specific constants
---------------------------------------------------------------------------*/
#define SPX_MAX_INT_TIME					0.2		// 200ms is the maximum integration time
#define SPX_MIN_INT_TIME					0.000001	// 1us   is the minimum integration time

	// scan states - these are the states reported by SPX_getDeviceStatus();
#define SPX_SCAN_STATE_IDLE				0x00 		// SPX waits for new scan to execute
#define SPX_SCAN_STATE_TRANSFER			0x20		// scan is done, waiting for data transfer to be finished
#define SPX_SCAN_STATE_IN_PROGRES		0x40		// scan is in progres, this message should never be get
#define SPX_SCAN_STATE_WAIT_TRIGGER 	0x80		// same as IDLE except that external trigger is armed
#define SPX_SCAN_STATE_UNKNOWN			0xFF  	// default value

	//	calibration data set
#define SPX_CAL_DATA_SET_FACTORY			0
#define SPX_CAL_DATA_SET_USER				1

/*---------------------------------------------------------------------------
 GLOBAL USER-CALLABLE FUNCTION DECLARATIONS (Exportable Functions)
---------------------------------------------------------------------------*/
	
/*-------------------------------------------------------------------------------
	Please look into LC1_Drv.fp for further information about this functions...
--------------------------------------------------------------------------------*/
	
// Init - Close
ViStatus _VI_FUNC SPX_init (ViRsrc rsc, ViInt32 tmo, ViPSession instrp);

ViStatus _VI_FUNC SPX_close (ViSession instrument);

// Configuration functions
ViStatus _VI_FUNC SPX_setIntTime (ViSession instr, ViReal64 time);
ViStatus _VI_FUNC SPX_getIntTime (ViSession instr, ViPReal64 time);

// Action/Status functions
ViStatus _VI_FUNC SPX_startScan (ViSession instr);
ViStatus _VI_FUNC SPX_startScanExtTrg (ViSession instr);
ViStatus _VI_FUNC SPX_startScanCont (ViSession instr);
ViStatus _VI_FUNC SPX_startScanContExtTrg (ViSession instr);
ViStatus _VI_FUNC SPX_getDeviceStatus (ViSession instr, ViPUInt8 deviceStatusByte);

// Data functions
ViStatus _VI_FUNC SPX_getScanData (ViSession instr, ViReal64 _VI_FAR scanDataArray[]);
ViStatus _VI_FUNC SPX_getRawScanData (ViSession instr, ViUInt16 _VI_FAR scanDataRawArray[]);

ViStatus _VI_FUNC SPX_setUserWavelengthData (ViSession instr, ViInt32 pixelDataArray[],
                                         ViReal64 wavelengthDataArray[], ViInt32 bufferLength);

ViStatus _VI_FUNC SPX_getUserWavelengthData (ViSession instr, ViInt32 pixelDataArray[],
                                             ViReal64 wavelengthDataArray[], ViInt32 *bufferLength);

ViStatus _VI_FUNC SPX_getWavelengthData (ViSession instr, ViReal64 wavelengthDataArray[],
													  ViReal64 *minimumWavelength, ViReal64 *maximumWavelength);

ViStatus _VI_FUNC SPX_getWavelengthDataExt (ViSession instr, ViUInt8 dataSet, ViReal64 wavelengthDataArray[],
													  ViReal64 *minimumWavelength, ViReal64 *maximumWavelength);


// Utility functions
ViStatus _VI_FUNC SPX_reset (ViSession instr);
ViStatus _VI_FUNC SPX_errorMessage (ViSession instr, ViStatus status, ViChar _VI_FAR message[]);
ViStatus _VI_FUNC SPX_identificationQuery (ViSession instr, ViChar _VI_FAR manufacturerName[], ViChar _VI_FAR instrumentName[], ViChar _VI_FAR instrumentSerialNumber[], ViChar _VI_FAR firmwareRevision[]);
ViStatus _VI_FUNC SPX_revisionQuery (ViSession instr, ViChar _VI_FAR instrumentDriverRevision[], _VI_FAR ViChar firmwareRevision[]);
ViStatus _VI_FUNC SPX_setUserText (ViSession instr, ViChar _VI_FAR userText[]);
ViStatus _VI_FUNC SPX_getUserText (ViSession instr, ViChar _VI_FAR userText[]);



#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif	/* __SPX_HEADER */

/****************************************************************************
  End of Header file
****************************************************************************/
