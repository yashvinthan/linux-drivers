/*
 *  MPIO.h
 *  Acxxx
 *
 *  Copyright (c) 2004-2014 Accusys, Inc. All rights reserved.
 *
 */


#include "OS_Define.h"

#ifndef MPIO_H
#define MPIO_H

#ifdef AME_Windows
	#define MPIO_Stop_Report_To_Another_Host	// This only can for Windows
#endif

#if defined (Enable_MPIO_message)

	#if defined (AME_MAC)
	#include <IOKit/IOLib.h>
	#define MPIO_Msg(fmt...) IOLog("MPIO:"); IOLog(fmt);
	#define MPIO_Msg_Err(fmt...) IOLog("MPIO Error !!!:"); IOLog(fmt);IODelay(100*1000);
	#define MPIO_Msg_Warming(fmt...) IOLog("MPIO Warming !!!:"); IOLog(fmt);IODelay(100*1000);
	#endif

	#if defined (AME_Linux)
	#include <linux/module.h>
	#define MPIO_Msg(fmt...) printk("MPIO:"); printk(fmt)
	#define MPIO_Msg_Err(fmt...) printk("MPIO Error !!!:"); printk(fmt)
	#define MPIO_Msg_Warming(fmt...) printk("MPIO Warming !!!:"); printk(fmt)
	#endif

	#if defined (AME_Windows)
	#include <ntddk.h>

	#define MPIO_Msg DbgPrint("MPIO:"); DbgPrint
	#define MPIO_Msg_Err DbgPrint("MPIO Error !!!:"); DbgPrint
	#define MPIO_Msg_Warming DbgPrint("MPIO Warming !!!:"); DbgPrint

	#endif

#else

	#if defined (AME_MAC)
	#include <IOKit/IOLib.h>
	#define MPIO_Msg(fmt...) //IOLog("MPIO:"); IOLog(fmt);
	#define MPIO_Msg_Err(fmt...) //IOLog("MPIO Error !!!:"); IOLog(fmt);IODelay(100*1000);
	#define MPIO_Msg_Warming(fmt...) //IOLog("MPIO Warming !!!:"); IOLog(fmt);IODelay(100*1000);
	#endif

	#if defined (AME_Linux)
	#include <linux/module.h>
	#define MPIO_Msg(fmt...) //printk("MPIO:"); printk(fmt)
	#define MPIO_Msg_Err(fmt...) //printk("MPIO Error !!!:"); printk(fmt)
	#define MPIO_Msg_Warming(fmt...) //printk("MPIO Warming !!!:"); printk(fmt)
	#endif

	#if defined (AME_Windows)
	#include <ntddk.h>

	#define MPIO_Msg //DbgPrint("MPIO:"); DbgPrint
	#define MPIO_Msg_Err //DbgPrint("MPIO Error !!!:"); DbgPrint
	#define MPIO_Msg_Warming //DbgPrint("MPIO Warming !!!:"); DbgPrint

	#endif

#endif


#ifndef MPIO_bit
    #define MPIO_bit(x)  (1 << x)
#endif



typedef struct _MPIO_DATA    PACKED MPIO_DATA, *pMPIO_DATA;


#define MPIO_MAX_support_HBA	4
#define MPIO_MAX_support_PATH	MPIO_MAX_support_HBA

#define MAX_MPIO_Queue_Depth 	MAX_Command_AME


typedef struct _MPIO_Get_PAGE_0_Data
{
	AME_U32			Ready;
    AME_U8          Production_ID[0x10];
	AME_U8          Model_Name[0x10];
	AME_U8          Serial_Number[0x10];

} PACKED MPIO_Get_PAGE_0_Data, *pMPIO_Get_PAGE_0_Data;


// #define CMD_type
#define MPIO_QUEUE_Host_Fast_handler_SCSI_REQUEST	0
#define MPIO_QUEUE_Host_Normal_handler_SCSI_REQUEST	1
#define MPIO_QUEUE_Host_Error_handler_SCSI_REQUEST	2


