/*
 *  AME_Queue.c
 *  Acxxx
 *
 *  Copyright (c) 2004-2014 Accusys, Inc. All rights reserved.
 *
 */


#include "AME_module.h"
#include "AME_import.h"
#include "AME_Queue.h"

#if defined (AME_MAC_DAS)
    #include "Acxxx.h"
#endif


void AME_Queue_Init_List(pAME_Queue_List pHead)
{
   pHead->pNext = pHead;
   pHead->pPrev = pHead;
   
   return;
}

void AME_Queue_add_head(pAME_Queue_List pHead,pAME_Queue_List pNew)
{
    pAME_Queue_List pNext = (pAME_Queue_List)pHead->pNext;
    
    pNext->pPrev = pNew;
    pNew->pNext = pNext;
    pNew->pPrev = pHead;
    pHead->pNext = pNew;
    
    return;
}


void AME_Queue_add_tail(pAME_Queue_List pHead,pAME_Queue_List pNew)
{
    pAME_Queue_List pPrev = (pAME_Queue_List)pHead->pPrev;
    
    pHead->pPrev = pNew;
    pNew->pNext = pHead;
    pNew->pPrev = pPrev;
    pPrev->pNext = pNew;
    
    return;
}

void AME_Queue_del_entry(pAME_Queue_List pEntry)
{
    pAME_Queue_List pPrev = (pAME_Queue_List)pEntry->pPrev;
    pAME_Queue_List pNext = (pAME_Queue_List)pEntry->pNext;
    
    pNext->pPrev = pPrev;
    pPrev->pNext = pNext;
    
    return;
}


AME_U32 AME_Queue_empty(pAME_Queue_List pHead)
{
    if (pHead->pNext == pHead) {
        return TRUE;
    } else {
        return FALSE;
    }
    
}


AME_U32 AME_Queue_Data_Init(pAME_Data pAMEData,pAME_Queue_DATA pAMEQueueData)
{
    AME_U32 i;
    
    pAMEQueueData->pAMEData = pAMEData;
    pAMEData->pAMEQueueData = pAMEQueueData;
    
    AME_Queue_Init_List(&pAMEQueueData->AME_Queue_List_Free);
    AME_Queue_Init_List(&pAMEQueueData->AME_Queue_List_SCSI);
    AME_Queue_Init_List(&pAMEQueueData->AME_Queue_List_InBand);
    
    pAMEQueueData->Current_InBand_ReqType = 0;
    pAMEQueueData->Current_InBand_Raid_ID = 0;
    pAMEQueueData->Current_InBand_Is_Abort = FALSE;
    pAMEQueueData->Current_InBand_Request_Buffer_Phy_addr = 0;
    pAMEQueueData->Current_InBand_InBand_callback = NULL;
    pAMEQueueData->AME_Queue_InBand_Count = 0;
    
    for (i=0; i<MAX_RAID_System_ID; i++) {
        pAMEQueueData->AME_Queue_Raid_CMD_Count[i] = 0;
    }
    
    for (i=0; i<AME_Queue_Support_Cmd; i++)
    {
        pAMEQueueData->AMEQueueBuffer[i].ReqInUse = FALSE;
        pAMEQueueData->AMEQueueBuffer[i].Raid_ID  = 0;
        pAMEQueueData->AMEQueueBuffer[i].Buffer_ID  = i;
        pAMEQueueData->AMEQueueBuffer[i].ReqType  = 0;
        pAMEQueueData->AMEQueueBuffer[i].Request_Buffer_Phy_addr = 0;
        pAMEQueueData->AMEQueueBuffer[i].callback = NULL;
        
        AME_Queue_add_tail(&pAMEQueueData->AME_Queue_List_Free,&pAMEQueueData->AMEQueueBuffer[i].AMEQueue_list);
    }
    
    return TRUE;
}

