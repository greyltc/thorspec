/* Implementation for linux-32 of types used in spxdrv, to replace those
 * given in the VISA implementation specification at
 * http://www.ivifoundation.org/specifications/default.aspx
 */

#ifndef __VITYPES_H__
#define __VITYPES_H__


typedef unsigned short ViBoolean;
typedef unsigned char  ViUInt8;
typedef unsigned short ViUInt16;
typedef unsigned long  ViUInt32;

typedef unsigned char* ViPUInt8;
typedef unsigned char* ViPBuf;
typedef unsigned short* ViPUInt16;
typedef unsigned long* ViPUInt32;
typedef signed long*   ViPInt32;

typedef unsigned char* ViBuf;

typedef signed short   ViInt16;
typedef signed long    ViInt32;

typedef float          ViReal32;
typedef double         ViReal64;
typedef double*        ViPReal64;

typedef char           ViChar;
typedef char*          ViString;
typedef ViChar*        ViPChar;


typedef ViUInt32       ViSession; 
typedef ViSession*     ViPSession;
typedef ViInt32        ViStatus;
typedef ViString       ViRsrc;

typedef ViUInt32       ViAttrState;
typedef ViUInt32       ViAccessMode;
typedef ViUInt32       ViObject;
typedef ViUInt32       ViAttr;

#define VI_SUCCESS (0L)
#define _VI_FAR
#define _VI_FUNC

#define _VI_ERROR            (-2147483647L-1)
#define VI_ERROR_PARAMETER1  (_VI_ERROR+0x3FFC0001L)
#define VI_ERROR_PARAMETER2  (_VI_ERROR+0x3FFC0002L)
#define VI_ERROR_PARAMETER3  (_VI_ERROR+0x3FFC0003L)
#define VI_ERROR_PARAMETER4  (_VI_ERROR+0x3FFC0004L)
#define VI_ERROR_PARAMETER5  (_VI_ERROR+0x3FFC0005L)
#define VI_ERROR_PARAMETER6  (_VI_ERROR+0x3FFC0006L)
#define VI_ERROR_PARAMETER7  (_VI_ERROR+0x3FFC0007L)
#define VI_ERROR_PARAMETER8  (_VI_ERROR+0x3FFC0008L)
#define VI_ERROR_SYSTEM_ERROR    (_VI_ERROR+0x3FFF0000L)
#define VI_ERROR_FAIL_ID_QUERY   (_VI_ERROR+0x3FFC0011L)
#define VI_ERROR_IO          (_VI_ERROR+0x3FFF003EL)
#define VI_ERROR_INV_PARAMETER      (_VI_ERROR+0x3FFF0078L)
#define VI_ERROR_INV_RESPONSE     (_VI_ERROR+0x3FFC0012L)
#define VI_ERROR_TMO                (_VI_ERROR+0x3FFF0015L)
#define VI_ERROR_RSRC_NFOUND        (_VI_ERROR+0x3FFF0011L)
#define VI_ERROR_RSRC_BUSY          (_VI_ERROR+0x3FFF0072L)

#define VI_WARN_NSUP_ID_QUERY     (0x3FFC0101L)
#define VI_WARN_NSUP_RESET        (0x3FFC0102L)
#define VI_WARN_NSUP_SELF_TEST    (0x3FFC0103L)
#define VI_WARN_NSUP_ERROR_QUERY  (0x3FFC0104L)
#define VI_WARN_NSUP_REV_QUERY    (0x3FFC0105L)
#define VI_WARN_UNKNOWN_STATUS    (0x3FFF0085L)

#define VI_ATTR_TMO_VALUE           (0x3FFF001AUL)
#define VI_ATTR_USB_BULK_IN_PIPE    (0x3FFF01A3UL)
#define VI_ATTR_USB_BULK_OUT_PIPE   (0x3FFF01A2UL)
#define VI_ATTR_USB_END_IN          (0x3FFF01A9UL)
#define VI_ATTR_USB_BULK_IN_STATUS  (0x3FFF01ADUL)

#define VI_ATTR_USER_DATA_32        (0x3FFF0007UL)
#define VI_ATTR_USER_DATA           VI_ATTR_USER_DATA_32
#define VI_ATTR_MANF_NAME           (0xBFFF0072UL)
#define VI_ATTR_MODEL_NAME          (0xBFFF0077UL)
#define VI_ATTR_USB_SERIAL_NUM      (0xBFFF01A0UL)
#define VI_ATTR_RM_SESSION          (0x3FFF00C4UL)
#define VI_ATTR_MANF_ID             (0x3FFF00D9UL)
#define VI_ATTR_MODEL_CODE          (0x3FFF00DFUL)

#define VI_NULL           (0)
#define VI_TRUE           (1)
#define VI_FALSE          (0)
#define VI_OFF            (0)
#define VI_ON             (1)
#define VI_USB_END_SHORT  (4)
#define VI_EXCLUSIVE_LOCK (1)

#define VI_USB_PIPE_STATE_UNKNOWN   (-1)
#define VI_USB_PIPE_READY           (0)
#define VI_USB_PIPE_STALLED         (1)

#define VI_READ_BUF                 (1)
#define VI_WRITE_BUF_DISCARD        (8)
#define VI_READ_BUF_DISCARD         (4)

#define VI_SUCCESS_TERM_CHAR (0x3FFF0005L)
#define VI_SUCCESS_MAX_CNT   (0x3FFF0006L)

#endif /* __VITYPES_H__ */