typedef struct _MPIO_Request_QUEUE_Data
{
	AME_U32			MPIO_BufferID;
	pMPIO_DATA		pHostMPIOData;
	AME_U32			Host_RAID_ID;
	AME_U32			AME_RAID_ID;
	AME_U32			Path_ID_of_Host_RAID;
} PACKED MPIO_Request_QUEUE_Data, *pMPIO_Request_QUEUE_Data;


typedef struct _MPIO_Request_QUEUE_Buffer
{
	MPIO_Request_QUEUE_Data MPIO_QUEUEData;
    PVOID           		pNext;
} PACKED MPIO_Request_QUEUE_Buffer, *pMPIO_Request_QUEUE_Buffer;


typedef struct _MPIO_Request_Buffer_QUEUE
{
    PVOID                   	MPIOfreequeuebuffer;
    AME_U32         			MPIOfreequeuebufferCount;
    MPIO_Request_QUEUE_Buffer	MPIOQueueBuffer[MAX_MPIO_Queue_Depth];
} PACKED MPIO_Request_Buffer_QUEUE, *pMPIO_Request_Buffer_QUEUE;


//#define for PathDATA.Path_State
#define MPIO_Path_stand_alone	MPIO_bit(0)
#define MPIO_Path_Built			MPIO_bit(1)
#define MPIO_Path_Disabled		MPIO_bit(2)	



typedef struct _Path_RAID_DATA
{
	AME_U32					Path_ID_of_Host_RAID;
	pMPIO_DATA				pMPIOData;
	AME_U32					AME_RAID_ID;	
} PACKED Path_RAID_DATA, *pPath_RAID_DATA;


typedef struct _MPIO_RAID_DATA
{
	AME_U32					ConcurrentCMDCount;
	AME_U32					TotalIssue_CMDCount;
	AME_U32					PATHCount;
	MPIO_Get_PAGE_0_Data 	GetPAGE0_Data;
	AME_U32 				PathCMDCount[MPIO_MAX_support_PATH];	
	Path_RAID_DATA			Path_RAIDData[MPIO_MAX_support_PATH];
} PACKED MPIO_RAID_DATA, *pMPIO_RAID_DATA;


typedef struct _RAID_DATA
{
	pMPIO_DATA				pMPIOData_original;
	pMPIO_DATA				pMPIOData_host;
	AME_U32					Path_State;
	AME_U32					host_RaidID;
	AME_U32					Path_Number;
	MPIO_Get_PAGE_0_Data 	GetPAGE0_Data;
} PACKED RAID_DATA, *pRAID_DATA;


typedef struct _MPIO_DATA
{
	PVOID						pAMEData;
	AME_U16                     Vendor_ID;
    AME_U16                     Device_ID;
	AME_U32                 	MPIO_Serial_Number;
	MPIO_RAID_DATA				MPIO_RAIDDATA[MAX_RAID_System_ID];
	RAID_DATA					RAIDDATA[MAX_RAID_System_ID];
	MPIO_Request_Buffer_QUEUE	MPIO_RequestBuffer_Queue;
	PVOID 						pMSGDATA;
};

typedef struct _MPIO_Global
{
	pMPIO_DATA	pMPIOData[MPIO_MAX_support_HBA];
} PACKED MPIO_Global, *pMPIO_Globala;


// Call by MPIO driver
AME_U32 MPIO_Path_Add(pMPIO_DATA pMPIODATA_host,pMPIO_DATA pMPIODATA_path,AME_U32 host_RaidID ,AME_U32 path_RaidID);
AME_U32 MPIO_Path_Remove(pMPIO_DATA pMPIODATA_host,pMPIO_DATA pMPIODATA_path,AME_U32 host_RaidID ,AME_U32 path_RaidID, AME_U32 host_data_keep);
AME_U32 MPIO_Check_Multiple_Path(pMPIO_DATA pMPIODATA,pMPIO_Get_PAGE_0_Data pMPIO_Get_PAGE0_Data ,AME_U32 Raid_ID);