AME_U32 AME_Queue_Data_ReInit(pAME_Data pAMEData)
{
    AME_U32 i;
    pAME_Queue_DATA pAMEQueueData = (pAME_Queue_DATA)pAMEData->pAMEQueueData;
    
    AME_spinlock(pAMEData);
    
    AME_Queue_Init_List(&pAMEQueueData->AME_Queue_List_Free);
    AME_Queue_Init_List(&pAMEQueueData->AME_Queue_List_SCSI);
    AME_Queue_Init_List(&pAMEQueueData->AME_Queue_List_InBand);
    
    pAMEQueueData->Current_InBand_ReqType = 0;
    pAMEQueueData->Current_InBand_Raid_ID = 0;
    pAMEQueueData->Current_InBand_Is_Abort = FALSE;
    pAMEQueueData->Current_InBand_Request_Buffer_Phy_addr = 0;
    pAMEQueueData->Current_InBand_InBand_callback = NULL;
    pAMEQueueData->AME_Queue_InBand_Count = 0;
    
    for (i=0; i<MAX_RAID_System_ID; i++) {
        pAMEQueueData->AME_Queue_Raid_CMD_Count[i] = 0;
    }
    
    for (i=0; i<AME_Queue_Support_Cmd; i++)
    {
        pAMEQueueData->AMEQueueBuffer[i].ReqInUse = FALSE;
        pAMEQueueData->AMEQueueBuffer[i].Raid_ID  = 0;
        pAMEQueueData->AMEQueueBuffer[i].Buffer_ID  = i;
        pAMEQueueData->AMEQueueBuffer[i].ReqType  = 0;
        pAMEQueueData->AMEQueueBuffer[i].Request_Buffer_Phy_addr = 0;
        pAMEQueueData->AMEQueueBuffer[i].callback = NULL;
        
        AME_Queue_add_tail(&pAMEQueueData->AME_Queue_List_Free,&pAMEQueueData->AMEQueueBuffer[i].AMEQueue_list);
    }
    
    AME_spinunlock(pAMEData);
    
    return TRUE;
}

AME_U32 AME_Queue_Clean_Error_Raid_CMD_Count(pAME_Data pAMEData,AME_U32 RAID_ID)
{
    pAME_Queue_DATA pAMEQueueData = (pAME_Queue_DATA)pAMEData->pAMEQueueData;
    
    AME_spinlock(pAMEData);
    pAMEQueueData->AME_Queue_Raid_CMD_Count[RAID_ID] = 0;
    AME_spinunlock(pAMEData);
    
    return TRUE;
}


AME_U32 AME_Queue_Get_Pended_SCSI_Command(pAME_Queue_DATA pAMEQueueData,pAME_Queue_Request pAMEQueueRequest)
{
    pAME_Queue_Buffer pAMEQueueBuffer;
    pAME_Queue_List pHead = &pAMEQueueData->AME_Queue_List_SCSI;
    pAME_Queue_List pNew  = (pAME_Queue_List)pHead->pNext;
    
    if (TRUE == AME_Queue_empty(pHead)) {
        return FALSE;
    }
    
    AME_Queue_del_entry(pNew);
    
    pAMEQueueBuffer = (pAME_Queue_Buffer)pNew;
    pAMEQueueRequest->Raid_ID = pAMEQueueBuffer->Raid_ID;
    pAMEQueueRequest->ReqType = pAMEQueueBuffer->ReqType;
    pAMEQueueRequest->Request_Buffer_Phy_addr = pAMEQueueBuffer->Request_Buffer_Phy_addr;
    
    // Clear QueueBuffer
    pAMEQueueBuffer->ReqInUse = FALSE;
    pAMEQueueBuffer->Raid_ID  = 0;
    pAMEQueueBuffer->ReqType  = 0;
    pAMEQueueBuffer->Request_Buffer_Phy_addr = 0;
    
    AME_Queue_add_tail(&pAMEQueueData->AME_Queue_List_Free,pNew);
    
    return TRUE;
}


AME_U32 AME_Queue_pending_SCSI_Command(pAME_Queue_DATA pAMEQueueData,AME_U32 RAID_ID,AME_U32 ReqType,AME_U64 Request_Buffer_Phy_addr)
{
    pAME_Queue_Buffer pAMEQueueBuffer;
    pAME_Queue_List pHead = &pAMEQueueData->AME_Queue_List_Free;
    pAME_Queue_List pNew  = (pAME_Queue_List)pHead->pNext;
    
    
    if (TRUE == AME_Queue_empty(pHead)) {
        AME_Msg_Err("AME Queue Error:  AME Queue Buffer empty!\n");
        return FALSE;
    }
    
    AME_Queue_del_entry(pNew);
    
    pAMEQueueBuffer = (pAME_Queue_Buffer)pNew;
    pAMEQueueBuffer->ReqInUse = TRUE;
    pAMEQueueBuffer->Raid_ID  = RAID_ID;
    pAMEQueueBuffer->ReqType  = ReqType;
    pAMEQueueBuffer->Request_Buffer_Phy_addr = Request_Buffer_Phy_addr;
    
    AME_Queue_add_tail(&pAMEQueueData->AME_Queue_List_SCSI,pNew);
    
    return TRUE;
}

