/****************************************************************************

	Thorlabs SPX_Drv - SP1-USB/SP2-USB SPxyz-USB spectrometer VISA instrument driver

 	Copyright: 	Copyright(c) 2005, 2006, 2007 Thorlabs GmbH (www.thorlabs.com)
 	Author:		Lutz Hoerl (lhoerl@thorlabs.com)

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


 	Source file

 	Date:       	Nov-28-2007
 	Built with:    NI LabWindows/CVI 8.5
 	Software-Nr:   09.167.200
 	Version:    	2.5

	Changelog:

	Lutz Hoerl:			Nov-03-2005 - Ver. 0.1 begun, mainly copied from 09163 LC1-USB driver
	Lutz Hoerl:			Nov-15-2005 - Ver. 1.0 first release
	Lutz Hoerl:			Nov-23-2005 - Ver. 1.1 added code to accept firmware Rev. 1.31
	Lutz Hoerl:			Feb-10-2006 - Ver. 1.2 added continuous readout mode
	Lutz Hoerl:			Apr-11-2006 - Ver. 1.3 BUGFIX order of data was incorrect (high wavelength mirrored to low and vice versa)
	Lutz Hoerl:			Aug-21-2007 - Ver. 2.0 recompiled with CVI 8.1.1, included now *.inf for 2000/XP and Vista
	Thomas Schlosser:	Nov-28-2007 - Ver. 2.1 - compiled with CVI 8.5
														  - NEW: New functions to save/read user wavelength adjustment data to/from the instruments nonvolatile memory
	Olaf Wohlmann		Dec-10-2007 - Ver. 2.2 - new EEPROM writing function
														  - calibration with ascending and descending arrays
	Olaf Wohlmann		Dec-14-2007 - Ver  2.3 - new EEPROM reading function
														  - advanced memory layout including sensor data
	Olaf Wohlmann		Dec-17-2007	- Ver	 2.4 - Vista drivers are only copied to program folder
	Olaf Wohlmann		Feb-01-2008	- Ver  2.5 - Vista drivers are installed in VISTA environment
														  
****************************************************************************/

/* Minimal mods to compile this with only GPL code on Linix */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "vitypes.h"
#include "spxusb.h"
#include "spxdrv.h"
#include "version.h"
/* turn off windows twaddle */
# define __declspec(dllexport)

/*===========================================================================
  Macros
===========================================================================*/
// SPX_DRIVER_REVISION was "2.5", now comes from from version.h  // Instrument driver revision
#define SPX_NO_REVISION_TXT				"not applicable"	// In function 'SPX_revisionQuery()' this is returned for the instrument revision if no connection exists.
#define SPX_MAX_STD_ANSWER					8						// the spectrometer returns some data

// Visa attributes
#define SPX_BULK_IN_PIPE_NUMBER			0x86
#define SPX_BULK_OUT_PIPE_NUMBER			0x02

// USB request types (bmRequestType)
#define REQTYPE_OUT							0x00
#define REQTYPE_IN							0x80

#define REQTYPE_STD							0x00
#define REQTYPE_CLASS						0x20
#define REQTYPE_VENDOR						0x40
#define REQTYPE_RESERVED					0x60

#define REQTYPE_DEVICE						0x00
#define REQTYPE_INTERF						0x01
#define REQTYPE_ENDPOINT					0x02
#define REQTYPE_OTHERS						0x03

// USB Requests
#define REQUEST_GET_DESCRIPTOR			0x06
#define REQUEST_LINECAM						0xA1
#define REQUEST_READ_EEPROM				0xB8
#define REQUEST_WRITE_EEPROM				0xB7
#define GET_PRODUCT_INFORMATION			0xBB

// SPX specific USB requests
// STOP SCAN
	#define STOP_SCAN_WVAL					0x0006
	#define STOP_SCAN_WINDEX				0x0000
	#define STOP_SCAN_LENGTH				0x0001
// SET INTEGRATION TIME
	#define SPX_SET_INT_TIME				0x0004
// REQUEST SCAN
	#define REQUEST_SCAN_WVAL				0x0005
	#define REQUEST_SCAN_ET_WVAL			0x0007
	#define REQUEST_SCAN_STATUS			0x0008
	#define REQUEST_SCANCONT_WVAL			0x0015
	#define REQUEST_SCANCONT_ET_WVAL		0x0017

// specific sub-requests for GET_PRODUCT_INFORMATION request
	#define GET_FIRMWARE_VERSION			0x0001
	#define GET_HARDWARE_REVISION			0x0002


// USB descriptor types
#define DESCRTYPE_DEVICE					1
#define DESCRTYPE_STRING					3

// USB descriptor length
#define DESCR_LEN_HEADER					2
#define DESCR_LEN_DEVICE					18

// SPX specific private constants
#define SPX_DEFAULT_INT_TIME				0.001		// 1ms
#define SPX_RESOLUTION_LIMIT 				65000		// ~16bit border of timer in ATMega
#define SPX_usec_PADDING					3			// we add 3usec to timer 1
#define SPX_TIMER2_TICKS					19 		// we add 2.5usec to timer 2
#define SPX_BLACK_START						37			// first even black pixel
#define SPX_BLACK_STOP						57			// first pixel after last odd black pixel
#define SPX_SCAN_START						68			// index of first pixel of eff. scan data
#define SPX_MAX_ADC_VALUE					4095		// this is the maximum value a 12bit ADC can produce
#define SPX_PIXEL_BUFFER					3068		// this is the number of words read out by the driver
#define SPX_CALIB_VALID_FLAG				0x5A		// this is the value for check bytes
#define SPX_USERCAL_VALID_FLAG			0x5A		// user wavelenght adjustment data is valid
#define SPX_MAX_BYTES_TO_TRANSFER		63


// ORIGINALLY FROM 'dscr.h' (the LC1/SP1/SP2 sources for the uC)
#define VENDOR_NAME_LENGTH					8
#define PRODUCT_NAME_LENGTH				30
#define SERIAL_NO_LABEL_LENGTH			4
#define SERIAL_NO_LENGTH					10
#define SW_VERSION_LENGTH					4
#define USER_LABEL_LENGTH					32
#define EE_CALIB_COEF_DATA_FLAG_LENGTH	2
#define EE_CALIB_COEF_DATA_LENGTH		32
#define CALIB_DATA_LENGTH					(EE_CALIB_COEF_DATA_FLAG_LENGTH + EE_CALIB_COEF_DATA_LENGTH)
#define OFFSET_MAX_LENGTH					sizeof(ViUInt16)

#define SENSOR_NUM_PIXEL_LENGTH			2
#define SENSOR_MAX_READ_PIXEL_LENGTH	2
#define SENSOR_MAX_ACTIVE_PIXEL_LENGTH	2
#define SENSOR_PIXEL_SIZE_LENGTH			2	

#define EE_USERCAL_DATA_FLAG_LENGTH		1
#define EE_USERCAL_DATA_LEN_LENGTH		1
#define EE_USERCAL_DATA_LENGTH			SPX_MAX_NUM_USR_ADJ * (sizeof(ViInt32) + sizeof(ViReal64))
#define USERCAL_FILE_LENGTH				(EE_USERCAL_DATA_FLAG_LENGTH + EE_USERCAL_DATA_LEN_LENGTH + EE_USERCAL_DATA_LENGTH)


#define EE_VENDOR_ID							0
#define EE_PRODUCT_ID						2
#define EE_DEVICE_RELEASE					4
#define EE_SERIAL_NUMBER 					6
#define EE_SW_VERSION_NUMBER				(EE_SERIAL_NUMBER				+ SERIAL_NO_LENGTH					)
#define EE_DEVICE_USER_LABEL				(EE_SW_VERSION_NUMBER		+ SW_VERSION_LENGTH					)
#define EE_CALIB_COEF_DATA_FLAG			(EE_DEVICE_USER_LABEL		+ USER_LABEL_LENGTH					)
#define EE_CALIB_COEF_DATA					(EE_CALIB_COEF_DATA_FLAG	+ EE_CALIB_COEF_DATA_FLAG_LENGTH	)
#define EE_EVEN_OFFSET_MAX					(EE_CALIB_COEF_DATA 			+ EE_CALIB_COEF_DATA_LENGTH		)
#define EE_ODD_OFFSET_MAX					(EE_EVEN_OFFSET_MAX 			+ OFFSET_MAX_LENGTH					)

#define EE_SENSOR_NUM_PIXEL				(EE_ODD_OFFSET_MAX			+ OFFSET_MAX_LENGTH					)
#define EE_SENSOR_MAX_READ_PIXEL			(EE_SENSOR_NUM_PIXEL			+ SENSOR_NUM_PIXEL_LENGTH			)
#define EE_SENSOR_MAX_ACTIVE_PIXEL		(EE_SENSOR_MAX_READ_PIXEL	+ SENSOR_MAX_READ_PIXEL_LENGTH	)
#define EE_SENSOR_PIXEL_SIZE				(EE_SENSOR_MAX_ACTIVE_PIXEL+ SENSOR_MAX_ACTIVE_PIXEL_LENGTH	)
#define EE_USERCAL_DATA_FLAG				(EE_SENSOR_PIXEL_SIZE		+ SENSOR_PIXEL_SIZE_LENGTH			)
#define EE_USERCAL_DATA_LEN				(EE_USERCAL_DATA_FLAG		+ EE_USERCAL_DATA_FLAG_LENGTH		)
#define EE_USERCAL_DATA						(EE_USERCAL_DATA_LEN			+ EE_USERCAL_DATA_LEN_LENGTH		)


