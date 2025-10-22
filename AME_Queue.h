/*
 *  AME_Queue.h
 *  Acxxx
 *
 *  Copyright (c) 2004-2014 Accusys, Inc. All rights reserved.
 *
 */


#include "OS_Define.h"

#ifndef AME_Queue_H
#define AME_Queue_H

#define AME_Queue_Support_Cmd   MAX_Command_AME

#define MAX_NT_341_concurrent_command   40
#define MAX_NT_2208_concurrent_command  510
#define MAX_NT_3108_concurrent_command  510


// Buffer Queue type define
#define ReqType_NT_SCSI_341            0x01
#define ReqType_NT_SCSI_2208           0x02
#define ReqType_NT_SCSI_3108           0x03
#define ReqType_NT_InBand_internal     0x20
#define ReqType_DAS_InBand_internal    0x21
#define ReqType_DAS_InBand_external    0x22
#define ReqType_Current_InBand_error   0x23


typedef struct _AME_Queue_List
{
    PVOID                   pPrev;
    PVOID                   pNext;
	
} PACKED AME_Queue_List, *pAME_Queue_List;


typedef struct _AME_Queue_Buffer
{
    AME_Queue_List			AMEQueue_list;
    AME_U8                  ReqInUse;
    AME_U32                 ReqType;
    AME_U32                 Raid_ID;
    AME_U32                 Buffer_ID;
    AME_U64                 Request_Buffer_Phy_addr;
    PVOID                   callback;
    
} PACKED AME_Queue_Buffer, *pAME_Queue_Buffer;


typedef struct _AME_Queue_DATA
{
	PVOID					pAMEData;
	AME_Queue_List			AME_Queue_List_Free;
	AME_Queue_List			AME_Queue_List_SCSI;
	AME_Queue_List			AME_Queue_List_InBand;
	AME_Queue_Buffer		AMEQueueBuffer[AME_Queue_Support_Cmd];
	AME_U32                 Current_InBand_ReqType;
    AME_U32                 Current_InBand_Raid_ID;
    AME_U32                 Current_InBand_Is_Abort;
    AME_U64                 Current_InBand_Request_Buffer_Phy_addr;
	PVOID                   Current_InBand_InBand_callback;
	AME_U32					AME_Queue_InBand_Count;
	AME_U32					AME_Queue_Raid_CMD_Count[MAX_RAID_System_ID];

} PACKED AME_Queue_DATA, *pAME_Queue_DATA;


typedef struct _AME_Queue_Request
{
    AME_U32                 Raid_ID;
    AME_U32                 ReqType;
    AME_U64                 Request_Buffer_Phy_addr;
    PVOID                   callback;

} PACKED AME_Queue_Request, *pAME_Queue_Request;



// Call by Host driver
extern  	AME_U32 AME_Queue_Data_Init(pAME_Data pAMEData,pAME_Queue_DATA pAMEQueueData);

// call by AME Module
extern  	AME_U32 AME_Queue_Data_ReInit(pAME_Data pAMEData);
extern		AME_U32 AME_Queue_Command(pAME_Data pAMEData,AME_U32 RAID_ID,AME_U32 ReqType,AME_U64 Request_Buffer_Phy_addr,PVOID callback);
extern  	AME_U32 AME_Queue_Command_SCSI_complete(pAME_Data pAMEData,AME_U32 RAID_ID);
extern  	AME_U32 AME_Queue_Command_InBand_complete(pAME_Data pAMEData);
extern  	PVOID   AME_Queue_Get_InBand_Command_callback(pAME_Data pAMEData);
extern  	AME_U32 AME_Queue_Abort_Command(pAME_Data pAMEData,AME_U64 Request_Buffer_Phy_addr);
extern  	AME_U32 AME_Queue_Clean_Error_Raid_CMD_Count(pAME_Data pAMEData,AME_U32 RAID_ID);

// call by AME Queue
extern  	AME_U32 AME_Queue_pending_SCSI_Command(pAME_Queue_DATA pAMEQueueData,AME_U32 RAID_ID,AME_U32 ReqType,AME_U64 Request_Buffer_Phy_addr);


#endif //AME_Queue_H