PVOID AME_Queue_Get_InBand_Command_callback(pAME_Data pAMEData)
{
    pAME_Queue_DATA pAMEQueueData = (pAME_Queue_DATA)pAMEData->pAMEQueueData;
    
    return pAMEQueueData->Current_InBand_InBand_callback;
    
}


AME_U32 AME_Queue_Get_Pended_InBand_Command(pAME_Queue_DATA pAMEQueueData,pAME_Queue_Request pAMEQueueRequest)
{
    pAME_Queue_Buffer pAMEQueueBuffer;
    pAME_Queue_List pHead = &pAMEQueueData->AME_Queue_List_InBand;
    pAME_Queue_List pNew  = (pAME_Queue_List)pHead->pNext;
    
    if (TRUE == AME_Queue_empty(pHead)) {
        return FALSE;
    }
    
    AME_Queue_del_entry(pNew);
    
    pAMEQueueBuffer = (pAME_Queue_Buffer)pNew;
    pAMEQueueRequest->Raid_ID = pAMEQueueBuffer->Raid_ID;
    pAMEQueueRequest->ReqType = pAMEQueueBuffer->ReqType;
    pAMEQueueRequest->Request_Buffer_Phy_addr = pAMEQueueBuffer->Request_Buffer_Phy_addr;
    pAMEQueueRequest->callback = pAMEQueueBuffer->callback;
    
    // Clear QueueBuffer
    pAMEQueueBuffer->ReqInUse = FALSE;
    pAMEQueueBuffer->Raid_ID  = 0;
    pAMEQueueBuffer->ReqType  = 0;
    pAMEQueueBuffer->Request_Buffer_Phy_addr = 0;
    pAMEQueueBuffer->callback = NULL;
    
    AME_Queue_add_tail(&pAMEQueueData->AME_Queue_List_Free,pNew);
    
    return TRUE;
}


AME_U32 AME_Queue_pending_InBand_Command(pAME_Queue_DATA pAMEQueueData,AME_U32 RAID_ID,AME_U32 ReqType,AME_U64 Request_Buffer_Phy_addr,PVOID callback)
{
    pAME_Queue_Buffer pAMEQueueBuffer;
    pAME_Queue_List pHead = &pAMEQueueData->AME_Queue_List_Free;
    pAME_Queue_List pNew  = (pAME_Queue_List)pHead->pNext;
    
    
    if (TRUE == AME_Queue_empty(pHead)) {
        AME_Msg_Err("AME Queue Error:  AME Queue Buffer empty!\n");
        return FALSE;
    }
    
    AME_Queue_del_entry(pNew);
    
    pAMEQueueBuffer = (pAME_Queue_Buffer)pNew;
    pAMEQueueBuffer->ReqInUse = TRUE;
    pAMEQueueBuffer->Raid_ID  = RAID_ID;
    pAMEQueueBuffer->ReqType  = ReqType;
    pAMEQueueBuffer->Request_Buffer_Phy_addr = Request_Buffer_Phy_addr;
    pAMEQueueBuffer->callback  = callback;
    
    if (ReqType == ReqType_DAS_InBand_external) {
        AME_Queue_add_head(&pAMEQueueData->AME_Queue_List_InBand,pNew);
    } else {
        AME_Queue_add_tail(&pAMEQueueData->AME_Queue_List_InBand,pNew);
    }
    
    return TRUE;
    
}