//	Debug
#ifdef _CVI_DEBUG_ 
	#define DEBUG_NO_EEPROM_WRITE	//	if defined no EEPROM data can be changed 	
#endif // _CVI_DEBUG_

/*===========================================================================
  Structures
===========================================================================*/

// static error list
typedef struct
{
	ViStatus err;
	ViString descr;
} SPX_err_descr_stat_t;


//	wavelength calibration array
typedef struct
{
	ViReal64			poly[4];					// polynomial coefficients for pixel - wavelength computation
	ViReal64			wl[SPX_NUM_PIXELS];	// array of wavelengths according to pixel number
	ViReal64			min;						// lower wavelength limit
	ViReal64			max;						// upper wavelength limit
} SPX_wl_cal_t;


// driver private data
typedef struct
{
	ViAttrState		timeout;

	ViChar			vendor[SPX_BUFFER_SIZE];
	ViChar			name[SPX_BUFFER_SIZE];
	ViChar			serNr[SPX_BUFFER_SIZE];
	int				version_major;
	int				version_minor;
	int				hardware_major;
	int				hardware_minor;
	ViUInt8 			timerReload;
	ViReal64 		intTime;
	unsigned char	TCCR1B;
	unsigned short	timer_ticks;
	unsigned short	additional_clock_ticks;

	unsigned short	Offset_Even_Max;
	unsigned short	Offset_Odd_Max;

	//	wavelength calibration data
	SPX_wl_cal_t	factory_cal;
	SPX_wl_cal_t	user_cal;
	char				user_cal_valid;
		//	user calibration supporting points
	ViInt32			user_cal_node_cnt;								//	number of user-defined supporting points
	ViInt32			user_cal_node_pixel[SPX_MAX_NUM_USR_ADJ]; //	pixel array of supporting points
	ViReal64			user_cal_node_wl[SPX_MAX_NUM_USR_ADJ];		//	wavelength array of supporting points
	
	ViUInt16 		rawScanData[SPX_PIXEL_BUFFER];
	ViChar			spx_answer_buf[SPX_MAX_STD_ANSWER];
	ViUInt16 		ReturnCount;
} SPX_data_t;


/*===========================================================================
  Prototypes
===========================================================================*/
// Range checking
static ViBoolean SPX_invalidViInt32Range (ViInt32 val, ViInt32 min, ViInt32 max);
#ifdef WE_MAY_NEED_THIS_LATER
static ViBoolean SPX_invalidViBooleanRange (ViBoolean val);
static ViBoolean SPX_invalidViInt16Range (ViInt16 val, ViInt16 min, ViInt16 max);
static ViBoolean SPX_invalidViUInt8Range (ViUInt8 val, ViUInt8 min, ViUInt8 max);
static ViBoolean SPX_invalidViUInt16Range (ViUInt16 val, ViUInt16 min, ViUInt16 max);
static ViBoolean SPX_invalidViUInt32Range (ViUInt32 val, ViUInt32 min, ViUInt32 max);
static ViBoolean SPX_invalidViReal32Range (ViReal32 val, ViReal32 min, ViReal32 max);
static ViBoolean SPX_invalidViReal64Range (ViReal64 val, ViReal64 min, ViReal64 max);
#endif

// Closing
static ViStatus SPX_initCleanUp (ViSession RMSession, ViPSession pInstr, ViStatus err);
static ViStatus SPX_initClose (ViPSession pInstr, ViStatus err);

// Misc
void SPX_getInstrumentRev(SPX_data_t *data, ViChar *str);
static ViStatus SPX_getWavelengthParameters (ViSession instr);
static ViStatus SPX_readEEFactoryPoly(ViSession instr, ViReal64 poly[]);
static ViStatus SPX_writeEEFactoryPoly(ViSession instr, ViReal64 poly[]);
static ViStatus SPX_readEEUserPoints(ViSession instr, ViInt32 pixel[], ViReal64 wl[], ViInt32 *cnt);
static ViStatus SPX_writeEEUserPoints(ViSession instr, ViInt32 pixel[], ViReal64 wl[], ViInt32 cnt);
static ViStatus SPX_checkNodes(ViInt32 pixel[], ViReal64 wl[], ViInt32 cnt);
static ViStatus SPX_nodes2poly(ViInt32 pixel[], ViReal64 wl[], ViInt32 cnt, ViReal64 poly[]);
static void SPX_initDefaultPoly(ViReal64 poly[]);
static ViStatus SPX_poly2wlArray(SPX_wl_cal_t *wl);
static ViStatus SPX_getDarkCurrentOffset (ViSession instr);
static ViStatus SPX_getFirmwareRevision (ViSession instr);
static ViStatus SPX_getHardwareRevision (ViSession instr);
static ViStatus SPX_acquireScanDataRaw (ViSession instr);
static ViStatus SPX_ProcessScanData (SPX_data_t *data, ViReal64 _VI_FAR scanDataArray[]);
static int LeastSquareInterpolation (int *x, double *y, int cnt, double *polyout);