// Call by Host driver
extern      AME_U32 MPIO_Init_Prepare(pMPIO_DATA pMPIODATA,pAME_Data pAMEData,pAME_Module_Prepare pAMEModulePrepare);
extern 		AME_U32 MPIO_Init_Setup(pMPIO_DATA pMPIODATA);
extern 		AME_U32 MPIO_Init(pMPIO_DATA pMPIODATA,pAME_Data pAMEData,pAME_Module_Init_Prepare pAMEModule_Init_Prepare);
extern 		AME_U32 MPIO_ReInit(pMPIO_DATA pMPIODATA,pAME_Data pAMEData,pAME_Module_Init_Prepare pAMEModule_Init_Prepare);
extern      AME_U32	MPIO_Start(pMPIO_DATA pMPIODATA);
extern      AME_U32 MPIO_Stop(pMPIO_DATA pMPIODATA);
extern      AME_U32 MPIO_Build_SCSI_Cmd(pMPIO_DATA pHost_MPIODATA,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command);
extern      AME_U32 MPIO_NT_Raid_Alive_Check (pMPIO_DATA pMPIODATA,AME_U32 Raid_ID);
extern 		AME_U32 MPIO_Lun_Alive_Check (pMPIO_DATA pHost_MPIODATA,AME_U32 Raid_ID,AME_U32 Lun_ID);
extern      AME_U32 MPIO_Timer(pMPIO_DATA pMPIODATA);
extern      AME_U32 MPIO_Build_InBand_Cmd(pMPIO_DATA pMPIODATA, pAME_Module_InBand_Command pAME_ModuleInBand_Command);
extern      AME_U32 MPIO_Cmd_Timeout(pMPIO_DATA pMPIODATA,PVOID pRequestExtension);
extern      AME_U32 MPIO_Cmd_All_Timeout(pMPIO_DATA pMPIODATA);
extern      AME_U32 MPIO_ISR(pMPIO_DATA pMPIODATA);

// MPIO call Host driver
extern      AME_U32 MPIO_Host_Rebuild_SCSI_REQUEST(pAME_REQUEST_CallBack pAME_REQUESTCallBack,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command);
extern      AME_U32 MPIO_Cmd_Mark_Path_remove(pMPIO_DATA pHost_MPIODATA);
extern		void MPIO_spinlock(pMPIO_DATA pMPIODATA);
extern		void MPIO_spinunlock(pMPIO_DATA pMPIODATA);

#if defined (AME_MAC)
extern      AME_U32 MPIO_Mac_Path_Ready(pMPIO_DATA pHost_MPIODATA);
#endif


// call by AME Module
extern		AME_U32 MPIO_AME_RAID_Ready(pAME_Data pAMEData,AME_U32 Raid_ID, AME_U64 Get_Page0_data);
extern		AME_U32 MPIO_AME_RAID_Out(pAME_Data pAMEData,AME_U32 Raid_ID);
extern  	AME_U32 MPIO_Host_Fast_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAME_REQUESTCallBack);
extern  	AME_U32 MPIO_Host_Normal_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAME_REQUESTCallBack);
extern  	AME_U32 MPIO_Host_Error_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAME_REQUESTCallBack);
extern		AME_U32 MPIO_Host_Scan_All_Lun (pAME_Data pAMEData,AME_U32 RAID_ID);
extern		AME_U32 MPIO_Host_Lun_Change (pAME_Data pAMEData,AME_U32 RAID_ID,AME_U32 lun_ID,AME_U32 State); 	// State: TRUE, means lun Add, FALSE, means lun remove
extern  	AME_U32 MPIO_Host_Normal_handler_IN_Band (pAME_REQUEST_CallBack pAMEREQUESTCallBack);

extern  	AME_U32 MPIO_Slave_Notify_Master_toget_Reply(pMPIO_DATA pPath_MPIODATA,AME_U32 RAID_ID);
extern		AME_U32 MPIO_Support_Check(pMPIO_DATA pMPIODATA);
extern  	pAME_Data MPIO_Get_Master_AME_Data(pMPIO_DATA pPath_MPIODATA,AME_U32 RAID_ID); // for Raid 0/1 used
extern      AME_U32 MPIO_Single_NT_Check(pAME_Data pAMEData); // check only one NT device can work

#endif //MPIO_H