AME_U32 AME_Queue_Repending_InBand_Command(pAME_Queue_DATA pAMEQueueData,AME_U32 RAID_ID,AME_U32 ReqType,AME_U64 Request_Buffer_Phy_addr,PVOID callback)
{
    pAME_Queue_Buffer pAMEQueueBuffer;
    pAME_Queue_List pHead = &pAMEQueueData->AME_Queue_List_Free;
    pAME_Queue_List pNew  = (pAME_Queue_List)pHead->pNext;
    
    if (TRUE == pAMEQueueData->Current_InBand_Is_Abort) {
        return TRUE;
    }
    
    
    if (TRUE == AME_Queue_empty(pHead)) {
        AME_Msg_Err("AME Queue Error:  AME Queue Buffer empty!\n");
        return FALSE;
    }
    
    AME_Queue_del_entry(pNew);
    
    pAMEQueueBuffer = (pAME_Queue_Buffer)pNew;
    pAMEQueueBuffer->ReqInUse = TRUE;
    pAMEQueueBuffer->Raid_ID  = pAMEQueueData->Current_InBand_Raid_ID;
    pAMEQueueBuffer->ReqType  = pAMEQueueData->Current_InBand_ReqType;
    pAMEQueueBuffer->Request_Buffer_Phy_addr = pAMEQueueData->Current_InBand_Request_Buffer_Phy_addr;
    pAMEQueueBuffer->callback  = pAMEQueueData->Current_InBand_InBand_callback;
    
    AME_Queue_add_head(&pAMEQueueData->AME_Queue_List_InBand,pNew);
    
    return TRUE;
    
}


/*
 * Function:  AME_Queue_Command
 * Description:  AME Queue Command support two command queue.
 *   1. SCSI command queue in NT environmet.
 *   2. InBand command queue in DAS environmet.
 */
AME_U32 AME_Queue_Command(pAME_Data pAMEData,AME_U32 RAID_ID,AME_U32 ReqType,AME_U64 Request_Buffer_Phy_addr,PVOID callback)
{
    AME_U32     returnstatus;
    AME_U32     MAX_concurrent_command = 0;
    pAME_Queue_DATA pAMEQueueData = (pAME_Queue_DATA)pAMEData->pAMEQueueData;
    
    
    switch (ReqType)
    {
        case ReqType_NT_InBand_internal:
        case ReqType_DAS_InBand_internal:
        case ReqType_DAS_InBand_external:
            if (0 == pAMEQueueData->AME_Queue_InBand_Count)
            {
                pAMEQueueData->Current_InBand_ReqType = ReqType;
                pAMEQueueData->Current_InBand_Raid_ID = RAID_ID;
                pAMEQueueData->Current_InBand_Is_Abort = FALSE;
                pAMEQueueData->Current_InBand_Request_Buffer_Phy_addr = Request_Buffer_Phy_addr;
                pAMEQueueData->Current_InBand_InBand_callback = callback;
                
                if (ReqType == ReqType_NT_InBand_internal) {
                    returnstatus = AME_NT_write_Host_Inbound_Queue_Port_0x40(pAMEData,RAID_ID,Request_Buffer_Phy_addr);
                } else {
                    returnstatus = AME_write_Host_Inbound_Queue_Port_0x40(pAMEData,(AME_U32)Request_Buffer_Phy_addr);
                }
                
                if (TRUE == returnstatus) {
                    AME_spinlock(pAMEData);
			        pAMEQueueData->AME_Queue_InBand_Count++;
			        AME_spinunlock(pAMEData);
			    }
			    
            }
            else {
                
                AME_spinlock(pAMEData);
                returnstatus = AME_Queue_pending_InBand_Command(pAMEQueueData,RAID_ID,ReqType,Request_Buffer_Phy_addr,callback);
                AME_spinunlock(pAMEData);
                
            }
            break;
            
        case ReqType_Current_InBand_error:
            AME_Msg_Err("Current_InBand_Cmd error retry again\n");
            AME_spinlock(pAMEData);
            returnstatus = AME_Queue_Repending_InBand_Command(pAMEQueueData,RAID_ID,ReqType,Request_Buffer_Phy_addr,callback);
            AME_spinunlock(pAMEData);
            break;
        
        case ReqType_NT_SCSI_341:
        case ReqType_NT_SCSI_2208:
        case ReqType_NT_SCSI_3108:
            if (ReqType == ReqType_NT_SCSI_341) {
                MAX_concurrent_command = MAX_NT_341_concurrent_command;
            } else if(ReqType == ReqType_NT_SCSI_2208){
                MAX_concurrent_command = MAX_NT_2208_concurrent_command;
            } else {
                MAX_concurrent_command = MAX_NT_3108_concurrent_command;
            }
            
            if (pAMEQueueData->AME_Queue_Raid_CMD_Count[RAID_ID] < MAX_concurrent_command)
            {
                returnstatus = AME_NT_write_Host_Inbound_Queue_Port_0x40(pAMEData,RAID_ID,Request_Buffer_Phy_addr);
                if (TRUE == returnstatus) {
                    AME_spinlock(pAMEData);
			        pAMEQueueData->AME_Queue_Raid_CMD_Count[RAID_ID]++;
			        AME_spinunlock(pAMEData);
			    }
			    
            } else {
                AME_spinlock(pAMEData);
                returnstatus = AME_Queue_pending_SCSI_Command(pAMEQueueData,RAID_ID,ReqType,Request_Buffer_Phy_addr);
                AME_spinunlock(pAMEData);
            }
            break;
        
        case NTRAIDType_MPIO_Slave:
            AME_spinlock(pAMEData);
			pAMEQueueData->AME_Queue_Raid_CMD_Count[RAID_ID]++;
			AME_spinunlock(pAMEData);
            break;
        
        default:
            returnstatus = FALSE;
            AME_Msg_Err("AME Queue Error:  unknow cmd type (0x%x)\n",ReqType);
            break;
    }
    
    return returnstatus;
}