__declspec(dllexport) ViStatus SPX_writeEEPROM(ViSession instr, ViUInt16	wValue, ViUInt16 wIndex, ViUInt16 wLength, char	buffer[]);
__declspec(dllexport) ViStatus SPX_readEEPROM(ViSession instr, unsigned char	bRequest, ViUInt16 wValue, ViUInt16 wIndex, ViUInt16 wLength, char buffer[], ViUInt16* iReturnCount);
/****************************************************************************

  Init/Close

****************************************************************************/
/*---------------------------------------------------------------------------
  Function: Initialize
  Purpose:  This function opens the instrument, queries the instrument
				for its ID, and initializes the instrument to a known state.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_init (ViRsrc resource, ViInt32 timeout, ViPSession pInstr)
{
	ViStatus    err = VI_SUCCESS;
	ViSession   rmngr = 0;
	ViInt32		i, noMatch;
	SPX_data_t	*data;

	ViChar	*id_vendors[] =
				{
					"Thorlabs",
					"THORLABS GMBH",
					"THORLABS",
					"THORLABS INC",
					NULL
				};

	ViChar	*id_names[] =
				{
					"SP1",
					"SP2",
					"CCS100",
					"CCS175",
					"SP1-USB",
					"SP2-USB",
					"SP1-USB Spectrometer         FW ver 1.30",
					"SP2-USB Spectrometer         FW ver 1.30",
					"SP1-USB Spectrometer                    ",
					"SP2-USB Spectrometer                    ",
					NULL
				};



	//Check input parameter ranges
	if (SPX_invalidViInt32Range (timeout, SPX_TIMEOUT_MIN, SPX_TIMEOUT_MAX))	return VI_ERROR_PARAMETER2;

	//Open instrument session
	if ((err = viOpenDefaultRM(&rmngr)) < 0) return err;
	if ((err = viOpen(rmngr, resource, VI_EXCLUSIVE_LOCK, timeout, pInstr)) < 0)
	{
		viClose (rmngr);
		return err;
	}

	// Configure Sessionchar           ViChar;
	if ((err = viSetAttribute(*pInstr, VI_ATTR_TMO_VALUE, timeout)) < 0)      								return SPX_initCleanUp(rmngr, pInstr, err);
	if ((err = viSetAttribute(*pInstr, VI_ATTR_USB_BULK_IN_PIPE,  SPX_BULK_IN_PIPE_NUMBER))  < 0) 	return SPX_initCleanUp(rmngr, pInstr, err);
	if ((err = viSetAttribute(*pInstr, VI_ATTR_USB_BULK_OUT_PIPE, SPX_BULK_OUT_PIPE_NUMBER)) < 0)  	return SPX_initCleanUp(rmngr, pInstr, err);
	if ((err = viSetAttribute(*pInstr, VI_ATTR_USB_END_IN, 		  VI_USB_END_SHORT))         < 0)  	return SPX_initCleanUp(rmngr, pInstr, err);


	// Allocate private driver data
	if((data = (SPX_data_t*)malloc(sizeof(SPX_data_t))) == NULL) return SPX_initCleanUp(rmngr, pInstr, VI_ERROR_SYSTEM_ERROR);	
	if((err = viSetAttribute (*pInstr, VI_ATTR_USER_DATA, (ViAttrState)data)) < 0)  return SPX_initClose(pInstr, err);

	// store the given timeout value for later use
	data->timeout = (ViAttrState)timeout;

	viGetAttribute(*pInstr, VI_ATTR_MANF_NAME, 		data->vendor);
	viGetAttribute(*pInstr, VI_ATTR_MODEL_NAME,		data->name);
	viGetAttribute(*pInstr, VI_ATTR_USB_SERIAL_NUM, data->serNr);

	// Does Vendor name match?
	i = 0;
	noMatch = VI_TRUE;
	while(noMatch && (id_vendors[i] != NULL))
	{
		noMatch = strcmp(data->vendor,id_vendors[i]);
		i ++;
	}
	if(noMatch)	return SPX_initClose(pInstr, VI_ERROR_FAIL_ID_QUERY);

	// Does Device name match?
	i = 0;
	noMatch = VI_TRUE;
	while(noMatch && (id_names[i] != NULL))
	{
		noMatch = strcmp(data->name, id_names[i]);
		i++;
	}
	if(noMatch) return SPX_initClose(pInstr, VI_ERROR_FAIL_ID_QUERY);

	// Reset device

	// Configure device
	// set the default integration time
	if((err = SPX_setIntTime (*pInstr, SPX_DEFAULT_INT_TIME))) return SPX_initClose(pInstr, err);

	// get wavelength to pixel calculation parameters
	if((err = SPX_getWavelengthParameters (*pInstr))) return SPX_initClose(pInstr, err);

	// get dark current offset values
	if((err = SPX_getDarkCurrentOffset (*pInstr))) return SPX_initClose(pInstr, err);

	// get firmware revision
	if((err = SPX_getFirmwareRevision (*pInstr))) return SPX_initClose(pInstr, err);

	// get hardware revision
	if((err = SPX_getHardwareRevision (*pInstr))) return SPX_initClose(pInstr, err);

	return VI_SUCCESS;
}


/*---------------------------------------------------------------------------
  Function: Close
  Purpose:  This function closes the instrument session.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_close (ViSession instr)
{
	return SPX_initClose(&instr, VI_SUCCESS);
}


/****************************************************************************

 Class: Configuration Functions.

****************************************************************************/
/*---------------------------------------------------------------------------
  Function: Set Integration Time
  Purpose:  sets the Spectrometers integration time
				regard this as a kind of hardware averaging
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_setIntTime (ViSession instr, ViReal64 intTime)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	ViUInt16 		wValue, wIndex, wLength;
	unsigned long	time_in_usec;
	unsigned char	additional_clock_ticks; // in .125 microseconds
	unsigned short timer_ticks;
	unsigned char	TCCR1B;

	// check the range
	if(intTime > SPX_MAX_INT_TIME)	return VI_ERROR_PARAMETER2;
	if(intTime < SPX_MIN_INT_TIME)	return VI_ERROR_PARAMETER2;

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	time_in_usec = (unsigned long)(1000000.0 * intTime); // whole microseconds

	// if greater (16 bit timer -) limit we must reduce resoultion
	if (time_in_usec > SPX_RESOLUTION_LIMIT)
	{
		TCCR1B = 0x03;
		additional_clock_ticks = (unsigned char)((((time_in_usec + SPX_usec_PADDING) % 8) * 8) + SPX_TIMER2_TICKS);  // 125ns ticks
		timer_ticks = (unsigned short)((time_in_usec + SPX_usec_PADDING) / 8);
	}
	else
	{
		TCCR1B = 0x02;
		additional_clock_ticks = SPX_TIMER2_TICKS;
		timer_ticks = (unsigned short)(time_in_usec + SPX_usec_PADDING);
	}

	timer_ticks = (unsigned short)((~timer_ticks) + 1); 								// timer value is  2's complement
	additional_clock_ticks = (unsigned char)((~additional_clock_ticks) + 1);	// value is  2's complement

	wValue   = (SPX_SET_INT_TIME & 0x00FF) | (timer_ticks & 0xFF00);											// command + TimerHB
	wIndex   = (timer_ticks & 0x00FF) | (((unsigned short)additional_clock_ticks << 8) & 0xFF00);	// TimerLB + Additional Ticks
	wLength  = 4;

	if((err = SPX_readEEPROM(instr, REQUEST_LINECAM, wValue, wIndex, wLength, (data->spx_answer_buf), &data->ReturnCount)) != VI_SUCCESS) return err;  
	if(data->ReturnCount != wLength) return VI_ERROR_IO;	// not all data read

	// on success store the given and the calculated values
	data->TCCR1B = TCCR1B;
	data->timer_ticks = timer_ticks;
	data->additional_clock_ticks = additional_clock_ticks;
	data->intTime = intTime;
	return VI_SUCCESS;
}


/*---------------------------------------------------------------------------
  Function: Get Integration Time
  Purpose:  gets back the Spectrometers integration time
				the time is calculted from the real used timer values and may
				therefore be slightly different from the value set with SPX_setIntTime();
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_getIntTime (ViSession instr, ViPReal64 intTime)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	unsigned long	time_in_usec;
	unsigned short timer_ticks;
	unsigned char	additional_clock_ticks; // in .125 microseconds


	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// from original value retrieve the way how to reconvert the integration time
	if ((unsigned long)(1000000.0 * data->intTime) > SPX_RESOLUTION_LIMIT)
	{
		timer_ticks = ~((unsigned short)(data->timer_ticks - 1));
		additional_clock_ticks = ~(data->additional_clock_ticks - 1);

		time_in_usec = (unsigned long)timer_ticks * 8 - SPX_usec_PADDING;
		time_in_usec += (unsigned long)(additional_clock_ticks - SPX_TIMER2_TICKS) / 8;
	}
	else
	{
		timer_ticks = ~((unsigned short)(data->timer_ticks - 1));
		time_in_usec = (unsigned long)timer_ticks - SPX_usec_PADDING;
	}

	*intTime = (ViReal64)time_in_usec * 0.000001;
	return VI_SUCCESS;
}



/****************************************************************************

 Class: Status/Action Functions.

****************************************************************************/
/*---------------------------------------------------------------------------
  Function: Start Scan
  Purpose:  makes the Spectrometer to immediately start a scan.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_startScan (ViSession instr)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt16			wValue, wIndex, wLength;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	unsigned char	bRequest;

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// REQUEST ONE SCAN
	bRequest = REQUEST_LINECAM;
	wValue = REQUEST_SCAN_WVAL | (data->TCCR1B << 8);
	wIndex = 0;
	wLength = 2;

	err = SPX_readEEPROM( instr, bRequest, wValue, wIndex, wLength, data->spx_answer_buf, &data->ReturnCount);

	return err;
}

/*---------------------------------------------------------------------------
  Function: Start Scan with external Trigger
  Purpose:  makes the Spectrometer to arm for an external trgger to start a scan.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_startScanExtTrg (ViSession instr)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt16			wValue, wIndex, wLength;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	unsigned char	bRequest;

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// REQUEST ONE SCAN with external trigger
	bRequest = REQUEST_LINECAM;
	wValue = REQUEST_SCAN_ET_WVAL | (data->TCCR1B << 8);
	wIndex = 0;
	wLength = 2;

	err = SPX_readEEPROM(instr, bRequest, wValue, wIndex, wLength, data->spx_answer_buf, &data->ReturnCount);

	return err;
}

/*---------------------------------------------------------------------------
  Function: Start Scan Cont
  Purpose:  makes the Spectrometer to immediately start scanning continuously.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_startScanCont (ViSession instr)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt16			wValue, wIndex, wLength;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	unsigned char	bRequest;

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// REQUEST ONE SCAN
	bRequest = REQUEST_LINECAM;
	wValue = REQUEST_SCANCONT_WVAL | (data->TCCR1B << 8);
	wIndex = 0;
	wLength = 2;

	err = SPX_readEEPROM(instr, bRequest, wValue, wIndex, wLength, data->spx_answer_buf, &data->ReturnCount);

	return err;
}

/*---------------------------------------------------------------------------
  Function: Start Scan Cont with external Trigger
  Purpose:  makes the Spectrometer to start scanning after an external trigger
				and rearming itself after readout of the scan data.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_startScanContExtTrg (ViSession instr)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt16			wValue, wIndex, wLength;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	unsigned char	bRequest;

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// REQUEST ONE SCAN
	bRequest = REQUEST_LINECAM;
	wValue = REQUEST_SCANCONT_ET_WVAL | (data->TCCR1B << 8);
	wIndex = 0;
	wLength = 2;

	err = SPX_readEEPROM(instr, bRequest, wValue, wIndex, wLength, data->spx_answer_buf, &data->ReturnCount);

	return err;
}


/*---------------------------------------------------------------------------
  Function: Get Device Stataus
  Purpose:  returns the state of a scan mainly used for status of external trigger
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_getDeviceStatus (ViSession instr, ViPUInt8 deviceStatusByte)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);

	*deviceStatusByte = SPX_SCAN_STATE_UNKNOWN;

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// REQUEST the state of the Spectrometer (triggered, scanning, ...)      
	if((err = SPX_readEEPROM(instr, REQUEST_LINECAM, REQUEST_SCAN_STATUS, 0, 2, data->spx_answer_buf, &data->ReturnCount)))	return err;

	if(data->ReturnCount == 1)
	{
		switch((unsigned char)data->spx_answer_buf[0])
		{
			case SPX_SCAN_STATE_IDLE 			:
			case SPX_SCAN_STATE_TRANSFER		:
			case SPX_SCAN_STATE_WAIT_TRIGGER :
															*deviceStatusByte = data->spx_answer_buf[0];
															break;
			default									:
															break;
		}
	}

	return err;
}


/****************************************************************************

 Class: Data Functions.

****************************************************************************/

/*---------------------------------------------------------------------------
  Function: Get Scan Data
  Purpose:  returns an array of scan data
---------------------------------------------------------------------------*/

