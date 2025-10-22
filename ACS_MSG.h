/*
 *  AME_Queue.h
 *  Acxxx
 *
 *  Copyright (c) 2004-2014 Accusys, Inc. All rights reserved.
 *
 */


#include "OS_Define.h"

#ifndef Message_H
#define Message_H


#if defined (AME_MAC)
#include <IOKit/IOLib.h>
#define Msg_title(fmt...) IOLog("ACS6x:"); IOLog(fmt);;IODelay(100*1000);
#define Msg(fmt...) IOLog(fmt);;IODelay(100*1000);
#define Msg_Err_title(fmt...) IOLog("!!! "); IOLog(fmt);IODelay(100*1000);
#define Msg_Err(fmt...) IOLog(fmt);IODelay(100*1000);
#endif

#if defined (AME_Linux)
#include <linux/module.h>
#define Msg_title(fmt...) printk("ACS6x:"); printk(fmt)
#define Msg(fmt...) printk(fmt)
#define Msg_Err_title(fmt...) printk("!!!"); printk(fmt)
#define Msg_Err(fmt...) printk(fmt)
#endif

#if defined (AME_Windows)
#include <ntddk.h>
#define Msg_title DbgPrint("ACS6x:"); DbgPrint
#define Msg DbgPrint
#define Msg_Err_title DbgPrint("!!!"); DbgPrint
#define Msg_Err DbgPrint
#endif


#if 0
#if defined (AME_MAC)
#include <IOKit/IOLib.h>
#define Msg(fmt) IOLog(fmt);
#endif

#if defined (AME_Linux)
#include <linux/module.h>
#define Msg(fmt) printk(fmt)
#endif

#if defined (AME_Windows)
#include <ntddk.h>
#define Msg DbgPrint
#endif

#endif

#define parameter_length_MAX				10

typedef struct _MSG_Additional_Data
{
	AME_U8			parameter_type;
	AME_U8 			parameter_length;
	AME_U8			reserved[6];
	union			{
		AME_U32		INT_data[parameter_length_MAX];
		AME_U8		Char_data[parameter_length_MAX*4];
	}parameter_data;
} PACKED MSG_Additional_Data, *pMSG_Additional_Data;


// for memory log event
#define ACS_PRIVATE_LOG_BUF_SIZE    32*1024	// 32k
#define ADJ_P_L_BUF_SIZE    ((PRIVATE_LOG_BUF_SIZE / sizeof(unsigned int)))

#define log_signature 0x55aaaa55
typedef struct _Log_header
{
	unsigned int 	signature;
	unsigned int 	shared_pingpong;
	unsigned int 	count_p[2];
} Log_header, *pLog_header;


typedef struct _Private_log_header
{
	unsigned int 	signature;
	unsigned int 	count;	
	unsigned int 	time_lo;
	unsigned int 	time_hi;
	unsigned int 	reserved1;
    unsigned char 	category;
    unsigned char 	code;
    unsigned char 	length_Additional;
	unsigned char	reserved2;
} Private_log_header, *pPrivate_log_header;



typedef struct _Private_log
{
	Private_log_header Private_logheader;
    MSG_Additional_Data MSG_AdditionalData;
} Private_log, *pPrivate_log;


typedef struct _Log_Memory_DATA
{
	PVOID		Log_virtualAddress;
	PVOID		current_Log_Address;
} Log_Memory_DATA, *pLog_Memory_DATA;


// end for memory log event
#define Msg_size	60

/* category
0~9 : 	for normal Driver
10~19 :	for MPIO
20~29 :	for AME
30~39 :	For Windows Driver
40~49 :	For Linux Driver
50~59 :	For MAC driver
60~69 :	For log

*/

/* ------------------ category ------------------ */
#define Driver_init				0	// category
	// code
	#define Driver_Init_start				0
	#define Driver_Init_Done_Success		1
	
	extern	char Driver_init_msg[][Msg_size];
/* ============== category end ============== */


/* ------------------ category ------------------ */
#define HBA_init			1	// category
	// code
	#define HBA_Init_start							0
	#define HBA_Init_Done_Success					1
	#define HBA_Restart_start						2
	#define HBA_Restart_Done_Success				3	
	#define HBA_Init_Allocate_Memory_Success		4
	#define HBA_Init_VendorID_DeviceID				5
	#define HBA_Init_Classcode						6

	extern char Driver_HBA_init_msg[][Msg_size];
	
/* ============== category end ============== */


/* ------------------ category ------------------ */
#define HBA_Stop			2	// category
	// code
	#define HBA_Stop_start				0
	#define HBA_Stop_Done_Success		1
	#define HBA_Pause_start				2
	#define HBA_Pause_Done_Success		3


	extern char HBA_Stop_msg[][Msg_size];
	
/* ============== category end ============== */


/* ------------------ category ------------------ */
#define MPIO_init				10	// category
	// code
	#define MPIO_Init_start							0
	#define MPIO_Init_Done_Success					1

/* ============== category end ============== */





/* ------------------ category ------------------ */
#define AME_init				20	// category
	// code
	#define AME_Init_start							0
	#define AME_Init_Done_Success					1	
/* ============== category end ============== */



/* ------------------ category ------------------ */
#define Driver_Windows_init		30	// category
	// code
	#define Init_start_Hibernation_mode			0
	#define Init_Done_Success_Hibernation_mode	1

	extern char Driver_Windows_init_msg[][Msg_size];
/* ============== category end ============== */


/* ------------------ category ------------------ */
#define Driver_Linux			40	// category
	// code
/* ============== category end ============== */


/* ------------------ category ------------------ */
#define Driver_MAC				50	// category
	// code
/* ============== category end ============== */



/* ------------------ category ------------------ */
#define Message_log		60	// category
	// code
	#define Message_log_start			0
	#define Message_log_stop			1
	#define Message_log_overrun			2

	extern char Message_log_msg[][Msg_size];
/* ============== category end ============== */








/* _MSG_Additional_Data */
#define parameter_type_decimal_integer		0
#define parameter_type_hexadecimal_integer	1
#define parameter_type_Character			2







typedef struct _MSG_DATA
{
	AME_U32                 MSG_Serial_Number;
    AME_U32                 MSG_number;
	AME_U32                 MSG_log_memory_number;
	Log_Memory_DATA			LogMemory_DATA;
} MSG_DATA, *pMSG_DATA;


extern	void ACS_message(pMSG_DATA pMSGDATA,AME_U8 category,AME_U8 code,pMSG_Additional_Data pMSG_AdditionalData);
extern 	AME_U32 ACS_LogMemory_SWAP(pMSG_DATA pMSGDATA);
extern	AME_U32 ACS_LogMemory_clear_oldEvent(pMSG_DATA pMSGDATA);
extern	void ACS_message_LogMemory(pMSG_DATA pMSGDATA,AME_U8 category,AME_U8 code,pMSG_Additional_Data pMSG_AdditionalData);
extern	AME_U32	MSG_Init(pMSG_DATA pMSGDATA,pMPIO_DATA pMPIODATA,pAME_Data pAMEData, PVOID Log_virtualAddress);

#ifdef Support_LogMemory
extern  AME_U32	ACS_convert_current_time(unsigned int *time_hi_p,  unsigned int *time_lo_p);
#endif

#ifdef AME_Windows
extern PTIME_FIELDS get_curTime();
#endif

#endif //Message_H