AME_U32 AME_Queue_Command_SCSI_complete(pAME_Data pAMEData,AME_U32 RAID_ID)
{
    AME_U32             returnstatus;
    AME_Queue_Request   AMEQueueRequest;
    pAME_Queue_DATA pAMEQueueData = (pAME_Queue_DATA)pAMEData->pAMEQueueData;
    
    // only support NT environment
    if (FALSE == AME_Check_is_NT_Mode(pAMEData)) {
        return TRUE;
    }
    
    AME_spinlock(pAMEData);
	returnstatus = AME_Queue_Get_Pended_SCSI_Command(pAMEQueueData,&AMEQueueRequest);
	if (FALSE == returnstatus) {
	    if (pAMEQueueData->AME_Queue_Raid_CMD_Count[RAID_ID] != 0) {
	        pAMEQueueData->AME_Queue_Raid_CMD_Count[RAID_ID]--;
	    } else {
	        AME_Msg_Err("AME_Queue_Command_SCSI_complete:  AME_Queue_Raid_CMD_Count[%d] = 0, can't -1 \n",RAID_ID);
	    }
	}
    AME_spinunlock(pAMEData);
    
    if (TRUE == returnstatus) {
        returnstatus = AME_NT_write_Host_Inbound_Queue_Port_0x40(pAMEData,AMEQueueRequest.Raid_ID,AMEQueueRequest.Request_Buffer_Phy_addr);
        if (FALSE == returnstatus) {
            // lost one cmd , but why can't send cmd to Raid ...
            AME_Msg_Err("AME_Queue_Command_SCSI_complete:  send next cmd fail\n");
            AME_spinlock(pAMEData);
            if (pAMEQueueData->AME_Queue_Raid_CMD_Count[RAID_ID] != 0) {
	            pAMEQueueData->AME_Queue_Raid_CMD_Count[RAID_ID]--;
	        } else {
	            AME_Msg_Err("AME_Queue_Command_SCSI_complete:  AME_Queue_Raid_CMD_Count[%d] = 0, can't -1 \n",RAID_ID);
	        }
            AME_spinunlock(pAMEData);
        }
    }
    
    return TRUE;
}


AME_U32 AME_Queue_Command_InBand_complete(pAME_Data pAMEData)
{
    AME_U32             returnstatus;
    AME_Queue_Request   AMEQueueRequest;
    pAME_Queue_DATA pAMEQueueData = (pAME_Queue_DATA)pAMEData->pAMEQueueData;
    
    AME_spinlock(pAMEData);
	returnstatus = AME_Queue_Get_Pended_InBand_Command(pAMEQueueData,&AMEQueueRequest);
	if (FALSE == returnstatus) {
	    pAMEQueueData->AME_Queue_InBand_Count--;
	}
    AME_spinunlock(pAMEData);
    
    if (TRUE == returnstatus) {
        
        pAMEQueueData->Current_InBand_InBand_callback = AMEQueueRequest.callback;
        
        if (AMEQueueRequest.ReqType == ReqType_NT_InBand_internal) {
            returnstatus = AME_NT_write_Host_Inbound_Queue_Port_0x40(pAMEData,AMEQueueRequest.Raid_ID,AMEQueueRequest.Request_Buffer_Phy_addr);
        } else {
            returnstatus = AME_write_Host_Inbound_Queue_Port_0x40(pAMEData,(AME_U32)AMEQueueRequest.Request_Buffer_Phy_addr);
        }
        
        if (FALSE == returnstatus) {
            // lost one cmd , but why can't send cmd to Raid ...
            AME_Msg_Err("AME_Queue_Command_InBand_complete:  send next cmd fail\n");
            AME_spinlock(pAMEData);
            pAMEQueueData->AME_Queue_InBand_Count--;
            AME_spinunlock(pAMEData);
        }
    }
    
    return TRUE;
}