ViStatus _VI_FUNC SPX_getScanData (ViSession instr, ViReal64 _VI_FAR scanDataArray[])
{
	SPX_data_t 		*data;
	ViStatus 		err;

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	if((err = SPX_acquireScanDataRaw (instr)) != VI_SUCCESS) return err;
	return SPX_ProcessScanData (data, scanDataArray);
}

/*---------------------------------------------------------------------------
  Function: Get Raw Scan Data
  Purpose:  returns an array of scan data
---------------------------------------------------------------------------*/

ViStatus _VI_FUNC SPX_getRawScanData (ViSession instr, ViUInt16 _VI_FAR scanDataRawArray[])
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt32			i;

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	if((err = SPX_acquireScanDataRaw (instr)) != VI_SUCCESS) return err;

	// copy, invert and reorder all data
	for(i = 0; i < (SPX_PIXEL_BUFFER - SPX_SCAN_START); i++)
	{
		scanDataRawArray[i] = SPX_MAX_ADC_VALUE - data->rawScanData[SPX_PIXEL_BUFFER - 1 - i];
	}	

	return VI_SUCCESS;
}

/*---------------------------------------------------------------------------
  Function: Set User Wavelength Data
  Purpose:  saves user defined reference data to the EEPROM
---------------------------------------------------------------------------*/

ViStatus _VI_FUNC SPX_setUserWavelengthData (ViSession instr, ViInt32 pixelDataArray[],
                                         ViReal64 wavelengthDataArray[], ViInt32 bufferLength)
{
#define FACTORY_ADJ_OFFSET			62749006

	SPX_data_t 		*data;
	SPX_wl_cal_t	cal;
	ViStatus			err;
	char				target;
	
	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;
	
	//	check valid buffer length and determine target
	if((bufferLength >= SPX_MIN_NUM_USR_ADJ) && (bufferLength <= SPX_MAX_NUM_USR_ADJ))
	{
		target = 1;	//	target is user adjustment data
	}
	else if ((bufferLength >= (SPX_MIN_NUM_USR_ADJ + FACTORY_ADJ_OFFSET)) 
					&& (bufferLength <= (SPX_MAX_NUM_USR_ADJ  + FACTORY_ADJ_OFFSET)))
	{
		bufferLength -= FACTORY_ADJ_OFFSET;
		target = 0;	//	target is factory adjustment data
	}
	else return VI_ERROR_INV_PARAMETER;
	
	//	check nodes array
	if((err = SPX_checkNodes(pixelDataArray, wavelengthDataArray, bufferLength)) != VI_SUCCESS)	return err;
	
	// calculate new coefficients...
	if((err = SPX_nodes2poly(pixelDataArray, wavelengthDataArray, bufferLength, cal.poly)) != VI_SUCCESS)	return err;
	
	// use the coefficients to calculate the new wavelength array...
	if((err = SPX_poly2wlArray(&cal)) != VI_SUCCESS)	return err;
	
	//	write new data to EEPROM and data structure
	if(target)
	{
		//	target is user adjustment data
		if((err = SPX_writeEEUserPoints(instr, pixelDataArray, wavelengthDataArray, bufferLength)) != VI_SUCCESS) return err;

		// copy new values to SPX data
		memcpy(&(data->user_cal), &cal, sizeof(SPX_wl_cal_t));
		memcpy(data->user_cal_node_pixel, pixelDataArray, sizeof(ViInt32) * bufferLength);
		memcpy(data->user_cal_node_wl, wavelengthDataArray, sizeof(ViReal64) * bufferLength);
		data->user_cal_node_cnt = bufferLength;
		data->user_cal_valid = 1;
	}
	else
	{
		//	target is factory adjustment data
		if((err = SPX_writeEEFactoryPoly(instr, cal.poly)) != VI_SUCCESS) return err;   
		
		// copy new values to SPX data
		memcpy(&(data->factory_cal), &cal, sizeof(SPX_wl_cal_t));
	}

	return VI_SUCCESS;   
}


ViStatus _VI_FUNC SPX_getUserWavelengthData (ViSession instr, ViInt32 pixelDataArray[],
                                             ViReal64 wavelengthDataArray[], ViInt32 *bufferLength)
{
	SPX_data_t 		*data;
	ViStatus			err;

	// get SPX data
	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;
	
	//	user adjustment data available?
	if(!data->user_cal_valid)
	{
		if(bufferLength != VI_NULL) *bufferLength = 0;
		return VI_ERROR_NO_USER_DATA;
	}
	
	//	copy data
	if(bufferLength != VI_NULL) *bufferLength = data->user_cal_node_cnt;
	if(pixelDataArray != VI_NULL)
	{
		memcpy(pixelDataArray, data->user_cal_node_pixel, data->user_cal_node_cnt * sizeof(ViInt32));
	}
	if(wavelengthDataArray != VI_NULL)
	{
		memcpy(wavelengthDataArray, data->user_cal_node_wl, data->user_cal_node_cnt * sizeof(ViReal64));
	}

	return VI_SUCCESS;
}


/*---------------------------------------------------------------------------
  Function: Get Wavelength Data
  Purpose:  returns an array of wavelength data
  
	### this function is obsolete ! ###
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_getWavelengthData (ViSession instr, ViReal64 wavelengthDataArray[], 
														ViReal64 *minimumWavelength, ViReal64 *maximumWavelength)
{
	return SPX_getWavelengthDataExt (instr, SPX_CAL_DATA_SET_FACTORY, wavelengthDataArray, minimumWavelength, maximumWavelength);
}


/*---------------------------------------------------------------------------
  Function: Get Wavelength Data Extended
  Purpose:  returns an array of wavelength data
---------------------------------------------------------------------------*/

ViStatus _VI_FUNC SPX_getWavelengthDataExt (ViSession instr, ViUInt8 dataSet, ViReal64 wavelengthDataArray[], 
												ViReal64 *minimumWavelength, ViReal64 *maximumWavelength)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	
	switch (dataSet)
	{
		case SPX_CAL_DATA_SET_FACTORY:
			//	copy wavelength array form factory data
			if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;
	 
			if(wavelengthDataArray != NULL)	memcpy(wavelengthDataArray, data->factory_cal.wl, (SPX_NUM_PIXELS * sizeof(ViReal64)));
			if(minimumWavelength != NULL)		*minimumWavelength = data->factory_cal.min;
			if(maximumWavelength != NULL)		*maximumWavelength = data->factory_cal.max;
			break;
			
		case SPX_CAL_DATA_SET_USER:
			//	copy wavelength array form user data
			if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;
			if(!data->user_cal_valid) return VI_ERROR_NO_USER_DATA;
	 
			if(wavelengthDataArray != NULL)	memcpy(wavelengthDataArray, data->user_cal.wl, (SPX_NUM_PIXELS * sizeof(ViReal64)));
			if(minimumWavelength != NULL)		*minimumWavelength = data->user_cal.min;
			if(maximumWavelength != NULL)		*maximumWavelength = data->user_cal.max;
			break;
			
		default:
			return VI_ERROR_INV_PARAMETER;
			break;
	}

	return VI_SUCCESS;
}


/****************************************************************************

 Class: Application Functions.

****************************************************************************/


/****************************************************************************

 Class: Utility Functions.

****************************************************************************/
/*---------------------------------------------------------------------------
 Function: Reset
 Purpose:  This function puts the instrument in a known state.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_reset (ViSession instr)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt8			bmType;
	ViUInt16 		wValue, wIndex, wLength;

	// get handle
	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// reset the integration time
	bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);

	wValue   = (SPX_SET_INT_TIME & 0x00FF) | (data->timer_ticks & 0xFF00);													// command + TimerHB
	wIndex   = (data->timer_ticks & 0x00FF) | (((unsigned short)data->additional_clock_ticks << 8) & 0xFF00);	// TimerLB + Additional Ticks
	wLength  = 4;

	
	if((err = SPX_readEEPROM(instr, REQUEST_LINECAM, wValue, wIndex, wLength, data->spx_answer_buf, &data->ReturnCount)))	return err;
	if(data->ReturnCount != wLength) return VI_ERROR_IO;	// not all data read

	// do an one time readout to collect garbled buffers
	err = viFlush (instr, VI_READ_BUF);

	return err;
}

/*---------------------------------------------------------------------------
 Function: Error Message
 Purpose:  This function translates the error return value from the
			  instrument driver into a user-readable string.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_errorMessage(ViSession instr, ViStatus stat, ViChar _VI_FAR msg[])
{
	ViInt32 		i, found;

	static SPX_err_descr_stat_t err_msg_list[] =
	{
		{VI_WARN_NSUP_ID_QUERY,   		"WARNING: ID Query not supported"         						},
		{VI_WARN_NSUP_RESET,          "WARNING: Reset not supported"            						},
		{VI_WARN_NSUP_SELF_TEST,      "WARNING: Self-test not supported"        						},
		{VI_WARN_NSUP_ERROR_QUERY,    "WARNING: Error Query not supported"      						},
		{VI_WARN_NSUP_REV_QUERY,      "WARNING: Revision Query not supported"							},
		{VI_WARN_SPX_DATA_NOT_READY,  "WARNING: No scan data availible, maybe not yet triggered"	},

		{VI_ERROR_PARAMETER1,         "Parameter 1 out of range" 											},
		{VI_ERROR_PARAMETER2,         "Parameter 2 out of range"												},
		{VI_ERROR_PARAMETER3,         "Parameter 3 out of range" 											},
		{VI_ERROR_PARAMETER4,         "Parameter 4 out of range" 											},
		{VI_ERROR_PARAMETER5,         "Parameter 5 out of range" 											},
		{VI_ERROR_PARAMETER6,         "Parameter 6 out of range" 											},
		{VI_ERROR_PARAMETER7,         "Parameter 7 out of range" 											},
		{VI_ERROR_PARAMETER8,         "Parameter 8 out of range"												},
		{VI_ERROR_FAIL_ID_QUERY,      "Instrument identification query failed"        	    		},
		{VI_ERROR_INV_RESPONSE,       "Errors occured interpreting instrument's response"	      },
		{VI_ERROR_READ_INCOMPLETE,    "Data readout from spectrometer was incomplete" 		     	},
		{VI_ERROR_NO_USER_DATA,       "No user wavelength adjustment data available"    				},
		{VI_ERROR_INV_USER_DATA,      "Invalid user wavelength adjustment data"		    				},

		{0 , VI_NULL}
	};

	// VISA errors
	if(viStatusDesc(instr, stat, msg) != VI_WARN_UNKNOWN_STATUS) return VI_SUCCESS;

	// Static driver/instrument errors
	i = 0;
	found = 0;
	while(!found && (err_msg_list[i].descr != NULL))
	{
		if(err_msg_list[i].err == stat) found = 1;
		else i ++;
	}
	if(found)
	{
		strncpy(msg, err_msg_list[i].descr, SPX_ERR_DESCR_BUFFER_SIZE);
		return VI_SUCCESS;
	}

	// not found
	viStatusDesc(instr, VI_WARN_UNKNOWN_STATUS, msg);
	return VI_WARN_UNKNOWN_STATUS;
}



/*---------------------------------------------------------------------------
 Function: Identification Query
 Purpose:  This function returns the instrument identification strings.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_identificationQuery (ViSession instr, ViChar _VI_FAR vendor[], ViChar _VI_FAR name[], ViChar _VI_FAR serial[], ViChar _VI_FAR revision[])
{
	ViStatus 	err;
	SPX_data_t 	*data;

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	if(vendor   != VI_NULL) strncpy(vendor,   data->vendor, SPX_BUFFER_SIZE);
	if(name     != VI_NULL) strncpy(name  ,   data->name,   SPX_BUFFER_SIZE);
	if(serial   != VI_NULL) strncpy(serial,   data->serNr,  SPX_BUFFER_SIZE);
	if(revision != VI_NULL) SPX_getInstrumentRev(data, revision);

	return VI_SUCCESS;
}


/*---------------------------------------------------------------------------
 Function: Revision Query
 Purpose:  This function returns the driver and instrument revisions.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_revisionQuery (ViSession instr, ViChar _VI_FAR driverRev[], ViChar _VI_FAR instrRev[])
{
	ViStatus 	err;
	SPX_data_t 	*data;

	// Firmware revision
	if(instrRev != VI_NULL)
	{
		if(instr != VI_NULL)
		{
			if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;
			SPX_getInstrumentRev(data, instrRev);
		}
		else
		{
			strncpy(instrRev, SPX_NO_REVISION_TXT, SPX_BUFFER_SIZE);
		}
	}
	// Driver revision
  	if(driverRev != VI_NULL)
  	{
  		strncpy(driverRev, SPX_DRIVER_REVISION, SPX_BUFFER_SIZE);
  	}
	return VI_SUCCESS;
}




/*---------------------------------------------------------------------------
	Function:	Set User Text
	Purpose:		This function writes the given string to the novolatile memory of
 					the spectrometer.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_setUserText (ViSession instr, ViChar _VI_FAR userText[])
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt16			wValue, wIndex, wLength;
	//ViUInt8			bmType = (REQTYPE_OUT | REQTYPE_VENDOR | REQTYPE_DEVICE);
	//unsigned char	bRequest;
	char				buf[USER_LABEL_LENGTH];

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// REQUEST a write to EEPROM
	//bRequest = REQUEST_WRITE_EEPROM;
	wValue = EE_DEVICE_USER_LABEL;		// this is the EEPROMs write address
	wIndex = 0;									// not used here
	wLength = USER_LABEL_LENGTH;			// this is the number of Bytes to write to EEPROM

	strncpy(buf, userText, (USER_LABEL_LENGTH - 1));	// strncpy will fill with '\0' when userText is smaller than (USER_LABEL_LENGTH-1)
	buf[USER_LABEL_LENGTH-1] = '\0';

	err = SPX_writeEEPROM(instr, wValue, wIndex, wLength, buf);

	return err;
}

/*---------------------------------------------------------------------------
	Function:	Get User Text
	Purpose:		This function reads the 'user text' string to the novolatile memory of
 					the spectrometer.
---------------------------------------------------------------------------*/
ViStatus _VI_FUNC SPX_getUserText (ViSession instr, ViChar _VI_FAR userText[])
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt16			wValue, wIndex, wLength;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	unsigned char	bRequest;
	char				buf[USER_LABEL_LENGTH];

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// REQUEST a read from EEPROM
	bRequest = REQUEST_READ_EEPROM;
	wValue = EE_DEVICE_USER_LABEL;		// this is the EEPROMs read address
	wIndex = 0;									// not used here
	wLength = USER_LABEL_LENGTH;			// this is the number of Bytes to read from EEPROM

	
	if((err = SPX_readEEPROM(instr, bRequest, wValue, wIndex, wLength, buf, &data->ReturnCount)) != VI_SUCCESS)	return err;

	memcpy(userText, buf, (USER_LABEL_LENGTH - 1));
	userText[USER_LABEL_LENGTH - 1] = '\0';
	return err;
}


/****************************************************************************

 UTILITY ROUTINES (Non-Exportable Functions)

****************************************************************************/
/*---------------------------------------------------------------------------
  Function: Acquire Scan Data Raw
  Purpose:  acquires the data from the spectrometer and stores them in the SPX_data_t struct
---------------------------------------------------------------------------*/
static ViStatus SPX_acquireScanDataRaw (ViSession instr)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt32			scan_data_count;
	ViInt16			in_pipe_status;

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	if((err = viGetAttribute (instr, VI_ATTR_USB_BULK_IN_STATUS, &in_pipe_status)) != VI_SUCCESS) return err;

	switch (in_pipe_status)
	{
		case VI_USB_PIPE_STATE_UNKNOWN	:
														break;
		case VI_USB_PIPE_READY				:
														break;
		case VI_USB_PIPE_STALLED			:
														return VI_WARN_SPX_DATA_NOT_READY;
														break;
	}

	// READOUT SCAN DATA
	err = viRead(instr, (ViPBuf)data->rawScanData, sizeof(unsigned short) * SPX_PIXEL_BUFFER, &scan_data_count);
	switch (err)
	{
		case VI_SUCCESS				:
		case VI_SUCCESS_TERM_CHAR	:
		case VI_SUCCESS_MAX_CNT		:
												if(scan_data_count != sizeof(unsigned short) * SPX_PIXEL_BUFFER)
													return VI_ERROR_READ_INCOMPLETE;
												break;
		case VI_ERROR_TMO				:
												return VI_WARN_SPX_DATA_NOT_READY;
												break;
		default							:
												return err;
												break;
	}

	return VI_SUCCESS;
}

/*---------------------------------------------------------------------------
  Function: Process Scan Data
  Purpose:  normalizes the scan data, corrects the scan with dark current
---------------------------------------------------------------------------*/

static ViStatus SPX_ProcessScanData (SPX_data_t *data, ViReal64 _VI_FAR scanDataArray[])
{
	ViUInt32			i;
	ViReal64			*result;
	ViUInt16			*scan;
	ViReal64			norm_fact_even, norm_fact_odd, dark_even, dark_odd;

	// invert all data
	scan   = data->rawScanData;
	for(i = 0; i < SPX_PIXEL_BUFFER; i++)
	{
		*scan = SPX_MAX_ADC_VALUE - *scan;
		scan++;
	}

	// calculate even and odd average
	dark_even = 0.0;
	dark_odd = 0.0;
	for(i = SPX_BLACK_START; i < SPX_BLACK_STOP; i+= 2)
	{
		dark_even += (ViReal64)(data->rawScanData[i]);
		dark_odd  += (ViReal64)(data->rawScanData[i+1]);
	}

	dark_even /= 0.5 * (ViReal64)((SPX_BLACK_STOP - SPX_BLACK_START));
	dark_odd  /= 0.5 * (ViReal64)((SPX_BLACK_STOP - SPX_BLACK_START));

	// limit the Offset to values stored in SPX (retrieved from SPx during init()-routine)
	if(dark_even > data->Offset_Even_Max)	dark_even = data->Offset_Even_Max;
	if(dark_odd  > data->Offset_Odd_Max)	dark_odd  = data->Offset_Odd_Max;


	// calculate the array with respect of the dark values
	result = scanDataArray;
	norm_fact_even = 1.0 / ((ViReal64)SPX_MAX_ADC_VALUE - dark_even);
	norm_fact_odd  = 1.0 / ((ViReal64)SPX_MAX_ADC_VALUE - dark_odd );

	for(i = 0; i < (SPX_PIXEL_BUFFER - SPX_SCAN_START); i+= 2)
	{
		*result = ((ViReal64)data->rawScanData[SPX_PIXEL_BUFFER - 1 - i] - dark_even) * norm_fact_even;
		if(*result < 0.0) *result = 0.0;
		result++;

		*result = ((ViReal64)data->rawScanData[SPX_PIXEL_BUFFER - 2 - i] - dark_odd) * norm_fact_odd;
		if(*result < 0.0) *result = 0.0;
		result++;
	}

	return VI_SUCCESS;
}