AME_U32 AME_Queue_Abort_InBand_Command(pAME_Queue_DATA pAMEQueueData,AME_U64 Request_Buffer_Phy_addr)
{
    pAME_Queue_Buffer   pAMEQueueBuffer;
    pAME_Queue_List pHead = &pAMEQueueData->AME_Queue_List_InBand;
    pAME_Queue_List pNext = pHead;
    
    // check is current InBand cmd first
    if (pAMEQueueData->Current_InBand_Request_Buffer_Phy_addr == Request_Buffer_Phy_addr) {
        pAMEQueueData->Current_InBand_Is_Abort = TRUE;
        return TRUE;
    }
    
    while (1)
    {
        pNext = (pAME_Queue_List)pNext->pNext;
        
        if (pNext == pHead) {
            return FALSE;
        }
        
        pAMEQueueBuffer = (pAME_Queue_Buffer)pNext;
        if (pAMEQueueBuffer->Request_Buffer_Phy_addr == Request_Buffer_Phy_addr) {
            
            AME_Queue_del_entry(pNext);
            
            // Clear QueueBuffer
            pAMEQueueBuffer->ReqInUse = FALSE;
            pAMEQueueBuffer->Raid_ID  = 0;
            pAMEQueueBuffer->ReqType  = 0;
            pAMEQueueBuffer->Request_Buffer_Phy_addr = 0;
            
            AME_Queue_add_tail(&pAMEQueueData->AME_Queue_List_Free,pNext);
            return TRUE;
        }
        
    }
    
}


AME_U32 AME_Queue_Abort_SCSI_Command(pAME_Queue_DATA pAMEQueueData,AME_U64 Request_Buffer_Phy_addr)
{
    pAME_Queue_Buffer   pAMEQueueBuffer;
    pAME_Queue_List pHead = &pAMEQueueData->AME_Queue_List_SCSI;
    pAME_Queue_List pNext = pHead;
    
    while (1)
    {
        pNext = (pAME_Queue_List)pNext->pNext;
        
        if (pNext == pHead) {
            return FALSE;
        }
        
        pAMEQueueBuffer = (pAME_Queue_Buffer)pNext;
        if (pAMEQueueBuffer->Request_Buffer_Phy_addr == Request_Buffer_Phy_addr) {
            AME_Queue_del_entry(pNext);
            
            // Clear QueueBuffer
            pAMEQueueBuffer->ReqInUse = FALSE;
            pAMEQueueBuffer->Raid_ID  = 0;
            pAMEQueueBuffer->ReqType  = 0;
            pAMEQueueBuffer->Request_Buffer_Phy_addr = 0;
            
            AME_Queue_add_tail(&pAMEQueueData->AME_Queue_List_Free,pNext);
            return TRUE;
        }
        
    }
    
}


AME_U32 AME_Queue_Abort_Command(pAME_Data pAMEData,AME_U64 Request_Buffer_Phy_addr)
{
    AME_U32     returnstatus;
    pAME_Queue_DATA pAMEQueueData = (pAME_Queue_DATA)pAMEData->pAMEQueueData;
    
    // only support NT environment
    if (FALSE == AME_Check_is_NT_Mode(pAMEData)) {
        return FALSE;
    }
    
    AME_spinlock(pAMEData);
    returnstatus = AME_Queue_Abort_SCSI_Command(pAMEQueueData,Request_Buffer_Phy_addr);
    AME_spinunlock(pAMEData);
    
    if (TRUE == returnstatus) {
        AME_Msg_Err("Abort request in SCSI list queue done ...\n");
        return TRUE;
    }
    
    AME_spinlock(pAMEData);
    returnstatus = AME_Queue_Abort_InBand_Command(pAMEQueueData,Request_Buffer_Phy_addr);
    AME_spinunlock(pAMEData);
    
    if (TRUE == returnstatus) {
        AME_Msg_Err("Abort request in InBand list queue done ...\n");
        return TRUE;
    }
    
    return FALSE;
}