/*---------------------------------------------------------------------------
	Function:	Get Wavelength Parameters
	Purpose:		This function reads the parameters necessary to calculate from
					pixels to wavelength and vice versa stored in EEPROM of the
					spectrometer and stores the values in the SPX_data_t structure.
---------------------------------------------------------------------------*/
static ViStatus SPX_getWavelengthParameters (ViSession instr)
{
	SPX_data_t		*data;
	ViStatus 		err;

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;
	
	//	init data structures
	data->user_cal_valid		= 0;
	
	//	read factory adjustment coefficients from EEPROM and calculate wavelength array
	//	return values (errors) ignored by intention
	SPX_readEEFactoryPoly(instr, data->factory_cal.poly);
	SPX_poly2wlArray(&(data->factory_cal));
	
	//	read user adjustment nodes from EEPROM and calculate coefficients and wavelength array
	data->user_cal_valid = 0;
	err = SPX_readEEUserPoints(instr, data->user_cal_node_pixel, data->user_cal_node_wl, &(data->user_cal_node_cnt));
	if(err == VI_SUCCESS) err = SPX_nodes2poly(data->user_cal_node_pixel, data->user_cal_node_wl, data->user_cal_node_cnt, data->user_cal.poly);
	if(err == VI_SUCCESS) err = SPX_poly2wlArray(&(data->user_cal));
	if(err == VI_SUCCESS) data->user_cal_valid = 1;
	
	return VI_SUCCESS;	//	errors ignored by intention
}


/*---------------------------------------------------------------------------
	Function:	Read EEPROM factory calibration data polynom coefficients
	Purpose:		This function reads the polynome coefficients necessary to 
					calculate from pixels to wavelength and vice versa stored in 
					the EEPROM of the spectrometer and stores the values in the 
					poly array. The poly array must contain 4 elements.
---------------------------------------------------------------------------*/
static ViStatus SPX_readEEFactoryPoly(ViSession instr, ViReal64 poly[])
{
	ViStatus 		err;
	ViUInt16			wValue, wIndex, wLength, wRcnt;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	unsigned char	bRequest;
	char				buf[CALIB_DATA_LENGTH];

	// REQUEST a read from EEPROM
	bRequest = REQUEST_READ_EEPROM;
	wValue = EE_CALIB_COEF_DATA_FLAG;	// this is the EEPROMs read address
	wIndex = 0;									// not used here
	wLength = CALIB_DATA_LENGTH;			// this is the number of Bytes to read from EEPROM

	if((err = SPX_readEEPROM(instr, bRequest, wValue, wIndex, wLength, buf, &wRcnt)) != VI_SUCCESS) return err;
	if(wRcnt != wLength) return VI_ERROR_IO;	// not all data read
	
	//	check if valid
	if((buf[0] != SPX_CALIB_VALID_FLAG) || (buf[1] != SPX_CALIB_VALID_FLAG))
	{
		SPX_initDefaultPoly(poly);
		return VI_ERROR_NO_USER_DATA;
	}
	
	//	copy valid data
	memcpy(poly, &buf[EE_CALIB_COEF_DATA_FLAG_LENGTH], EE_CALIB_COEF_DATA_LENGTH);

	return VI_SUCCESS;
}


/*---------------------------------------------------------------------------
	Function:	Write EEPROM factory calibration coefficients
	Purpose:		This function writes the factory calibration coefficients to the
					EEPROM.
					The coefficients array must contain four elements.
---------------------------------------------------------------------------*/
static ViStatus SPX_writeEEFactoryPoly(ViSession instr, ViReal64 poly[])
{
	//	write factory adjustment data to EEPROM
#ifndef DEBUG_NO_EEPROM_WRITE  
	ViStatus 		err;
	//ViUInt8			bmWriteType = (REQTYPE_OUT | REQTYPE_VENDOR | REQTYPE_DEVICE);
#endif	//	#ifndef DEBUG_NO_EEPROM_WRITE
	ViUInt16			wValue, wIndex, wLength;
	//unsigned char	bRequest;
	char				buf[CALIB_DATA_LENGTH];

	// set valid flags
	buf[0] = SPX_CALIB_VALID_FLAG;
	buf[1] = SPX_CALIB_VALID_FLAG;  
	
	// copy data
	memcpy(&buf[EE_CALIB_COEF_DATA_FLAG_LENGTH], poly, EE_CALIB_COEF_DATA_LENGTH);
	
	// REQUEST a read from EEPROM
	//bRequest = REQUEST_WRITE_EEPROM;
	wValue = EE_CALIB_COEF_DATA_FLAG;	// this is the EEPROMs read address
	wIndex = 0;									// not used here
	wLength = CALIB_DATA_LENGTH;			// this is the number of Bytes to read from EEPROM

#ifndef DEBUG_NO_EEPROM_WRITE 	
	// write data
	if((err = SPX_writeEEPROM(instr, wValue, wIndex, wLength, buf)) != VI_SUCCESS) return err;	
#endif	//	#ifndef DEBUG_NO_EEPROM_WRITE
		
	return VI_SUCCESS;
}


/*---------------------------------------------------------------------------
	Function:	Read EEPROM user calibration nodes
	Purpose:		This function reads the user-defined supporting points stored 
					in the EEPROM of the spectrometer, necessary to calculate 
					polynome coefficients to calculate from pixels to wavelength 
					and vice versa stored in the EEPROM of the spectrometer and 
					stores the values in the poly array.
					The nodes array must contain at least SPX_MAX_NUM_USR_ADJ
					elements.
---------------------------------------------------------------------------*/
static ViStatus SPX_readEEUserPoints(ViSession instr, ViInt32 pixel[], ViReal64 wl[], ViInt32 *cnt)
{
	ViStatus 		err;
	ViUInt16			wValue, wIndex, wLength, wRcnt;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	unsigned char	bRequest;
	int				i;
	char				buf[USERCAL_FILE_LENGTH];

	int 				iSumBytesToTransfer = USERCAL_FILE_LENGTH;
	int 				iLoop = 0;
	
	*cnt = 0;

	// REQUEST a read from EEPROM
	bRequest = REQUEST_READ_EEPROM;
	wValue = EE_USERCAL_DATA_FLAG;		// this is the EEPROMs read address
	wIndex = 0;									// not used here
	wLength = USERCAL_FILE_LENGTH;		// this is the number of Bytes to read from EEPROM

	while(iSumBytesToTransfer > 0)
	{
		wValue += iLoop * SPX_MAX_BYTES_TO_TRANSFER;
		
		if(iSumBytesToTransfer >= SPX_MAX_BYTES_TO_TRANSFER) 
		{
			wLength = SPX_MAX_BYTES_TO_TRANSFER;		
		}
		else
		{
			wLength = iSumBytesToTransfer; 	
		}
		
		if((err = SPX_readEEPROM(instr, bRequest, wValue, wIndex, wLength, &buf[iLoop * SPX_MAX_BYTES_TO_TRANSFER], &wRcnt)) != VI_SUCCESS) return err;  
		if(wRcnt != wLength) return VI_ERROR_IO;
		
		iLoop++;	
		iSumBytesToTransfer -= SPX_MAX_BYTES_TO_TRANSFER;
	}
	
	//	check if valid
	if(buf[0] != SPX_USERCAL_VALID_FLAG)	return VI_ERROR_NO_USER_DATA;
	
	//	check node count
	if((buf[1] < SPX_MIN_NUM_USR_ADJ) || (buf[1] > SPX_MAX_NUM_USR_ADJ))	return VI_ERROR_INV_USER_DATA;
	*cnt = (ViInt32)buf[1];
	
	// copy sampling points
	for (i = 0; i < buf[1]; i++)
	{
		memcpy(&pixel[i], &buf[EE_USERCAL_DATA_FLAG_LENGTH + EE_USERCAL_DATA_LEN_LENGTH + i * (sizeof(ViInt32) + sizeof(ViReal64))], sizeof(ViInt32));
		memcpy(&wl[i], &buf[EE_USERCAL_DATA_FLAG_LENGTH + EE_USERCAL_DATA_LEN_LENGTH + sizeof(ViInt32) + i * (sizeof(ViInt32) + sizeof(ViReal64))], sizeof(ViReal64));
	}

	return VI_SUCCESS;
}

	
/*---------------------------------------------------------------------------
	Function:	Write EEPROM user calibration nodes
	Purpose:		This function writes the user-defined supporting points to the
					instruments EEPROM.
					The nodes array must contain at least SPX_MIN_NUM_USR_ADJ
					elements and a maximum of SPX_MAX_NUM_USR_ADJ elements.
---------------------------------------------------------------------------*/
static ViStatus SPX_writeEEUserPoints(ViSession instr, ViInt32 pixel[], ViReal64 wl[], ViInt32 cnt)
{
#ifndef DEBUG_NO_EEPROM_WRITE
	ViStatus 		err;
	//ViUInt8			bmWriteType = (REQTYPE_OUT | REQTYPE_VENDOR | REQTYPE_DEVICE);
	int				iSumBytesToTransfer = USERCAL_FILE_LENGTH;
	int 				iLoop = 0;
#endif	//	#ifndef DEBUG_NO_EEPROM_WRITE	
	ViUInt16			wValue, wIndex, wLength;   
	//unsigned char	bRequest;
	int				i;
	char				buf[USERCAL_FILE_LENGTH];

	// set valid flags...
	buf[0] = SPX_USERCAL_VALID_FLAG;
	buf[1] = (char)cnt; 
	for (i = 0; i < cnt; i++)
	{
		memcpy(&buf[EE_USERCAL_DATA_FLAG_LENGTH + EE_USERCAL_DATA_LEN_LENGTH + i * (sizeof(ViInt32) + sizeof(ViReal64))], &pixel[i], sizeof(ViInt32));
		memcpy(&buf[EE_USERCAL_DATA_FLAG_LENGTH + EE_USERCAL_DATA_LEN_LENGTH + sizeof(ViInt32) + i * (sizeof(ViInt32) + sizeof(ViReal64))], &wl[i], sizeof(ViReal64));
	}
	
	// REQUEST to write to User Adjustment EEPROM
	//bRequest	= REQUEST_WRITE_EEPROM;
	wValue	= EE_USERCAL_DATA_FLAG;		// this is the User Adjustment EEPROMs write address
	wIndex	= 0;								// not used here
	wLength	= USERCAL_FILE_LENGTH;		// this is the number of Bytes to write to EEPROM

#ifndef DEBUG_NO_EEPROM_WRITE	
	// write data
	// we cannot write more than 63 bytes at once...
	while(iSumBytesToTransfer > 0)
	{
		wValue += iLoop * SPX_MAX_BYTES_TO_TRANSFER;
		
		if(iSumBytesToTransfer >= SPX_MAX_BYTES_TO_TRANSFER) 
		{
			wLength = SPX_MAX_BYTES_TO_TRANSFER;		
		}
		else
		{
			wLength = iSumBytesToTransfer; 	
		}
		if((err = SPX_writeEEPROM(instr, wValue, wIndex, wLength, buf)) != VI_SUCCESS) return err;
		iLoop++;	
		iSumBytesToTransfer -= SPX_MAX_BYTES_TO_TRANSFER;
	}
#endif	//	#ifndef DEBUG_NO_EEPROM_WRITE
	
	return VI_SUCCESS;
}

/*	old function checking only for increasing values...
static ViStatus SPX_checkNodes(ViInt32 pixel[], ViReal64 wl[], ViInt32 cnt)
{
	int		i;
	ViInt32	p;
	ViReal64 d;

	//	check valid buffer length and determine target
	if((cnt < SPX_MIN_NUM_USR_ADJ) || (cnt > SPX_MAX_NUM_USR_ADJ)) return VI_ERROR_INV_USER_DATA;

	// check pixel range
	if((pixel[0] < 0) || (pixel[cnt - 1] > (SPX_NUM_PIXELS - 1)))	return VI_ERROR_INV_USER_DATA;
	
	// check wavelength range
	if(wl[0] <= 0.0)	return VI_ERROR_INV_USER_DATA; 
	
	// check monoton ascending wavelength and pixel values    
	p = pixel[0];
	d = wl[0];
	for(i = 1; i < cnt; i++)
	{
		if((pixel[i] <= p) || (wl[i] <= d))	return VI_ERROR_INV_USER_DATA;
		p = pixel[i];
		d = wl[i];
	}
	
	return VI_SUCCESS;
}*/

static ViStatus SPX_checkNodes(ViInt32 pixel[], ViReal64 wl[], ViInt32 cnt)
{
	int		i;
	ViInt32	p;
	ViReal64 d;
	int 		iDirectionFlag = 0;	// 1 means increasing, -1 means decreasing, 0 is an error

	//	check valid buffer length and determine target
	if((cnt < SPX_MIN_NUM_USR_ADJ) || (cnt > SPX_MAX_NUM_USR_ADJ)) return VI_ERROR_INV_USER_DATA;

	// check if values are decreasing
	if(wl[0] < wl[1])
	{
		iDirectionFlag = 1;	
	}
	else if(wl[0] > wl[1])  
	{
		iDirectionFlag = -1;	
	}
	else
		return VI_ERROR_INV_USER_DATA;
	
	// check pixel range
	if((pixel[0] < 0) || (pixel[cnt - 1] > (SPX_NUM_PIXELS - 1)))	return VI_ERROR_INV_USER_DATA;
	
	// check wavelength range
	if(wl[0] <= 0.0)	return VI_ERROR_INV_USER_DATA; 
	
	// check monoton ascending wavelength and pixel values    
	p = pixel[0];
	d = wl[0];
	for(i = 1; i < cnt; i++)
	{
		// check increasing pixels...
		if(pixel[i] <= p)	return VI_ERROR_INV_USER_DATA;
		
		if(iDirectionFlag == 1)	// increasing
		{
			if(wl[i] <= d)	return VI_ERROR_INV_USER_DATA;	
		}
		else
		{
			if(wl[i] >= d)	return VI_ERROR_INV_USER_DATA;
		}

		p = pixel[i];
		d = wl[i];
	}
	
	return VI_SUCCESS;
}


/*---------------------------------------------------------------------------
	Function:	Nodes to Polynome
	Purpose:		Calculates polynome coefficients from user defined supporting
					points.
---------------------------------------------------------------------------*/
static ViStatus SPX_nodes2poly(ViInt32 pixel[], ViReal64 wl[], ViInt32 cnt, ViReal64 poly[])
{
	if(LeastSquareInterpolation ((int *)pixel, (double *)wl, (int)cnt, (double *)poly)) return VI_ERROR_INV_USER_DATA;
	return VI_SUCCESS;
}


/*---------------------------------------------------------------------------
	Function:	Init default polynom coefficients
	Purpose:		Init a polynome coefficien array with default values.
					1 pixel = 1nm. The poly array must contain 4 elements.
---------------------------------------------------------------------------*/
static void SPX_initDefaultPoly(ViReal64 poly[])
{
	poly[0] = 0.0;
	poly[1] = 1.0;	// this will lead to 1 pixel = 1 nm
	poly[2] = 0.0;
	poly[3] = 0.0;
}


/*---------------------------------------------------------------------------
	Function:	Polynom to Wavelength Array
	Purpose:		Calculates wavelenth array from polynom coefficients.
					The poly array must contain 4 elements.
---------------------------------------------------------------------------*/
static ViStatus SPX_poly2wlArray(SPX_wl_cal_t *wl)
{
	int		i;
	ViReal64	d = 0.0;
	int iDirectionFlag = 0;	// 1 means increasing, -1 means decreasing, 0 is an error
	
	for (i = 0; i < SPX_NUM_PIXELS; i++)
		wl->wl[i] = wl->poly[0] + (double)i * (wl->poly[1] + (double)i * (wl->poly[2] + (double)i * wl->poly[3]));

	
	// check if values are decreasing
	if(wl->wl[0] < wl->wl[1])
	{
		iDirectionFlag = 1;	
	}
	else if(wl->wl[0] > wl->wl[1])  
	{
		iDirectionFlag = -1;	
	}
	else
		return VI_ERROR_INV_USER_DATA;
	
	
	d = wl->wl[0];
	for(i = 1; i < SPX_NUM_PIXELS; i++)
	{
		if(iDirectionFlag == 1)	// increasing
		{
			if(wl->wl[i] <= d)	return VI_ERROR_INV_USER_DATA;	
		}
		else
		{
			if(wl->wl[i] >= d)	return VI_ERROR_INV_USER_DATA;
		}

		d = wl->wl[i];
	}
	
	if(iDirectionFlag == 1)
	{
		wl->min		= wl->poly[0];
		wl->max		= wl->wl[SPX_NUM_PIXELS - 1];
	}
	else
	{
		wl->min		= wl->wl[SPX_NUM_PIXELS - 1]; 
		wl->max		= wl->poly[0];
	}
	
	return VI_SUCCESS;
}


/* old function checking only for increasing values...
static ViStatus SPX_poly2wlArray(SPX_wl_cal_t *wl)
{
	int		i;
	ViReal64	d = 0.0;
	
	for (i = 0; i < SPX_NUM_PIXELS; i++)
	{
		wl->wl[i] = wl->poly[0] + (double)i * (wl->poly[1] + (double)i * (wl->poly[2] + (double)i * wl->poly[3]));
		if(wl->wl[i] <= d) 
			return VI_ERROR_INV_USER_DATA;	//	check monoton ascending wavelength values
		d = wl->wl[i];
	}
	wl->min		= wl->poly[0];
	wl->max		= wl->wl[SPX_NUM_PIXELS - 1];
	
	return VI_SUCCESS;
}
 */

int LeastSquareInterpolation (int *x, double *y, int cnt, double *polyout){
    // Calculate poly[0..3] from arrays of cnt points. nonzero return is error 
    // We don't implement this - factory calibration sees fine.
    polyout[0] = 0;
    polyout[1] = 1.0;
    polyout[2] = 0;
    polyout[3] = 0;
    return VI_SUCCESS;
}   

/*---------------------------------------------------------------------------
	Function:	Get Dark Current Offset
	Purpose:		This function reads the dark current values for even and
					odd pixels stored in EEPROM of the spectrometer and stores
					the values in the SPX_data_t structure.
---------------------------------------------------------------------------*/
static ViStatus SPX_getDarkCurrentOffset (ViSession instr)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt16			wValue, wIndex, wLength;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	unsigned char	bRequest;
	char				buf[2*sizeof(ViUInt16)];

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// REQUEST a read from EEPROM
	bRequest = REQUEST_READ_EEPROM;
	wValue = EE_EVEN_OFFSET_MAX;			// this is the EEPROMs read address
	wIndex = 0;									// not used here
	wLength = 2*sizeof(ViUInt16);			// this is the number of Bytes to read from EEPROM

	if((err = SPX_readEEPROM(instr, bRequest, wValue, wIndex, wLength, buf, &data->ReturnCount)) != VI_SUCCESS) return err;

	memcpy(&data->Offset_Even_Max, buf, sizeof(ViUInt16));
	memcpy(&data->Offset_Odd_Max,  &buf[sizeof(ViUInt16)], sizeof(ViUInt16));
	return err;
}

/*---------------------------------------------------------------------------
	Function:	Get Firmware Revision
	Purpose:		This function reads the firmware revision stored in CODE
					of the spectrometer and stores the values in the SPX_data_t structure.
---------------------------------------------------------------------------*/
static ViStatus SPX_getFirmwareRevision (ViSession instr)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt16			wValue, wIndex, wLength;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	unsigned char	bRequest;
	char				buf[2*sizeof(ViUInt8)];

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// set to default value
	data->version_major = 0;
	data->version_minor = 0;

	// REQUEST a read from EEPROM
	bRequest = GET_PRODUCT_INFORMATION;
	wValue = GET_FIRMWARE_VERSION;		// this is the sub-request
	wIndex = 0;									// not used here
	wLength = 2*sizeof(ViUInt8);			// number of bytes to read

	if((err = SPX_readEEPROM(instr, bRequest, wValue, wIndex, wLength, buf, &data->ReturnCount)) != VI_SUCCESS) return err;  

	// overwrite default values when device returned two bytes
	if(data->ReturnCount == 2*sizeof(ViUInt8))
	{
		data->version_major = (int)buf[0];
		data->version_minor = (int)buf[1];
	}

	return err;
}

/*---------------------------------------------------------------------------
	Function:	Get Hardware Revision
	Purpose:		This function reads the hardware revision stored in CODE
					of the spectrometer and stores the values in the SPX_data_t structure.
---------------------------------------------------------------------------*/
static ViStatus SPX_getHardwareRevision (ViSession instr)
{
	SPX_data_t 		*data;
	ViStatus 		err;
	ViUInt16			wValue, wIndex, wLength;
	//ViUInt8			bmType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	unsigned char	bRequest;
	char				buf[2*sizeof(ViUInt8)];

	if((err = viGetAttribute(instr, VI_ATTR_USER_DATA, &data)) != VI_SUCCESS) return err;

	// set to default value
	data->hardware_major = 0;
	data->hardware_minor = 0;

	// REQUEST a read from EEPROM
	bRequest = GET_PRODUCT_INFORMATION;
	wValue = GET_HARDWARE_REVISION;		// this is the sub-request
	wIndex = 0;									// not used here
	wLength = 2*sizeof(ViUInt8);			// number of bytes to read

	if((err = SPX_readEEPROM(instr, bRequest, wValue, wIndex, wLength, buf, &data->ReturnCount)) != VI_SUCCESS) return err;  

	// overwrite default values when device returned two bytes
	if(data->ReturnCount == 2*sizeof(ViUInt8))
	{
		data->hardware_major = (int)buf[0];
		data->hardware_minor = (int)buf[1];
	}

	return err;
}

/*---------------------------------------------------------------------------
 Function: Initialize Clean Up / Initialize Close
---------------------------------------------------------------------------*/
static ViStatus SPX_initCleanUp (ViSession rm, ViPSession pInstr, ViStatus err)
{
	viClose (*pInstr);
	viClose (rm);
	*pInstr = VI_NULL;
	return err;
}


static ViStatus SPX_initClose (ViPSession pInstr, ViStatus err)
{
	ViStatus		stat;
	ViSession 	rm;
	SPX_data_t *data;

	viGetAttribute(*pInstr, VI_ATTR_RM_SESSION, &rm);
	
	viGetAttribute(*pInstr, VI_ATTR_USER_DATA, &data);
	
	if(viGetAttribute(*pInstr, VI_ATTR_USER_DATA, &data) == VI_SUCCESS) free(data);
	stat = viClose(*pInstr);
	viClose(rm);
	pInstr = VI_NULL;
	return (err ? err : stat);
}


/*---------------------------------------------------------------------------
 Function: Format instrument revision into buffer
---------------------------------------------------------------------------*/
void SPX_getInstrumentRev(SPX_data_t *data, ViChar *str)
{
	sprintf(str, "%d.%d", data->version_major, data->version_minor);
}


/*---------------------------------------------------------------------------
 Function: Writes data stored in buffer to EEPROM.
---------------------------------------------------------------------------*/
__declspec(dllexport) ViStatus SPX_writeEEPROM(ViSession instr, ViUInt16	wValue, ViUInt16 wIndex, ViUInt16 wLength, char	buffer[])
{
	ViStatus 		err;
	unsigned char 	bRequest = REQUEST_WRITE_EEPROM;
	ViUInt8			bmWriteType = (REQTYPE_OUT | REQTYPE_VENDOR | REQTYPE_DEVICE);  
	
	// write data
	if((err = viUsbControlOut (instr, bmWriteType, bRequest, wValue, wIndex, wLength, buffer)) != VI_SUCCESS)  return err;
	
	return VI_SUCCESS;		  
}


/*---------------------------------------------------------------------------
 Function: Reads data stored in EEPROM to buffer.
---------------------------------------------------------------------------*/
__declspec(dllexport) ViStatus SPX_readEEPROM(ViSession instr, unsigned char	bRequest, ViUInt16 wValue, ViUInt16 wIndex, ViUInt16 wLength, char buffer[], ViUInt16* iReturnCount)
{
	ViStatus 		err;  
	ViUInt8			bmReadType = (REQTYPE_IN | REQTYPE_VENDOR | REQTYPE_DEVICE);
	
	// read data
	if((err = viUsbControlIn (instr, bmReadType, bRequest, wValue, wIndex, wLength, buffer, iReturnCount)) != VI_SUCCESS)	return err; 
	
	return VI_SUCCESS;		  
}


/*---------------------------------------------------------------------------
 Functions:Range checking
 Purpose:  This group of functions check ranges.
			  If the value is out of range, the return value is
			  VI_TRUE, otherwise the return value is VI_FALSE.
---------------------------------------------------------------------------*/
#ifdef WE_MAY_NEED_THIS_LATER
static ViBoolean SPX_invalidViBooleanRange (ViBoolean val)
{
	return ((val != VI_FALSE && val != VI_TRUE) ? VI_TRUE : VI_FALSE);
}


static ViBoolean SPX_invalidViInt16Range (ViInt16 val, ViInt16 min, ViInt16 max)
{
	return ((val < min || val > max) ? VI_TRUE : VI_FALSE);
}
#endif

static ViBoolean SPX_invalidViInt32Range (ViInt32 val, ViInt32 min, ViInt32 max)
{
	return ((val < min || val > max) ? VI_TRUE : VI_FALSE);
}

#ifdef WE_MAY_NEED_THIS_LATER

static ViBoolean SPX_invalidViUInt8Range (ViUInt8 val, ViUInt8 min, ViUInt8 max)
{
	return ((val < min || val > max) ? VI_TRUE : VI_FALSE);
}


static ViBoolean SPX_invalidViUInt16Range (ViUInt16 val, ViUInt16 min, ViUInt16 max)
{
	return ((val < min || val > max) ? VI_TRUE : VI_FALSE);
}


static ViBoolean SPX_invalidViUInt32Range (ViUInt32 val, ViUInt32 min, ViUInt32 max)
{
	return ((val < min || val > max) ? VI_TRUE : VI_FALSE);
}


static ViBoolean SPX_invalidViReal32Range (ViReal32 val, ViReal32 min, ViReal32 max)
{
	return ((val < min || val > max) ? VI_TRUE : VI_FALSE);
}


static ViBoolean SPX_invalidViReal64Range (ViReal64 val, ViReal64 min, ViReal64 max)
{
	return ((val < min || val > max) ? VI_TRUE : VI_FALSE);
}

#endif


/****************************************************************************

  End of Source file

****************************************************************************/

