/*
 *  AME_module.cpp
 *  Acxxx
 *
 *  Copyright (c) 2004-2014 Accusys, Inc. All rights reserved.
 *
 */

#include "AME_module.h"
#include "AME_import.h"
#include "MPIO.h"
#include "AME_Queue.h"
#include "ACS_MSG.h"

#if defined (AME_MAC_DAS)
    #include "Acxxx.h"
#endif


void AME_Cal_Buffer_Size(pAME_Data pAMEData,pAME_Buffer_Size pAMEBufferSize,AME_U16 DeviceID)
{
    AME_U32     Request_frame_size;
    AME_U32     Reply_frame_size;
    AME_U32     Sense_buffer_size;
    AME_U32     NT_Reply_queue_size;
    AME_U32     NT_Event_queue_size;

    // Request frame alignment : 256 byte,Reply frame alignment : 256 byte, others needn't
    Request_frame_size = REQUEST_MSG_Header_SIZE + 16*pAMEData->SG_per_command;
    if (Request_frame_size % 256) {
        Request_frame_size = ( (Request_frame_size/256) + 1 )*256;
    }

    Reply_frame_size = AME_REPLY_Message_SIZE;
    if (Reply_frame_size % 256) {
        Reply_frame_size = ( (Reply_frame_size/256) + 1 )*256;
    }
    
    // sense buffer needn't alignment.
    Sense_buffer_size = AME_SENSE_BUFFER_SIZE;
    if (Sense_buffer_size % 256) {
        Sense_buffer_size = ( (Sense_buffer_size/256) + 1 )*256;
    }
    
    // NT need add Software reply queue size and event queue size
    NT_Reply_queue_size = sizeof(AME_U32)*MAX_NT_Reply_Elements;
    if (NT_Reply_queue_size % 256) {
        NT_Reply_queue_size = ( (NT_Reply_queue_size/256) + 1 )*256;
    }
    
    NT_Event_queue_size = MAX_NT_LUN_Event_Size*MAX_NT_Event_Elements;
    if (NT_Event_queue_size % 256) {
        NT_Event_queue_size = ( (NT_Event_queue_size/256) + 1 )*256;
    }
    
    pAMEBufferSize->Request_frame_size  = Request_frame_size;
    pAMEBufferSize->Reply_frame_size    = Reply_frame_size;
    pAMEBufferSize->Sense_buffer_size   = Sense_buffer_size;
    pAMEBufferSize->NT_Reply_queue_size = NT_Reply_queue_size;
    pAMEBufferSize->NT_Event_queue_size = NT_Event_queue_size;
    
    return;
}


AME_U32 AME_Send_Free_ReplyMsg(pAME_Data pAMEData,AME_U32 BufferID)
{
    if (TRUE == AME_Check_is_DAS_Mode(pAMEData))
    {
        AME_Memory_zero((PVOID)pAMEData->AMEReplyBufferData[BufferID].Reply_Buffer_Vir_addr,pAMEData->Reply_frame_size_PerCommand);
        AME_write_Host_outbound_Queue_Port_0x44(pAMEData, (AME_U32)pAMEData->AMEReplyBufferData[BufferID].Reply_Buffer_Phy_addr);
        return TRUE;
    }

    return FALSE;
}


AME_U32 AME_Get_Reply_buffer_ID(pAME_Data pAMEData,AME_U32 AME_Buffer_PhyAddress_Low,AME_U32 *pReplyID,AME_U32 *pBufferID,pAMEDefaultReply *pReplyFrame)
{
    AME_U32 index,IS_Normal_Reply = FALSE;
    AME_U64 Reply_addr;
    pAMEDefaultReply ReplyFrame;

    Reply_addr = ((AME_U64)pAMEData->Glo_High_Address << 32) + (AME_U64)AME_Buffer_PhyAddress_Low;
    
    for (index=0;index<pAMEData->Total_Support_command;index++) {
        
        if (Reply_addr == pAMEData->AMEReplyBufferData[index].Reply_Buffer_Phy_addr) {
            *pReplyID = index;
            IS_Normal_Reply = TRUE;
            break;
        }
        
        //AME_Msg_Err("(%d)Get ISR Reply address addr(%p) index %d reply(%p))\n",
        //            pAMEData->AME_Serial_Number,
        //            (PVOID)Reply_addr,
        //            index,
        //            (PVOID)pAMEData->AMEReplyBufferData[index].Reply_Buffer_Phy_addr);
        
    }
    
    if (IS_Normal_Reply == FALSE) {
        
        AME_U64 NT_Event_Reply_address = 0;
        
        // Check is NT event reply 
        // ReplyID Error, Check is NT Event Reply ?
        if (TRUE == AME_NT_Check_Is_Event_Reply(pAMEData,AME_Buffer_PhyAddress_Low,&NT_Event_Reply_address)) {
            *pReplyFrame = (pAMEDefaultReply)NT_Event_Reply_address;
            *pBufferID = 0;   // AME_FUNCTION_EVENT_EXT_SWITCH, BufferID unused , don't care.
            //AME_Msg("(%d)AME_FUNCTION_EVENT_EXT_SWITCH. BufferID = %d ,ReplyID = %d\n",pAMEData->AME_Serial_Number,BufferID,ReplyID);
            return TRUE;
        }
        
        AME_Msg_Err("(%d)Get ISR Reply address error, addr(0x%x) Min(0x%llx) Max(0x%llx)\n",
                        pAMEData->AME_Serial_Number,
                        AME_Buffer_PhyAddress_Low,
                        pAMEData->AMEReplyBufferData[0].Reply_Buffer_Phy_addr,
                        pAMEData->AMEReplyBufferData[pAMEData->Total_Support_command-1].Reply_Buffer_Phy_addr);
        return FALSE;
        
    }
    
    // Check Request Buffer ID
    ReplyFrame = (pAMEDefaultReply)pAMEData->AMEReplyBufferData[*pReplyID].Reply_Buffer_Vir_addr;
    *pReplyFrame = ReplyFrame;
    *pBufferID = ReplyFrame->MsgIdentifier>>2;
    
    if ((*pBufferID < 1) ||
        (*pBufferID > (pAMEData->Total_Support_command-1)) ||
        (FALSE == pAMEData->AMEBufferData[*pBufferID].ReqInUse)) {
        
        if (ReplyFrame->Function == AME_FUNCTION_EVENT_SWITCH) {
            // In AME_FUNCTION_EVENT_SWITCH have two case
            // 1. Turn on/off Raid event switch, it's BufferID needed to release.
            // 2. Receive Raid's event, this's BufferID don't care.
            //AME_Msg_Err("(%d)AME_FUNCTION_EVENT_SWITCH. BufferID = %d ,ReplyID = %d\n",pAMEData->AME_Serial_Number,BufferID,ReplyID);
            return TRUE;
        }
        
        AME_Msg_Err("(%d)Normal BufferID (%d) ERR, ReplyID (%d),ReplyFrame->Function (0x%x) ...\n",pAMEData->AME_Serial_Number,*pBufferID,*pReplyID,ReplyFrame->Function);
        return FALSE;
    }
    
    return TRUE;
}


AME_U32 AME_Init_Prepare(pAME_Data pAMEData,pAME_Module_Prepare pAMEModulePrepare)
{
    AME_U32         Total_memory_size;
    AME_U32         Total_memory_size_PerCommand;
    AME_Buffer_Size AMEBufferSize;
    AME_U16         DeviceID = pAMEModulePrepare->DeviceID;

    pAMEData->SG_per_command = pAMEModulePrepare->SG_per_command;
    // pAMEModulePrepare->Total_Support_command is driver register outstand cmd count
    pAMEData->Total_Support_command = MAX_Command_AME;
    
    AME_Cal_Buffer_Size(pAMEData,&AMEBufferSize,DeviceID);
    
    Total_memory_size_PerCommand = ( AMEBufferSize.Request_frame_size + 
                                     AMEBufferSize.Reply_frame_size + 
                                     AMEBufferSize.Sense_buffer_size );

    pAMEData->Request_frame_size_PerCommand = AMEBufferSize.Request_frame_size;
    pAMEData->Reply_frame_size_PerCommand   = AMEBufferSize.Reply_frame_size;
    pAMEData->Sense_buffer_size_PerCommand  = AMEBufferSize.Sense_buffer_size;
    pAMEData->AMEBufferSize_PerCommand      = Total_memory_size_PerCommand;
    pAMEData->NT_Reply_queue_size_PerRaid   = AMEBufferSize.NT_Reply_queue_size;
    pAMEData->NT_Event_queue_size_PerRaid   = AMEBufferSize.NT_Event_queue_size;
    
    Total_memory_size = pAMEData->Total_Support_command*Total_memory_size_PerCommand +
                        MAX_RAID_System_ID*pAMEData->NT_Reply_queue_size_PerRaid +
                        MAX_RAID_System_ID*pAMEData->NT_Event_queue_size_PerRaid +
                        INBAND_External_BUFFER_SIZE +
                        INBAND_Internal_BUFFER_SIZE;
    
    // Request frame alignment : 256 byte,Reply frame alignment : 256 byte, others needn't
    Total_memory_size = Total_memory_size + 256;
    
    return  Total_memory_size;
}


void AME_Request_Buffer_Data_Clear(pAME_Buffer_Data pAMEBufferData)
{
    pAMEBufferData->Raid_ID = 0;    //Raid Error handle for NT
    pAMEBufferData->ReqInUse = FALSE;
    pAMEBufferData->ReqTimeouted = FALSE;
    pAMEBufferData->pNext = NULL;
    pAMEBufferData->pRequestExtension = NULL;
    pAMEBufferData->pMPIORequestExtension = NULL;
    pAMEBufferData->pAME_Raid_QueueBuffer = NULL;   //for RAID 0/1, but NT unused
    pAMEBufferData->ReqIn_Path_Remove = FALSE;
    pAMEBufferData->pAMEData_Path = NULL;
    pAMEBufferData->Path_Raid_ID = 0;
    
    return;
}


void AME_Request_Buffer_Queue_Init(pAME_Data pAMEData)
{
    AME_U32  i;
    
    pAMEData->AMEBufferissueCommandCount = 0;

    // Clear Buffer Data
    for (i=0;i<pAMEData->Total_Support_command;i++) {
        AME_Request_Buffer_Data_Clear((pAME_Buffer_Data)(&pAMEData->AMEBufferData[i]));
    }
    
    // The MsgIdentifier can't be 0, so ingore the AMEBufferData[0]
    pAMEData->AMEBuffer_Head = (PVOID)(&pAMEData->AMEBufferData[1]);
    pAMEData->AMEBuffer_Tail = (PVOID)(&pAMEData->AMEBufferData[(pAMEData->Total_Support_command - 1)]);
    
    for (i=1;i<pAMEData->Total_Support_command;i++) {
        if (i != (pAMEData->Total_Support_command - 1)) {
            pAMEData->AMEBufferData[i].pNext = (PVOID)(&pAMEData->AMEBufferData[i+1]);
        } else {
            pAMEData->AMEBufferData[i].pNext = NULL;
        }
    }
    
    return;
}


AME_U32 AME_Request_Buffer_Queue_Allocate(pAME_Data pAMEData)
{
    pAME_Buffer_Data pAMEBufferData;

    if (pAMEData->AMEBufferissueCommandCount > (pAMEData->Total_Support_command-1)) // no free buffer
    {
        AME_Msg_Err("(%d)AME_Request_Buffer_Queue_Allocate Fail. AMEBufferissueCommandCount(%d)\n",pAMEData->AME_Serial_Number,pAMEData->AMEBufferissueCommandCount);
        return (AME_U32_NONE);
    }

    AME_spinlock(pAMEData);
    
    if (pAMEData->AMEBuffer_Head == pAMEData->AMEBuffer_Tail) {
        AME_spinunlock(pAMEData);
        AME_Msg_Err("(%d)AME_Request_Buffer_Queue_Allocate Fail. No Buffer.\n",pAMEData->AME_Serial_Number);
        return (AME_U32_NONE);
    }
    
    pAMEBufferData = (pAME_Buffer_Data)pAMEData->AMEBuffer_Head;
    pAMEData->AMEBuffer_Head = pAMEBufferData->pNext;
    
    pAMEBufferData->pNext = NULL;
    pAMEBufferData->ReqInUse = TRUE;
    
    pAMEData->AMEBufferissueCommandCount++;
    AME_spinunlock(pAMEData);
    
    AME_Memory_zero((PVOID)pAMEBufferData->Request_Buffer_Vir_addr,pAMEData->Request_frame_size_PerCommand);
    AME_Memory_zero((PVOID)pAMEBufferData->Sense_Buffer_Vir_addr,pAMEData->Sense_buffer_size_PerCommand);
    
    return pAMEBufferData->BufferID;
}


void AME_Request_Buffer_Queue_Release(pAME_Data pAMEData, AME_U32 BufferID)
{
    pAME_Buffer_Data pAMEBufferData;
    
    AME_spinlock(pAMEData);
    
    pAMEBufferData = &pAMEData->AMEBufferData[BufferID];
    
    if (FALSE == pAMEBufferData->ReqInUse) {
        AME_spinunlock(pAMEData);
        AME_Msg_Err("(%d)AME_Request_Buffer_Queue_Release Err:  BufferID %d not InUse\n",pAMEData->AME_Serial_Number,BufferID);
        return;
    }
    
    if (pAMEData->AMEBufferissueCommandCount != 0) {
        pAMEData->AMEBufferissueCommandCount--;
    } else {
        AME_Msg_Err("(%d)AME_Request_Buffer_Queue_Release Err:  AMEBufferissueCommandCount is zero\n",pAMEData->AME_Serial_Number);
    }
    
    AME_Request_Buffer_Data_Clear(pAMEBufferData);
    
    ((pAME_Buffer_Data)pAMEData->AMEBuffer_Tail)->pNext = (PVOID)pAMEBufferData;
    pAMEData->AMEBuffer_Tail = (PVOID)pAMEBufferData;

    AME_spinunlock(pAMEData);
    
    return;
}


void AME_Reply_Buffer_Init(pAME_Data pAMEData)
{
    AME_U32     i;
    
    // Only Das need do this ...
    if (TRUE == AME_Check_is_DAS_Mode(pAMEData))
    {
        for (i=1;i<(pAMEData->Total_Support_command-1);i++) {
            AME_Send_Free_ReplyMsg(pAMEData,i);    
        }
    }

    return;
}


AME_U64 AME_alignment_memory_address(AME_U64 mem_addr,AME_U32 boundary)
{
    AME_U32 Temp_Offset = 0;
    AME_U32 Temp_mem_addr = (AME_U32)mem_addr;
    
    // avoid OS 64 bit and 32 bit mod and divide complier error issue
    if ((Temp_mem_addr % boundary) != 0) {
        Temp_mem_addr = (Temp_mem_addr/boundary + 1) * boundary;
        Temp_Offset = Temp_mem_addr - (AME_U32)mem_addr;
        mem_addr += (AME_U64)Temp_Offset;
    }
    return mem_addr;
}


void AME_Buffer_Data_Init(pAME_Data pAMEData)
{
    AME_U32     i,j;
    AME_U64     StartPhyAddress,StartVirAddress,addr_offset;
    
    if (pAMEData->Init_Memory_by_AME_module == TRUE)
    {
    
        // make sure get Start Phy/Vir addess match alignment size
        // NT alignment size 256, Das alignment size 16, driver get maximun.
        StartPhyAddress = AME_alignment_memory_address(pAMEData->AME_Buffer_physicalStartAddress,256);
        if (StartPhyAddress != pAMEData->AME_Buffer_physicalStartAddress) {
            AME_Msg("(%d)AME_Buffer_Data_Init:  Start phy/vir addess not match alignment size\n",pAMEData->AME_Serial_Number);
            addr_offset = StartPhyAddress - pAMEData->AME_Buffer_physicalStartAddress;
            pAMEData->AME_Buffer_physicalStartAddress += addr_offset;
            pAMEData->AME_Buffer_virtualStartAddress  += addr_offset;
        }
        
        // init the address ...
        for (i=0;i<pAMEData->Total_Support_command;i++) {
            
            StartPhyAddress = pAMEData->AME_Buffer_physicalStartAddress + (i*pAMEData->AMEBufferSize_PerCommand);
            StartVirAddress = pAMEData->AME_Buffer_virtualStartAddress  + (i*pAMEData->AMEBufferSize_PerCommand);
            
            // Init Request buffer
            pAMEData->AMEBufferData[i].Request_Buffer_Phy_addr = StartPhyAddress;
            pAMEData->AMEBufferData[i].Request_Buffer_Vir_addr = StartVirAddress;
            
            // Init BDL buffer
            pAMEData->AMEBufferData[i].Request_BDL_Phy_addr = StartPhyAddress + REQUEST_MSG_Header_SIZE;
            pAMEData->AMEBufferData[i].Request_BDL_Vir_addr = StartVirAddress + REQUEST_MSG_Header_SIZE;
            
            // Init Sense buffer
            pAMEData->AMEBufferData[i].Sense_Buffer_Phy_addr = StartPhyAddress + pAMEData->Request_frame_size_PerCommand;
            pAMEData->AMEBufferData[i].Sense_Buffer_Vir_addr = StartVirAddress + pAMEData->Request_frame_size_PerCommand;
            
            // Init Reply buffer
            pAMEData->AMEReplyBufferData[i].Reply_Buffer_Phy_addr = StartPhyAddress + pAMEData->Request_frame_size_PerCommand + pAMEData->Sense_buffer_size_PerCommand;
            pAMEData->AMEReplyBufferData[i].Reply_Buffer_Vir_addr = StartVirAddress + pAMEData->Request_frame_size_PerCommand + pAMEData->Sense_buffer_size_PerCommand;
            
        }
        
        // Init NT Reply queue address
        for (i=0; i<MAX_RAID_System_ID; i++)
        {
            pAMEData->AME_RAIDData[i].NT_Raid_Reply_queue_Start_Phy_addr = pAMEData->AME_Buffer_physicalStartAddress
                                                                         + (pAMEData->Total_Support_command*pAMEData->AMEBufferSize_PerCommand)
                                                                         + (i*pAMEData->NT_Reply_queue_size_PerRaid);
            
            pAMEData->AME_RAIDData[i].NT_Raid_Reply_queue_Start_Vir_addr = pAMEData->AME_Buffer_virtualStartAddress
                                                                         + (pAMEData->Total_Support_command*pAMEData->AMEBufferSize_PerCommand)
                                                                         + (i*pAMEData->NT_Reply_queue_size_PerRaid);
        }
        
        // Init NT Event queue address
        for (i=0; i<MAX_RAID_System_ID; i++)
        {
            pAMEData->AME_RAIDData[i].NT_Raid_Event_queue_Start_Phy_addr = pAMEData->AME_Buffer_physicalStartAddress
                                                                         + (pAMEData->Total_Support_command*pAMEData->AMEBufferSize_PerCommand)
                                                                         + (MAX_RAID_System_ID*pAMEData->NT_Reply_queue_size_PerRaid)
                                                                         + (i*pAMEData->NT_Event_queue_size_PerRaid);
            
            pAMEData->AME_RAIDData[i].NT_Raid_Event_queue_Start_Vir_addr = pAMEData->AME_Buffer_virtualStartAddress
                                                                         + (pAMEData->Total_Support_command*pAMEData->AMEBufferSize_PerCommand)
                                                                         + (MAX_RAID_System_ID*pAMEData->NT_Reply_queue_size_PerRaid)
                                                                         + (i*pAMEData->NT_Event_queue_size_PerRaid);
        }
        
        // Init InBandBuffer address ...
        pAMEData->AMEInBandBufferData.InBand_External_Buffer_Phy_addr = pAMEData->AME_Buffer_physicalStartAddress
                                                             + (pAMEData->Total_Support_command*pAMEData->AMEBufferSize_PerCommand)
                                                             + (MAX_RAID_System_ID*pAMEData->NT_Reply_queue_size_PerRaid)
                                                             + (MAX_RAID_System_ID*pAMEData->NT_Event_queue_size_PerRaid);
        
        pAMEData->AMEInBandBufferData.InBand_External_Buffer_Vir_addr = pAMEData->AME_Buffer_virtualStartAddress
                                                             + (pAMEData->Total_Support_command*pAMEData->AMEBufferSize_PerCommand)
                                                             + (MAX_RAID_System_ID*pAMEData->NT_Reply_queue_size_PerRaid)
                                                             + (MAX_RAID_System_ID*pAMEData->NT_Event_queue_size_PerRaid);
    
        pAMEData->AMEInBandBufferData.InBand_Internal_Buffer_Phy_addr = pAMEData->AME_Buffer_physicalStartAddress
                                                             + (pAMEData->Total_Support_command*pAMEData->AMEBufferSize_PerCommand)
                                                             + (MAX_RAID_System_ID*pAMEData->NT_Reply_queue_size_PerRaid)
                                                             + (MAX_RAID_System_ID*pAMEData->NT_Event_queue_size_PerRaid)
                                                             + INBAND_External_BUFFER_SIZE;
        
        pAMEData->AMEInBandBufferData.InBand_Internal_Buffer_Vir_addr = pAMEData->AME_Buffer_virtualStartAddress
                                                             + (pAMEData->Total_Support_command*pAMEData->AMEBufferSize_PerCommand)
                                                             + (MAX_RAID_System_ID*pAMEData->NT_Reply_queue_size_PerRaid)
                                                             + (MAX_RAID_System_ID*pAMEData->NT_Event_queue_size_PerRaid)
                                                             + INBAND_External_BUFFER_SIZE;
                                                             
    }
    
    // Init AMEBufferData
    for (i=0;i<pAMEData->Total_Support_command;i++) {
        pAMEData->AMEBufferData[i].BufferID=i;
        pAMEData->AMEBufferData[i].ReqInUse=FALSE;
        pAMEData->AMEBufferData[i].ReqTimeouted=FALSE;
        pAMEData->AMEBufferData[i].pRequestExtension=0;
        pAMEData->AMEBufferData[i].pMPIORequestExtension=0;
        pAMEData->AMEBufferData[i].pAME_Raid_QueueBuffer=0;  // for RAID 0/1, but NT unused
        pAMEData->AMEBufferData[i].ReqIn_Path_Remove=FALSE;
        pAMEData->AMEBufferData[i].pAMEData_Path=0;
        pAMEData->AMEBufferData[i].Path_Raid_ID=0;
    }
    
    // Memory set 0xFFFFFFFF to NT reply Queue
    for (i=0;i<MAX_RAID_System_ID;i++) {
        for (j=0;j<MAX_NT_Reply_Elements;j++) {
            /* Clear Reply Queue Entry state */
            *((AME_U32 *)pAMEData->AME_RAIDData[i].NT_Raid_Reply_queue_Start_Vir_addr + j) = MUEmpty;
        }
    }
    
    AME_Request_Buffer_Queue_Init(pAMEData);
    
    
    // Fixed Win storport hibernation error issue.
    // Wait AME_Host_RAID_Ready to Init Das reply buffer.
    //AME_Reply_Buffer_Init(pAMEData);
    
    return;
}


/*******************************************************************
* Function:  AME_NT_Cleanup_Reply_Queue_EntryState
*******************************************************************/
void AME_NT_Cleanup_Reply_Queue_EntryState(pAME_Data pAMEData,AME_U32 Raid_ID)
{
    AME_U32  Read_Index;
    
    for (Read_Index=0; Read_Index < MAX_NT_Reply_Elements; Read_Index++) {
        /* Clear Reply Queue Entry state */
        *((AME_U32 *)pAMEData->AME_RAIDData[Raid_ID].NT_Raid_Reply_queue_Start_Vir_addr + Read_Index) = MUEmpty;
    }
    
}


AME_U32 AME_Event_Put_log(pAME_Data pAMEData, AME_U8 *string)
{
    AME_U32 Head,Tail;
    
    Head = pAMEData->AMEEventlogqueue.head;
    Tail = pAMEData->AMEEventlogqueue.tail;

    AME_Memory_copy((PVOID)pAMEData->AMEEventlogqueue.eventlog[Head].eventstring, (PVOID)string, AME_EVENT_STRING_LENGTH);

    if (++Head == EVENT_LOG_QUEUE_SIZE) {
        Head=0;
    }
    
    if (Head == Tail) {
        if (++Tail == EVENT_LOG_QUEUE_SIZE) {
            Tail=0;
        }
    }
    
    pAMEData->AMEEventlogqueue.head = Head;
    pAMEData->AMEEventlogqueue.tail = Tail;
    
    return TRUE;
    
}


AME_U32 AME_Event_Get_log(pAME_Data pAMEData, AME_U8 *string)
{
    AME_U32 Head,Tail;
    
    Head = pAMEData->AMEEventlogqueue.head;
    Tail = pAMEData->AMEEventlogqueue.tail;
    
    /* Event empty */
    if (Head == Tail) {
        return FALSE;
    }
    
    AME_Memory_copy((PVOID)string, (PVOID)pAMEData->AMEEventlogqueue.eventlog[Tail].eventstring, AME_EVENT_STRING_LENGTH);

    if (++Tail == EVENT_LOG_QUEUE_SIZE) {
        Tail=0;
    }

    pAMEData->AMEEventlogqueue.tail = Tail;

    return TRUE;
}


/* 
 * FUNCTION:  AME_Event_Turn_Switch
 * @SWitchState:
 *      AME_EVENT_SWTICH_ON  (0x01)
 *      AME_EVENT_SWTICH_OFF (0x00)
 */
AME_U32 AME_Event_Turn_Switch(pAME_Data pAMEData, AME_U8 SWitchState)
{
    AME_U32                     BufferID=0;
    pAMEEventSwitchRequest_DAS  EventSwitchRequest;

    if (pAMEData->AMEEventSwitchState == SWitchState) {
        return TRUE;
    }
    
    if ( (BufferID=AME_Request_Buffer_Queue_Allocate(pAMEData)) == AME_U32_NONE ) {
        return FALSE;
    }
    
    EventSwitchRequest = (pAMEEventSwitchRequest_DAS)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
    EventSwitchRequest->Switch = SWitchState;
    EventSwitchRequest->Function = AME_FUNCTION_EVENT_SWITCH;
    EventSwitchRequest->MsgIdentifier = (BufferID << 2);
    
    if ( FALSE == AME_write_Host_Inbound_Queue_Port_0x40(pAMEData,(AME_U32)pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr) ) {
        AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
        return FALSE;
    }
    
    pAMEData->AMEEventSwitchState = SWitchState;
    
    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Normal_handler_EVENT_SWITCH (pAME_REQUEST_CallBack pAMEREQUESTCallBack)
{
    pAME_Data pAMEData = pAMEREQUESTCallBack->pAMEData;
    pAMEEventSwitchReply EventSwitchReply  = (pAMEEventSwitchReply) pAMEREQUESTCallBack->pReplyFrame;
    
    
    // Copy Event into EventQueue
    AME_Event_Put_log(pAMEData, EventSwitchReply->EDB);
    
    /* Lun Change Event */
    if (EventSwitchReply->EDB[0] == 0x14) {
        AME_spinlock(pAMEData);
        pAMEData->AME_RAIDData[0].Lun_Change_State |= Lun_Change_State_Need_check;
        AME_spinunlock(pAMEData);
    }
    
    /* Turn on/off Switch event, need to release request buffer */
    if ((EventSwitchReply->EDB[0]==0x55)||(EventSwitchReply->EDB[0]==0xAA)) {
        /* 2208 3108 New error handling : When Host driver receive EventTurnOnSwitch reply, clear to 0x0000.*/
       if ( (pAMEData->Device_ID == AME_2208_DID1_DAS) ||
          (pAMEData->Device_ID == AME_2208_DID2_DAS) ||
          (pAMEData->Device_ID == AME_2208_DID3_DAS) ||
          (pAMEData->Device_ID == AME_2208_DID4_DAS) ||
          (pAMEData->Device_ID == AME_3108_DID0_DAS) ||
          (pAMEData->Device_ID == AME_3108_DID1_DAS) ) {
           AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_2208_3108_SCARTCHPAD2_REG_OFFSET,0x00010000); 
        }

        return FALSE;
    }
       
    return TRUE;
}

AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Normal_handler_EVENT_EXT_SWITCH (pAME_REQUEST_CallBack pAMEREQUESTCallBack)
{
    pAME_Data pAMEData = pAMEREQUESTCallBack->pAMEData;
    AME_U32   Raid_ID  = (AME_U32)pAMEREQUESTCallBack->Raid_ID;
    pAMEEventSwitchReply EventSwitchReply  = (pAMEEventSwitchReply) pAMEREQUESTCallBack->pReplyFrame;
    
    /* Lun Change Event */
    if ( (EventSwitchReply->EDB[0] == 0x14) && (pAMEData->AME_RAIDData[Raid_ID].InBand_Loaded_Ready == TRUE) ) {

        if (EventSwitchReply->EDB[1] == 0x11){          
            if (EventSwitchReply->EDB[2]==pAMEData->AME_NTHost_data.PCISerialNumber){
                AME_spinlock(pAMEData);
                pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State |= Lun_Mask_Need_check;
                AME_spinunlock(pAMEData);                       
            }
        }
        else{
            AME_spinlock(pAMEData);
            pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State |= Lun_Change_State_Need_check;
            AME_spinunlock(pAMEData);
        }

    }
       
    return TRUE;
}


AME_U32 AME_InBand_Get_Page_DAS(pAME_Data pAMEData,AME_U8 page_index,AME_U32 BufferID,AME_U32 (*callback)(pAME_REQUEST_CallBack pAMERequestCallBack))
{
    pAMEInBandRequest_DAS       InBandRequest;
    
    InBandRequest = (pAMEInBandRequest_DAS)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
    InBandRequest->Flags = AME_INBAND_FLAGS_READ;
    InBandRequest->Function = AME_FUNCTION_IN_BAND;
    InBandRequest->MsgIdentifier = (BufferID << 2);
    
    InBandRequest->DataLength = 0x200; // Controller Page Size
    InBandRequest->InBandCDB[0]  = 0x01;
    InBandRequest->InBandCDB[1]  = 0x01;
    InBandRequest->InBandCDB[2]  = page_index;
    
    InBandRequest->BDL.Address_Low = (AME_U32)pAMEData->AMEInBandBufferData.InBand_Internal_Buffer_Phy_addr;
    InBandRequest->BDL.Address_High = (AME_U32)(pAMEData->AMEInBandBufferData.InBand_Internal_Buffer_Phy_addr>>32);
    InBandRequest->BDL.FlagsLength = InBandRequest->DataLength | ( (AME_FLAGS_DIR_DTR_TO_HOST | AME_FLAGS_64_BIT_ADDRESSING) << 24);

    if (FALSE == AME_Queue_Command(pAMEData,0,ReqType_DAS_InBand_internal,pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr,(PVOID)callback)) {
        return FALSE;
    }

    return TRUE;
}

AME_U32 AME_InBand_Get_Page_NT(pAME_Data pAMEData,AME_U8 page_index,AME_U32 BufferID,AME_U32 Raid_ID,AME_U32 (*callback)(pAME_REQUEST_CallBack pAMERequestCallBack))
{
    pAMEInBandRequest_NT       InBandRequest;
    
    pAMEData->AMEBufferData[BufferID].Raid_ID = Raid_ID;  // Raid Error handle for NT
    
    InBandRequest = (pAMEInBandRequest_NT)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
    InBandRequest->Flags = AME_INBAND_FLAGS_READ;
    InBandRequest->Function = AME_FUNCTION_IN_BAND_EXT;
    InBandRequest->MsgIdentifier = (BufferID << 2);
    
    InBandRequest->DataLength = 0x200; // Controller Page Size
    InBandRequest->InBandCDB[0]  = 0x01;
    InBandRequest->InBandCDB[1]  = 0x01;
    InBandRequest->InBandCDB[2]  = page_index;
    
    InBandRequest->PCISerialNumber = pAMEData->AME_NTHost_data.PCISerialNumber;
    
    InBandRequest->ReplyAddressLow = (AME_U32)pAMEData->AMEReplyBufferData[BufferID].Reply_Buffer_Phy_addr +
                                      pAMEData->AME_NTHost_data.NT_linkside_BAR2_lowaddress;
    
    InBandRequest->BDL.Address_Low = (AME_U32)pAMEData->AMEInBandBufferData.InBand_Internal_Buffer_Phy_addr +
                                      pAMEData->AME_NTHost_data.NT_linkside_BAR2_lowaddress;
                                      
    InBandRequest->BDL.Address_High = (AME_U32)(pAMEData->AMEInBandBufferData.InBand_Internal_Buffer_Phy_addr>>32) + 
                                       pAMEData->AME_NTHost_data.NT_linkside_BAR2_highaddress;
                                       
    InBandRequest->BDL.FlagsLength = InBandRequest->DataLength | ( (AME_FLAGS_DIR_DTR_TO_HOST | AME_FLAGS_64_BIT_ADDRESSING) << 24);

    if (FALSE == AME_Queue_Command(pAMEData,Raid_ID,ReqType_NT_InBand_internal,pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr,(PVOID)callback)) {
        return FALSE;
    }

    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_InBand_Get_Page(pAME_Data pAMEData,AME_U32 Raid_ID,AME_U8 page_index,AME_U32 (*callback)(pAME_REQUEST_CallBack pAMERequestCallBack))
{
    AME_U32  BufferID = 0;

    if ( (BufferID=AME_Request_Buffer_Queue_Allocate(pAMEData)) == AME_U32_NONE ) {
        return FALSE;
    }
    
    if (TRUE == AME_Check_is_NT_Mode(pAMEData))
    {
        if (FALSE == AME_InBand_Get_Page_NT(pAMEData,page_index,BufferID,Raid_ID,callback)) {
            AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
            return FALSE;
        }
    } else {
        if (FALSE == AME_InBand_Get_Page_DAS(pAMEData,page_index,BufferID,callback)) {
            AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
            return FALSE;
        }
    }
    
    return TRUE;
}

AME_U32 AME_Build_InBand_Cmd(pAME_Data pAMEData,pAME_Module_InBand_Command pAME_ModuleInBand_Command)
{
    AME_U32                     BufferID=0;
    pAMEInBandRequest_DAS       InBandRequest;

    PVOID InBand_CDB = pAME_ModuleInBand_Command->InBand_CDB;
    PVOID InBand_Buffer_addr = pAME_ModuleInBand_Command->InBand_Buffer_addr;
    PVOID pRequestExtension = pAME_ModuleInBand_Command->pRequestExtension;
            
    // Fixed Bug 7861,  Thunderbolt HotPlug issue.
    if (AME_Get_RAID_Ready(pAMEData) == FALSE) {
        return FALSE;
    }
    
    /* Raid only support one InBand cmd */
    if (pAMEData->AMEInBandBufferData.InBandFlag == INBANDCMD_IDLE) {
        pAMEData->AMEInBandBufferData.InBandFlag = INBANDCMD_USING;
        pAMEData->AMEInBandBufferData.InBandReply = 0;
        pAMEData->AMEInBandBufferData.InBandErrorCode = 0;
        pAMEData->AMEInBandBufferData.InBand_Length = 0;
        pAMEData->AMEInBandBufferData.callback = NULL;  // must set callback to NULL
    } else {
        AME_Msg_Err("(%d)AME_Build_InBand_Cmd:  External AME_InBand_Cmd using...\n",pAMEData->AME_Serial_Number);
        return FALSE;
    }
    
    if ( (BufferID=AME_Request_Buffer_Queue_Allocate(pAMEData)) == AME_U32_NONE ) {
        return FALSE;
    }
    
    InBandRequest = (pAMEInBandRequest_DAS)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
    InBandRequest->Function = AME_FUNCTION_IN_BAND;
    InBandRequest->MsgIdentifier = (BufferID << 2);
    AME_Memory_copy((PVOID)InBandRequest->InBandCDB,(PVOID)InBand_CDB,INBAND_CMD_LENGHT);
    
    // Handle InBand Request
    switch (InBandRequest->InBandCDB[0])
    {
        case 0x01: // Get Page
        case 0xB2: // Read Signature
            InBandRequest->Flags = AME_INBAND_FLAGS_READ;
            InBandRequest->DataLength = 0x200; // Controller Page Size
            break;
        
        case 0x93: // Expander Update 
        case 0x94: // SysRom Update
        case 0x95: // BootRom Update
        case 0x96: // OptionRom Update
            InBandRequest->Flags = AME_INBAND_FLAGS_WRITE;
            InBandRequest->DataLength = (AME_U32)( *((AME_U32 *)(&(InBandRequest->InBandCDB[2]))) );
            break;
            
        case 0xA8: // RTC Get
            InBandRequest->Flags = AME_INBAND_FLAGS_READ;
            InBandRequest->DataLength = 0x07; // RTC Size
            break;
        
        case 0xB1: // Write Signature
            InBandRequest->Flags = AME_INBAND_FLAGS_WRITE;
            InBandRequest->DataLength = 0x200; // Signature Size
            break;
        
        //  Windows only ??? Still used ??? ->
        case 0xCA: // ECHO Read
            InBandRequest->Flags = AME_INBAND_FLAGS_READ;
            InBandRequest->DataLength = (AME_U32)( *((AME_U16 *)(&(InBandRequest->InBandCDB[2]))) );
            break;
        
        case 0xCB: // ECHO Write
            InBandRequest->Flags = AME_INBAND_FLAGS_WRITE;
            InBandRequest->DataLength = (AME_U32)( *((AME_U16 *)(&(InBandRequest->InBandCDB[2]))) );
            break;
        // ~Windows only ??? Still used ??? <-
        
        default:
            InBandRequest->Flags = AME_INBAND_FLAGS_NODATATRANSFER;
            InBandRequest->DataLength = 0x00;
            break;
    }
    
    switch (InBandRequest->Flags)
    {
        case AME_INBAND_FLAGS_READ:
            InBandRequest->BDL.Address_Low = (AME_U32)pAMEData->AMEInBandBufferData.InBand_External_Buffer_Phy_addr;
            InBandRequest->BDL.Address_High = (AME_U32)(pAMEData->AMEInBandBufferData.InBand_External_Buffer_Phy_addr>>32);
            InBandRequest->BDL.FlagsLength = InBandRequest->DataLength | ( (AME_FLAGS_DIR_DTR_TO_HOST | AME_FLAGS_64_BIT_ADDRESSING) << 24);
            break;
            
        case AME_INBAND_FLAGS_WRITE:
            AME_Memory_copy((PVOID)pAMEData->AMEInBandBufferData.InBand_External_Buffer_Vir_addr,InBand_Buffer_addr,InBandRequest->DataLength);
            InBandRequest->BDL.Address_Low = (AME_U32)pAMEData->AMEInBandBufferData.InBand_External_Buffer_Phy_addr;
            InBandRequest->BDL.Address_High = (AME_U32)(pAMEData->AMEInBandBufferData.InBand_External_Buffer_Phy_addr>>32);
            InBandRequest->BDL.FlagsLength = InBandRequest->DataLength | ( (AME_FLAGS_DIR_HOST_TO_DTR | AME_FLAGS_64_BIT_ADDRESSING) << 24);
            break;
            
        default:
            /* AME_INBAND_FLAGS_NODATATRANSFER */
            break;
            
    }
    
    pAMEData->AMEInBandBufferData.InBand_Length = InBandRequest->DataLength;
    pAMEData->AMEInBandBufferData.InBand_DataBuffer = InBand_Buffer_addr;
    pAMEData->AMEBufferData[BufferID].pRequestExtension = pRequestExtension;
    
    if (FALSE == AME_Queue_Command(pAMEData,0,ReqType_DAS_InBand_external,pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr,NULL)) {
        return FALSE;
    }
    
    return TRUE;
}


AME_U32 AME_Modify_GetPage_Link_Speed_Lanes(pAME_REQUEST_CallBack pAMEREQUESTCallBack,AME_U32 BufferID)
{
    pAME_Data pAMEData = pAMEREQUESTCallBack->pAMEData;
    pAMEInBandRequest_DAS  InBandRequest;
    AME_U8  *pCDB;

    InBandRequest = (pAMEInBandRequest_DAS)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
    
    pCDB = (AME_U8  *)InBandRequest->InBandCDB;

    // Get page command here, modify link status field.
    
    if (pCDB[0] == 0x01 && pCDB[2] == 0x00)
    {
        
        AME_U32             LinkCtrlSts;
        AME_U8              *InBand_Buffer;
    
        LinkCtrlSts = pAMEData->AME_Raid_Link_Status;
        
        if (LinkCtrlSts != FALSE) {

            InBand_Buffer = (AME_U8 *)pAMEData->AMEInBandBufferData.InBand_External_Buffer_Vir_addr;

            // Modify the link speed
            LinkCtrlSts >>= 16;
            *(InBand_Buffer + 0x60) = (AME_U8)(LinkCtrlSts & 0x0F);

            // Modify the link Lanes
            LinkCtrlSts >>= 4;
            
            // In 8717 HBA modify lanes from 8 lanes to 4 lanes
            if ((LinkCtrlSts & 0x0F) == 0x08) {
                *(InBand_Buffer + 0x61) = (AME_U8)(0x04);
            } else {
                *(InBand_Buffer + 0x61) = (AME_U8)(LinkCtrlSts & 0x0F);
            }
            
            return TRUE;
        }

        return FALSE;
    }           
    
    return FALSE;
}


void 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_report_Lun_Change(pAME_Data pAMEData)
{
    AME_U32 Lun_ID,RAID_ID;

    for (RAID_ID=0; RAID_ID<MAX_RAID_System_ID; RAID_ID++)
    {
        // in lun change checking
        if (pAMEData->AME_RAIDData[RAID_ID].Lun_Change_State != Lun_Change_State_NONE) {
            return;
        }
        for (Lun_ID=0; Lun_ID<MAX_RAID_LUN_ID; Lun_ID++)
        {
            if (pAMEData->AME_RAIDData[RAID_ID].Lun_Change[Lun_ID] != Lun_Change_NONE) {

                if (pAMEData->AME_RAIDData[RAID_ID].Lun_Change[Lun_ID] & Lun_Change_Add) {
                    AME_Msg("(%d)Raid %d Lun %d Add\n",pAMEData->AME_Serial_Number,RAID_ID,Lun_ID);
                    MPIO_Host_Lun_Change(pAMEData,RAID_ID,Lun_ID,Lun_Add);
                }
                else {
                    AME_Msg("(%d)Raid %d Lun %d Remove\n",pAMEData->AME_Serial_Number,RAID_ID,Lun_ID);
                    MPIO_Host_Lun_Change(pAMEData,RAID_ID,Lun_ID,Lun_Remove);
                }
        
                pAMEData->AME_RAIDData[RAID_ID].Lun_Change[Lun_ID] = Lun_Change_NONE;
            }
        }
    }
    
    return;
}


void 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_NT_Report_Raid_Add(pAME_Data pAMEData)
{
    AME_U32 Raid_ID,Lun_ID;
    
    for (Raid_ID=0; Raid_ID<MAX_RAID_System_ID; Raid_ID++)
    {
        if ( (pAMEData->AME_NTHost_data.Raid_Change_Topology >> Raid_ID) & 0x01 ) {
            
            pAMEData->AME_NTHost_data.Raid_Change_Topology -= (0x01 << Raid_ID);
            
            if (pAMEData->AME_RAIDData[Raid_ID].Raid_Error_Handle_State == TRUE) {
                pAMEData->AME_NTHost_data.Error_Raid_bitmap -= (0x01 << Raid_ID);
                pAMEData->AME_RAIDData[Raid_ID].Raid_Error_Handle_State = FALSE;
                AME_Msg("(%d)Raid Error Handle:  NT Raid (%d) Error Handle Done ...\n",pAMEData->AME_Serial_Number,Raid_ID);
                continue;
            }
            
            if ( (pAMEData->AME_NTHost_data.Raid_All_Topology >> Raid_ID) & 0x01 ) {
                
                for (Lun_ID=0; Lun_ID<MAX_RAID_LUN_ID; Lun_ID++) {
                    
                    if ((pAMEData->AME_RAIDData[Raid_ID].Lun_Data[Lun_ID] != Lun_Not_Exist) &&
                        (pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[Lun_ID] == UnMask)) {
                        MPIO_Host_Lun_Change(pAMEData,Raid_ID,Lun_ID,Lun_Add);
                    }
                    
                    pAMEData->AME_RAIDData[Raid_ID].Lun_Change[Lun_ID] = Lun_Change_NONE;
                }
            }
           
        }
    }
    
    return;
}


void 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_NT_Report_Raid_Remove(pAME_Data pAMEData,AME_U32 Raid_ID)
{
    AME_U32 Lun_ID;
    
    /* Clear Raid SendVir_Ready Flag */
    pAMEData->AME_RAIDData[Raid_ID].SendVir_Ready = FALSE;
    pAMEData->AME_RAIDData[Raid_ID].InBand_Loaded_Ready = FALSE;
    
    /* Clear Reply Queue entry state */
    AME_NT_Cleanup_Reply_Queue_EntryState(pAMEData,Raid_ID);

    for (Lun_ID=0; Lun_ID<MAX_RAID_LUN_ID; Lun_ID++)
    {
        if ((pAMEData->AME_RAIDData[Raid_ID].Lun_Data[Lun_ID] != Lun_Not_Exist)&&
            (pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[Lun_ID] == UnMask)) {
            MPIO_Host_Lun_Change(pAMEData,Raid_ID,Lun_ID,Lun_Remove);
        }
        
        pAMEData->AME_RAIDData[Raid_ID].Lun_Data[Lun_ID] = Lun_Not_Exist;
        pAMEData->AME_RAIDData[Raid_ID].Lun_Change[Lun_ID] = Lun_Change_NONE;
        pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State = Lun_Change_State_NONE;
    }
    
    MPIO_AME_RAID_Out(pAMEData,Raid_ID);
    
    return;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Normal_handler_IN_BAND (pAME_REQUEST_CallBack pAMEREQUESTCallBack)
{
    pAME_Data pAMEData = pAMEREQUESTCallBack->pAMEData;
    AME_U32   BufferID = pAMEREQUESTCallBack->BufferID;
    pAMEInBandReply InBandReply  = (pAMEInBandReply) pAMEREQUESTCallBack->pReplyFrame;
    AME_U32   (*InBand_callback)(pAME_REQUEST_CallBack) = (AME_U32 (*)(pAME_REQUEST_CallBack))AME_Queue_Get_InBand_Command_callback(pAMEData);
    
    if (InBand_callback == NULL) { // external inband
        
        pAMEData->AMEInBandBufferData.InBandReply = InBandReply->ReplyStatus;
        pAMEData->AMEInBandBufferData.InBandErrorCode = InBandReply->InBandErrorCode;
        
        if (InBandReply->ReplyStatus == AME_REPLYSTATUS_SUCCESS) {
            pAMEData->AMEInBandBufferData.InBandFlag = INBANDCMD_COMPLETE;
            if (pAMEData->AMEInBandBufferData.InBand_Length) {
                AME_Modify_GetPage_Link_Speed_Lanes(pAMEREQUESTCallBack,BufferID);
                AME_Memory_copy((PVOID)pAMEData->AMEInBandBufferData.InBand_DataBuffer,
                                (PVOID)pAMEData->AMEInBandBufferData.InBand_External_Buffer_Vir_addr,
                                 pAMEData->AMEInBandBufferData.InBand_Length);
            }
        }
        else {
            
            // Return error code to GUI
            #if defined (AME_Linux)
                AME_U8 *return_code = (AME_U8 *)pAMEData->AMEInBandBufferData.InBand_DataBuffer;
                *return_code = (AME_U8)InBandReply->InBandErrorCode;
                return_code++;
                *return_code = (AME_U8)InBandReply->ReplyStatus;
            #else
                // IN Mac and Windows OS, inband buffer include SRB_IO_CONTROL and AME_Inband struct.
                // Mac and Windows GC receice SRB_IO_CONTROL->ReturnCode to check error code.
                // return_code address = InBand_DataBuffer -   UInt32  ReturnCode - UInt32  Length - UInt16 inband Cmd Code - Cmd Length.
                pAMEInBandRequest_DAS  InBandRequest = (pAMEInBandRequest_DAS)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
                AME_U8 *pSIC = (AME_U8 *)pAMEData->AMEInBandBufferData.InBand_DataBuffer-10-InBandRequest->InBandCDB[1];
                AME_U32 *return_code = (AME_U32 *)pSIC;
                *return_code = (((AME_U32)InBandReply->ReplyStatus & 0x000000FF) | (((AME_U32)InBandReply->InBandErrorCode << 16) & 0xFFFF0000));
            #endif
            
            pAMEData->AMEInBandBufferData.InBandFlag = INBANDCMD_FAILED;
            AME_Msg_Err("(%d)External InBand Error:  Inband Request not success!\n",pAMEData->AME_Serial_Number);
            AME_Msg_Err("(%d)                        Inband ReplyStatus 0x%x\n",pAMEData->AME_Serial_Number,InBandReply->ReplyStatus);
            AME_Msg_Err("(%d)                        Inband ErrorCode   0x%x\n",pAMEData->AME_Serial_Number,InBandReply->InBandErrorCode);
        }
        
        MPIO_Host_Normal_handler_IN_Band(pAMEREQUESTCallBack);
        pAMEData->AMEInBandBufferData.InBandFlag = INBANDCMD_IDLE;
    }
    
    else { // internal inband
        if (InBandReply->ReplyStatus == AME_REPLYSTATUS_SUCCESS) {
            InBand_callback(pAMEREQUESTCallBack);
            AME_report_Lun_Change(pAMEData);
        }
        else {
            AME_Msg_Err("(%d)Internal InBand Error:  Inband Request not success, retry again!\n",pAMEData->AME_Serial_Number);
            AME_Msg_Err("(%d)                        Inband ReplyStatus 0x%x\n",pAMEData->AME_Serial_Number,InBandReply->ReplyStatus);
            AME_Msg_Err("(%d)                        Inband ErrorCode   0x%x\n",pAMEData->AME_Serial_Number,InBandReply->InBandErrorCode);
            AME_Queue_Command(pAMEData,0,ReqType_Current_InBand_error,0,NULL);
        }
    }
    
    AME_Queue_Command_InBand_complete(pAMEData);
    
    return TRUE;
}


AME_U32 AME_Lun_Alive_Check(pAME_Data pAMEData,AME_U32 Raid_ID,AME_U32 Lun_ID)
{
    if ((pAMEData->AME_RAIDData[Raid_ID].Lun_Data[Lun_ID] == Lun_Not_Exist)||
        (pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[Lun_ID] == Mask)) {
        return FALSE;
    }
    
    return TRUE;
}


AME_U32 AME_NT_Raid_Alive_Check (pAME_Data pAMEData,AME_U32 Raid_ID)
{
    /* 
     * In some OS, after driver loaded will call scan Lun ,but our negotiation with SW and RAID not ready.
     * Result in Raid error like pcie link error.
     * SendVir_Ready and InBand_Loaded_Ready Flag make sure before negotiation done not send any cmd to Raid.
     */

     if ( (pAMEData->AME_NTHost_data.Raid_All_Topology >> Raid_ID) & 0x01 ) {
        
        if ( (pAMEData->AME_RAIDData[Raid_ID].SendVir_Ready == TRUE) &&
             (pAMEData->AME_RAIDData[Raid_ID].InBand_Loaded_Ready == TRUE) ) {
            return TRUE;
        }
     }
        
    return FALSE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Normal_handler_VIRTUAL_REPLY_QUEUE3 (pAME_REQUEST_CallBack pAMEREQUESTCallBack)
{
    pAME_Data pAMEData = pAMEREQUESTCallBack->pAMEData;
    AME_U32   Raid_ID  = (AME_U32)pAMEREQUESTCallBack->Raid_ID;
    AME_U32   BufferID = pAMEREQUESTCallBack->BufferID;
    pAMEVirtualReplyQueue3Reply_t pAMEVirtualReplyQueue3Reply  = (pAMEVirtualReplyQueue3Reply_t) pAMEREQUESTCallBack->pReplyFrame;
    
    if (pAMEVirtualReplyQueue3Reply->ReplyStatus != AME_REPLYSTATUS_SUCCESS) {
        AME_Msg_Err("(%d)AME_Normal_handler_VIRTUAL_REPLY_QUEUE3:  Error ReplyStatus = %x\n",pAMEData->AME_Serial_Number, pAMEVirtualReplyQueue3Reply->ReplyStatus);
        return FALSE;
    }
    
    /* Enable Raid SendVir_Ready Flag */
    //pAMEData->AME_RAIDData[Raid_ID].SendVir_Ready = TRUE;
    AME_InBand_Get_Page(pAMEData,Raid_ID,Lun_Mask_Page,AME_InBand_Loaded_Get_Lun_Mask_Page_callback);
    AME_InBand_Get_Page(pAMEData,Raid_ID,0,AME_InBand_Loaded_Get_Page_0_callback);
    
    // Workaround fixed PCIe link error (link phase lock) issue
    pAMEData->AME_NTHost_data.Workaround_87B0_Done = FALSE;
    AME_Msg("(%d)AME_Normal_handler_VIRTUAL_REPLY_QUEUE3:  Raid_ID %d complete.\n",pAMEData->AME_Serial_Number,Raid_ID);
    
    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Build_SendVirtual_Cmd (pAME_Data pAMEData,AME_U32 Raid_ID)
{
    pAME_VIRTUAL_REPLY_QUEUE3_REQUEST_NT    VirtualReplyQueueRequest;
    AME_U32  BufferID = 0;
    AME_U32  High_Address = 0;
    AME_U32  High_Offset = 0;

    if ( (BufferID=AME_Request_Buffer_Queue_Allocate(pAMEData)) == AME_U32_NONE ) {
        return FALSE;
    }
    
    /* Fixed read FreeQ index error issue. */
    if (AME_NT_Error_handle_FreeQ(pAMEData,Raid_ID) == FALSE) {
        AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
        return FALSE;
    }
    
    pAMEData->AMEBufferData[BufferID].Raid_ID = Raid_ID;  // Raid Error handle for NT
    
    /* Clear Raid SendVir_Ready Flag */
    #if defined (Enable_AME_RAID_R0)
        pAMEData->AME_RAIDData[Raid_ID].Read_Index = 0;  // For Software Raid
    #endif
    pAMEData->AME_RAIDData[Raid_ID].SendVir_Ready = FALSE;
    pAMEData->AME_RAIDData[Raid_ID].InBand_Loaded_Ready = FALSE;
    
    /* First Send high address to Raid */
    High_Address = pAMEData->Glo_High_Address + pAMEData->AME_NTHost_data.NT_linkside_BAR2_highaddress;
    High_Offset = pAMEData->AME_NTHost_data.PCISerialNumber;
    
    switch (pAMEData->AME_RAIDData[Raid_ID].NTRAIDType)
    {
        case NTRAIDType_341:
            // 341 Raid
            High_Offset <<= 2;
            High_Offset += 0x100;
            High_Offset += Raid_ID*ACS62_BAR_OFFSET + pAMEData->AME_NTHost_data.NTBar_Offset;
            
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,High_Offset,High_Address);
            
            /* Need delay to waiting Raid kernel get High address */
            AME_IO_milliseconds_Delay(200);
            AME_Msg("(%d)Send_VirtualReplyQueue(Raid ID %d) 341 Raid device \n",pAMEData->AME_Serial_Number,Raid_ID);
            break;
        
        case NTRAIDType_2208:
            // 2208 Raid
            High_Offset <<= 4;
            High_Offset += 0x2000;
            High_Offset += Raid_ID*ACS2208_BAR_OFFSET + pAMEData->AME_NTHost_data.NTBar_Offset;
            
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,High_Offset,High_Address);
            
            /* Notify High address register by Doorbell */
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Inbound_Doorbell_PORT + Raid_ID*ACS2208_BAR_OFFSET,0x40000000);
            AME_Msg("(%d)Register (Raid ID %d) NT High address :  0x%08X\n",pAMEData->AME_Serial_Number,Raid_ID,High_Address);
            
            /* Need delay to waiting Raid kernel get High address */
            AME_IO_milliseconds_Delay(200);
            AME_Msg("(%d)Send_VirtualReplyQueue(Raid ID %d) 2208 Raid device \n",pAMEData->AME_Serial_Number,Raid_ID);
            break;
            
        case NTRAIDType_3108:
            // 3108 Raid
            High_Offset <<= 4;
            High_Offset += 0x2000;
            High_Offset += Raid_ID*ACS3108_BAR_OFFSET + pAMEData->AME_NTHost_data.NTBar_Offset;
            
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,High_Offset,High_Address);
            
            /* Notify High address register by Doorbell */
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Inbound_Doorbell_PORT + Raid_ID*ACS3108_BAR_OFFSET,0x40000000);
            AME_Msg("(%d)Register (Raid ID %d) NT High address :  0x%08X\n",pAMEData->AME_Serial_Number,Raid_ID,High_Address);
            
            /* Need delay to waiting Raid kernel get High address */
            AME_IO_milliseconds_Delay(200);
            AME_Msg("(%d)Send_VirtualReplyQueue(Raid ID %d) 3108 Raid device \n",pAMEData->AME_Serial_Number,Raid_ID);
            break;
        
        default:
            AME_Msg_Err("(%d)Error:  Unknow index %d Raid type, can't send request cmd.\n",pAMEData->AME_Serial_Number,Raid_ID);
            return FALSE;
    }
    
    
    /* Build Send Virtual data */
    VirtualReplyQueueRequest = (pAME_VIRTUAL_REPLY_QUEUE3_REQUEST_NT)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
    VirtualReplyQueueRequest->Reserved[0] = 0;
    VirtualReplyQueueRequest->Reserved[1] = 0;
    VirtualReplyQueueRequest->Reserved[2] = 0;
    VirtualReplyQueueRequest->Function = AME_FUNCTION_VIRTUAL_REPLY_QUEUE3;
    VirtualReplyQueueRequest->MsgIdentifier = (BufferID << 2);
    
    VirtualReplyQueueRequest->PCISerialNumber = pAMEData->AME_NTHost_data.PCISerialNumber;
    VirtualReplyQueueRequest->ReplyAddressLow = (AME_U32)pAMEData->AMEReplyBufferData[BufferID].Reply_Buffer_Phy_addr +
                                                pAMEData->AME_NTHost_data.NT_linkside_BAR2_lowaddress;
    VirtualReplyQueueRequest->ReplyQueueLow = (AME_U32)pAMEData->AME_RAIDData[Raid_ID].NT_Raid_Reply_queue_Start_Phy_addr +
                                              pAMEData->AME_NTHost_data.NT_linkside_BAR2_lowaddress;
    VirtualReplyQueueRequest->RemoteGlobalHigh = High_Address;
    VirtualReplyQueueRequest->EventReplyStartAddrLow = (AME_U32)pAMEData->AME_RAIDData[Raid_ID].NT_Raid_Event_queue_Start_Phy_addr +
                                                       pAMEData->AME_NTHost_data.NT_linkside_BAR2_lowaddress;
  
    
    // Send VIRTUAL_REPLY_QUEUE3 Request
    if (FALSE == AME_NT_write_Host_Inbound_Queue_Port_0x40(pAMEData,Raid_ID,(AME_U64)pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr)) {
        AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
        AME_Msg_Err("(%d)Send_VirtualReplyQueue(Raid ID %d) fail ...\n",pAMEData->AME_Serial_Number,Raid_ID);
    }
    
    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Build_Das_SendVirtual_Cmd (pAME_Data pAMEData,pAME_Data pAMEData_path,AME_U32 Raid_ID)
{
    pAME_VIRTUAL_REPLY_QUEUE3_REQUEST_NT    VirtualReplyQueueRequest;
    AME_U32  BufferID = 0;
    AME_U32  High_Offset = 0;
    AME_U32  High_Address = 0;
    
    AME_U32 PCISerialNumber = 0;
    AME_U32 NT_REQUEST_Phy_addr = 0;

    /* Data Init */
    /* Clear Raid SendVir_Ready Flag */
    pAMEData->AME_NTHost_data.PCISerialNumber = 1;
    pAMEData->AME_RAIDData[Raid_ID].SendVir_Ready = FALSE;
    pAMEData->AME_RAIDData[Raid_ID].InBand_Loaded_Ready = FALSE;
    pAMEData->AME_RAIDData[Raid_ID].NTRAIDType = NTRAIDType_SOFR_Raid;
    pAMEData->AME_NTHost_data.Raid_All_Topology |= 1<<Raid_ID;
    
    /* First Send high address to Raid, for 2208 Raid type */
    High_Address = pAMEData->Glo_High_Address;
    High_Offset  = pAMEData->AME_NTHost_data.PCISerialNumber;
    High_Offset <<= 4;
    High_Offset += 0x2000;
    AME_Address_Write_32(pAMEData_path,pAMEData_path->Raid_Base_Address,High_Offset,High_Address);
    
    /* Notify High address register by Doorbell */
    AME_Address_Write_32(pAMEData_path,pAMEData_path->Raid_Base_Address,AME_Inbound_Doorbell_PORT,0x40000000);
    AME_IO_milliseconds_Delay(200);  /* Need delay to waiting Raid kernel get High address */
    //AME_Msg("(%d)Register (Raid ID %d) NT High address :  0x%08X\n",pAMEData->AME_Serial_Number,Raid_ID,High_Address);
    
    if ((BufferID=AME_Request_Buffer_Queue_Allocate(pAMEData)) == AME_U32_NONE) {
        return FALSE;
    }
    
    pAMEData->AMEBufferData[BufferID].Raid_ID = Raid_ID;  // Raid Error handle for NT
    
    /* Build Send Virtual data */
    VirtualReplyQueueRequest = (pAME_VIRTUAL_REPLY_QUEUE3_REQUEST_NT)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
    VirtualReplyQueueRequest->Reserved[0] = 0;
    VirtualReplyQueueRequest->Reserved[1] = 0;
    VirtualReplyQueueRequest->Reserved[2] = 0;
    VirtualReplyQueueRequest->Function = AME_FUNCTION_VIRTUAL_REPLY_QUEUE3;
    VirtualReplyQueueRequest->MsgIdentifier = (BufferID << 2);
    
    VirtualReplyQueueRequest->PCISerialNumber = pAMEData->AME_NTHost_data.PCISerialNumber;
    VirtualReplyQueueRequest->ReplyAddressLow = (AME_U32)pAMEData->AMEReplyBufferData[BufferID].Reply_Buffer_Phy_addr;
    VirtualReplyQueueRequest->ReplyQueueLow = (AME_U32)pAMEData->AME_RAIDData[Raid_ID].NT_Raid_Reply_queue_Start_Phy_addr;
    VirtualReplyQueueRequest->RemoteGlobalHigh = High_Address;
    VirtualReplyQueueRequest->EventReplyStartAddrLow = (AME_U32)pAMEData->AME_RAIDData[Raid_ID].NT_Raid_Event_queue_Start_Phy_addr;
    
    
    PCISerialNumber = pAMEData->AME_NTHost_data.PCISerialNumber;
    PCISerialNumber = ((PCISerialNumber - 1) & 0x1F) << 3;
    PCISerialNumber |= 0x6;
    NT_REQUEST_Phy_addr = (AME_U32)pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr | PCISerialNumber;
    
    if ( FALSE == AME_write_Host_Inbound_Queue_Port_0x40(pAMEData_path,NT_REQUEST_Phy_addr) ) {
        AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
        return FALSE;
    }
    
    return TRUE;
}


void
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_NT_Get_SW_INFO(pAME_Data pAMEData)
{
    AME_U32  Raid_ID;
    AME_U32  Reg_0x00 = 0;
    AME_U32  Bar2ATR = 0;
    AME_U32  Bar2Setup = 0;
    AME_U32  NT_SW_Ready_State = 0;
    AME_U32  PCISerialNumber = 0;
    AME_U32  NT_linkside_BAR2_lowaddress = 0;
    AME_U32  NT_linkside_BAR2_highaddress = 0;
    AME_U32  NEW_Raid_All_Topology = 0;
    AME_U32  NEW_Raid_2208_3108_Topology = 0;
    AME_U32  NEW_Raid_Change_Topology = 0;
    AME_U32  NT_Raid_Type = 0;
    
    NT_SW_Ready_State = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.SCRATCH_REG_4);
    PCISerialNumber = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.SCRATCH_REG_0);
    NT_linkside_BAR2_lowaddress = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.SCRATCH_REG_1);
    NT_linkside_BAR2_highaddress = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.SCRATCH_REG_3);
    NEW_Raid_All_Topology = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.SCRATCH_REG_2);
    NEW_Raid_2208_3108_Topology = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.SCRATCH_REG_5);
    NEW_Raid_Change_Topology = NEW_Raid_All_Topology ^ pAMEData->AME_NTHost_data.Raid_All_Topology;
    
    
        
    // Setting NT Raid type
    for (Raid_ID=0; Raid_ID<MAX_RAID_System_ID; Raid_ID++) {
        
        if ((NEW_Raid_Change_Topology >> Raid_ID) & 0x01) {
            
            if ((NEW_Raid_2208_3108_Topology >> Raid_ID) & 0x01) {
            
                /* Fixed read FreeQ index error issue. */
                AME_NT_Error_handle_FreeQ(pAMEData,Raid_ID);
                
                NT_Raid_Type = AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_RaidType_REG_OFFSET+Raid_ID*ACS62_BAR_OFFSET);
                AME_Msg("Raid_ID(%d) : NT_Raid_Type = %x\n",Raid_ID, NT_Raid_Type);
                if( (0x626214d6 == NT_Raid_Type) || (0x624014d6 == NT_Raid_Type) ) {
                    pAMEData->AME_RAIDData[Raid_ID].NTRAIDType = NTRAIDType_3108;                
                } else {
                    pAMEData->AME_RAIDData[Raid_ID].NTRAIDType = NTRAIDType_2208;
                }
            } else {
                pAMEData->AME_RAIDData[Raid_ID].NTRAIDType = NTRAIDType_341;
            }
            
        }
        
    }
    
    
    if (PCISerialNumber != pAMEData->AME_NTHost_data.PCISerialNumber)
    {
        pAMEData->AME_NTHost_data.NT_SW_Ready_State = NT_SW_Ready_State;
        pAMEData->AME_NTHost_data.PCISerialNumber = PCISerialNumber;
        pAMEData->AME_NTHost_data.NT_linkside_BAR2_lowaddress = NT_linkside_BAR2_lowaddress;
        pAMEData->AME_NTHost_data.NT_linkside_BAR2_highaddress = NT_linkside_BAR2_highaddress;
        pAMEData->AME_NTHost_data.Raid_All_Topology = NEW_Raid_All_Topology;
        pAMEData->AME_NTHost_data.Raid_2208_3108_Topology = NEW_Raid_2208_3108_Topology;
        
        /* 
         * Workaround: IF NT Link side Bar2 transfer Base address not default address (0x80000000)
         * need change base address , to claim 32MB Virtual Site BAR2 (Fixed bug in SW12)
         */
        Bar2Setup = 0xFE000000; // we claim 32MB Virtual Site BAR2
        Bar2ATR = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET);
        pAMEData->AME_NTHost_data.NTBar_Offset = (~Bar2Setup) & Bar2ATR;
    
        /* Set NT_REQUEST_MSG_PORT */
        for (Raid_ID=0; Raid_ID<MAX_RAID_System_ID; Raid_ID++) {
            pAMEData->AME_RAIDData[Raid_ID].NT_REQUEST_MSG_PORT = AME_REQUEST_MSG_PORT + 
                                                                  Raid_ID*ACS2208_BAR_OFFSET +
                                                                  pAMEData->AME_NTHost_data.NTBar_Offset;
        }             
        
    } else {

        // Raid Hotplug in/out
        pAMEData->AME_NTHost_data.Raid_All_Topology = NEW_Raid_All_Topology;
        pAMEData->AME_NTHost_data.Raid_2208_3108_Topology = NEW_Raid_2208_3108_Topology;
        pAMEData->AME_NTHost_data.Raid_Change_Topology |= NEW_Raid_Change_Topology;
        
        for (Raid_ID=0; Raid_ID<MAX_RAID_System_ID; Raid_ID++)
        {
            if ( (pAMEData->AME_NTHost_data.Raid_Change_Topology >> Raid_ID) & 0x01 ) {
                
                /* Clear Raid SendVir_Ready Flag */
                pAMEData->AME_RAIDData[Raid_ID].SendVir_Ready = FALSE;
                pAMEData->AME_RAIDData[Raid_ID].InBand_Loaded_Ready = FALSE;
                
            }
        }
    }
    
    AME_Msg("(%d)Get SW INFO:  %s\n",pAMEData->AME_Serial_Number,(pAMEData->AME_NTHost_data.NT_SW_Ready_State == 0)?("SW not ready to negotiation..."):("SW ready to negotiation..."));
    AME_Msg("(%d)Get SW INFO:  PCISerialNumber = %02d\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.PCISerialNumber);
    AME_Msg("(%d)Get SW INFO:  NTBar_Offset    = 0x%08x\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.NTBar_Offset);
    AME_Msg("(%d)Get SW INFO:  NT_linkside_BAR2_lowaddress  = 0x%08x\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.NT_linkside_BAR2_lowaddress);
    AME_Msg("(%d)Get SW INFO:  NT_linkside_BAR2_highaddress = 0x%08x\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.NT_linkside_BAR2_highaddress);
    AME_Msg("(%d)Get SW INFO:  Raid_All_Topology    = 0x%08x\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.Raid_All_Topology);
    AME_Msg("(%d)Get SW INFO:  Raid_2208_3108_Topology   = 0x%08x\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.Raid_2208_3108_Topology);
    AME_Msg("(%d)Get SW INFO:  Raid_Change_Topology = 0x%08x\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.Raid_Change_Topology);
    
    return;
}


AME_U32
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_NT_Cleanup_Error_Raid_Request(pAME_Data pAMEData,AME_U32 Raid_ID)
{
    AME_U32 BufferID;
    AME_REQUEST_CallBack    AMEREQUESTCallBack;
    
    AME_Queue_Clean_Error_Raid_CMD_Count(pAMEData,Raid_ID);
    
    for (BufferID=0 ; BufferID<pAMEData->Total_Support_command; BufferID++) {
        
        if ( (pAMEData->AMEBufferData[BufferID].ReqInUse == TRUE) &&
             (pAMEData->AMEBufferData[BufferID].Raid_ID == Raid_ID) ) {
            
            if (pAMEData->AMEBufferData[BufferID].ReqTimeouted == TRUE) {
                AME_Msg_Err("*%d)BufferID[%d] request marked Timeout\n",pAMEData->AME_Serial_Number,BufferID);
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                continue;
            }
            
            AMEREQUESTCallBack.pAMEData=pAMEData;
            AMEREQUESTCallBack.pRequestExtension = pAMEData->AMEBufferData[BufferID].pRequestExtension;
            AMEREQUESTCallBack.pMPIORequestExtension = pAMEData->AMEBufferData[BufferID].pMPIORequestExtension;
            AMEREQUESTCallBack.pAME_Raid_QueueBuffer = pAMEData->AMEBufferData[BufferID].pAME_Raid_QueueBuffer;  // for RAID 0/1
            AMEREQUESTCallBack.pRequest = 0x00;
            AMEREQUESTCallBack.pReplyFrame = 0x00;
            AMEREQUESTCallBack.pSCSISenseBuffer = 0x00;
            AMEREQUESTCallBack.Raid_ID = Raid_ID;
            AMEREQUESTCallBack.TransferedLength = 0x00;
            
            AME_Queue_Abort_Command(pAMEData,pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr);
            
            MPIO_Host_Error_handler_SCSI_REQUEST(&(AMEREQUESTCallBack));
            AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
            
            AME_Msg_Err("AME_NT_Cleanup_Error_Raid_Request (BufferID %d)\n",BufferID);
        }
    }
    
    return TRUE;
}


/*******************************************************************
* PLX DoorBell bit 12 13 14 15 used for MPIO
* bit 12, MPIO Slave notify Master cmd complete
* bit 13, MPIO Master notify Slave to get cmd and send it
* bit 14, MPIO notify Master Raid path remove
* bit 15, Reserve
********************************************************************/
AME_U32 AME_NT_Notify_Master_toget_Reply_Fifo(pAME_Data pAMEData)
{
    if (TRUE == AME_Check_is_NT_Mode(pAMEData)) {
        AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_REG,0x1000);
    }
#if defined (Enable_AME_RAID_R0)
    else {
        //AME_Msg_Err("(%d)AME_NT_Notify_Master_toget_Reply_Fifo ...\n",pAMEData->AME_Serial_Number);
        AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,0x9C,0x04);
    }
#endif
    
    return TRUE;
}

AME_U32 AME_NT_Notify_Slave_toget_Req_Fifo(pAME_Data pAMEData)
{
    if (TRUE == AME_Check_is_NT_Mode(pAMEData)) {
        AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_REG,0x2000);
    }
#if defined (Enable_AME_RAID_R0)
    else {
        //AME_Msg_Err("(%d)AME_NT_Notify_Slave_toget_Req_Fifo ...\n",pAMEData->AME_Serial_Number);
        AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,0x9C,0x02);
    }
#endif
    
    return TRUE;
}

AME_U32 AME_NT_Notify_Master_Raid_path_remove(pAME_Data pAMEData)
{
    if (TRUE == AME_Check_is_NT_Mode(pAMEData)) {
        AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_REG,0x4000);
    }
    
    return TRUE;
}


/*******************************************************************
* Function : AME_ISR_NT_Function_Handler
* bit  0, PLX_Interrupt = 0x01, NT Normal SCSI, InBand or Event cmd reply
* bit  1, PLX_Interrupt = 0x02, NT Init or Raid hotplug in/out issue
* bit  2, PLX_Interrupt = 0x04, Raid link Error handler Start (SW not support)
* bit  3, PLX_Interrupt = 0x08, Raid link Error handler Complete
* bit 12, PLX_Interrupt = 0x1000, MPIO Slave notify Master cmd complete
* bit 13, PLX_Interrupt = 0x2000, MPIO Master notify Slave to get cmd and send it
* bit 14, PLX_Interrupt = 0x4000, MPIO notify Master Raid path remove
********************************************************************/
AME_U32
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_ISR_NT_Function_Handler(pAME_Data pAMEData,AME_U32 *pComplete_Cmd_index)
{
    AME_U32  PLX_Interrupt = 0;
    
    if (TRUE == AME_Check_is_NT_Mode(pAMEData))
    {
        //Get NT DooorBell interrupt Message
        PLX_Interrupt = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_REG);
        
        /* Not NT ISR */
        if (PLX_Interrupt == 0x00) {
            return FALSE;
        }
        
        /* Check PLX_Interrupt is not NT Door Bell Valid bit
         * When the Thunderbolt unplug of Sonnet Echo express, will get this unexpected interrupt.
         */
        if (PLX_Interrupt & (~NT_Valid_DoorBell_bit)) {
            AME_Msg_Err("(%d)Receive wrong Interrupt reason 0x%x\n",pAMEData->AME_Serial_Number,PLX_Interrupt);
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG,PLX_Interrupt);
            return FALSE;
        }
        
        if (PLX_Interrupt & 0x01) {
            PLX_Interrupt -= 0x01;
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG,0x01);
        }
        
        if (PLX_Interrupt & 0x02) {
            PLX_Interrupt -= 0x02;
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG,0x02);
            AME_NT_Get_SW_INFO(pAMEData);
        }
        
        if (PLX_Interrupt & 0x04) {
            PLX_Interrupt -= 0x04;
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG,0x04);
        }
        
        if (PLX_Interrupt & 0x08) {
            
            AME_U32 Raid_ID,Error_Raid_bitmap;
            
            PLX_Interrupt -= 0x08;
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG,0x08);
            
            Error_Raid_bitmap = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.SCRATCH_REG_6);
            
            for (Raid_ID=0; Raid_ID<MAX_RAID_System_ID; Raid_ID++)
            {
                if ( (Error_Raid_bitmap >> Raid_ID) & 0x01 ) {
                    
                    AME_Msg("(%d)Raid Error Handle:  NT Raid (%d) Error Handle Start ...\n",pAMEData->AME_Serial_Number,Raid_ID);
                    pAMEData->AME_RAIDData[Raid_ID].Raid_Error_Handle_State = TRUE;
                    
                    /* Clear Error Raid request */
                    AME_Msg_Err("(%d)AME Error:  AME_NT_Cleanup_Error_Raid_Request (Raid %d)...\n",pAMEData->AME_Serial_Number,Raid_ID);
                    AME_NT_Cleanup_Error_Raid_Request(pAMEData,Raid_ID);
                    
                    /* Clear Reply Queue entry state */
                    AME_NT_Cleanup_Reply_Queue_EntryState(pAMEData,Raid_ID);
                }
            }
            
            /* Notify AME_NT_Hotplug_Handler to Send_VirtualReplyQueue to Error Raid */
            pAMEData->AME_NTHost_data.Error_Raid_bitmap |= Error_Raid_bitmap;
            pAMEData->AME_NTHost_data.Raid_Change_Topology |= pAMEData->AME_NTHost_data.Error_Raid_bitmap;
            
        }
        
        if (PLX_Interrupt & 0x4000) {
            PLX_Interrupt -= 0x4000;
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG,0x4000);

            MPIO_Cmd_Mark_Path_remove((pMPIO_DATA)pAMEData->pMPIOData);
            AME_CleanupPathRemoveRequest(pAMEData);
        }
        
        *pComplete_Cmd_index += 1;
    }
    
    return TRUE;
}


AME_U32
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_ISR_Das_Raid_Function_Handler(pAME_Data pAMEData)
{
    AME_U32 Raid_Interrupt = 0;
    
    /* In 2208, AME_HOST_INT_STATUS_REGISTER bit 3 is doorbell ISR, bit 4 is FIFO ISR */
    Raid_Interrupt = AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_STATUS_REGISTER);
    
    if (Raid_Interrupt == 0x00) {
        return FALSE;
    }
    
    if (Raid_Interrupt & 0x04) {
        // Master get Reply from Slave, this doorbell value is 0x04.
        //Raid_Interrupt = AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,0x9C);
        
        AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,0xA0,0xFFFFFFFF);
    }
    
    return TRUE;
}


/*******************************************************************
* I2O DoorBell bit 1 2 3 4 used for new error handler
* bit 0, Reserved (Utility_A)
* bit 1, RAID restart soon
* bit 2, Clear previous command (host driver can return previous uncomplete command soon)
* bit 3, Pause command ( host stop issueing command to RAID, Only for RAID test)
* bit 4, Resume command , and RAID check host alive per second ( host can issue command to RAID)
* bit 5-31 Reserved 
********************************************************************/
AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_ISR_Das_outbound_doorbell_handler(pAME_Data pAMEData)
{
    AME_U32   InterruptReason;
    
    if ( (pAMEData->Device_ID == AME_2208_DID1_DAS) ||
         (pAMEData->Device_ID == AME_2208_DID2_DAS) ||
         (pAMEData->Device_ID == AME_2208_DID3_DAS) ||
         (pAMEData->Device_ID == AME_2208_DID4_DAS) ||
         (pAMEData->Device_ID == AME_3108_DID0_DAS) ||
         (pAMEData->Device_ID == AME_3108_DID1_DAS) ) {
         
          InterruptReason = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_PORT); 
          if (InterruptReason) {                       
              if ((InterruptReason >> 1) & 0x01) {
                  /* RAID restart soon */
                  AME_Msg("(%d) Receive Outbound_Doorbell : 0x%x\n",pAMEData->AME_Serial_Number,InterruptReason);
                  AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_Clear_PORT,0x1 << 1);
                  AME_Msg_Err("(%d) RAID restart soon \n",pAMEData->AME_Serial_Number);
              } 
              else if ((InterruptReason >> 2) & 0x01) {
                  /* Clear previous command (host driver can return previous uncomplete command soon) */
                  AME_Msg("(%d) Receive Outbound_Doorbell : 0x%x\n",pAMEData->AME_Serial_Number,InterruptReason);
                  AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_Clear_PORT,0x01 << 2);
                  AME_Msg_Err("(%d) Clear previous command \n",pAMEData->AME_Serial_Number);
                  
                  AME_NT_Cleanup_Error_Raid_Request(pAMEData,0);
              }   
              else if ((InterruptReason >> 3) & 0x01) {
                  /*  Pause command ( host stop issueing command to RAID) */
                  AME_Msg("(%d) Receive Outbound_Doorbell : 0x%x\n",pAMEData->AME_Serial_Number,InterruptReason);
                  AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_Clear_PORT,0x01 << 3);
                  AME_Msg_Err("(%d) Pause command \n",pAMEData->AME_Serial_Number);
                  
                  pAMEData->AME_RAIDData[0].Bar_address_lost = TRUE;
              }
              else if ((InterruptReason >> 4) & 0x01) {
                  /* Resume command , and RAID check host alive per second ( host can issue command to RAID) */
                  AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_Clear_PORT,0x01 << 4);
                  //AME_Msg_Err("(%d) Resume command \n",pAMEData->AME_Serial_Number);
                  
                  pAMEData->AME_RAIDData[0].Bar_address_lost = FALSE;
              }
              else {
                  if (InterruptReason == 0x01)
                  {
                      return TRUE;
                  }
                  AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_Clear_PORT,InterruptReason);
                  AME_Msg_Err("(%d) Unknown Outbound_Doorbell InterruptReason : 0x%x \n",pAMEData->AME_Serial_Number,InterruptReason);
              }
          }
      }
    
    return TRUE;
}


AME_U32 AME_Get_ReplyMsg_DAS(pAME_Data pAMEData,AME_U32 *pRaid_ID,AME_U32 *pReplyMsg)
{
    *pReplyMsg = AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_REPLY_MSG_PORT);
        
    if (*pReplyMsg != MUEmpty) {
        *pRaid_ID = 0;
        return TRUE;
    }
    
    return FALSE;
}


AME_U32 AME_Get_ReplyMsg_NT(pAME_Data pAMEData,AME_U32 *pRaid_ID,AME_U32 *pReplyMsg)
{
    AME_U32  Raid_Index;
    AME_U32  Read_Index = pAMEData->AME_NTHost_data.Read_Index;
       
    // Scan all Reply status
    for (  ; Read_Index < MAX_NT_Reply_Elements; Read_Index++)
    {
        for (Raid_Index=0 ; Raid_Index < MAX_RAID_System_ID; Raid_Index++)
        {
            if ((pAMEData->AME_NTHost_data.Raid_All_Topology >> Raid_Index) & 0x01)
            {
                // Get ReplyMsg
                *pReplyMsg = *((AME_U32 *)pAMEData->AME_RAIDData[Raid_Index].NT_Raid_Reply_queue_Start_Vir_addr + Read_Index);
                
                if (*pReplyMsg != MUEmpty) {
                    
                    /* Clear Reply Queue Entry state */
                    *((AME_U32 *)pAMEData->AME_RAIDData[Raid_Index].NT_Raid_Reply_queue_Start_Vir_addr + Read_Index) = MUEmpty;
                    
                    *pRaid_ID = Raid_Index;
                    
                    pAMEData->AME_NTHost_data.Read_Index = Read_Index;
                    //AME_Msg("AME Debug: Raid_Index = %02d  Read_Index = %03d ReplyMsg = 0x%08x, Virtual Reply addr = 0x%p\n",*pRaid_ID,Read_Index,*pReplyMsg,((AME_U32 *)pAMEData->AME_RAIDData[Raid_Index].NT_Raid_Reply_queue_Start_Vir_addr + Read_Index));
                    
                    return TRUE;
                }
                
            }
        }
    }
    
    pAMEData->AME_NTHost_data.Read_Index = 0;
    
    return FALSE;
}


AME_U32 AME_Get_ReplyMsg_Soft_Raid(pAME_Data pAMEData,AME_U32 *pRaid_ID,AME_U32 *pReplyMsg)
{
    AME_U32  Raid_Index;
    AME_U32  Read_Index = pAMEData->AME_NTHost_data.Read_Index;
    
    // Scan all Reply status
    if (pAMEData->AME_NTHost_data.Raid_All_Topology == 0x00) {
        return FALSE;
    }
    
    for (Raid_Index=0 ; Raid_Index < (Soft_Raid_Target-1); Raid_Index++) {
        
        Read_Index = pAMEData->AME_RAIDData[Raid_Index].Read_Index;
        
        // Get ReplyMsg
        *pReplyMsg = *((AME_U32 *)pAMEData->AME_RAIDData[Raid_Index].NT_Raid_Reply_queue_Start_Vir_addr + Read_Index);
        
        if (*pReplyMsg != MUEmpty) {
            
            /* Clear Reply Queue Entry state */
            *((AME_U32 *)pAMEData->AME_RAIDData[Raid_Index].NT_Raid_Reply_queue_Start_Vir_addr + Read_Index) = MUEmpty;
            
            *pRaid_ID = Raid_Index;
            
            if (++pAMEData->AME_RAIDData[Raid_Index].Read_Index == MAX_NT_Reply_Elements) {
                pAMEData->AME_RAIDData[Raid_Index].Read_Index = 0;
            }
            
            return TRUE;
        }
        
    }
    
    return FALSE;
}


AME_U32 AME_Get_ReplyMsg(pAME_Data pAMEData,AME_U32 *pRaid_ID,AME_U32 *pReplyMsg)
{
    if (TRUE == AME_Check_is_NT_Mode(pAMEData))
    {
        // NT environment
        if (TRUE == AME_Get_ReplyMsg_NT(pAMEData,pRaid_ID,pReplyMsg)) {
            return TRUE;
        }
    
    } else {
        
        // Das software Raid get reply from slave
        #if defined (Enable_AME_RAID_R0)
            if (TRUE == AME_Get_ReplyMsg_Soft_Raid(pAMEData,pRaid_ID,pReplyMsg)) {
                return TRUE;
            }
        #endif
        
        // Das environment
        if (TRUE == AME_Get_ReplyMsg_DAS(pAMEData,pRaid_ID,pReplyMsg)) {
            return TRUE;
        }
        
    }

    return FALSE;
    
}


AME_U32 AME_NT_Check_Is_Event_Reply(pAME_Data pAMEData,AME_U32 AME_Buffer_PhyAddress_Low,AME_U64 *pNT_Event_Reply_address)
{
    AME_U32  i,AME_Event_Start_Address_Low;
    AME_U64  AME_Event_Address;
    AME_U64  AME_Event_Phy_to_Vir_Offset;
    pAMEDefaultReply   ReplyFrame;
    
    if (FALSE == AME_Check_is_NT_Mode(pAMEData)) {
        return FALSE;
    }
    
    // Normal OS , Vir address always > Phy address
    for (i=0; i<MAX_RAID_System_ID; i++) {
        
        AME_Event_Start_Address_Low = (AME_U32)pAMEData->AME_RAIDData[i].NT_Raid_Event_queue_Start_Phy_addr;
        
        if ((AME_Buffer_PhyAddress_Low >= AME_Event_Start_Address_Low) &&
            (AME_Buffer_PhyAddress_Low < (AME_Event_Start_Address_Low + pAMEData->NT_Reply_queue_size_PerRaid)) ){
            AME_Event_Phy_to_Vir_Offset = pAMEData->AME_RAIDData[i].NT_Raid_Event_queue_Start_Vir_addr -
                                          pAMEData->AME_RAIDData[i].NT_Raid_Event_queue_Start_Phy_addr;
            break;
        }
        
        // Find no match Even memory range.
        if (i == (MAX_RAID_System_ID-1)) {
            return FALSE;
        }
        
    }
    
    AME_Event_Address = ((AME_U64)pAMEData->Glo_High_Address << 32) +
                        (AME_U64)AME_Buffer_PhyAddress_Low +
                        AME_Event_Phy_to_Vir_Offset;

    ReplyFrame = (pAMEDefaultReply)AME_Event_Address;
    
    if (ReplyFrame->Function == AME_FUNCTION_EVENT_EXT_SWITCH) {
        *pNT_Event_Reply_address = AME_Event_Address;
        return TRUE;
    }
    
    return FALSE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_ISR(pAME_Data pAMEData)
{
    AME_U32                 Raid_ID = 0,BufferID = 0,ReplyID = 0,ReplyMsg = 0,Complete_Cmd_index = 0;
    pAMEDefaultReply        ReplyFrame;
    AME_REQUEST_CallBack    AMEREQUESTCallBack;
    
    #if defined (Enable_AME_RAID_R0)
        AME_U32 Master_Notify = FALSE;
        if (FALSE == AME_ISR_Das_Raid_Function_Handler(pAMEData)) {
            return FALSE;
        }
    #endif
    
    AME_ISR_Das_outbound_doorbell_handler(pAMEData);
    
    if (AME_Raid_2208_3108_Bar_lost_Check(pAMEData) == TRUE) {
        AME_Msg_Err("(%d) In ISR, AME_Raid_2208_3108_Bar_lost_Check return TURE  ...\n",pAMEData->AME_Serial_Number);
        return TRUE;
    }
    
    if (FALSE == AME_ISR_NT_Function_Handler(pAMEData,&Complete_Cmd_index)) {
        return FALSE;
    }  
    
    while (1)
    {
        #if defined (Enable_AME_RAID_R0)
            if ((Master_Notify != TRUE) && (Complete_Cmd_index > 2)) {
                return TRUE;
            }
        #endif
        
        // Get reply message
        if (FALSE == AME_Get_ReplyMsg(pAMEData,&Raid_ID,&ReplyMsg)) {
            
            #if defined (Enable_AME_RAID_R0)
                if (Master_Notify == TRUE) {
                    //AME_Msg("(%d) Receive Master ISR (0x%x) notify to Master ...\n",pAMEData->AME_Serial_Number,ReplyMsg);
                    MPIO_Slave_Notify_Master_toget_Reply((pMPIO_DATA)pAMEData->pMPIOData,0);
                }
            #endif
            
            if (Complete_Cmd_index > 0) {
    
                // Fixed NT lose ISR issue.
                if (TRUE == AME_Check_is_NT_Mode(pAMEData)) {
                    
                    AME_U32  PLX_Interrupt = 0;
                    
                    //Get NT DooorBell interrupt Message
                    PLX_Interrupt = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_REG);
                    if ((PLX_Interrupt == 0x00)&&(Complete_Cmd_index>1)) {
                        pAMEData->Next_NT_ISR_Check = TRUE;
                    }
                }
        
                return TRUE;
            } else {
                return FALSE;
            }
        }
        
        #if defined (Enable_AME_RAID_R0)
            if ((ReplyMsg & 0xFFFFFF00) == 0xFFFFFF00) {
                Master_Notify = TRUE;
                Complete_Cmd_index++;
                continue;
            }
        #endif
        
        // Hanlde Fast reply and Normal reply
        if ( !(ReplyMsg & AME_NORMAL_REPLY_N_BIT) ) {
            
            // Fast Reply
            BufferID = ReplyMsg>>2;
            
            if ((BufferID < 1) ||
                (BufferID > (pAMEData->Total_Support_command-1)) ||
                (FALSE == pAMEData->AMEBufferData[BufferID].ReqInUse)) {
                AME_Msg_Err("(%d)Fast BufferID (%d) ERR, ReplyMsg(0x%x)...\n",pAMEData->AME_Serial_Number,BufferID,ReplyMsg);
                continue;
            }

            if (pAMEData->AMEBufferData[BufferID].ReqTimeouted == TRUE) {
                AME_Msg_Err("(%d)Fast Reply: BufferID[%d] request marked Timeout\n",pAMEData->AME_Serial_Number,BufferID);
            }
            else {
                
                AMEREQUESTCallBack.pAMEData = pAMEData;
                AMEREQUESTCallBack.Raid_ID = Raid_ID;
                AMEREQUESTCallBack.BufferID = BufferID;
                AMEREQUESTCallBack.pRequestExtension = pAMEData->AMEBufferData[BufferID].pRequestExtension;
                AMEREQUESTCallBack.pMPIORequestExtension = pAMEData->AMEBufferData[BufferID].pMPIORequestExtension;
                AMEREQUESTCallBack.pAME_Raid_QueueBuffer = pAMEData->AMEBufferData[BufferID].pAME_Raid_QueueBuffer;  // for RAID 0/1, but NT unused
                AMEREQUESTCallBack.pRequest = (PVOID)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
                AMEREQUESTCallBack.pReplyFrame = NULL;
                AMEREQUESTCallBack.pSCSISenseBuffer = NULL;
                AMEREQUESTCallBack.TransferedLength = ((pAMESCSIRequest_NT)AMEREQUESTCallBack.pRequest)->DataLength;
                
                // Wait call Back Functtion ready...
                //AMEREQUESTCallBack.pRequestExtension = (PVOID)pAMESCSIREQUEST->SCSIRequest;
                
                MPIO_Host_Fast_handler_SCSI_REQUEST(&(AMEREQUESTCallBack));
            }
            
            AME_Queue_Command_SCSI_complete(pAMEData,Raid_ID);
            
            AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
            
        } else {
            
            // Normal Reply
            if (FALSE == AME_Get_Reply_buffer_ID(pAMEData,ReplyMsg<<1,&ReplyID,&BufferID,&ReplyFrame)) {
                continue;
            }
            
            if (pAMEData->AMEBufferData[BufferID].ReqTimeouted == TRUE) {
                AME_Msg_Err("(%d)Normal Reply: BufferID[%d] request marked Timeout\n",pAMEData->AME_Serial_Number,BufferID);
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                AME_Send_Free_ReplyMsg(pAMEData,ReplyID);
                continue;
            }

            AMEREQUESTCallBack.pAMEData = pAMEData;
            AMEREQUESTCallBack.Raid_ID  = Raid_ID;
            AMEREQUESTCallBack.BufferID = BufferID;
            AMEREQUESTCallBack.pRequestExtension = pAMEData->AMEBufferData[BufferID].pRequestExtension;
            AMEREQUESTCallBack.pMPIORequestExtension = pAMEData->AMEBufferData[BufferID].pMPIORequestExtension;
            AMEREQUESTCallBack.pAME_Raid_QueueBuffer = pAMEData->AMEBufferData[BufferID].pAME_Raid_QueueBuffer;  // for RAID 0/1, but NT unused
            AMEREQUESTCallBack.pRequest = (PVOID)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
            AMEREQUESTCallBack.pReplyFrame = ReplyFrame;
            AMEREQUESTCallBack.pSCSISenseBuffer = (PVOID)pAMEData->AMEBufferData[BufferID].Sense_Buffer_Vir_addr;
            AMEREQUESTCallBack.TransferedLength = ((pAMESCSIRequest_NT)AMEREQUESTCallBack.pRequest)->DataLength;
            
            // Wait call Back Functtion ready...
            //AMEREQUESTCallBack.pRequestExtension = (PVOID)pAMESCSIREQUEST->SCSIRequest;
            //pAMESCSIREQUEST->callback( (pAME_REQUEST_CallBack)&AMEREQUESTCallBack );
            
            switch (ReplyFrame->Function)
            {
                // Das Function
                case AME_FUNCTION_SCSI_REQUEST:
                    MPIO_Host_Normal_handler_SCSI_REQUEST(&(AMEREQUESTCallBack));
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    AME_Send_Free_ReplyMsg(pAMEData,ReplyID);
                    break;
                
                case AME_FUNCTION_SCSI_TASK_MGMT:
                    //Normal_handler_SCSI_TASK_MGMT(&(AMEREQUESTCallBack));
                    //AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    //AME_Send_Free_ReplyMsg(pAMEData,ReplyID);
                    break;
                
                case AME_FUNCTION_IN_BAND:
                    AME_Normal_handler_IN_BAND(&(AMEREQUESTCallBack));
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    AME_Send_Free_ReplyMsg(pAMEData,ReplyID);
                    break;
                
                case AME_FUNCTION_EVENT_SWITCH:
                    if ( FALSE == AME_Normal_handler_EVENT_SWITCH(&(AMEREQUESTCallBack)) ) {
                        /* Turn on/off Switch event, need to release request buffer */
                        //AME_Msg("(%d)AME_ISR:  Turn on/off SW event.\n",pAMEData->AME_Serial_Number);
                        AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    }
                    AME_Send_Free_ReplyMsg(pAMEData,ReplyID);
                    break;
                    
                // NT Function
                case AME_FUNCTION_VIRTUAL_REPLY_QUEUE3:
                    AME_Normal_handler_VIRTUAL_REPLY_QUEUE3(&(AMEREQUESTCallBack));
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    break;
                
                case AME_FUNCTION_SCSI_EXT2_REQUEST:
                    MPIO_Host_Normal_handler_SCSI_REQUEST(&(AMEREQUESTCallBack));
                    AME_Queue_Command_SCSI_complete(pAMEData,Raid_ID);
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    break;
                    
                case AME_FUNCTION_IN_BAND_EXT:
                    AME_Normal_handler_IN_BAND(&(AMEREQUESTCallBack));
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    break;
                
                case AME_FUNCTION_EVENT_EXT_SWITCH:
                    AME_Normal_handler_EVENT_EXT_SWITCH(&(AMEREQUESTCallBack));
                    break;
                    
                default:
                    AME_Msg_Err("(%d)Error:  Unknow ReplyFrame->Function = %x\n",pAMEData->AME_Serial_Number,ReplyFrame->Function);
                    break;
            }
            
        }
        
        Complete_Cmd_index++;
    }
    
    AME_Msg_Err("(%d)AME_ISR:  Why go here ???\n",pAMEData->AME_Serial_Number);
    return TRUE;
    
}

AME_U32 AME_Check_is_NT_Mode(pAME_Data pAMEData)
{
    
    if ( (pAMEData->Device_ID == AME_87B0_DID_NT) ||
         (pAMEData->Device_ID == AME_8608_DID_NT) ||
         (pAMEData->Device_ID == AME_8508_DID_NT) ) {
        
        return TRUE;
    }
    
    return FALSE;
}

AME_U32 AME_Check_is_DAS_Mode(pAME_Data pAMEData)
{
    switch (pAMEData->Device_ID)
    {
        case AME_6101_DID_DAS:
        case AME_6201_DID_DAS:
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            return TRUE;
            
        default:
            return FALSE;
    }
    
}


AME_U32 AME_Device_Support_Check(pAME_Module_Support_Check pAMEModule_Support_Check)
{
    AME_U32 Vendor_ID,Device_ID,Sub_Vendor_ID,Sub_Device_ID,Device_ClassCode,ret = FALSE;
    Vendor_ID = pAMEModule_Support_Check->Device_Reg0x00 & 0x0000FFFF;
    Device_ID = (pAMEModule_Support_Check->Device_Reg0x00 & 0xFFFF0000) >> 16;
    Device_ClassCode = (pAMEModule_Support_Check->Device_Reg0x08 & 0xFFFFFF00) >> 8;
    Sub_Vendor_ID = pAMEModule_Support_Check->Device_Reg0x2C & 0x0000FFFF;
    Sub_Device_ID = (pAMEModule_Support_Check->Device_Reg0x2C & 0xFFFF0000) >> 16;
    
    if ( (Vendor_ID != Vendor_ACS) &&
         (Vendor_ID != Vendor_PLX) ) {
        AME_Msg_Err("AME_Device_Support_Check Faile, not suppport this Vendor_ID = 0x%x\n",Vendor_ID);
        return FALSE;
    }
    
    switch (Device_ID)
    {
        // Check DAS device
        case AME_6101_DID_DAS:
        case AME_6201_DID_DAS:
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            ret = TRUE;
            break;
            
        // Check NT device
        case AME_8608_DID_NT:
        case AME_87B0_DID_NT:
            if ((Sub_Vendor_ID == Vendor_ACS) &&
                (Device_ClassCode == Other_Bridge_Class)) {
                ret = TRUE;
            }
            break;
            
        case AME_8508_DID_NT:
            if ((Sub_Vendor_ID == Vendor_PLX) &&
                (Device_ClassCode == Other_Bridge_Class)) {
                ret = TRUE;
            }
            break;
            
        default:
            break;
        
    }
    
    //if (ret == FALSE) {
    //    AME_Msg_Err("AME_Device_Support_Check Faile, not suppport this device!\n");
    //    AME_Msg_Err("Vendor_ID = 0x%x, Device_ID = 0x%x\n",Vendor_ID,Device_ID);
    //    AME_Msg_Err("Sub_Vendor_ID = 0x%x, Sub_Device_ID = 0x%x\n",Sub_Vendor_ID,Sub_Device_ID);
    //    AME_Msg_Err("Device_ClassCode = 0x%x\n",Device_ClassCode);
    //}
    
    return ret;
}


AME_U32
AME_eeprom_read(pAME_Data pAMEData,AME_U32 addr,AME_U32 *pBuffer)
{
    if ( (addr%2) != 0) {
        AME_Msg_Err("(%d)Read eeprom:  addr(0x%x) error\n",pAMEData->AME_Serial_Number,addr);
        return FALSE;
    }
    
    if ( (addr%4) == 0) {
        
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x260,0x6000|(addr>>2));
        AME_IO_milliseconds_Delay(10);
        *pBuffer = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x260);
        *pBuffer = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x264);
        *pBuffer = AMEByteSwap32(*pBuffer);
    
    } else {
        
        // Maybe it's odd setting, cross two doubleword size.
        AME_U32 Buffer1,Buffer2,Buffer3;
        
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x260,0x6000|(addr>>2));
        AME_IO_milliseconds_Delay(10);
        Buffer1 = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x260);
        Buffer1 = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x264);
        Buffer1 = AMEByteSwap32(Buffer1);
        
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x260,0x6000|((addr>>2)+1));
        AME_IO_milliseconds_Delay(10);
        Buffer2 = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x260);
        Buffer2 = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x264);
        Buffer2 = AMEByteSwap32(Buffer2);
        
        Buffer3 = (Buffer1<<16)&0xFFFF0000;
        Buffer3 += (Buffer2>>16)&0xFFFF;
        *pBuffer = Buffer3;
        //AME_Msg("Buffer1 = %x, Buffer2 = %x , Buffer3 = %x, addr = %x\n",Buffer1,Buffer2,Buffer3,addr);
                       
    }
    
    return TRUE;
}


AME_U32
AME_eeprom_write(pAME_Data pAMEData,AME_U32 addr,AME_U32 *pBuffer)
{
    AME_U32 value = 0,temp_value = 0,retry_index = 0;
    
    if ( (addr%4) != 0) {
        AME_Msg_Err("(%d)Write eeprom:  addr(0x%x) error\n",pAMEData->AME_Serial_Number,addr);
        return FALSE;
    }
    
    while (1)
    {
        value = *pBuffer;
        value = AMEByteSwap32(value);
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x264,value);
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x260,0xC000);
        AME_IO_milliseconds_Delay(10); 
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x260,0x4000|(addr>>2));
        AME_IO_milliseconds_Delay(10);
        
        AME_eeprom_read(pAMEData,addr,&temp_value);
        
        if ((*pBuffer) == temp_value) {
            return TRUE;
        }
        
        if (++retry_index>10) {
            break;
        }
    }
    
    AME_Msg_Err("(%d)Write eeprom:  write eeprom error addr(0x%x) (0x%08x):(0x%08x)\n",pAMEData->AME_Serial_Number,addr,*pBuffer,temp_value);
    return FALSE;
}


AME_U32
AME_eeprom_update_Z2M_Gen3(pAME_Data pAMEData)
{
    // version 0103
    
    AME_U32 index,value,eeprom_data[120];
    
    eeprom_data[0] = 0x5a00aa01;
    eeprom_data[1] = 0x83004a00;
    eeprom_data[2] = 0x0100ff02;
    eeprom_data[3] = 0x03318e00;
    eeprom_data[4] = 0xff02302e;
    eeprom_data[5] = 0x8e00ff02;
    eeprom_data[6] = 0x0331ae00;
    eeprom_data[7] = 0xff02302e;
    eeprom_data[8] = 0xae00ff02;
    eeprom_data[9] = 0x0331ce00;
    eeprom_data[10] = 0xff02302e;
    eeprom_data[11] = 0xce00ff02;
    eeprom_data[12] = 0x0331ee00;
    eeprom_data[13] = 0xff02302e;
    eeprom_data[14] = 0xee00ff02;
    eeprom_data[15] = 0xa3c08a00;
    eeprom_data[16] = 0xff02ffbf;
    eeprom_data[17] = 0x8a00ff02;
    eeprom_data[18] = 0xa3c0aa00;
    eeprom_data[19] = 0xff02ffbf;
    eeprom_data[20] = 0xaa00ff02;
    eeprom_data[21] = 0xa3c0ca00;
    eeprom_data[22] = 0xff02ffbf;
    eeprom_data[23] = 0xca00ff02;
    eeprom_data[24] = 0xa3c0ea00;
    eeprom_data[25] = 0xff02ffbf;
    eeprom_data[26] = 0xea00ff02;
    eeprom_data[27] = 0x061b8e00;
    eeprom_data[28] = 0xff020660;
    eeprom_data[29] = 0x8e00ff02;
    eeprom_data[30] = 0x061bae00;
    eeprom_data[31] = 0xff020660;
    eeprom_data[32] = 0xae00ff02;
    eeprom_data[33] = 0x061bce00;
    eeprom_data[34] = 0xff020660;
    eeprom_data[35] = 0xce00ff02;
    eeprom_data[36] = 0x061bee00;
    eeprom_data[37] = 0xff020660;
    eeprom_data[38] = 0xee00ff02;
    eeprom_data[39] = 0x60ee8a00;
    eeprom_data[40] = 0xff0283ae;
    eeprom_data[41] = 0x8a00ff02;
    eeprom_data[42] = 0x60eeaa00;
    eeprom_data[43] = 0xff0283ae;
    eeprom_data[44] = 0xaa00ff02;
    eeprom_data[45] = 0x60eeca00;
    eeprom_data[46] = 0xff0283ae;
    eeprom_data[47] = 0xca00ff02;
    eeprom_data[48] = 0x60eeea00;
    eeprom_data[49] = 0xff0283ae;
    eeprom_data[50] = 0xea00f602;
    eeprom_data[51] = 0x060000cc;
    eeprom_data[52] = 0x46040107;
    eeprom_data[53] = 0x01074704;
    eeprom_data[54] = 0x01070107;
    eeprom_data[55] = 0x46080107;
    eeprom_data[56] = 0x01074708;
    eeprom_data[57] = 0x01070107;
    eeprom_data[58] = 0xe0020610;
    eeprom_data[59] = 0x0100e602;
    eeprom_data[60] = 0x00fb1f9f;
    eeprom_data[61] = 0xe70200fb;
    eeprom_data[62] = 0x1f9fe802;
    eeprom_data[63] = 0x00fb1f9f;
    eeprom_data[64] = 0xe0020620;
    eeprom_data[65] = 0x0100e602;
    eeprom_data[66] = 0x00fb1f9f;
    eeprom_data[67] = 0xe70200fb;
    eeprom_data[68] = 0x1f9fe802;
    eeprom_data[69] = 0x00fb1f9f;
    eeprom_data[70] = 0xc0000b00;
    eeprom_data[71] = 0x0000c300;
    eeprom_data[72] = 0x01000080;
    eeprom_data[73] = 0x81000000;
    eeprom_data[74] = 0x00ff8900;
    eeprom_data[75] = 0x00020000;
    eeprom_data[76] = 0xf9020600;
    eeprom_data[77] = 0x0000c700;
    eeprom_data[78] = 0xa5a5c3c3;
    eeprom_data[79] = 0xd8000021;
    eeprom_data[80] = 0x00008c02;
    eeprom_data[81] = 0x10000013;
    eeprom_data[82] = 0x80016d49;
    eeprom_data[83] = 0x00248901;
    eeprom_data[84] = 0x10030000;
    eeprom_data[85] = 0x1b000080;
    eeprom_data[86] = 0x00001b04;
    eeprom_data[87] = 0x00800000;
    eeprom_data[88] = 0x1b080080;
    eeprom_data[89] = 0x00000d00;
    eeprom_data[90] = 0x48000000;
    eeprom_data[91] = 0x2a000000;
    eeprom_data[92] = 0x000000e4;
    eeprom_data[93] = 0xb5102d51;
    eeprom_data[94] = 0x0be4d614;
    eeprom_data[95] = 0x2d511be4;
    eeprom_data[96] = 0xc08f0000;
    eeprom_data[97] = 0x1be4c08f;
    eeprom_data[98] = 0x000034e4;
    eeprom_data[99] = 0x00000000;
    eeprom_data[100] = 0x35e40000;
    eeprom_data[101] = 0x00fe00e0;
    eeprom_data[102] = 0xb5102e51;
    eeprom_data[103] = 0x1be0c08f;
    eeprom_data[104] = 0x00003ae0;
    eeprom_data[105] = 0x0c000000;
    eeprom_data[106] = 0x3be080ff;
    eeprom_data[107] = 0xffffffff;
    eeprom_data[108] = 0xffffffff;
    eeprom_data[109] = 0xffffffff;
    eeprom_data[110] = 0xffffffff;
    eeprom_data[111] = 0xffffffff;
    eeprom_data[112] = 0xffffffff;
    eeprom_data[113] = 0xffffffff;
    eeprom_data[114] = 0xffffffff;
    eeprom_data[115] = 0xffff55aa;
    eeprom_data[116] = 0xaa550303;
    eeprom_data[117] = 0x0103ffff;
    eeprom_data[118] = 0xffffffff;
    eeprom_data[119] = 0xffffffff;

    AME_Msg("(%d)AME_eeprom_update:  Update Z2M Gen3 eeprom to version 01.03 done.\n",pAMEData->AME_Serial_Number);
    
    for (index=0; index<120; index++) {
        value = eeprom_data[index];
        AME_eeprom_write(pAMEData,index<<2,&value);
    }
    
    return TRUE;
    
}

AME_U32
AME_eeprom_update_Z2D_Gen3(pAME_Data pAMEData)
{
    // version 0101
    
    AME_U32 index,value,eeprom_data[100];
    
    eeprom_data[0] = 0x5a005001;
    eeprom_data[1] = 0x83004a00;
    eeprom_data[2] = 0x0100ff02;
    eeprom_data[3] = 0x03318e00;
    eeprom_data[4] = 0xff02302e;
    eeprom_data[5] = 0x8e00ff02;
    eeprom_data[6] = 0x0331ae00;
    eeprom_data[7] = 0xff02302e;
    eeprom_data[8] = 0xae00ff02;
    eeprom_data[9] = 0x0331ce00;
    eeprom_data[10] = 0xff02302e;
    eeprom_data[11] = 0xce00ff02;
    eeprom_data[12] = 0x0331ee00;
    eeprom_data[13] = 0xff02302e;
    eeprom_data[14] = 0xee00ff02;
    eeprom_data[15] = 0xa3c08a00;
    eeprom_data[16] = 0xff02ffbf;
    eeprom_data[17] = 0x8a00ff02;
    eeprom_data[18] = 0xa3c0aa00;
    eeprom_data[19] = 0xff02ffbf;
    eeprom_data[20] = 0xaa00ff02;
    eeprom_data[21] = 0xa3c0ca00;
    eeprom_data[22] = 0xff02ffbf;
    eeprom_data[23] = 0xca00ff02;
    eeprom_data[24] = 0xa3c0ea00;
    eeprom_data[25] = 0xff02ffbf;
    eeprom_data[26] = 0xea00ff02;
    eeprom_data[27] = 0x061b8e00;
    eeprom_data[28] = 0xff020660;
    eeprom_data[29] = 0x8e00ff02;
    eeprom_data[30] = 0x061bae00;
    eeprom_data[31] = 0xff020660;
    eeprom_data[32] = 0xae00ff02;
    eeprom_data[33] = 0x061bce00;
    eeprom_data[34] = 0xff020660;
    eeprom_data[35] = 0xce00ff02;
    eeprom_data[36] = 0x061bee00;
    eeprom_data[37] = 0xff020660;
    eeprom_data[38] = 0xee00ff02;
    eeprom_data[39] = 0x60ee8a00;
    eeprom_data[40] = 0xff0283ae;
    eeprom_data[41] = 0x8a00ff02;
    eeprom_data[42] = 0x60eeaa00;
    eeprom_data[43] = 0xff0283ae;
    eeprom_data[44] = 0xaa00ff02;
    eeprom_data[45] = 0x60eeca00;
    eeprom_data[46] = 0xff0283ae;
    eeprom_data[47] = 0xca00ff02;
    eeprom_data[48] = 0x60eeea00;
    eeprom_data[49] = 0xff0283ae;
    eeprom_data[50] = 0xea00e002;
    eeprom_data[51] = 0x06100100;
    eeprom_data[52] = 0xe60200fb;
    eeprom_data[53] = 0x1f9fe702;
    eeprom_data[54] = 0x00fb1f9f;
    eeprom_data[55] = 0xe80200fb;
    eeprom_data[56] = 0x1f9fe002;
    eeprom_data[57] = 0x06200100;
    eeprom_data[58] = 0xe60200fb;
    eeprom_data[59] = 0x1f9fe702;
    eeprom_data[60] = 0x00fb1f9f;
    eeprom_data[61] = 0xe80200fb;
    eeprom_data[62] = 0x1f9fc000;
    eeprom_data[63] = 0x03000000;
    eeprom_data[64] = 0xc3000100;
    eeprom_data[65] = 0x00808100;
    eeprom_data[66] = 0x000000ff;
    eeprom_data[67] = 0x89000002;
    eeprom_data[68] = 0x0000f902;
    eeprom_data[69] = 0x06000000;
    eeprom_data[70] = 0xc700a5a5;
    eeprom_data[71] = 0xc3c3d800;
    eeprom_data[72] = 0x00000000;
    eeprom_data[73] = 0x80016d49;
    eeprom_data[74] = 0x00248901;
    eeprom_data[75] = 0x18030000;
    eeprom_data[76] = 0x8c021000;
    eeprom_data[77] = 0x0011f602;
    eeprom_data[78] = 0x060000cc;
    eeprom_data[79] = 0x46040107;
    eeprom_data[80] = 0x01074704;
    eeprom_data[81] = 0x01070107;
    eeprom_data[82] = 0x46080107;
    eeprom_data[83] = 0x01074708;
    eeprom_data[84] = 0x01070107;
    eeprom_data[85] = 0xffffffff;
    eeprom_data[86] = 0xffffffff;
    eeprom_data[87] = 0xffffffff;
    eeprom_data[88] = 0xffffffff;
    eeprom_data[89] = 0xffffffff;
    eeprom_data[90] = 0xffffffff;
    eeprom_data[91] = 0xffffffff;
    eeprom_data[92] = 0xffffffff;
    eeprom_data[93] = 0x55aaaa55;
    eeprom_data[94] = 0x03050101;
    eeprom_data[95] = 0xffffffff;
    eeprom_data[96] = 0xffffffff;
    eeprom_data[97] = 0xffffffff;
    eeprom_data[98] = 0xffffffff;
    eeprom_data[99] = 0xffffffff;
    

    AME_Msg("(%d)AME_eeprom_update:  Update Z2D Gen3 type eeprom to version 01.01 done.\n",pAMEData->AME_Serial_Number);
    
    for (index=0; index<100; index++) {
        value = eeprom_data[index];
        AME_eeprom_write(pAMEData,index<<2,&value);
    }
    
    return TRUE;
    
}

AME_U32
AME_eeprom_update_Z1M_NT(pAME_Data pAMEData)
{
    // version 0104
    
    AME_U32 index,value,eeprom_data[100];
    
    eeprom_data[0] = 0x5a009c00;
    eeprom_data[1] = 0x0d004800;
    eeprom_data[2] = 0x00005d01;
    eeprom_data[3] = 0x02000000;
    eeprom_data[4] = 0x61030000;
    eeprom_data[5] = 0x00fe2a00;
    eeprom_data[6] = 0x00000000;
    eeprom_data[7] = 0x34040000;
    eeprom_data[8] = 0x00006007;
    eeprom_data[9] = 0x00000000;
    eeprom_data[10] = 0x35040000;
    eeprom_data[11] = 0x00fe6107;
    eeprom_data[12] = 0x000000fe;
    eeprom_data[13] = 0x3ac00c00;
    eeprom_data[14] = 0x00003bc0;
    eeprom_data[15] = 0x80ffffff;
    eeprom_data[16] = 0x8000ff00;
    eeprom_data[17] = 0x0f008100;
    eeprom_data[18] = 0xffffff00;
    eeprom_data[19] = 0x8104ffff;
    eeprom_data[20] = 0xff008f00;
    eeprom_data[21] = 0x809c1400;
    eeprom_data[22] = 0x8c000101;
    eeprom_data[23] = 0x01007700;
    eeprom_data[24] = 0x40002631;
    eeprom_data[25] = 0x1b000080;
    eeprom_data[26] = 0x00008c00;
    eeprom_data[27] = 0x00000000;
    eeprom_data[28] = 0x0004b510;
    eeprom_data[29] = 0x08860b04;
    eeprom_data[30] = 0xd6140886;
    eeprom_data[31] = 0x1b040080;
    eeprom_data[32] = 0x00001bc0;
    eeprom_data[33] = 0xc08f0000;
    eeprom_data[34] = 0xe8029f9f;
    eeprom_data[35] = 0x9f9fec02;
    eeprom_data[36] = 0x90909090;
    eeprom_data[37] = 0x94000100;
    eeprom_data[38] = 0x00009500;
    eeprom_data[39] = 0x01000000;
    eeprom_data[40] = 0xffffffff;
    eeprom_data[41] = 0xffffffff;
    eeprom_data[42] = 0xffffffff;
    eeprom_data[43] = 0xffffffff;
    eeprom_data[44] = 0xffffffff;
    eeprom_data[45] = 0xffffffff;
    eeprom_data[46] = 0xffffffff;
    eeprom_data[47] = 0xffffffff;
    eeprom_data[48] = 0x55aaaa55;
    eeprom_data[49] = 0x03040104;
    eeprom_data[50] = 0xffffffff;
    eeprom_data[51] = 0xffffffff;
    eeprom_data[52] = 0xffffffff;
    eeprom_data[53] = 0xffffffff;
    eeprom_data[54] = 0xffffffff;
    eeprom_data[55] = 0xffffffff;
    eeprom_data[56] = 0xffffffff;
    eeprom_data[57] = 0xffffffff;
    eeprom_data[58] = 0xffffffff;
    eeprom_data[59] = 0xffffffff;
    eeprom_data[60] = 0xffffffff;
    eeprom_data[61] = 0xffffffff;
    eeprom_data[62] = 0xffffffff;
    eeprom_data[63] = 0xffffffff;
    eeprom_data[64] = 0xffffffff;
    eeprom_data[65] = 0xffffffff;
    eeprom_data[66] = 0xffffffff;
    eeprom_data[67] = 0xffffffff;
    eeprom_data[68] = 0xffffffff;
    eeprom_data[69] = 0xffffffff;
    eeprom_data[70] = 0xffffffff;
    eeprom_data[71] = 0xffffffff;
    eeprom_data[72] = 0xffffffff;
    eeprom_data[73] = 0xffffffff;
    eeprom_data[74] = 0xffffffff;
    eeprom_data[75] = 0xffffffff;
    eeprom_data[76] = 0xffffffff;
    eeprom_data[77] = 0xffffffff;
    eeprom_data[78] = 0xffffffff;
    eeprom_data[79] = 0xffffffff;
    eeprom_data[80] = 0xffffffff;
    eeprom_data[81] = 0xffffffff;
    eeprom_data[82] = 0xffffffff;
    eeprom_data[83] = 0xffffffff;
    eeprom_data[84] = 0xffffffff;
    eeprom_data[85] = 0xffffffff;
    eeprom_data[86] = 0xffffffff;
    eeprom_data[87] = 0xffffffff;
    eeprom_data[88] = 0xffffffff;
    eeprom_data[89] = 0xffffffff;
    eeprom_data[90] = 0xffffffff;
    eeprom_data[91] = 0xffffffff;
    eeprom_data[92] = 0xffffffff;
    eeprom_data[93] = 0xffffffff;
    eeprom_data[94] = 0xffffffff;
    eeprom_data[95] = 0xffffffff;
    eeprom_data[96] = 0xffffffff;
    eeprom_data[97] = 0xffffffff;
    eeprom_data[98] = 0xffffffff;
    eeprom_data[99] = 0xffffffff;

    AME_Msg("(%d)AME_eeprom_update:  Update Z1M NT type eeprom to version 01.04 done.\n",pAMEData->AME_Serial_Number);
    
    for (index=0; index<100; index++) {
        value = eeprom_data[index];
        AME_eeprom_write(pAMEData,index<<2,&value);
    }
    
    return TRUE;
    
}


AME_U32
AME_eeprom_update_C1M_NT(pAME_Data pAMEData)
{
    // version 0101
    
    AME_U32 index,value,eeprom_data[100];
    
    eeprom_data[0] = 0x5a00a200;
    eeprom_data[1] = 0x77000000;
    eeprom_data[2] = 0x26110d00;
    eeprom_data[3] = 0x48000000;
    eeprom_data[4] = 0x5d010200;
    eeprom_data[5] = 0x00006103;
    eeprom_data[6] = 0x000000fe;
    eeprom_data[7] = 0x2a000000;
    eeprom_data[8] = 0x00003404;
    eeprom_data[9] = 0x00000000;
    eeprom_data[10] = 0x60070000;
    eeprom_data[11] = 0x00003504;
    eeprom_data[12] = 0x000000fe;
    eeprom_data[13] = 0x61070000;
    eeprom_data[14] = 0x00fe3ac0;
    eeprom_data[15] = 0x0c000000;
    eeprom_data[16] = 0x3bc080ff;
    eeprom_data[17] = 0xffff8000;
    eeprom_data[18] = 0xff000f00;
    eeprom_data[19] = 0x8100ffff;
    eeprom_data[20] = 0xff008104;
    eeprom_data[21] = 0xffffff00;
    eeprom_data[22] = 0x8f00809c;
    eeprom_data[23] = 0x14008c00;
    eeprom_data[24] = 0x01010100;
    eeprom_data[25] = 0x77004000;
    eeprom_data[26] = 0x26311b00;
    eeprom_data[27] = 0x00800000;
    eeprom_data[28] = 0x8c000000;
    eeprom_data[29] = 0x00000004;
    eeprom_data[30] = 0xb5100886;
    eeprom_data[31] = 0x0b04d614;
    eeprom_data[32] = 0x08861b04;
    eeprom_data[33] = 0x00800000;
    eeprom_data[34] = 0x1bc0c08f;
    eeprom_data[35] = 0x0000e802;
    eeprom_data[36] = 0x9f9f9f9f;
    eeprom_data[37] = 0xec029090;
    eeprom_data[38] = 0x90909400;
    eeprom_data[39] = 0x01000000;
    eeprom_data[40] = 0x95000100;
    eeprom_data[41] = 0x0000ffff;
    eeprom_data[42] = 0xffffffff;
    eeprom_data[43] = 0xffffffff;
    eeprom_data[44] = 0xffffffff;
    eeprom_data[45] = 0xffffffff;
    eeprom_data[46] = 0xffffffff;
    eeprom_data[47] = 0xffffffff;
    eeprom_data[48] = 0xffffffff;
    eeprom_data[49] = 0xffff55aa;
    eeprom_data[50] = 0xaa550402;
    eeprom_data[51] = 0x0101ffff;
    eeprom_data[52] = 0xffffffff;
    eeprom_data[53] = 0xffffffff;
    eeprom_data[54] = 0xffffffff;
    eeprom_data[55] = 0xffffffff;
    eeprom_data[56] = 0xffffffff;
    eeprom_data[57] = 0xffffffff;
    eeprom_data[58] = 0xffffffff;
    eeprom_data[59] = 0xffffffff;
    eeprom_data[60] = 0xffffffff;
    eeprom_data[61] = 0xffffffff;
    eeprom_data[62] = 0xffffffff;
    eeprom_data[63] = 0xffffffff;
    eeprom_data[64] = 0xffffffff;
    eeprom_data[65] = 0xffffffff;
    eeprom_data[66] = 0xffffffff;
    eeprom_data[67] = 0xffffffff;
    eeprom_data[68] = 0xffffffff;
    eeprom_data[69] = 0xffffffff;
    eeprom_data[70] = 0xffffffff;
    eeprom_data[71] = 0xffffffff;
    eeprom_data[72] = 0xffffffff;
    eeprom_data[73] = 0xffffffff;
    eeprom_data[74] = 0xffffffff;
    eeprom_data[75] = 0xffffffff;
    eeprom_data[76] = 0xffffffff;
    eeprom_data[77] = 0xffffffff;
    eeprom_data[78] = 0xffffffff;
    eeprom_data[79] = 0xffffffff;
    eeprom_data[80] = 0xffffffff;
    eeprom_data[81] = 0xffffffff;
    eeprom_data[82] = 0xffffffff;
    eeprom_data[83] = 0xffffffff;
    eeprom_data[84] = 0xffffffff;
    eeprom_data[85] = 0xffffffff;
    eeprom_data[86] = 0xffffffff;
    eeprom_data[87] = 0xffffffff;
    eeprom_data[88] = 0xffffffff;
    eeprom_data[89] = 0xffffffff;
    eeprom_data[90] = 0xffffffff;
    eeprom_data[91] = 0xffffffff;
    eeprom_data[92] = 0xffffffff;
    eeprom_data[93] = 0xffffffff;
    eeprom_data[94] = 0xffffffff;
    eeprom_data[95] = 0xffffffff;
    eeprom_data[96] = 0xffffffff;
    eeprom_data[97] = 0xffffffff;
    eeprom_data[98] = 0xffffffff;
    eeprom_data[99] = 0xffffffff;

    AME_Msg("(%d)AME_eeprom_update:  Update C1M NT type eeprom to version 01.01 done.\n",pAMEData->AME_Serial_Number);
    
    for (index=0; index<100; index++) {
        value = eeprom_data[index];
        AME_eeprom_write(pAMEData,index<<2,&value);
    }
    
    return TRUE;
    
}


AME_U32
AME_eeprom_update(pAME_Data pAMEData)
{
    AME_U16 length = 0,Device_type = 0,version = 0;
    AME_U32 addr,Sub_DID_VID,serial_number = 0,value = 0;
    AME_U32 Bridge_Bus,Bridge_Slot;
    AME_U32 Temp_Bus,Temp_Slot,Temp_Reg_0x00;
    
    
    /* Get PLX eeprom length */
    AME_eeprom_read(pAMEData,0x00,&value);
    length = (AME_U16)(value&0xFFFF);
    length = AMEByteSwap16(length);
    
    /* Check PLX header */
    if ((value>>16) != 0x5A00) {
        AME_Msg_Err("(%d)AME_eeprom_update:  Not PLX eeprom type, eeprom check failed. (0x%08x)\n",pAMEData->AME_Serial_Number,value);
        return FALSE;
    }
    
    /* Check Accusys signature, if Accusys signature error, maybe this is RD sample, go to update. */
    addr = (AME_U32)length + 36;
    AME_eeprom_read(pAMEData,addr,&value);    
    
    
    if (value != 0x55AAAA55) {        
        AME_Msg_Err("(%d)AME_eeprom_update:  ACCUSYS signature not found , do not update eeprom.\n",pAMEData->AME_Serial_Number);       
        return TRUE;
    }
    
    /* Get Serial number */
    addr = (AME_U32)length + 4;
    AME_eeprom_read(pAMEData,addr,&serial_number);
    
    /* Get device type and version number */
    addr = (AME_U32)length + 40;
    AME_eeprom_read(pAMEData,addr,&value);
    Device_type = value>>16;
    version = value&0xFFFF;
    
    AME_Msg("(%d)AME_eeprom_update:  value %x\n",pAMEData->AME_Serial_Number,value);
    
    /* Check eeprom is need update */
    // 03 (HBA) 03 (Z2M-G3 PEX8717)     , version 0103
    // 03 (HBA) 04 (Z1M (PEX8608) NT)   , version 0104
    // 03 (HBA) 05 (Z2D-G3 PEX8717)     , version 0101
    // 04 (HBA) 02 (C1M (PEX8608) NT)   , version 0101
    AME_Msg("(%d)AME_eeprom_update:  Device_type %04x\n",pAMEData->AME_Serial_Number,Device_type);
    switch (Device_type)
    {
        case Eeprom_type_Z1M_NT: 
            if (version < 0x0104) {
                AME_Msg("(%d)AME_eeprom_update:  Z1M NT type eeprom is old version %02x.%02x\n",pAMEData->AME_Serial_Number,version>>8,version&0xFF);
                AME_eeprom_update_Z1M_NT(pAMEData);
            } else {
                AME_Msg("(%d)AME_eeprom_update:  Z1M NT type eeprom version is latest version %02x.%02x.\n",pAMEData->AME_Serial_Number,version>>8,version&0xFF);
            }
            break;
            
        case Eeprom_type_C1M_NT: 
            if (version < 0x0101) {
                AME_Msg("(%d)AME_eeprom_update:  C1M NT type eeprom is old version %02x.%02x\n",pAMEData->AME_Serial_Number,version>>8,version&0xFF);
                AME_eeprom_update_C1M_NT(pAMEData);
            } else {
                AME_Msg("(%d)AME_eeprom_update:  C1M NT type eeprom version is latest version %02x.%02x.\n",pAMEData->AME_Serial_Number,version>>8,version&0xFF);
            }
            break;
        case Eeprom_type_Z2M_G3:
            if (version < 0x0103) {
                AME_Msg("(%d)AME_eeprom_update:  Z2M Gen3 eeprom is old version %02x.%02x\n",pAMEData->AME_Serial_Number,version>>8,version&0xFF);
                AME_eeprom_update_Z2M_Gen3(pAMEData);
            } else {
                AME_Msg("(%d)AME_eeprom_update:  Z2M Gen3 eeprom version is latest version %02x.%02x.\n",pAMEData->AME_Serial_Number,version>>8,version&0xFF);
            }
            break;
        case Eeprom_type_Z2D_G3:
            if (version < 0x0101) {
                AME_Msg("(%d)AME_eeprom_update:  Z2D Gen3 eeprom is old version %02x.%02x\n",pAMEData->AME_Serial_Number,version>>8,version&0xFF);
                AME_eeprom_update_Z2D_Gen3(pAMEData);
            } else {
                AME_Msg("(%d)AME_eeprom_update:  Z2D Gen3 eeprom version is latest version %02x.%02x.\n",pAMEData->AME_Serial_Number,version>>8,version&0xFF);
            }
            break;
        default:
            break;
    }
    
    //for (addr=0; addr<0x194; addr+=4) {
    //    AME_eeprom_read(pAMEData,addr,&value);
    //    AME_Msg("Sam Debug:  addr(0x%08x) eeprom_data[%d] = 0x%08x;\n",addr,addr/4,value);
    //}
    
    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Init_main(pAME_Data pAMEData,pAME_Module_Init_Prepare pAMEModule_Init_Prepare)
{
    AME_U32 RAID_ID,LUN_ID,i,Reg_0x04;

    /* Setting AME_module Data */
    pAMEData->Device_BusNumber    = pAMEModule_Init_Prepare->Device_BusNumber;
    pAMEData->Vendor_ID           = pAMEModule_Init_Prepare->Vendor_ID;
    pAMEData->Device_ID           = pAMEModule_Init_Prepare->Device_ID;
    pAMEData->Raid_Base_Address   = pAMEModule_Init_Prepare->Raid_Base_Address;
    pAMEData->pDeviceExtension    = pAMEModule_Init_Prepare->pDeviceExtension;
    pAMEData->Glo_High_Address    = pAMEModule_Init_Prepare->Glo_High_Address;
    pAMEData->Polling_init_Enable = pAMEModule_Init_Prepare->Polling_init_Enable;
    pAMEData->Init_Memory_by_AME_module  = pAMEModule_Init_Prepare->Init_Memory_by_AME_module;
    pAMEData->AME_Buffer_physicalStartAddress = pAMEModule_Init_Prepare->AME_Buffer_physicalStartAddress;
    pAMEData->AME_Buffer_virtualStartAddress  = pAMEModule_Init_Prepare->AME_Buffer_virtualStartAddress;
    pAMEData->AME_RAID_Ready = FALSE;       // for DAS timer init
    pAMEData->Next_NT_ISR_Check = FALSE;    // for NT lose ISR issue

    //Init AME_NTHost_data
    AME_Memory_zero(&(pAMEData->AME_NTHost_data), sizeof(pAMEData->AME_NTHost_data));
    
    // Enable BusMaster and IO space
    Reg_0x04 = AME_Raid_PCI_Config_Read(pAMEData,0x04);
    AME_Raid_PCI_Config_Write(pAMEData,0x04,Reg_0x04|0x07);

    // AMEData init
    pAMEData->Message_SIZE = REQUEST_MSG_SIZE;
    pAMEData->AMEEventlogqueue.head = 0;
    pAMEData->AMEEventlogqueue.tail = 0;
    pAMEData->AMEEventSwitchState = AME_EVENT_SWTICH_OFF;
    pAMEData->AMEInBandBufferData.InBandFlag = INBANDCMD_IDLE;
    
    // Init Lun change data.
    for (RAID_ID=0; RAID_ID<MAX_RAID_System_ID; RAID_ID++) {
        
        pAMEData->AME_RAIDData[RAID_ID].SendVir_Ready = FALSE;
        pAMEData->AME_RAIDData[RAID_ID].InBand_Loaded_Ready = FALSE;
        pAMEData->AME_RAIDData[RAID_ID].Lun_Change_State = Lun_Change_State_NONE;
        
        for (LUN_ID=0; LUN_ID<MAX_RAID_LUN_ID; LUN_ID++) {
            pAMEData->AME_RAIDData[RAID_ID].Lun_Data[LUN_ID] = Lun_Not_Exist;
            pAMEData->AME_RAIDData[RAID_ID].Lun_Mask[LUN_ID] = UnMask;
            pAMEData->AME_RAIDData[RAID_ID].Lun_Change[LUN_ID] = Lun_Change_NONE;
        }
    }

    // Init Led data, Mapping HBA IO space and check HBA link
    pAMEData->AME_LED_data.LED_HBA_Type = 0;
    pAMEData->AME_LED_data.LED_HBA_Port_Number = 0;
    pAMEData->AME_LED_data.LED_HBA_Base_Address = 0;
    pAMEData->AME_LED_data.LED_Raid_Base_Address = 0;
    
    // Init Led data, Mapping HBA IO space and check HBA link
    if (AME_HBA_Init(pAMEData) == FALSE) {
        AME_Msg_Err("(%d)AME_HBA_Init Fail...\n",pAMEData->AME_Serial_Number);
        return FALSE;
    }

    if (AME_Raid_Init(pAMEData) == FALSE) {
        AME_Msg_Err("(%d)AME_Raid_Init Fail...\n",pAMEData->AME_Serial_Number);
        return FALSE;
    }
    
    // Init Buffer request, reply ,event ...
    AME_Buffer_Data_Init(pAMEData);
    
    // only support one NT device check
    if (TRUE == AME_Check_is_NT_Mode(pAMEData)) {
        if (pAMEData->ReInit == FALSE) {
            if (FALSE == MPIO_Single_NT_Check(pAMEData)) {
                pAMEData->Is_First_NT_device = FALSE;
                AME_Msg_Err("(%d)NT device %x not support, because only support one NT device. !!!\n",pAMEData->AME_Serial_Number,pAMEData->Device_ID);
            } else {
                pAMEData->Is_First_NT_device = TRUE;
            }
        }
    }
    
    if (TRUE == pAMEData->Polling_init_Enable) {
        
        // AME_Init Polling make sure NT negotaition done
        if (TRUE == AME_Check_is_NT_Mode(pAMEData)) {
            
            AME_Msg("(%d)AME_Init Polling Start ******************************\n",pAMEData->AME_Serial_Number);
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_MASK_REG,0xFFFF);
                 
            while (pAMEData->Polling_init_Enable == TRUE) {
                
                // Only support one NT device
                if (pAMEData->Is_First_NT_device == FALSE) {
                    
                    // When driver polling state,and device not first NT device
                    // Driver never complete, unless driver stop poling state.
                    pAMEData->Polling_init_Enable = FALSE;
                    AME_Msg("(%d)AME_Init Polling Stop, only support one NT device ***\n",pAMEData->AME_Serial_Number);
                    break;
                }
                
                /* Call MPIO_Timer */
                AME_Timer(pAMEData);
                AME_IO_milliseconds_Delay(1);
            
                //Because send 2 inband commands at AME_Timer( LUN mask page and page0) and just can issue 1 inband command
                // to RAID (another is queued and till 1st done, so call multiple times of AME_ISR().
                // at AME_ISR(), will check is the 1st is done and will issue the queued commmand to RAID.
                for(i=0 ; i<10 ; i++){
                    AME_IO_milliseconds_Delay(1);   
                    AME_ISR(pAMEData);
                } //James
            
            }
                 
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_MASK_REG,0xFFFF);
            AME_Msg("(%d)AME_Init Polling Done  ******************************\n",pAMEData->AME_Serial_Number);
            
        }
        else {
            
            AME_U32  Wait_Index=0;
            
            while (1) {
                
                if (AME_Check_RAID_Ready_Status(pAMEData) == TRUE) {
                    AME_Host_RAID_Ready(pAMEData);
                    pAMEData->Polling_init_Enable = FALSE;
                    break;
                }
                
                if ( (Wait_Index++) > 100 ) {
                    AME_Msg_Err("(%d)AME_Raid_Init Fail\n",pAMEData->AME_Serial_Number);
                    return FALSE;
                } else {
                    AME_IO_milliseconds_Delay(1000); // Wait 1 sec
                    AME_Msg("(%d)Waiting AME Raid Init:[%d]\n",pAMEData->AME_Serial_Number,Wait_Index);
                }
            }
            
        }
        
    }
    
    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Init(pAME_Data pAMEData,pAME_Module_Init_Prepare pAMEModule_Init_Prepare)
{
    AME_U32 return_status;
    
    AME_Msg("(%d)AME_Init: AME_module version %s, build date %s\n",pAMEData->AME_Serial_Number,AME_Module_version,AME_Module_Modify_Time);

    ACS_message((pMSG_DATA)(pAMEData->pMSGDATA),AME_init,AME_Init_start,NULL);
    
    pAMEData->ReInit = FALSE;

    return_status = AME_Init_main(pAMEData,pAMEModule_Init_Prepare);

    ACS_message((pMSG_DATA)(pAMEData->pMSGDATA),AME_init,AME_Init_Done_Success,NULL);
    
    return return_status;

}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_ReInit(pAME_Data pAMEData,pAME_Module_Init_Prepare pAMEModule_Init_Prepare)
{
    AME_Msg("(%d)AME_ReInit: AME_module version %s, build date %s\n",pAMEData->AME_Serial_Number,AME_Module_version,AME_Module_Modify_Time);
    pAMEData->ReInit = TRUE;
    return (AME_Init_main(pAMEData,pAMEModule_Init_Prepare));
}


AME_U32 AME_InBand_Loaded_Get_Page_0_callback(pAME_REQUEST_CallBack pAMERequestCallBack)
{
    pAME_Data   pAMEData = pAMERequestCallBack->pAMEData;
    AME_U32     Raid_ID  = pAMERequestCallBack->Raid_ID;
    AME_U32     i;
    AME_U16     Lun_Data;
    
    pAMEData->AME_RAIDData[Raid_ID].SendVir_Ready = TRUE;
    pAMEData->AME_RAIDData[Raid_ID].InBand_Loaded_Ready = TRUE;
    
    for (i=0; i<MAX_RAID_LUN_ID; i++)
    {
        Lun_Data = *((AME_U16 *)(pAMEData->AMEInBandBufferData.InBand_Internal_Buffer_Vir_addr+0x80) + i);

        pAMEData->AME_RAIDData[Raid_ID].Lun_Data[i] = Lun_Data;
        pAMEData->AME_RAIDData[Raid_ID].Lun_Change[i] = Lun_Change_NONE;
        
        // show exist Raid and lun message
        if ((pAMEData->AME_RAIDData[Raid_ID].Lun_Data[i] != Lun_Not_Exist) &&
            (pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[i] != Mask)) {
            AME_Msg("(%d)Find Raid(%d) Lun(%d) exist.\n",pAMEData->AME_Serial_Number,Raid_ID,i);
        }
        
    }

    MPIO_AME_RAID_Ready(pAMEData,Raid_ID,pAMEData->AMEInBandBufferData.InBand_Internal_Buffer_Vir_addr);

    // Sam Debug: Mask for mac complier pass ->
    //if (pAMEData->ReInit != TRUE) {
    //  MPIO_Host_Scan_All_Lun(pAMEData,Raid_ID);
    //}
    // Sam Debug: Mask for mac complier pass <-
                    
    // Only DAS need to turn on event switch
    if (TRUE == AME_Check_is_DAS_Mode(pAMEData))
    {
        /* Turn On Raid Event switch */
        AME_Event_Turn_Switch(pAMEData,AME_EVENT_SWTICH_ON);
    }
    
    return TRUE;
}


AME_U32 AME_InBand_Get_Page_0_callback(pAME_REQUEST_CallBack pAMERequestCallBack)
{
    pAME_Data   pAMEData = pAMERequestCallBack->pAMEData;
    AME_U32     Raid_ID  = pAMERequestCallBack->Raid_ID;
    AME_U32     i;
    AME_U16     Lun_Data;
    
    
    if (pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State & Lun_Change_State_Need_check) {
        // new lun change get when checking, ingore this and wait timer reget page0 again.
        return TRUE;
    }
    
    AME_spinlock(pAMEData);

    
    pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State = Lun_Change_State_NONE;

    for (i=0; i<MAX_RAID_LUN_ID; i++)
    {
        Lun_Data = *((AME_U16 *)(pAMEData->AMEInBandBufferData.InBand_Internal_Buffer_Vir_addr+0x80) + i);
        if (pAMEData->AME_RAIDData[Raid_ID].Lun_Data[i] != Lun_Data) {

            if ((pAMEData->AME_RAIDData[Raid_ID].Lun_Data[i] == Lun_Not_Exist)&&
                (pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[i] == UnMask)) {
                pAMEData->AME_RAIDData[Raid_ID].Lun_Change[i] = Lun_Change_Add;
            }
            else if (pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[i] == UnMask){
                pAMEData->AME_RAIDData[Raid_ID].Lun_Change[i] = Lun_Change_Remove;
            }
        }
        pAMEData->AME_RAIDData[Raid_ID].Lun_Data[i] = Lun_Data;
    }
    
    AME_spinunlock(pAMEData);

    return TRUE;
}

AME_U32 
AME_InBand_Loaded_Get_Lun_Mask_Page_callback(pAME_REQUEST_CallBack pAMERequestCallBack)
{
    
    pAME_Data   pAMEData = pAMERequestCallBack->pAMEData;
    AME_U32     Raid_ID  = pAMERequestCallBack->Raid_ID;
    AME_U32     i,j,x,y;
    AME_U8      Lun_Mask_Data;
    AME_U32     Port_Number = pAMEData->AME_NTHost_data.PCISerialNumber;

    //Thif is not for frist loaded get page using.
    /*
    if (pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State & Lun_Mask_Need_check) {
        return TRUE;
    }
    */
    
    AME_spinlock(pAMEData);

    // need be set to Lun_Change_State_NONE at AME_InBand_Get_Page_0_callback()
    //pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State = Lun_Change_State_NONE;

    for (i=0,j=0 ; i<8 ; i++,j+=8)
    {
        Lun_Mask_Data = *((AME_U8 *)(pAMEData->AMEInBandBufferData.InBand_Internal_Buffer_Vir_addr+(Port_Number*16)) + i);
        for(x=0;x<8;x++){

            if((Lun_Mask_Data>>x)&0x01){
                pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[j+x] = Mask;   
            } else{
                pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[j+x] = UnMask; 
            }
            
        }           
    }

    AME_spinunlock(pAMEData);

    return TRUE;
}

AME_U32 AME_InBand_Get_Lun_Mask_Page_callback(pAME_REQUEST_CallBack pAMERequestCallBack)
{
    
    pAME_Data   pAMEData = pAMERequestCallBack->pAMEData;
    AME_U32     Raid_ID  = pAMERequestCallBack->Raid_ID;
    AME_U32     i,j,x,y;
    AME_U8      Lun_Mask_Data;
    AME_U32     Port_Number = pAMEData->AME_NTHost_data.PCISerialNumber;
    
    
    if (pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State & Lun_Mask_Need_check) {
        // new lun mask event get when checking, ingore this and wait timer reget page0 again.      
        return TRUE;
    }
    
    AME_spinlock(pAMEData);

    pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State = Lun_Change_State_NONE;

    for (i=0,j=0 ; i<8 ; i++,j+=8)
    {
        Lun_Mask_Data = *((AME_U8 *)(pAMEData->AMEInBandBufferData.InBand_Internal_Buffer_Vir_addr+(Port_Number*16)) + i);
        for(x=0;x<8;x++){
            
            if (pAMEData->AME_RAIDData[Raid_ID].Lun_Data[j+x] != Lun_Not_Exist){
                if(pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[j+x]!=((Lun_Mask_Data>>x)&0x01)){

                    if(pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[j+x] == Mask){
                        pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[j+x] = UnMask;
                        AME_Msg("Port[%d]  RAID[%d]  Lun[%d]  UnMask\n",Port_Number,Raid_ID,j+x);
                        pAMEData->AME_RAIDData[Raid_ID].Lun_Change[j+x] = Lun_Change_Add;
                    }
                    else{
                        pAMEData->AME_RAIDData[Raid_ID].Lun_Mask[j+x] = Mask;
                        AME_Msg("Port[%d]  RAID[%d]  Lun[%d]  Mask\n",Port_Number,Raid_ID,j+x);
                        pAMEData->AME_RAIDData[Raid_ID].Lun_Change[j+x] = Lun_Change_Remove;
                    }
                    
                }               
            }       

        }           
    }

    AME_spinunlock(pAMEData);

    return TRUE;
}
AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Lun_Change_Check(pAME_Data pAMEData)
{
    AME_U32 Raid_ID;
    
    for (Raid_ID=0; Raid_ID<MAX_RAID_System_ID; Raid_ID++)
    {
        if (pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State & Lun_Change_State_Need_check) {
                if ((pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State & Lun_Change_State_Checking) == 0) {

                // get newer page 0
                if (AME_InBand_Get_Page(pAMEData,Raid_ID,0,AME_InBand_Get_Page_0_callback) == TRUE) {
                    AME_spinlock(pAMEData);
                    pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State = Lun_Change_State_Checking;
                    AME_spinunlock(pAMEData);
                }
                // else, means had inband command doing, will redo at next timer.
                break;
            }
        }

        else if (pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State & Lun_Mask_Need_check) {
            if ((pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State & Lun_Change_State_Checking) == 0) {        

                // get newer lun mask page
                if (AME_InBand_Get_Page(pAMEData,Raid_ID,Lun_Mask_Page,AME_InBand_Get_Lun_Mask_Page_callback) == TRUE) {
                    AME_spinlock(pAMEData);
                    pAMEData->AME_RAIDData[Raid_ID].Lun_Change_State = Lun_Change_State_Checking;
                    AME_spinunlock(pAMEData);
                }
                // else, means had inband command doing, will redo at next timer.
                break;
            }
        }

    }   
    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Start(pAME_Data pAMEData)
{
    // Call from AME_HBA_Init
    /* Turn ON HBA LED GPIO */
    //AME_LED_Start(pAMEData);

    /* 
     * Only DAS need to turn on event switch, and Raid ID = 0
     */
    if (TRUE == AME_Check_is_DAS_Mode(pAMEData))
    {
        // Send Reply address to Raid hardware Fifo
        AME_Reply_Buffer_Init(pAMEData);
        
        // Get the page 0 when driver loaded and then Turn On Raid Event switch
        AME_InBand_Get_Page(pAMEData,0,0,AME_InBand_Loaded_Get_Page_0_callback);
    }
    
    return TRUE;
}

AME_U32 AME_Stop(pAME_Data pAMEData)
{
    
    AME_LED_Stop(pAMEData);
    
    /* Disable AME message unit interrupt */
    if ( (pAMEData->Device_ID == AME_2208_DID1_DAS) ||
        (pAMEData->Device_ID == AME_2208_DID2_DAS) ||
        (pAMEData->Device_ID == AME_2208_DID3_DAS) ||
        (pAMEData->Device_ID == AME_2208_DID4_DAS) ||
        (pAMEData->Device_ID == AME_3108_DID0_DAS) ||
        (pAMEData->Device_ID == AME_3108_DID1_DAS) ) {
         AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_MASK_REGISTER, 0x05);
    }
    
    return TRUE;
}

AME_U32 AME_Get_Raid_Link_State(pAME_Data pAMEData)
{
    AME_U32 Temp_Reg_0x00 = 0;
    AME_U32 Temp_Reg_0x78 = 0;
    
    switch (pAMEData->Device_ID)
    {
        case AME_6201_DID_DAS:
            Temp_Reg_0x00 = AME_PCI_Config_Read_32(pAMEData,pAMEData->Device_BusNumber-2,0,0,0x00);
            if (Temp_Reg_0x00 == PLX_Brige_8518) {
                Temp_Reg_0x78 = AME_PCI_Config_Read_32(pAMEData,pAMEData->Device_BusNumber-2,0,0,0x78);
                return Temp_Reg_0x78;
            }
            break;
            
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            if (pAMEData->AME_LED_data.LED_HBA_Base_Address) {
                Temp_Reg_0x78 = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x78);
                return Temp_Reg_0x78;
            }
            break;
            
        default:
            /* Others don't need link state modify */
            break;
    }
    
    return FALSE;
}

AME_U32 AME_NT_Raid_Bar_lost_Check(pAME_Data pAMEData,AME_U32 Raid_ID)
{
    if (pAMEData->AME_RAIDData[Raid_ID].Raid_Error_Handle_State == TRUE) {
        return TRUE;
    }
    
    return FALSE;
}

AME_U32 AME_NT_Raid_2208_Check_Bar_State(pAME_Data pAMEData,AME_U32 Raid_ID)
{
    AME_U32 SCARTCHPAD1;
    
    if (pAMEData->AME_RAIDData[Raid_ID].NTRAIDType != NTRAIDType_2208) {
        return TRUE;
    }
    
    SCARTCHPAD1 = AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,Raid_ID*ACS2208_BAR_OFFSET + AME_2208_3108_SCARTCHPAD1_REG_OFFSET);
    
    if (SCARTCHPAD1 != AME_HIM_CRTLRDY) {
        pAMEData->AME_RAIDData[Raid_ID].Raid_Error_Handle_State = TRUE;
        AME_Msg("Raid %d error, SCARTCHPAD1 = 0x%x\n",Raid_ID,SCARTCHPAD1);
        return FALSE;
    }
    
    return TRUE;
}

AME_U32 AME_NT_Raid_3108_Check_Bar_State(pAME_Data pAMEData,AME_U32 Raid_ID)
{
    AME_U32 SCARTCHPAD1;
    
    if (pAMEData->AME_RAIDData[Raid_ID].NTRAIDType != NTRAIDType_3108) {
        return TRUE;
    }
    
    SCARTCHPAD1 = AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,Raid_ID*ACS3108_BAR_OFFSET + AME_2208_3108_SCARTCHPAD1_REG_OFFSET);
    
    if (SCARTCHPAD1 != AME_HIM_CRTLRDY) {
        pAMEData->AME_RAIDData[Raid_ID].Raid_Error_Handle_State = TRUE;
        AME_Msg("Raid %d error, SCARTCHPAD1 = 0x%x\n",Raid_ID,SCARTCHPAD1);
        return FALSE;
    }
    
    return TRUE;
}

AME_U32 AME_Raid_2208_3108_Bar_lost_Check(pAME_Data pAMEData)
{
    // Only support 2208 DAS
    if ( (pAMEData->Device_ID != AME_2208_DID1_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID2_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID3_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID4_DAS) &&
         (pAMEData->Device_ID != AME_3108_DID0_DAS) &&
         (pAMEData->Device_ID != AME_3108_DID1_DAS))
    {
        return FALSE;
    }
    
    if (pAMEData->AME_RAIDData[0].Bar_address_lost == TRUE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

AME_U32 AME_Raid_2208_3108_Check_Bar_State(pAME_Data pAMEData)
{
    AME_U32 Reg_0x10,Reg_0x14;
        
    Reg_0x10 = AME_Raid_PCI_Config_Read(pAMEData,0x10);
    Reg_0x14 = AME_Raid_PCI_Config_Read(pAMEData,0x14);
    
    // Bug fixed:  In Mac OS sometime get 0xFFFFFFFF value, maybe get value failed, not bar lose issue.
    if ((Reg_0x10 == AME_U32_NONE)|| (Reg_0x14 == AME_U32_NONE)) {
        //AME_Msg_Err("(%d)AME_Raid_2208_3108_Check_Bar_State:  Reg_0x10(0x%x:0x%x) Reg_x014(0x%x:0x%x), get value error.\n",pAMEData->AME_Serial_Number,Reg_0x10,pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x10,Reg_0x14,pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x14);
        return TRUE;
    }
    
    if ((Reg_0x10 != pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x10) ||
        (Reg_0x14 != pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x14) ) {
        pAMEData->AME_RAIDData[0].Bar_address_lost = TRUE;
        AME_Msg_Err("(%d)AME 2208 3108 Raid Bar error:  Reg_0x10(0x%x:0x%x) Reg_x014(0x%x:0x%x), Start to error handle.\n",pAMEData->AME_Serial_Number,Reg_0x10,pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x10,Reg_0x14,pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x14);
        return FALSE;
    }
    
    return TRUE;
}

AME_U32 AME_Raid_2208_3108_Error_ReInit(pAME_Data pAMEData,AME_U32 AME_ReInit_State)
{
    AME_U32  InterruptReason = 0;
    static AME_U32  Wait_Index = 0;
    
    switch (AME_ReInit_State)
    {
        case 1:
            /* Disable AME message unit interrupt */
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_MASK_REGISTER, 0x0F);
            
            /* Clear Old reply address by Doorbell */
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Inbound_Doorbell_PORT,0xFFFFFFFF);
            
            Wait_Index = 0;
            return 2;
        
        case 2:
            /* Clear Outbound doorbell 0x10 for RAID check host alive issue */
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_Clear_PORT,0x10);
            
            /* Send High address by Doorbell */
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Inbound_Doorbell_PORT,(pAMEData->Glo_High_Address | 0x80000000));
            return 3;
            
        case 3:
            /* Wait Raid return Ready */
            InterruptReason = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_PORT);
            if (InterruptReason) {
                // AME_Raid_2208_ReInit OK
                /* Clear Door Bell value */
                AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_Clear_PORT,InterruptReason);
                /* Clear ScratchPad1 */
                AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_2208_3108_SCARTCHPAD1_REG_OFFSET,0x00);
                /* set AME_HOST_INT_MASK_REGISTER */
                AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_MASK_REGISTER, 0x01);
                return 4;
            }
            
            if (++Wait_Index>100) {
                // AME_Raid_2208_ReInit fail
                return 0xFF;
            }
            return 3;
        
        default:
            break;
    }
    
    AME_Msg_Err("(%d)AME_Raid_2208_3108_Error_ReInit: State Error, why go here ???\n",pAMEData->AME_Serial_Number);
    return FALSE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Raid_2208_3108_Link_Error_Check(pAME_Data pAMEData)
{
    AME_U32     Reg_0x04,SCARTCHPAD1,SCARTCHPAD2;
    
    /* Fixed 2208 3108 Raid Link error issue, not 2208 device return FALSE */
    if ( (pAMEData->Device_ID != AME_2208_DID1_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID2_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID3_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID4_DAS) &&
         (pAMEData->Device_ID != AME_3108_DID0_DAS) &&
         (pAMEData->Device_ID != AME_3108_DID1_DAS) ) {
        return FALSE;
    }

    if (AME_Raid_2208_3108_Check_Bar_State(pAMEData) == FALSE) {
        if (pAMEData->AME_RAIDData[0].Error_handle_State > 4) {
            pAMEData->AME_RAIDData[0].Error_handle_State = 0;
        } else {
            /* Keep Error_handle_State from step 0 to step 5 */
            pAMEData->AME_RAIDData[0].Error_handle_State++;
        }
    }
    
    switch (pAMEData->AME_RAIDData[0].Error_handle_State)
    {
        case 1:
            AME_Msg_Err("(%d)AME Error:  Wait Raid 2 sec to ready state...\n",pAMEData->AME_Serial_Number);
            break;
        case 2:
            AME_Msg_Err("(%d)AME Error:  Wait Raid 1 sec to ready state...\n",pAMEData->AME_Serial_Number);
            break;
            
        case 3:
            #if defined (AME_MAC)
                if (TRUE == AME_Host_check_TB()) {
                    AME_Msg_Err("(%d)AME Error:  In Thunderbolt mode don't hot reset...\n",pAMEData->AME_Serial_Number);
                    break;
                }
            #endif
            if (pAMEData->AME_LED_data.LED_Raid_Base_Address) {
                AME_Msg_Err("(%d)AME Error:  Handle do hot reset...\n",pAMEData->AME_Serial_Number);
                if ((pAMEData->Device_ID == AME_2208_DID3_DAS) || (pAMEData->Device_ID == AME_2208_DID4_DAS) || (pAMEData->Device_ID == AME_3108_DID0_DAS) || (pAMEData->Device_ID == AME_3108_DID1_DAS)) {  // B08S2-PS / A08S3-PS / A16S3-PS
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_Raid_Base_Address,0x003c, 0x00400112);
                    AME_IO_milliseconds_Delay(100);
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_Raid_Base_Address,0x003c, 0x00000112);
                } else {  // A12S2-PS/A16S2-PS
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_Raid_Base_Address,0x503c, 0x00400112);
                    AME_IO_milliseconds_Delay(100);
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_Raid_Base_Address,0x503c, 0x00000112);
                }
            } else {
                AME_Msg_Err("(%d)AME Error:  Raid_Base_Address is null, can't do hot reset...\n",pAMEData->AME_Serial_Number);
            }
            break;
        
        case 4:
            // Wait re-link to raid.
            break;
            
        case 5:
            if (AME_Raid_2208_3108_Check_Bar_State(pAMEData) == FALSE) {
                AME_Msg_Err("(%d)AME Error:  Fill Bar address ...\n",pAMEData->AME_Serial_Number);
                AME_Raid_PCI_Config_Write(pAMEData,0x10,pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x10);
                AME_Raid_PCI_Config_Write(pAMEData,0x14,pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x14);
                
               
                /* Clear Error Raid request */
                AME_Msg_Err("(%d)AME Error:  AME_NT_Cleanup_Error_Raid_Request ...\n",pAMEData->AME_Serial_Number);
                AME_NT_Cleanup_Error_Raid_Request(pAMEData,0);
            }
            
            Reg_0x04 = AME_Raid_PCI_Config_Read(pAMEData,0x04);
            if ((Reg_0x04&0x07) != 0x07) {
                AME_Msg_Err("(%d)AME Error:  Enable Bus master ...\n",pAMEData->AME_Serial_Number);
                AME_Raid_PCI_Config_Write(pAMEData,0x04,0x00100007);
            }
            
            SCARTCHPAD1 = AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_2208_3108_SCARTCHPAD1_REG_OFFSET);
            AME_Msg_Err("(%d)AME Error:  2208 3108 Raid error, wait Raid kernel restart, SCARTCHPAD1 = 0x%x.\n",pAMEData->AME_Serial_Number,SCARTCHPAD1);
            
            if (SCARTCHPAD1 == AME_HIM_CRTLRDY) {
                pAMEData->AME_RAIDData[0].AME_ReInit_State = 1;
                pAMEData->AME_RAIDData[0].Error_handle_State = 6;
                AME_Msg_Err("(%d)AME Error:  2208 3108 Raid error, Raid kernel restart done.\n",pAMEData->AME_Serial_Number);
                
                /* 2208 3108 New error handling : set ScratchPad2 = 0x0001ffff (version 0001, During Error Stage)*/
                AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_2208_3108_SCARTCHPAD2_REG_OFFSET,0x0001FFFF);
                }                     
            break;
            
        case 6:
            AME_Msg_Err("(%d)AME Error:  AME_Raid_ReInit state (0x%x)...\n",pAMEData->AME_Serial_Number,pAMEData->AME_RAIDData[0].AME_ReInit_State);
            pAMEData->AME_RAIDData[0].AME_ReInit_State = (AME_U32)AME_Raid_2208_3108_Error_ReInit(pAMEData,pAMEData->AME_RAIDData[0].AME_ReInit_State);
            
            if (pAMEData->AME_RAIDData[0].AME_ReInit_State == 4) {
                AME_Msg_Err("(%d)AME Error:  AME_Raid_ReInit Done\n",pAMEData->AME_Serial_Number);
                
                AME_Reply_Buffer_Init(pAMEData);
                AME_Msg("(%d)AME_Timer:  Send_Free_ReplyMsg Done\n",pAMEData->AME_Serial_Number);
                
                pAMEData->AME_RAIDData[0].Bar_address_lost = FALSE;
                pAMEData->AME_RAIDData[0].Error_handle_State = 0;
                return TRUE;
            } else if (pAMEData->AME_RAIDData[0].AME_ReInit_State == 0xFF) {
                AME_Msg_Err("(%d)AME Error:  AME_Raid_ReInit Fail, Retry Raid error handle again...\n",pAMEData->AME_Serial_Number);
                AME_Raid_PCI_Config_Write(pAMEData,0x10,0x00);
                AME_Raid_PCI_Config_Write(pAMEData,0x14,0x00);
                pAMEData->AME_RAIDData[0].Error_handle_State = 0;
            }
            break;
        
        default:
            break;
        
    }
    
    return FALSE;
}


AME_U32 AME_Raid_2208_3108_init(pAME_Data pAMEData)
{
   /* DTR_HandShake
    * Send Init message and High address to Raid kernel ,
    * and wait Raid return succused message.
    */
    
    /* Disable AME message unit interrupt */
    AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_MASK_REGISTER, 0x0F);
    AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_Clear_PORT,0x01);
    
    /* Clear Old reply address by Doorbell */
    AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Inbound_Doorbell_PORT,0xFFFFFFFF);
    AME_IO_milliseconds_Delay(1000); // Wait 1 sec
    
    /* 2208 3108 New error handling : Set SCARTCHPAD2 to 0x00010000.*/
    AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_2208_3108_SCARTCHPAD2_REG_OFFSET,0x00010000); 
    
    /* Clear Outbound doorbell 0x10 for RAID check host alive issue */
    AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_Clear_PORT,0x10);
    
    /* Send High address by Doorbell */
    AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Inbound_Doorbell_PORT,
                            (pAMEData->Glo_High_Address | 0x80000000));
     
    return TRUE;
}


AME_U32 AME_Check_RAID_2208_3108_Ready_Status(pAME_Data pAMEData)
{
    /* Wait Raid return Ready */

    AME_U32  InterruptReason;
    
    InterruptReason = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_PORT);

    if (InterruptReason) {
        AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Outbound_Doorbell_Clear_PORT,InterruptReason);
        AME_Msg("(%d)2208 3108 Raid is ready.\n",pAMEData->AME_Serial_Number);

        /* set AME_HOST_INT_MASK_REGISTER */
        #if defined (Enable_AME_RAID_R0)
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_MASK_REGISTER, 0x01);
        #else
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_MASK_REGISTER, 0x01);
        #endif
    
        //  DAS 2208 Raid Link Error handle ->
        pAMEData->AME_RAIDData[0].Bar_address_lost = FALSE;
        pAMEData->AME_RAIDData[0].Error_handle_State = FALSE;
        pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x10 = AME_Raid_PCI_Config_Read(pAMEData,0x10);
        pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x14 = AME_Raid_PCI_Config_Read(pAMEData,0x14);
        //AME_Msg("(%d)2208 Raid Bar Reg 0x10 = %x\n",pAMEData->AME_Serial_Number,pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x10);
        //AME_Msg("(%d)2208 Raid Bar Reg 0x14 = %x\n",pAMEData->AME_Serial_Number,pAMEData->AME_RAIDData[0].AME_Raid_Bar_Reg_0x14);
        //  DAS 2208 Raid Link Error handle <-   

        return TRUE;
    }
    else{
        AME_Msg("(%d)2208 Raid not ready!\n",pAMEData->AME_Serial_Number);
        return FALSE;
    }
    
}


AME_U32 AME_Raid_341_init(pAME_Data pAMEData)
{
   /* DTR_HandShake
    * Send Init message and High address to Raid kernel ,
    * and wait Raid return succused message.
    */
    
    /* Clear AME message unit interrupt */
    AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_STATUS_REGISTER, 0x01);
    
    /* Send High address by Doorbell */
    AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_MSG_REGISTER,pAMEData->Glo_High_Address);
    
    return TRUE;
}

AME_U32 AME_Check_RAID_341_Ready_Status(pAME_Data pAMEData)
{
    /* Wait Raid return Ready */

    AME_U32  InterruptReason;

    InterruptReason = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_STATUS_REGISTER);
    if (InterruptReason & AME_HIS_CRTLRDY_INTERRUPT) {

        AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_STATUS_REGISTER,0x01);
        AME_Msg("(%d)341 Raid is ready.\n",pAMEData->AME_Serial_Number);

         /* set AME_HOST_INT_MASK_REGISTER */
        AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_MASK_REGISTER, 0x17);
        return TRUE;
    }        

    else{
        AME_Msg("(%d)341 Raid not ready!\n",pAMEData->AME_Serial_Number);
        return FALSE;
    } 
   
}


AME_U32 AME_Check_RAID_Ready_Status(pAME_Data pAMEData)
{
    AME_U32     ReturnStatus;
    
    switch (pAMEData->Device_ID)
    {
        case AME_6101_DID_DAS:
        case AME_6201_DID_DAS:
            ReturnStatus = AME_Check_RAID_341_Ready_Status(pAMEData);
            break;
        
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            ReturnStatus = AME_Check_RAID_2208_3108_Ready_Status(pAMEData);
             break;
             
        case AME_8508_DID_NT:
        case AME_8608_DID_NT:
        case AME_87B0_DID_NT:
            ReturnStatus = TRUE; //NT always return TRUE
            break;

        default:
            AME_Msg_Err("(%d)Raid %x not support\n",pAMEData->AME_Serial_Number,pAMEData->Device_ID);
              ReturnStatus =  FALSE;
              break;
    }

     if (ReturnStatus == TRUE){
        pAMEData->AME_RAID_Ready = TRUE;
     }

     return ReturnStatus;
}


AME_U32 AME_Get_RAID_Ready(pAME_Data pAMEData)
{
    if (pAMEData->AME_RAID_Ready == TRUE){
        return TRUE;
    }
    else{
        return FALSE;
    }
}


/*******************************************************************************
 *
 * Function   :  AME_NT_PlxErrataWorkAround_NtBarShadow
 *
 * Description:  Implements work-around for NT BAR shadow errata
 *
 ******************************************************************************/
AME_U32 AME_NT_PlxErrataWorkAround_NtBarShadow(pAME_Data pAMEData)
{
    //AME_U8  i;
    //AME_U16 offset;
    AME_U32 RegValue;
    
    if ( (pAMEData->Device_ID != AME_8608_DID_NT) && 
         (pAMEData->Device_ID != AME_8508_DID_NT) ) {
        return TRUE;
    }

    /*****************************************************
     * This work-around handles the errata for NT Virtual
     * side BAR registers.  For BARs 2-5, the BAR space
     * is not accessible until the shadow registers are
     * updated manually.
     *
     * The procedure is to first read the BAR Setup registers
     * and write them back to themselves.  Then the PCI BAR
     * register is read and the same value is written back.
     * This updates the internal shadow registers of the PLX
     * chip and makes the BAR space accessible for NT data
     * transfers.
     *
     * Note: The BAR setup register must be written before
     *       the PCI BAR register.
     ****************************************************/

    AME_Msg("(%d)Implementing NT Virtual-side BAR shadow errata work-around...\n",pAMEData->AME_Serial_Number);
    
    // Write BAR Setup back to itself
    AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x100D4,0xFE000000);
    RegValue = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,0x100D4);
    AME_Msg("(%d)Errata work-around:  Bar2  0xD4 = %x\n",pAMEData->AME_Serial_Number,RegValue);
    
    // Read the corresponding PCI BAR
    RegValue = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,0x10018);
    AME_Msg("(%d)Errata work-around:  Bar2  0x18 = %x\n",pAMEData->AME_Serial_Number,RegValue);

    if (RegValue == 0) {// On MAC, when wakeup, this value may be 0, not restore by OS, so need restore by ourself
        if (pAMEData->ReInit == TRUE) {
            RegValue=pAMEData->AME_RestoreData.NT_Bar2_0x18;
            AME_Msg("(%d)Errata work-around:  restore Bar2  0x18 = %x\n",pAMEData->AME_Serial_Number,RegValue);
        }
        else {
            AME_Msg_Err("(%d)Errata work-around Error:  Bar2  0x18 = %x\n",pAMEData->AME_Serial_Number,RegValue);
            return FALSE;
        }
    }

    pAMEData->AME_RestoreData.NT_Bar2_0x18=RegValue;
    // Write PCI BAR back to itself
    AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x10018,RegValue);

    // Read & write-back BAR setup and BAR registers for BARs 2-5
    //for (i = 2; i <= 5; i++)
    //{
    //    // Set offset for NT BAR setup register (D4h = Virtual-side BAR 2 Setup)
    //    offset = 0x100D4 + ((i-2) * sizeof(AME_U32));
    //
    //    // Get Virtual-side BAR setup register
    //    RegValue = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,offset);
    //    AME_Msg("Errata work-around1:  offset %x  RegValue = %x, NT_Base_Address = %p\n",offset,RegValue,pAMEData->NT_Base_Address);
    //    
    //    // Check if BAR is enabled
    //    if (RegValue & (1 << 31))
    //    {
    //        AME_Msg("Errata work-around:  Bar%d  RegValue = %x\n",i,RegValue);
    //        
    //        // Write BAR Setup back to itself
    //        AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,offset,RegValue);
    //
    //        // Read the corresponding PCI BAR
    //        RegValue = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,0x10010 + (i*sizeof(AME_U32)));
    //
    //        // Write PCI BAR back to itself
    //        AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x10010 + (i*sizeof(AME_U32)),RegValue);
    //        
    //    }
    //}
    
    return TRUE;
}



AME_U32 AME_NT_8508_init(pAME_Data pAMEData)
{
    /* PlxErrataWorkAround_NtBarShadow */
    AME_NT_PlxErrataWorkAround_NtBarShadow(pAMEData);
    
    pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_REG = NT_VIRTUAL_SET_VIRTUAL_IRQ_OFFSET_8508;
    pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG = NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_OFFSET_8508;
    pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_MASK_REG = NT_VIRTUAL_SET_VIRTUAL_IRQ_MASK_OFFSET_8508;
    pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_MASK_REG = NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_MASK_OFFSET_8508;
    
    pAMEData->AME_NTHost_data.NT_SET_Link_IRQ_REG = NT_VIRTUAL_SET_Link_IRQ_OFFSET_8508;
    pAMEData->AME_NTHost_data.NT_CLEAR_Link_IRQ_REG = NT_VIRTUAL_CLEAR_Link_IRQ_OFFSET_8508;
    pAMEData->AME_NTHost_data.NT_SET_Link_IRQ_MASK_REG = NT_VIRTUAL_SET_Link_IRQ_MASK_OFFSET_8508;
    pAMEData->AME_NTHost_data.NT_CLEAR_Link_IRQ_MASK_REG = NT_VIRTUAL_CLEAR_Link_IRQ_MASK_OFFSET_8508;
    
    pAMEData->AME_NTHost_data.NT_LINK_LINKSTATE_REG_OFFSET = NT_LINK_LINKSTATE_REG_OFFSET_8508;
    pAMEData->AME_NTHost_data.NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET = NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET_8508;
    
    pAMEData->AME_NTHost_data.Read_Index = 0;
    pAMEData->AME_NTHost_data.Hotplug_State = 0;
    pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index = 0;
    
    pAMEData->AME_NTHost_data.SCRATCH_REG_0 = NT_VIRTUAL_SCARTCHPAD0_REG_OFFSET_8508;
    pAMEData->AME_NTHost_data.SCRATCH_REG_1 = NT_VIRTUAL_SCARTCHPAD1_REG_OFFSET_8508;
    pAMEData->AME_NTHost_data.SCRATCH_REG_2 = NT_VIRTUAL_SCARTCHPAD2_REG_OFFSET_8508;
    pAMEData->AME_NTHost_data.SCRATCH_REG_3 = NT_VIRTUAL_SCARTCHPAD3_REG_OFFSET_8508;
    pAMEData->AME_NTHost_data.SCRATCH_REG_4 = NT_VIRTUAL_SCARTCHPAD4_REG_OFFSET_8508;
    pAMEData->AME_NTHost_data.SCRATCH_REG_5 = NT_VIRTUAL_SCARTCHPAD5_REG_OFFSET_8508;
    pAMEData->AME_NTHost_data.SCRATCH_REG_6 = NT_VIRTUAL_SCARTCHPAD6_REG_OFFSET_8508;
    pAMEData->AME_NTHost_data.SCRATCH_REG_7 = NT_VIRTUAL_SCARTCHPAD7_REG_OFFSET_8508;
    
    return TRUE;
}

/***************************************************************
 * Function:  AME_NT_8608_Lookup_Table_Init
 * Description:
 *      Setting 8608 need bus,device and function number
 *      ythuang: Addition setting for PLX8608
 *      From experimental: we should set Lookup table for all ACS62s
 *      Even we don't need read transaction of ACS62 from driver
 *      How to get Request ID: Capture the requestID from header log
 *      Theory: Create an UR PCIe request and the UR reqeustID will be 
 *      recorded in packet header log (offset 0xFD0 ~ 0xFDC)
 ***************************************************************/
AME_U32 AME_NT_8608_Lookup_Table_Init(pAME_Data pAMEData)
{
    AME_U32 i,tmpreg;
    
    if (pAMEData->Device_ID != AME_8608_DID_NT) {
        return FALSE;
    }
    
    //1. Clear all erorr on 0xFB8 by write clear
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,0x10FB8);
    AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x10FB8,tmpreg);
    
    // Enable unsupport request error report Mask.
    // In Dell R720xd, unmask unsupport request error report result Win OS to panic.
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,0x10FBC);
    AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x10FBC,0x100000);
    
    //2. Fire an unsupport request (must write multiple times)
    for (i=0; i<5; i++) {
        AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,0x200,tmpreg);
    }
    
    //3. Check if UR error happened
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,0x10FB8);
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,0x10FBC);
       
    //4. Check request ID in head log 0xFD4 [31:16]
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,0x10FD4);
    tmpreg = ( 0x80000000 | (tmpreg>>16) );
    
    //5. Fill the request ID in MMIO register offset 0xD94
    AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x10D94,tmpreg);
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,0x10D94);
    AME_Msg("(%d)Fill Request ID 0x%08x in Look up table (0xD94)\n",pAMEData->AME_Serial_Number,tmpreg);
    
    if (tmpreg == AME_U32_NONE) {
        AME_Msg_Err("(%d)NT Epprom init Error!\n",pAMEData->AME_Serial_Number);
        return FALSE;
    }
    
    return TRUE;
}

/***************************************************************
 * Function:  AME_NT_8608_Change_Link_to_Gen2
 * Description:
 *      Old version 8608 NT Card , default Link is Gen 1, need driver to pull up
 ***************************************************************/
void AME_NT_8608_Change_Link_to_Gen2(pAME_Data pAMEData)
{
    AME_U32 Link_Side_Link_State = 0;
    
    if (pAMEData->Device_ID == AME_8608_DID_NT)
    {
        Link_Side_Link_State = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_LINK_LINKSTATE_REG_OFFSET);
    
        if (! (Link_Side_Link_State & 0x20000) ) {
            AME_Msg("(%d)Try to pull up 8608 NT Card Link from Gen1 to Gen2.\n",pAMEData->AME_Serial_Number);
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x98,0x2);
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x10098,0x2);
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x11098,0x2);
        }
    }
    
    return;
}

AME_U32 AME_NT_8608_init(pAME_Data pAMEData)
{
    /* PlxErrataWorkAround_NtBarShadow */
    AME_NT_PlxErrataWorkAround_NtBarShadow(pAMEData);
    
    /* Fill the request ID in MMIO register offset 0xD94 */
    if (AME_NT_8608_Lookup_Table_Init(pAMEData) == FALSE) {
        return FALSE;
    }
    
    pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_REG = NT_VIRTUAL_SET_VIRTUAL_IRQ_OFFSET_8608;
    pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG = NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_OFFSET_8608;
    pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_MASK_REG = NT_VIRTUAL_SET_VIRTUAL_IRQ_MASK_OFFSET_8608;
    pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_MASK_REG = NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_MASK_OFFSET_8608;
    
    pAMEData->AME_NTHost_data.NT_SET_Link_IRQ_REG = NT_VIRTUAL_SET_Link_IRQ_OFFSET_8608;
    pAMEData->AME_NTHost_data.NT_CLEAR_Link_IRQ_REG = NT_VIRTUAL_CLEAR_Link_IRQ_OFFSET_8608;
    pAMEData->AME_NTHost_data.NT_SET_Link_IRQ_MASK_REG = NT_VIRTUAL_SET_Link_IRQ_MASK_OFFSET_8608;
    pAMEData->AME_NTHost_data.NT_CLEAR_Link_IRQ_MASK_REG = NT_VIRTUAL_CLEAR_Link_IRQ_MASK_OFFSET_8608;
    
    pAMEData->AME_NTHost_data.NT_LINK_LINKSTATE_REG_OFFSET = NT_LINK_LINKSTATE_REG_OFFSET_8608;
    pAMEData->AME_NTHost_data.NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET = NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET_8608;
    
    pAMEData->AME_NTHost_data.Read_Index = 0;
    pAMEData->AME_NTHost_data.Hotplug_State = 0;
    pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index = 0;
    
    pAMEData->AME_NTHost_data.SCRATCH_REG_0 = NT_VIRTUAL_SCARTCHPAD0_REG_OFFSET_8608;
    pAMEData->AME_NTHost_data.SCRATCH_REG_1 = NT_VIRTUAL_SCARTCHPAD1_REG_OFFSET_8608;
    pAMEData->AME_NTHost_data.SCRATCH_REG_2 = NT_VIRTUAL_SCARTCHPAD2_REG_OFFSET_8608;
    pAMEData->AME_NTHost_data.SCRATCH_REG_3 = NT_VIRTUAL_SCARTCHPAD3_REG_OFFSET_8608;
    pAMEData->AME_NTHost_data.SCRATCH_REG_4 = NT_VIRTUAL_SCARTCHPAD4_REG_OFFSET_8608;
    pAMEData->AME_NTHost_data.SCRATCH_REG_5 = NT_VIRTUAL_SCARTCHPAD5_REG_OFFSET_8608;
    pAMEData->AME_NTHost_data.SCRATCH_REG_6 = NT_VIRTUAL_SCARTCHPAD6_REG_OFFSET_8608;
    pAMEData->AME_NTHost_data.SCRATCH_REG_7 = NT_VIRTUAL_SCARTCHPAD7_REG_OFFSET_8608;
    
    return TRUE;
}


/***************************************************************
 * Function:  AME_NT_8717_Lookup_Table_Init
 * Description:
 *      Setting 8717 need bus,device and function number
 *      ythuang: Addition setting for PLX8717
 *      From experimental: we should set Lookup table for all ACS62s
 *      Even we don't need read transaction of ACS62 from driver
 *      How to get Request ID: Capture the requestID from header log
 *      Theory: Create an UR PCIe request and the UR reqeustID will be 
 *      recorded in packet header log (offset 0xFD0 ~ 0xFDC)
 ***************************************************************/
AME_U32 AME_NT_8717_Lookup_Table_Init(pAME_Data pAMEData)
{
    AME_U32  i,tmpreg,tmpreg_0xC90,Vir_Offset,Link_Offset;
    
    if (pAMEData->Device_ID != AME_87B0_DID_NT) {
        return FALSE;
    }
    
    if ((pAMEData->AME_NTHost_data.Port_Number == 1)) {
        Vir_Offset  = 0x3E000;
        Link_Offset = 0x3F000;
    } else {
        Vir_Offset  = 0x3C000;
        Link_Offset = 0x3D000;
    }
    
    //1. Clear all erorr on 0xFB8 by write clear
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,(Vir_Offset+0xFB8));
    AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,(Vir_Offset+0xFB8),tmpreg);
    
    // Enable unsupport request error report Mask.
    // In Dell R720xd, unmask unsupport request error report result Win OS to panic.
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,(Vir_Offset+0xFBC));
    AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,(Vir_Offset+0xFBC),0x100000);
    
    //2. Fire an unsupport request (must write multiple times)
    for (i=0; i<5; i++) {
        AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,0x200,tmpreg);
    }
    
    //3. Check if UR error happened
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,(Vir_Offset+0xFB8));
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,(Vir_Offset+0xFBC));
       
    //4. Check request ID in head log 0xFD4 [31:16]
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,(Vir_Offset+0xFD4));
    //tmpreg = ( (tmpreg&0xFFFF0000) | (0x01<<16) );
    tmpreg = (tmpreg>>16) | 0x01;
    
    //5. Get chip support Request ID from link-side 0xC90
    tmpreg_0xC90 = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,(Link_Offset+0xC90));
    tmpreg_0xC90 &= 0xFFFF;
    tmpreg_0xC90 |= 0x01;
    
    //Stop used PLX support Request ID from link-side 0xC90.
    //Some case result to OS panic.
    //tmpreg = tmpreg | (tmpreg_0xC90<<16);
    
    //6. Fill the request ID in MMIO register offset 0xD94
    AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,(Vir_Offset+0xD94),tmpreg);
    tmpreg = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,(Vir_Offset+0xD94));

    AME_Msg("(%d)NT: Init NT 87B0 Port %d, 0xD94 Request ID = 0x%08x\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.Port_Number,tmpreg);
    
    if (tmpreg == AME_U32_NONE) {
        AME_Msg_Err("(%d)NT Epprom init Error!\n",pAMEData->AME_Serial_Number);
        return FALSE;
    }
    
    return TRUE;
}

AME_U32 AME_NT_87B0_init(pAME_Data pAMEData)
{

    // Workaround fixed HBA 8717 PCIe link phase lock issue
    AME_NT_87B0_Workaround(pAMEData);
    
    /* Fill the request ID in MMIO register offset 0xD94 0xD98 */
    AME_NT_8717_Lookup_Table_Init(pAMEData);
    
    if (pAMEData->AME_NTHost_data.Port_Number == 1)
    {
        pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_REG = NT_VIRTUAL_SET_VIRTUAL_IRQ_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG = NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_MASK_REG = NT_VIRTUAL_SET_VIRTUAL_IRQ_MASK_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_MASK_REG = NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_MASK_OFFSET_87B0_P0;
        
        pAMEData->AME_NTHost_data.NT_SET_Link_IRQ_REG = NT_VIRTUAL_SET_Link_IRQ_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.NT_CLEAR_Link_IRQ_REG = NT_VIRTUAL_CLEAR_Link_IRQ_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.NT_SET_Link_IRQ_MASK_REG = NT_VIRTUAL_SET_Link_IRQ_MASK_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.NT_CLEAR_Link_IRQ_MASK_REG = NT_VIRTUAL_CLEAR_Link_IRQ_MASK_OFFSET_87B0_P0;
        
        pAMEData->AME_NTHost_data.NT_LINK_LINKSTATE_REG_OFFSET = NT_LINK_LINKSTATE_REG_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET = NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET_87B0_P0;
        
        pAMEData->AME_NTHost_data.Read_Index = 0;
        pAMEData->AME_NTHost_data.Hotplug_State = 0;
        pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index = 0;
        
        pAMEData->AME_NTHost_data.SCRATCH_REG_0 = NT_VIRTUAL_SCARTCHPAD0_REG_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.SCRATCH_REG_1 = NT_VIRTUAL_SCARTCHPAD1_REG_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.SCRATCH_REG_2 = NT_VIRTUAL_SCARTCHPAD2_REG_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.SCRATCH_REG_3 = NT_VIRTUAL_SCARTCHPAD3_REG_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.SCRATCH_REG_4 = NT_VIRTUAL_SCARTCHPAD4_REG_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.SCRATCH_REG_5 = NT_VIRTUAL_SCARTCHPAD5_REG_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.SCRATCH_REG_6 = NT_VIRTUAL_SCARTCHPAD6_REG_OFFSET_87B0_P0;
        pAMEData->AME_NTHost_data.SCRATCH_REG_7 = NT_VIRTUAL_SCARTCHPAD7_REG_OFFSET_87B0_P0;
    }
    else { // Port 2
        
        pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_REG = NT_VIRTUAL_SET_VIRTUAL_IRQ_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG = NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_MASK_REG = NT_VIRTUAL_SET_VIRTUAL_IRQ_MASK_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_MASK_REG = NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_MASK_OFFSET_87B0_P1;
        
        pAMEData->AME_NTHost_data.NT_SET_Link_IRQ_REG = NT_VIRTUAL_SET_Link_IRQ_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.NT_CLEAR_Link_IRQ_REG = NT_VIRTUAL_CLEAR_Link_IRQ_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.NT_SET_Link_IRQ_MASK_REG = NT_VIRTUAL_SET_Link_IRQ_MASK_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.NT_CLEAR_Link_IRQ_MASK_REG = NT_VIRTUAL_CLEAR_Link_IRQ_MASK_OFFSET_87B0_P1;
        
        pAMEData->AME_NTHost_data.NT_LINK_LINKSTATE_REG_OFFSET = NT_LINK_LINKSTATE_REG_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET = NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET_87B0_P1;
        
        pAMEData->AME_NTHost_data.Read_Index = 0;
        pAMEData->AME_NTHost_data.Hotplug_State = 0;
        pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index = 0;
        
        pAMEData->AME_NTHost_data.SCRATCH_REG_0 = NT_VIRTUAL_SCARTCHPAD0_REG_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.SCRATCH_REG_1 = NT_VIRTUAL_SCARTCHPAD1_REG_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.SCRATCH_REG_2 = NT_VIRTUAL_SCARTCHPAD2_REG_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.SCRATCH_REG_3 = NT_VIRTUAL_SCARTCHPAD3_REG_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.SCRATCH_REG_4 = NT_VIRTUAL_SCARTCHPAD4_REG_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.SCRATCH_REG_5 = NT_VIRTUAL_SCARTCHPAD5_REG_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.SCRATCH_REG_6 = NT_VIRTUAL_SCARTCHPAD6_REG_OFFSET_87B0_P1;
        pAMEData->AME_NTHost_data.SCRATCH_REG_7 = NT_VIRTUAL_SCARTCHPAD7_REG_OFFSET_87B0_P1;
        
    }
    
    return TRUE;
}




AME_U32 AME_Raid_Init(pAME_Data pAMEData)
{
    AME_U32     returnstatus = FALSE;
    
    switch (pAMEData->Device_ID)
    {
        case AME_6101_DID_DAS:
        case AME_6201_DID_DAS:
            returnstatus = AME_Raid_341_init(pAMEData);
            break;
        
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            returnstatus = AME_Raid_2208_3108_init(pAMEData);
            break;
            
        case AME_8508_DID_NT:
            returnstatus = AME_NT_8508_init(pAMEData);
            break;
            
        case AME_8608_DID_NT:
            returnstatus = AME_NT_8608_init(pAMEData);
            break;
            
        case AME_87B0_DID_NT:
            returnstatus = AME_NT_87B0_init(pAMEData);
            break;
            
        default:
            AME_Msg_Err("(%d)Raid %x not support\n",pAMEData->AME_Serial_Number,pAMEData->Device_ID);
            break;
    }
    
    return returnstatus;
}


void AME_LED_Start(pAME_Data pAMEData)
{
    AME_U32  Temp_Reg = 0;
    
    if (pAMEData->AME_LED_data.LED_HBA_Base_Address == NULL) {
        return;
    }
    
    switch (pAMEData->Device_ID)
    {
        case AME_87B0_DID_NT:
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            switch (pAMEData->AME_LED_data.LED_HBA_Type)
            {
                case PLX_Brige_8608:
                    /* Enable HBA gpio 29 direction */
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x630, 0x8000000);
                    /* Off HBA LED */
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x648, 0x2000);
                    break;
                    
                case PLX_Brige_8717:
                    /* Enable HBA gpio 29 direction */
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x600, 0x24B6C96D);
                    /* Off HBA LED */
                    Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x624);
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x624, Temp_Reg|0x300);
                    break;
                    
                default:
                    break;
            }
            break;
        
        case AME_8608_DID_NT:
            /* Enable HBA gpio 29 direction */
            AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x630, 0x8000000);
            /* Off HBA LED */
            AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x648, 0x2000);
            break;
            
        default:
            break;
            
    }

    return;    
}

void AME_LED_ON(pAME_Data pAMEData)
{
    AME_U32  Temp_Reg = 0;
    
    if (pAMEData->AME_LED_data.LED_HBA_Base_Address == NULL) {
        return;
    }
    
    switch (pAMEData->Device_ID)
    {
        case AME_87B0_DID_NT:
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            /* Access HBA LED */
            switch (pAMEData->AME_LED_data.LED_HBA_Type)
            {
                case PLX_Brige_8608:
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x648, 0x00);
                    break;
                
                case PLX_Brige_8717:
                    Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x624);
                    if (pAMEData->AME_LED_data.LED_HBA_Port_Number == 1){
                        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x624,Temp_Reg&0xFFFFFEFF);
                    } else {
                        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x624,Temp_Reg&0xFFFFFDFF);
                    }
                    break;
                    
                default:
                    break;
            }
            break;
        
        case AME_8608_DID_NT:
            /* Access HBA LED */
            AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x648, 0x00);
            break;
            
        default:
            break;
    }
    
    return;
}

void AME_LED_OFF(pAME_Data pAMEData)
{
    AME_U32  Temp_Reg = 0;
    
    if (pAMEData->AME_LED_data.LED_HBA_Base_Address == NULL) {
        return;
    }
    
    switch (pAMEData->Device_ID)
    {
        case AME_87B0_DID_NT:
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            /* Off HBA LED */
            switch (pAMEData->AME_LED_data.LED_HBA_Type)
            {
                case PLX_Brige_8608:
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x648, 0x2000);
                    break;
                
                case PLX_Brige_8717:
                    Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x624);
                    if (pAMEData->AME_LED_data.LED_HBA_Port_Number == 1){
                        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x624,Temp_Reg|0x100);
                    } else {
                        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x624,Temp_Reg|0x200);
                    }
                    break;
                    
                default:
                    break;
            }
            break;
            
        case AME_8608_DID_NT:
            /* Off HBA LED */
            AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x648, 0x2000);
            break;
            
        default:
            break;
    }
    
    return;
}

void AME_LED_Stop(pAME_Data pAMEData)
{
    AME_U32  Temp_Reg = 0;
    
    if (pAMEData->AME_LED_data.LED_HBA_Base_Address == NULL) {
        return;
    }
    
    switch (pAMEData->Device_ID)
    {
        case AME_87B0_DID_NT:
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            /* Off HBA LED */
            switch (pAMEData->AME_LED_data.LED_HBA_Type)
            {
                case PLX_Brige_8608:
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x648, 0x2000);
                    break;
                    
                case PLX_Brige_8717:
                    Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x624);
                    AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x624, Temp_Reg|0x300);
                    break;
                    
                default:
                    break;
            }
            break;
            
        case AME_8608_DID_NT:
            /* Off HBA LED */
            AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x648, 0x2000);
            break;
            
        default:
            break;
            
    }
    
    return;
}


/***************************************************************
 * Function:  AME_HBA_Link_Error_check
 * Description:
 *   1. Fixed Z2M Gen3 HBA 8717 up-stream link downgrade 
 *      to Gen1 issue in Mac OS.
 *   2. Fixed in Raid 2208, HBA link downgrade issue.
 *      Retry 5 times to retrain link to normal link state.
 *      Support DAS HBA Bridge 8608,8717
 ***************************************************************/
void AME_HBA_Link_Error_check(pAME_Data pAMEData)
{
    AME_U16     retry_index = 0;
    AME_U32     Temp_Reg = 0;
    AME_U32     Lanes_Set_bits = 0;
    AME_U32     Lanes_Set_shift_bits = 0;
    AME_U32     Reset_lanes_Reg = 0;
    AME_U32     Retrain_Link_Reg = 0;
    AME_U32     Link_Status_Control_Reg = 0;
    
    if (pAMEData->AME_LED_data.LED_HBA_Base_Address == NULL) {
        return;
    }
    
    // Fixed Z2M Gen3 HBA 8717 up-stream link downgrade to Gen1 issue in Mac OS.
    #ifdef AME_MAC
        if (pAMEData->AME_LED_data.LED_HBA_Type == PLX_Brige_8717) {
            
            Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x78);
            
            if (Temp_Reg == 0x00810000) {
                
                AME_U32 Temp_Bus,Temp_Slot,Temp_Reg_0x18;
                AME_U32 Temp_PriBus,Temp_SecBus,Temp_SubBus;
                AME_U32 Device_PriBus,Device_SecBus,Device_SubBus,Find_Target = FALSE;
                
                AME_Msg("(%d)AME_HBA_Link_check upstream link error , current link Gen %d  Lanes %d , retry workaround ...\n",pAMEData->AME_Serial_Number,(Temp_Reg>>16)&0xF,(Temp_Reg>>20)&0xF);
                
                Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x18);
                Device_PriBus = (Temp_Reg&0xFF);
                Device_SecBus = (Temp_Reg>>8)&0xFF;
                Device_SubBus = (Temp_Reg>>16)&0xFF;
                //AME_Msg("AME MSG:  Device_PriBus = %x, Device_SecBus = %x, Device_SubBus = %x *************************\n",Device_PriBus,Device_SecBus,Device_SubBus);
                
                for (Temp_Bus=0;Temp_Bus<pAMEData->Device_BusNumber;Temp_Bus++) {
                    
                    for (Temp_Slot=0;Temp_Slot<32;Temp_Slot++) {
                        
                        Temp_Reg_0x18 = AME_PCI_Config_Read_32(pAMEData,Temp_Bus,Temp_Slot,0,0x18);
                        Temp_PriBus = (Temp_Reg_0x18&0xFF);
                        Temp_SecBus = (Temp_Reg_0x18>>8)&0xFF;
                        Temp_SubBus = (Temp_Reg_0x18>>16)&0xFF;
                        
                        if ((Device_PriBus == Temp_SecBus) && (Device_SubBus == Temp_SubBus)) {
                            Find_Target = TRUE;
                            //AME_Msg("AME MSG:  Temp_Bus = %x, Temp_Slot = %x, Temp_PriBus = %x, Temp_SecBus = %x, Temp_SubBus = %x ********************\n",Temp_Bus,Temp_Slot,Temp_PriBus,Temp_SecBus,Temp_SubBus);
                            break;
                        }
                        
                    }
                    
                    if (Find_Target == TRUE) {
                        
                        AME_U32 Capability_ID,Capability_Version,Next_Point;
                        
                        Temp_Reg = AME_PCI_Config_Read_32(pAMEData,Temp_Bus,Temp_Slot,0,0x34);
                        Next_Point = Temp_Reg&0xFF;
                        
                        while (1) {
                            
                            Temp_Reg = AME_PCI_Config_Read_32(pAMEData,Temp_Bus,Temp_Slot,0,Next_Point);
                            Capability_ID = Temp_Reg&0xFF;
                            Capability_Version = (Temp_Reg>>16)&0xF;
                            
                            // Capability_ID 0x10 is PCI Express Capability List Register
                            // Make sure Capability_Version is 2.
                            if ((Capability_ID == 0x10) && (Capability_Version == 0x02)) {
                                
                                // Change target link to Gen2
                                Temp_Reg = AME_PCI_Config_Read_32(pAMEData,Temp_Bus,Temp_Slot,0,Next_Point+0x30);
                                Temp_Reg = Temp_Reg&(~0xF)|0x02;
                                AME_PCI_Config_Write_32(pAMEData,Temp_Bus,Temp_Slot,0,Next_Point+0x30,Temp_Reg);
                                //AME_Msg("AME MSG:  Temp_Bus = %x, Temp_Slot = %x, Next_Point+0x30 = %x, value = %x ******************************\n",Temp_Bus,Temp_Slot,Next_Point+0x30,Temp_Reg);
                                
                                // Retrain link
                                Temp_Reg = AME_PCI_Config_Read_32(pAMEData,Temp_Bus,Temp_Slot,0,Next_Point+0x10);
                                Temp_Reg = Temp_Reg|0x20;
                                AME_PCI_Config_Write_32(pAMEData,Temp_Bus,Temp_Slot,0,Next_Point+0x10,Temp_Reg);
                                
                                Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,0x78);
                                AME_Msg("AME MSG:  Temp_Bus = %x, Temp_Slot = %x, Next_Point+0x30 = %x, value = %x, driver modify done ...\n",Temp_Bus,Temp_Slot,Next_Point+0x30,Temp_Reg);
                                
                            }
                            
                            Next_Point = (Temp_Reg>>8)&0xFF;
                            
                            if (Next_Point == 0x00) {
                                break;
                            }
                            
                        }
                        
                        break;
                    }
                    
                }
                
            }
            
        }
    #endif
    
    // Only support 2208 DAS
    // Fixed HBA down-stream link lanes error.
    if ( (pAMEData->Device_ID != AME_2208_DID1_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID2_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID3_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID4_DAS) )
    {
        return;
    }
    
    switch (pAMEData->AME_LED_data.LED_HBA_Type)
    {
        case PLX_Brige_8608:
            Lanes_Set_bits = 0xF0000;
            Lanes_Set_shift_bits = 16;
            Reset_lanes_Reg = 0x254;
            Retrain_Link_Reg = 0x1078;
            Link_Status_Control_Reg = 0x1098;
            break;
        
        case PLX_Brige_8717:
            if (pAMEData->AME_LED_data.LED_HBA_Port_Number == 1) {
                Lanes_Set_bits = 0xF00;
                Lanes_Set_shift_bits = 8;
                Reset_lanes_Reg = 0x248;
                Retrain_Link_Reg = 0x1078;
                Link_Status_Control_Reg = 0x1098;
            } else {
                Lanes_Set_bits = 0x70000;
                Lanes_Set_shift_bits = 16;
                Reset_lanes_Reg = 0x248;
                Retrain_Link_Reg = 0x2078;
                Link_Status_Control_Reg = 0x2098;
            }
            break;
            
        default:
            //AME_Msg("AME_HBA_Link_Error_check Unsupport Device %x...\n",pAMEData->AME_LED_data.LED_HBA_Type);
            return;
    }
    
    while (retry_index++ < 5) {
        
        Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,Retrain_Link_Reg);
        Temp_Reg = (Temp_Reg>>16)& 0xFF;
        
        if ((Temp_Reg == 0x42)||
            (Temp_Reg == 0x43) ) {
            /* Link correct */
            AME_Msg("(%d)AME_HBA_Link_check OK , Gen %d  Lanes 4 ...\n",pAMEData->AME_Serial_Number,Temp_Reg&0xF);
            break;
        }
        
        AME_Msg("(%d)AME_HBA_Link_check error: %d times Link state = Gen %d  Lanes %d ...\n",pAMEData->AME_Serial_Number,retry_index,Temp_Reg&0xF,(Temp_Reg>>4)&0xF);
        
        /* Downgrade to Gen 1 lanes 4 */
        Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,Reset_lanes_Reg);
        Temp_Reg = Temp_Reg & (~Lanes_Set_bits);
        Temp_Reg = Temp_Reg | (4<<Lanes_Set_shift_bits);
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,Reset_lanes_Reg, Temp_Reg);
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,Link_Status_Control_Reg, 0x01);
        AME_Msg("(%d)AME_HBA_Link_check error:  Set Gen1 Retrain_Link_Reg = %08x ...\n",pAMEData->AME_Serial_Number,Temp_Reg);
        
        Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,Retrain_Link_Reg);
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,Retrain_Link_Reg, (Temp_Reg|0x20) );
        AME_IO_milliseconds_Delay(2000);
        
        /* Upgrade to Gen 2 lanes 4 */
        Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,Reset_lanes_Reg);
        Temp_Reg = Temp_Reg & (~Lanes_Set_bits);
        Temp_Reg = Temp_Reg | (4<<Lanes_Set_shift_bits);
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,Reset_lanes_Reg, Temp_Reg);
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,Link_Status_Control_Reg, 0x02);
        AME_Msg("(%d)AME_HBA_Link_check error:  Set Gen2 Retrain_Link_Reg = %08x ...\n",pAMEData->AME_Serial_Number,Temp_Reg);
        
        Temp_Reg = AME_Address_read_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,Retrain_Link_Reg);
        AME_Address_Write_32(pAMEData,pAMEData->AME_LED_data.LED_HBA_Base_Address,Retrain_Link_Reg, (Temp_Reg|0x20) );
        AME_IO_milliseconds_Delay(2000);

    }
    
    return;
}


AME_U32 AME_Find_Upstream_Bridge(pAME_Data pAMEData,AME_U32 Target_Bus,AME_U32 *pUpstreamt_Bus,AME_U32 *pUpstream_Slot)
{
    AME_U32 PriBus,SecBus,SubBus;
    AME_U32 Temp_Bus,Temp_Slot;
    AME_U32 Temp_Reg_0x0C = 0;
    AME_U32 Temp_Reg_0x18 = 0;
    AME_U32 Temp_DeviceID = 0;
    
    for (Temp_Bus=(Target_Bus-1);Temp_Bus>0;Temp_Bus--) {
        
        for (Temp_Slot=0;Temp_Slot<32;Temp_Slot++) {
            
            // PCIE reg bits 23-16 Header Type - 00h Standard Header - 01h PCI-to-PCI Bridge - 02h CardBus Bridge
            Temp_Reg_0x0C = AME_PCI_Config_Read_32(pAMEData,Temp_Bus,Temp_Slot,0,0x0C);
            if (((Temp_Reg_0x0C>>16)&0xFF) != 0x01) {
                continue;
            }
            
            Temp_Reg_0x18 = AME_PCI_Config_Read_32(pAMEData,Temp_Bus,Temp_Slot,0,0x18);
            PriBus = (Temp_Reg_0x18&0xFF);
            SecBus = (Temp_Reg_0x18>>8)&0xFF;
            SubBus = (Temp_Reg_0x18>>16)&0xFF;
            
            if (Target_Bus == SecBus) {
                *pUpstreamt_Bus = Temp_Bus;
                *pUpstream_Slot = Temp_Slot;
                //AME_Msg("AME MSG: Find DeviceID = %x Bus = %x Slot = %x PriBus = %x SecBus = %x SubBus = %x\n",Temp_Reg_0x00,*pUpstreamt_Bus,*pUpstream_Slot,PriBus,SecBus,SubBus);
                //AME_Msg("AME MSG: Find upstream Target Bridge Upstreamt_Bus = %x,Upstream_Slot = %x\n",*pUpstreamt_Bus,*pUpstream_Slot);
                return TRUE;
            }
            
        }
        
    }
    
    return FALSE;
}


AME_U32 AME_Find_HBA_Bridge_NT(pAME_Data pAMEData)
{
    AME_U32 Bridge_Bus,Bridge_Slot;
    AME_U32 Temp_Bus,Temp_Slot,Temp_Reg_0x00;
    
    if (TRUE == AME_Find_Upstream_Bridge(pAMEData,pAMEData->Device_BusNumber,&Temp_Bus,&Temp_Slot)) {
        
        Bridge_Bus = Temp_Bus;
        
        Temp_Reg_0x00 = AME_PCI_Config_Read_32(pAMEData,Temp_Bus,Temp_Slot,0,0x00);
        
        switch (Temp_Reg_0x00) {
            case PLX_Brige_8508:
            case PLX_Brige_8608:
            case PLX_Brige_8717:
                pAMEData->AME_LED_data.LED_HBA_Type = Temp_Reg_0x00;
                pAMEData->AME_LED_data.LED_HBA_Port_Number = Temp_Slot;
                break;
                
            default:
                AME_Msg("AME MSG: unknow HBA Bridge Type = %x\n",Temp_Reg_0x00);
                return FALSE;
        }
        
        if (TRUE == AME_Find_Upstream_Bridge(pAMEData,Bridge_Bus,&Temp_Bus,&Temp_Slot)) {
            Bridge_Bus = Temp_Bus;
            Bridge_Slot = Temp_Slot;
            pAMEData->AME_LED_data.LED_HBA_Base_Address = AME_IOMapping_Bridge(pAMEData,Bridge_Bus,Bridge_Slot,0);
            pAMEData->NT_Base_Address = pAMEData->AME_LED_data.LED_HBA_Base_Address;
            pAMEData->AME_NTHost_data.Port_Number = pAMEData->AME_LED_data.LED_HBA_Port_Number;
            AME_Msg("AME HBA_Init:  Find HBA Bridge Type = %x HBA Port = %x \n",pAMEData->AME_LED_data.LED_HBA_Type,pAMEData->AME_LED_data.LED_HBA_Port_Number);
            AME_Msg("AME HBA_Init:  Find HBA Bridge Bus = %x, Slot = %x, HBA_Base_Address = %p\n",Bridge_Bus,Bridge_Slot,pAMEData->AME_LED_data.LED_HBA_Base_Address);
            return TRUE;
        }
        
    }
    
    return FALSE;
}


AME_U32 AME_Find_HBA_Bridge_DAS(pAME_Data pAMEData)
{
    AME_U32 Bridge_Bus,Bridge_Slot;
    AME_U32 Temp_Bus,Temp_Slot,Temp_Reg_0x00;
    
    if (TRUE == AME_Find_Upstream_Bridge(pAMEData,pAMEData->Device_BusNumber,&Temp_Bus,&Temp_Slot)) {
        
        Bridge_Bus = Temp_Bus;
        
        if (TRUE == AME_Find_Upstream_Bridge(pAMEData,Bridge_Bus,&Temp_Bus,&Temp_Slot)) {
            
            Bridge_Bus = Temp_Bus;
            
            if (TRUE == AME_Find_Upstream_Bridge(pAMEData,Bridge_Bus,&Temp_Bus,&Temp_Slot)) {
                
                Bridge_Bus = Temp_Bus;
                
                Temp_Reg_0x00 = AME_PCI_Config_Read_32(pAMEData,Temp_Bus,Temp_Slot,0,0x00);
                
                switch (Temp_Reg_0x00) {
                    case PLX_Brige_8608:
                    case PLX_Brige_8717:
                        pAMEData->AME_LED_data.LED_HBA_Type = Temp_Reg_0x00;
                        pAMEData->AME_LED_data.LED_HBA_Port_Number = Temp_Slot;
                        break;
                        
                    default:
                        AME_Msg("AME MSG: unknow HBA Bridge Type = %x\n",Temp_Reg_0x00);
                        return FALSE;
                }
                
                if (TRUE == AME_Find_Upstream_Bridge(pAMEData,Bridge_Bus,&Temp_Bus,&Temp_Slot)) {
                    Bridge_Bus = Temp_Bus;
                    Bridge_Slot = Temp_Slot;
                    pAMEData->AME_LED_data.LED_HBA_Base_Address = AME_IOMapping_Bridge(pAMEData,Bridge_Bus,Bridge_Slot,0);
                    AME_Msg("AME HBA_Init:  Find HBA Bridge Type = %x HBA Port = %x \n",pAMEData->AME_LED_data.LED_HBA_Type,pAMEData->AME_LED_data.LED_HBA_Port_Number);
                    AME_Msg("AME HBA_Init:  Find HBA Bridge Bus = %x, Slot = %x, HBA_Base_Address = %p\n",Bridge_Bus,Bridge_Slot,pAMEData->AME_LED_data.LED_HBA_Base_Address);
                    return TRUE;
                }
                
            }
            
        }
        
    }
    
    return FALSE;
}


AME_U32 AME_Find_Raid_Bridge(pAME_Data pAMEData)
{
    AME_U32 Temp_Bus,Temp_Slot;
    AME_U32 Bridge_Bus,Bridge_Slot;
    
    if (TRUE == AME_Find_Upstream_Bridge(pAMEData,pAMEData->Device_BusNumber,&Temp_Bus,&Temp_Slot)) {
        
        Bridge_Bus = Temp_Bus;
        
        if (TRUE == AME_Find_Upstream_Bridge(pAMEData,Bridge_Bus,&Temp_Bus,&Temp_Slot)) {
            Bridge_Bus = Temp_Bus;
            Bridge_Slot = Temp_Slot;
            pAMEData->AME_LED_data.LED_Raid_Base_Address = AME_IOMapping_Bridge(pAMEData,Bridge_Bus,Bridge_Slot,0);
            AME_Msg("AME HBA_Init:  Find Raid Bridge Bus = %x, Slot = %x, Raid_Base_Address = %p\n",Bridge_Bus,Bridge_Slot,pAMEData->AME_LED_data.LED_Raid_Base_Address);
            return TRUE;
        }
        
    }
    
    return FALSE;
}


AME_U32 AME_HBA_Init(pAME_Data pAMEData)
{
    switch (pAMEData->Device_ID)
    {
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            if (pAMEData->AME_LED_data.LED_HBA_Base_Address == NULL) {
                AME_Find_HBA_Bridge_DAS(pAMEData);
            }
            if (pAMEData->AME_LED_data.LED_Raid_Base_Address == NULL) {
                AME_Find_Raid_Bridge(pAMEData);
            }
            break;
            
        case AME_8508_DID_NT:
        case AME_8608_DID_NT:
        case AME_87B0_DID_NT:
            if (FALSE == AME_Find_HBA_Bridge_NT(pAMEData)) {
                AME_Msg("(%d)AME_Find_HBA_Bridge_NT failed (%x)  ...\n",pAMEData->AME_Serial_Number,pAMEData->Device_ID);
                return FALSE;
            }
            break;
        
        default:
            AME_Msg("(%d)Unsupport LED control Device %x  ...\n",pAMEData->AME_Serial_Number,pAMEData->Device_ID);
            break;
    }
    
    // Update eeprom.
    #if defined (Enable_AME_eeprom_update)
        if (pAMEData->AME_LED_data.LED_HBA_Base_Address != NULL) {
            AME_eeprom_update(pAMEData);
        }
    #endif
    
    /*
     * Raid HBA Link error check
     * Fixed in Raid 2208, HBA link downgrade issue.
     * Retry link to normal link state.
     */
    AME_HBA_Link_Error_check(pAMEData);
    
    /*
     * Support GC/GS get truly Raid link status
     */
    pAMEData->AME_Raid_Link_Status = AME_Get_Raid_Link_State(pAMEData);
    
    /* Turn ON HBA LED GPIO */
    AME_LED_Start(pAMEData);
    
    return TRUE;
}


AME_U64 AMEByteSwap64(AME_U64 x)
{
    AME_U64 ret;
    AME_U32 hi, lo;
    hi = AMEByteSwap32((AME_U32)(x >> 32));
    lo = AMEByteSwap32((AME_U32)x);
    ret = (((AME_U64)lo) << 32) | ((AME_U64)hi);
    return ret;
}

AME_U32 AME_IS_RW_CMD(AME_U8 scsi_cmd)
{
    AME_U8 ret;
    switch(scsi_cmd) {
        case SCSIOP_READ:
        case SCSIOP_WRITE:
        case SCSIOP_READ6:
        case SCSIOP_WRITE6:
        case SCSIOP_READ12:
        case SCSIOP_WRITE12:
        case SCSIOP_READ16:
        case SCSIOP_WRITE16:
            ret = TRUE;
            break;
        default:
            ret = FALSE;
            break;
    }
    return ret;
}


AME_U32 AME_3108_PrepareSG_SCSIReadWrite(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_DAS      *SCSIRequest;
    char                    *SGLStart;
    pAME_RAID_SCSIRW_SG     pAME_RAIDSG;
    pAME_Host_SG            pHostSG;
    pBDLList                pBDL_64;
            
    AME_U32     i;
    AME_U32     TotalSeg = 0;       

    SCSIRequest = (pAMESCSIRequest_DAS)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg = pAME_SCSI_REQUEST->SGCount;
    pHostSG = (pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    

    if (TotalSeg <= ((pAMEData->Message_SIZE-48)/sizeof(AME_RAID_SCSIRW_SG))) {

        /* direct mode */
        
        // align 16 bytes
        SGLStart = (char *)&SCSIRequest->BDL;
        SGLStart += 8;
        pAME_RAIDSG = (pAME_RAID_SCSIRW_SG) SGLStart;

    } else {
        
        /* List mode */

        AME_U64  tempaddress;
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        
        tempaddress = pAME_SCSI_REQUEST->BDL_physicalStartAddress;
        pBDL_64->Addr_HighPart = ((AME_U32)(tempaddress >> 32));
        pBDL_64->Addr_LowPart  = ((AME_U32)(tempaddress));
        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pAME_RAID_SCSIRW_SG) pAME_SCSI_REQUEST->BDL_virtualStartAddress;
    }

    for (i=0;i<TotalSeg;i++) {
        pAME_RAIDSG->Address = ((AME_U64)(pHostSG->Address));
        pAME_RAIDSG->BufferLength = ((AME_U32)(pHostSG->BufferLength));
        pAME_RAIDSG->ExtensionBit = 0;
        pAME_RAIDSG++;
        pHostSG++;
    }
    
    pAME_RAIDSG--;
    pAME_RAIDSG->ExtensionBit |= 0x40000000;
    
    return TRUE;
}


AME_U32 AME_3108_PrepareSG_Not_SCSIReadWrite(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_DAS         *SCSIRequest;
    pBDLList                    pAME_RAIDSG;
    pAME_Host_SG                pHostSG;
    pBDLList                    pBDL_64;
            
    AME_U32     TotalSeg = 0;       
    AME_U32     i;
    AME_U32     DataControl;
    AME_U8      BDLDirection = 0;
    AME_U64     tempaddress;

    SCSIRequest = (pAMESCSIRequest_DAS)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg = pAME_SCSI_REQUEST->SGCount;
    pHostSG = (pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    DataControl = SCSIRequest->Control;
    
    if (DataControl & AME_SCSI_CONTROL_READ)
        BDLDirection = AME_FLAGS_DIR_DTR_TO_HOST;
    else if (DataControl & AME_SCSI_CONTROL_WRITE)
        BDLDirection = AME_FLAGS_DIR_HOST_TO_DTR;
        
    if (TotalSeg==0) // none data command
        return TRUE;
    
    if (TotalSeg <= ((pAMEData->Message_SIZE-40)/sizeof(BDLList))) {

        /* direct mode */
        pAME_RAIDSG = (pBDLList) &SCSIRequest->BDL;

    } else {
        
        /* List mode */
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        
        tempaddress = (AME_U64)pAME_SCSI_REQUEST->BDL_physicalStartAddress;
        pBDL_64->Addr_HighPart = ((AME_U32)(tempaddress >> 32));
        pBDL_64->Addr_LowPart  = ((AME_U32)(tempaddress));
        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pBDLList) pAME_SCSI_REQUEST->BDL_virtualStartAddress;
    }

    for (i=0;i<TotalSeg;i++) {

        tempaddress = (AME_U64)pHostSG->Address;
        pAME_RAIDSG->Addr_HighPart = ((AME_U32)(tempaddress >> 32));
        pAME_RAIDSG->Addr_LowPart  = ((AME_U32)(tempaddress));
        pAME_RAIDSG->FlagsLength = AME_FLAGS_64_BIT_ADDRESSING << 24 | BDLDirection << 24 | AME_BDL_BASIC_ELEMENT << 24 | pHostSG->BufferLength ;

        pAME_RAIDSG++;
        pHostSG++;
    }

    return TRUE;
}


AME_U32 AME_2208_PrepareSG_SCSIReadWrite(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_DAS      *SCSIRequest;
    char                    *SGLStart;
    pAME_RAID_SCSIRW_SG     pAME_RAIDSG;
    pAME_Host_SG            pHostSG;
    pBDLList                pBDL_64;
            
    AME_U32     i;
    AME_U32     TotalSeg = 0;       

    SCSIRequest=(pAMESCSIRequest_DAS)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg=pAME_SCSI_REQUEST->SGCount;
    pHostSG=(pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    

    if (TotalSeg <= ((pAMEData->Message_SIZE-48)/sizeof(AME_RAID_SCSIRW_SG))) {

        // direct mode

        // align 16 bytes for 460/2208
        SGLStart = (char *)&SCSIRequest->BDL;
        SGLStart += 8;
        pAME_RAIDSG = (pAME_RAID_SCSIRW_SG) SGLStart;

    } else {
        
        //
        // List mode

        AME_U64     tempaddress;
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        // describe list 
        //pBDL_64->Address = AMEByteSwapHigh32_low32((AME_U64)(pAME_SCSI_REQUEST->BDL_physicalStartAddress));


        // Not 8 byte swap, high 4 byte swap and low 4 byte swap.
        tempaddress= (AME_U64)pAME_SCSI_REQUEST->BDL_physicalStartAddress;
        pBDL_64->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pBDL_64->Addr_LowPart= ((AME_U32)(tempaddress));

        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pAME_RAID_SCSIRW_SG) pAME_SCSI_REQUEST->BDL_virtualStartAddress;

    }


    for(i=0; i < TotalSeg; i++)
    {

        pAME_RAIDSG->Address = ((AME_U64)(pHostSG->Address));

        pAME_RAIDSG->BufferLength = ((AME_U32)(pHostSG->BufferLength));

        pAME_RAIDSG->ExtensionBit = 0;


        pAME_RAIDSG++;
        pHostSG++;
    
    }
    
    return TRUE;
}


AME_U32 AME_2208_PrepareSG_Not_SCSIReadWrite(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_DAS         *SCSIRequest;
    pBDLList                    pAME_RAIDSG;
    pAME_Host_SG                pHostSG;
    pBDLList                    pBDL_64;
            
    AME_U32     TotalSeg = 0;       
    AME_U32     i;
    AME_U32     DataControl;
    AME_U8      BDLDirection = 0;
    AME_U64     tempaddress;


    SCSIRequest=(pAMESCSIRequest_DAS)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg=pAME_SCSI_REQUEST->SGCount;
    pHostSG=(pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    DataControl=SCSIRequest->Control;
    
    if (DataControl & AME_SCSI_CONTROL_READ)
        BDLDirection = AME_FLAGS_DIR_DTR_TO_HOST;
    else if (DataControl & AME_SCSI_CONTROL_WRITE)
        BDLDirection = AME_FLAGS_DIR_HOST_TO_DTR;
        
    if (TotalSeg==0) // none data command
        return TRUE;
    
    if (TotalSeg <= ((pAMEData->Message_SIZE-40)/sizeof(BDLList))) {

        // direct mode
        pAME_RAIDSG = (pBDLList) &SCSIRequest->BDL;

    }
    else {
        //
        // List mode

        
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        // describe list 

        tempaddress= (AME_U64)pAME_SCSI_REQUEST->BDL_physicalStartAddress;
        pBDL_64->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pBDL_64->Addr_LowPart= ((AME_U32)(tempaddress));

        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pBDLList) pAME_SCSI_REQUEST->BDL_virtualStartAddress;

    }


    for(i=0; i < TotalSeg; i++)
    {
        tempaddress= (AME_U64)pHostSG->Address;
        pAME_RAIDSG->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pAME_RAIDSG->Addr_LowPart= ((AME_U32)(tempaddress));
        
        pAME_RAIDSG->FlagsLength = AME_FLAGS_64_BIT_ADDRESSING << 24 | BDLDirection << 24 | AME_BDL_BASIC_ELEMENT << 24 | pHostSG->BufferLength;
        pAME_RAIDSG->FlagsLength = (pAME_RAIDSG->FlagsLength);

        pAME_RAIDSG++;
        pHostSG++;
    }

    return TRUE;
}

// Retrun true: I2O I2O_outbound FIFO feee, can issue command.
AME_U32 AME_2208_Check_I2O_outbound_Queue_Free(pAME_Data pAMEData)
{
    AME_U16 data16_68,data16_6a;
    AME_U32 count = 0;

    // Only support 2208 DAS , 3108 queue is enough
    if ( (pAMEData->Device_ID != AME_2208_DID1_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID2_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID3_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID4_DAS) ) 
    {
        return TRUE;
    }

    //Check FIF0 not full, 2208 just have 16x 64bit FIFO.
    while (1) {
        data16_68=(AME_U16)AME_Address_read_16(pAMEData,pAMEData->Raid_Base_Address, AME_Outbound_Free_List_Consumer_Index);
        data16_6a=(AME_U16)AME_Address_read_16(pAMEData,pAMEData->Raid_Base_Address, AME_Outbound_Free_List_Producer_Index);
    
        if (((data16_6a+1) & 0x0F) != (data16_68 & 0x0F)) {
            break;
        } else {
            AME_IO_microseconds_Delay(1);
            
            // 10000 microseconds is not enough, some special cases, need about 100000 ms and up this to 1 second. 
            if (++count >= 1000000) {
                AME_Msg_Err("(%d)AME_2208_Check_I2O_outbound_Queue_Free, Fail [0x68:0x6a]=[%x:%x]\n",pAMEData->AME_Serial_Number,data16_68,data16_6a);
                return FALSE;               
            }
        }
            
    }
    return TRUE;

}


// Retrun true: I2O I2O_Inbound FIFO feee, can issue command.
AME_U32 AME_2208_3108_Check_I2O_Inbound_Queue_Free(pAME_Data pAMEData)
{
    AME_U32 count = 0;
    AME_U32 Raid_Freequeue_index = 0;
    AME_U32 Free_1,Free_2;
    AME_U32 Post_1,Post_2;

    // Only support 2208 3108 DAS
    if ( (pAMEData->Device_ID != AME_2208_DID1_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID2_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID3_DAS) &&
         (pAMEData->Device_ID != AME_2208_DID4_DAS) &&
         (pAMEData->Device_ID != AME_3108_DID0_DAS) &&
         (pAMEData->Device_ID != AME_3108_DID1_DAS) ) 
    {
        return TRUE;
    }

    /* Check Free Queue index */
    while (1)
    {
        Raid_Freequeue_index = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_REQUEST_MSG_PORT);
    
        if (Raid_Freequeue_index != AME_U32_NONE) {
            break;
        }
        
        if (count == 0) {
            Free_1 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_Inbound_Free_List_Consumer_Index);   
            Post_1 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_Inbound_Post_List_Consumer_Index);
        }
        
        AME_IO_microseconds_Delay(1);
        if (++count >= 1000000) {
            Free_2 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_Inbound_Free_List_Consumer_Index);
            Post_2 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_Inbound_Post_List_Consumer_Index);
            AME_Msg_Err("(%d)AME_2208_3108_Check_I2O_Inbound_Queue_Free, Fail. Das Raid, 0x60:[%x][%x], 0x64:[%x][%x] \n",pAMEData->AME_Serial_Number,Free_1,Free_2,Post_1,Post_2);
            return FALSE;
        }
    }
        
    return TRUE;

}


AME_U32 AME_write_Host_Inbound_Queue_Port_0x40(pAME_Data pAMEData,AME_U32 value)
{
    /* Masked //AME_spinlock and //AME_spinunlock,
     * use the same lock variable, cause dead lock.
     * In DAS mode, AME_write_Host_Inbound_Queue_Port_0x40 not needed lock.
     */

    AME_U32     returnstatus = TRUE;
    
    if (TRUE == AME_2208_3108_Check_I2O_Inbound_Queue_Free(pAMEData)) {
        AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_REQUEST_MSG_PORT, value);
    } 
    else
        returnstatus = FALSE;
    
    return returnstatus;
}
    

AME_U32 AME_write_Host_outbound_Queue_Port_0x44(pAME_Data pAMEData,  AME_U32 value)
{
    AME_U32     returnstatus=TRUE;
    
    AME_spinlock(pAMEData);
    
    if (TRUE == AME_2208_Check_I2O_outbound_Queue_Free(pAMEData))
    {
        AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_REPLY_MSG_PORT, value);
        
        // for 2208, send Doorbell with value 0x10000000 after write the reply port.
        if ( (pAMEData->Device_ID == AME_2208_DID1_DAS) ||
             (pAMEData->Device_ID == AME_2208_DID2_DAS) ||
             (pAMEData->Device_ID == AME_2208_DID3_DAS) ||
             (pAMEData->Device_ID == AME_2208_DID4_DAS) )
        {   // 2208 DAS
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_Inbound_Doorbell_PORT, 0x10000000);
        }
        
    } 
    else
        returnstatus = FALSE;
    
    AME_spinunlock(pAMEData);
    
    return returnstatus;
}


AME_U32 AME_2208_3108_Send_SCSI_Command(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AME_U32 returnstatus;
    
    returnstatus = AME_write_Host_Inbound_Queue_Port_0x40(pAMEData,(AME_U32)(pAME_SCSI_REQUEST->SCSIRequest_physicalStartAddress));
    
    return returnstatus;
}


AME_U32 AME_DAS_SOFT_RAID_Send_SCSI_Command(pAME_Data pAMEData,pAME_Data pAMEData_Path,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AME_U32 returnstatus;
    AME_U32 PCISerialNumber = 0;
    AME_U32 NT_REQUEST_Phy_addr = 0;
    
    PCISerialNumber = pAMEData->AME_NTHost_data.PCISerialNumber;
    PCISerialNumber = ((PCISerialNumber - 1) & 0x1F) << 3;
    PCISerialNumber |= 0x6;
    NT_REQUEST_Phy_addr = (AME_U32)pAME_SCSI_REQUEST->SCSIRequest_physicalStartAddress | PCISerialNumber;
    
    // Send to path AME request port.
    returnstatus = AME_write_Host_Inbound_Queue_Port_0x40(pAMEData_Path,NT_REQUEST_Phy_addr);
    
    return returnstatus;
}


AME_U32 AME_3108_PrepareSG(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AME_U32  returnstatus;
    AMESCSIRequest_DAS  *SCSIRequest;

    SCSIRequest=(pAMESCSIRequest_DAS)pAME_SCSI_REQUEST->SCSIRequest;

    if (AME_IS_RW_CMD(SCSIRequest->CDB[0])) {
        returnstatus=AME_3108_PrepareSG_SCSIReadWrite(pAMEData,pAME_SCSI_REQUEST);
    } else {
        returnstatus=AME_3108_PrepareSG_Not_SCSIReadWrite(pAMEData,pAME_SCSI_REQUEST);
    }
    
    return returnstatus;
}


AME_U32 AME_2208_PrepareSG(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AME_U32     returnstatus;
    AMESCSIRequest_DAS          *SCSIRequest;

    SCSIRequest=(pAMESCSIRequest_DAS)pAME_SCSI_REQUEST->SCSIRequest;

    if (AME_IS_RW_CMD(SCSIRequest->CDB[0])) {
        returnstatus=AME_2208_PrepareSG_SCSIReadWrite(pAMEData,pAME_SCSI_REQUEST);
    } else {
        returnstatus=AME_2208_PrepareSG_Not_SCSIReadWrite(pAMEData,pAME_SCSI_REQUEST);
    }
    
    return returnstatus;
}


AME_U32 AME_2208_Soft_Raid_PrepareSG_SCSIReadWrite(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_NT       *SCSIRequest;
    pAME_RAID_SCSIRW_SG     pAME_RAIDSG;
    pAME_Host_SG            pHostSG;
    pBDLList                pBDL_64;
            
    AME_U32     TotalSeg = 0;       
    AME_U32     i;

    SCSIRequest=(pAMESCSIRequest_NT)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg=pAME_SCSI_REQUEST->SGCount;
    pHostSG=(pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    
    if (TotalSeg <= ((pAMEData->Message_SIZE-48-16)/sizeof(AME_RAID_SCSIRW_SG))) {

        // direct mode
        pAME_RAIDSG = (pAME_RAID_SCSIRW_SG) &SCSIRequest->BDL;
        
        pAME_RAIDSG->Address = 0;
        pAME_RAIDSG->BufferLength = 0;
        pAME_RAIDSG->ExtensionBit = 0;
        
        pAME_RAIDSG++;
    

    }
    else {
        //
        // List mode

        AME_U64     tempaddress;
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        // describe list 
        //pBDL_64->Address = AMEByteSwapHigh32_low32((AME_U64)(pAME_SCSI_REQUEST->BDL_physicalStartAddress));


        // Not 8 byte swap, high 4 byte swap and low 4 byte swap.
        tempaddress = pAME_SCSI_REQUEST->BDL_physicalStartAddress;
        
        pBDL_64->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pBDL_64->Addr_LowPart= ((AME_U32)(tempaddress));

        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pAME_RAID_SCSIRW_SG) pAME_SCSI_REQUEST->BDL_virtualStartAddress;

    }


    for(i=0; i < TotalSeg; i++)
    {

        pAME_RAIDSG->Address = pHostSG->Address;  

        pAME_RAIDSG->BufferLength = ((AME_U32)(pHostSG->BufferLength));

        pAME_RAIDSG->ExtensionBit = 0;


        pAME_RAIDSG++;
        pHostSG++;
    
    }
    
    return TRUE;
}


AME_U32 AME_2208_Soft_Raid_PrepareSG_Not_SCSIReadWrite(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_NT           *SCSIRequest;
    pBDLList                    pAME_RAIDSG;
    pAME_Host_SG                pHostSG;
    pBDLList                    pBDL_64;
            
    AME_U32     TotalSeg = 0;       
    AME_U32     i;
    AME_U32     DataControl;
    AME_U8      BDLDirection = 0;
    AME_U64     tempaddress;

    SCSIRequest=(pAMESCSIRequest_NT)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg=pAME_SCSI_REQUEST->SGCount;
    pHostSG=(pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    DataControl=SCSIRequest->Control;
    
    if (DataControl & AME_SCSI_CONTROL_READ)
        BDLDirection = AME_FLAGS_DIR_DTR_TO_HOST;
    else if (DataControl & AME_SCSI_CONTROL_WRITE)
        BDLDirection = AME_FLAGS_DIR_HOST_TO_DTR;
        
    if (TotalSeg==0) // none data command
        return TRUE;
    
    if (TotalSeg <= ((pAMEData->Message_SIZE-40)/sizeof(BDLList))) {

        // direct mode
        pAME_RAIDSG = (pBDLList) &SCSIRequest->BDL;

    }
    else {
        //
        // List mode

        
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        // describe list 

        tempaddress = pAME_SCSI_REQUEST->BDL_physicalStartAddress;

        pBDL_64->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pBDL_64->Addr_LowPart= ((AME_U32)(tempaddress));

        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pBDLList) pAME_SCSI_REQUEST->BDL_virtualStartAddress;

    }


    for(i=0; i < TotalSeg; i++)
    {

        tempaddress = pHostSG->Address;

        pAME_RAIDSG->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pAME_RAIDSG->Addr_LowPart= ((AME_U32)(tempaddress));
        

        pAME_RAIDSG->FlagsLength=AME_FLAGS_64_BIT_ADDRESSING << 24 | BDLDirection << 24 | AME_BDL_BASIC_ELEMENT << 24 | pHostSG->BufferLength ;

        pAME_RAIDSG->FlagsLength = (pAME_RAIDSG->FlagsLength);


        pAME_RAIDSG++;
        pHostSG++;
    
    }

    
    return TRUE;
}

AME_U32 AME_2208_Soft_Raid_PrepareSG(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AME_U32     returnstatus;
    AMESCSIRequest_NT          *SCSIRequest;

    SCSIRequest=(pAMESCSIRequest_NT)pAME_SCSI_REQUEST->SCSIRequest;

    if (AME_IS_RW_CMD(SCSIRequest->CDB[0])) {
        returnstatus=AME_2208_Soft_Raid_PrepareSG_SCSIReadWrite(pAMEData,pAME_SCSI_REQUEST);
    } else {
        returnstatus=AME_2208_Soft_Raid_PrepareSG_Not_SCSIReadWrite(pAMEData,pAME_SCSI_REQUEST);
    }
    
    return returnstatus;
}


AME_U32 AME_6201_6101_PrepareSG_Command(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_DAS          *SCSIRequest;
    pBDLList                    pAME_RAIDSG;
    pAME_Host_SG                pHostSG;
    pBDLList                    pBDL_64;
            
    AME_U32     TotalSeg = 0;       
    AME_U32     i;
    AME_U32     DataControl;
    AME_U8      BDLDirection = 0;
    AME_U64     tempaddress;
    AME_U32     tempBufferLength;  

    SCSIRequest=(pAMESCSIRequest_DAS)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg=pAME_SCSI_REQUEST->SGCount;
    pHostSG=(pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    DataControl=SCSIRequest->Control;
    
    if (DataControl & AME_SCSI_CONTROL_READ)
        BDLDirection = AME_FLAGS_DIR_DTR_TO_HOST;
    else if (DataControl & AME_SCSI_CONTROL_WRITE)
        BDLDirection = AME_FLAGS_DIR_HOST_TO_DTR;
        
    if (TotalSeg==0) // none data command
        return TRUE;
    
    if (TotalSeg <= ((pAMEData->Message_SIZE-40)/sizeof(BDLList))) {

        // direct mode
        pAME_RAIDSG = (pBDLList) &SCSIRequest->BDL;

    }
    else {
        //
        // List mode

        
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        // describe list 

        tempaddress= (AME_U64)pAME_SCSI_REQUEST->BDL_physicalStartAddress;
        pBDL_64->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pBDL_64->Addr_LowPart= ((AME_U32)(tempaddress));

        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pBDLList) pAME_SCSI_REQUEST->BDL_virtualStartAddress;

    }


    for(i=0; i < TotalSeg; i++)
    {

        tempaddress= pHostSG->Address;
        tempBufferLength = pHostSG->BufferLength;
        
        pHostSG->Address = 0;
        pHostSG->BufferLength = 0;
        pHostSG->Reserve = 0;
        
        pAME_RAIDSG->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pAME_RAIDSG->Addr_LowPart= ((AME_U32)(tempaddress));

        pAME_RAIDSG->FlagsLength=AME_FLAGS_64_BIT_ADDRESSING << 24 | BDLDirection << 24 | AME_BDL_BASIC_ELEMENT << 24 | tempBufferLength ;

        pAME_RAIDSG++;
        pHostSG++;
    
    }

    
    return TRUE;
}


AME_U32 AME_6201_6201_Send_SCSI_Command(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{

    AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,AME_REQUEST_MSG_PORT, (AME_U32)(pAME_SCSI_REQUEST->SCSIRequest_physicalStartAddress));

    return TRUE;

}


AME_U32 AME_6201_6101_PrepareSG(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AME_U32     returnstatus;

    returnstatus = AME_6201_6101_PrepareSG_Command(pAMEData,pAME_SCSI_REQUEST);

    return returnstatus;
}


AME_U32 AME_NT_8608_Error_handle_FreeQ(pAME_Data pAMEData, AME_U32 RAID_System_Index)
{
    AME_U32 i,j,k;
    AME_U32 value;
    AME_U32 Raid_Freequeue_index = 0xFFFFFFFF;
    
    Raid_Freequeue_index = (AME_U32)AME_Address_read_32(pAMEData,
                                                        pAMEData->Raid_Base_Address,
                                                        RAID_System_Index*ACS62_BAR_OFFSET + AME_HOST_INT_MASK_REGISTER);
                                  
    if (Raid_Freequeue_index != 0xFFFFFFFF) {
        return TRUE;
    }
    
    for (i=0;i<256;i++) /* Bus No */
    {
        for (j=0;j<32;j++) /* Dev No */
        {
            for (k=0;k<8;k++) /* Func No */
            {
                value = 0;
                value |= 0x80000000; /* enabel LUT */
                value |= i << 8;     /* merge Bus No */
                value |= j << 3;     /* merge Dev No */
                value |= k;          /* merge Func No */
                AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x10D98,value);
                AME_IO_microseconds_Delay(10); 
                Raid_Freequeue_index = (AME_U32)AME_Address_read_32(pAMEData,
                                                        pAMEData->Raid_Base_Address,
                                                        RAID_System_Index*ACS62_BAR_OFFSET + AME_HOST_INT_MASK_REGISTER);
                                                        
                if (Raid_Freequeue_index != 0xFFFFFFFF)
                {
                    AME_Msg("(%d)Fill Request ID 0x%08x in Look up table (0xD98)\n",pAMEData->AME_Serial_Number,value);
                    return TRUE;
                }
                
            }
        }
    }
    
    return FALSE;

}


AME_U32 AME_NT_8717_Error_handle_FreeQ(pAME_Data pAMEData, AME_U32 RAID_System_Index)
{
    AME_U32 i,j;
    AME_U32 value,Vir_Offset,Link_Offset;
    AME_U32 Raid_Freequeue_index = 0xFFFFFFFF;
    
    if ((pAMEData->AME_NTHost_data.Port_Number == 1)) {
        Vir_Offset  = 0x3E000;
        Link_Offset = 0x3F000;
    } else {
        Vir_Offset  = 0x3C000;
        Link_Offset = 0x3D000;
    }
    
    Raid_Freequeue_index = (AME_U32)AME_Address_read_32(pAMEData,
                                                        pAMEData->Raid_Base_Address,
                                                        RAID_System_Index*ACS62_BAR_OFFSET + AME_HOST_INT_MASK_REGISTER);
                                  
    if (Raid_Freequeue_index != 0xFFFFFFFF) {
        return TRUE;
    }
    
    for (i=0;i<256;i++) /* Bus No */
    {
        for (j=0;j<32;j++) /* Dev No */
        {
            value = 0;
            value |= 0x01; /* enabel LUT */
            value |= i << 8;     /* merge Bus No */
            value |= j << 3;     /* merge Dev No */
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,(Vir_Offset+0xD98),value);
            AME_IO_microseconds_Delay(10);
            Raid_Freequeue_index = (AME_U32)AME_Address_read_32(pAMEData,
                                                    pAMEData->Raid_Base_Address,
                                                    RAID_System_Index*ACS62_BAR_OFFSET + AME_HOST_INT_MASK_REGISTER);
                                                    
            if (Raid_Freequeue_index != 0xFFFFFFFF) {
                AME_Msg("(%d)Fill Request ID 0x%08x in Look up table (0xD98)\n",pAMEData->AME_Serial_Number,value);
                return TRUE;
            }
        }
    }
    
    return FALSE;

}


AME_U32 AME_NT_Error_handle_FreeQ(pAME_Data pAMEData, AME_U32 RAID_System_Index)
{
    
    switch (pAMEData->Device_ID)
    {
        case AME_8608_DID_NT:
            if (AME_NT_8608_Error_handle_FreeQ(pAMEData, RAID_System_Index) == FALSE) {
                AME_Msg_Err("(%d)AME_NT_Error_handle_FreeQ  Fail.\n",pAMEData->AME_Serial_Number);
                return FALSE;
            }
            break;
        
        case AME_87B0_DID_NT:
            if (AME_NT_8717_Error_handle_FreeQ(pAMEData, RAID_System_Index) == FALSE) {
                AME_Msg_Err("(%d)AME_NT_Error_handle_FreeQ  Fail.\n",pAMEData->AME_Serial_Number);
                return FALSE;
            }
            break;
            
        default:
            break;
    }
    
    
    //if (pAMEData->Device_ID == AME_8608_DID_NT) {
    //    if (AME_NT_8608_Error_handle_FreeQ(pAMEData, RAID_System_Index) == FALSE) {
    //        AME_Msg_Err("(%d)AME_NT_Error_handle_FreeQ  Fail.\n",pAMEData->AME_Serial_Number);
    //        return FALSE;
    //    }
    //}
    
    return TRUE;
}


AME_U32 AME_341_NT_PrepareSG_command(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_NT           *SCSIRequest;
    pBDLList                    pAME_RAIDSG;
    pAME_Host_SG                pHostSG;
    pBDLList                    pBDL_64;
            
    AME_U32     TotalSeg = 0;       
    AME_U32     i;
    AME_U32     DataControl;
    AME_U8      BDLDirection = 0;
    AME_U64     tempaddress;
    AME_U32     tempBufferLength; 

    SCSIRequest=(pAMESCSIRequest_NT)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg=pAME_SCSI_REQUEST->SGCount;
    pHostSG=(pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    DataControl=SCSIRequest->Control;
    

    if (DataControl & AME_SCSI_CONTROL_READ)
        BDLDirection = AME_FLAGS_DIR_DTR_TO_HOST;
    else if (DataControl & AME_SCSI_CONTROL_WRITE)
        BDLDirection = AME_FLAGS_DIR_HOST_TO_DTR;
        
    if (TotalSeg==0) // none data command
        return TRUE;
    
    if (TotalSeg <= ((pAMEData->Message_SIZE-48)/sizeof(BDLList))) {

        // direct mode
        pAME_RAIDSG = (pBDLList) &SCSIRequest->BDL;

    } else {
        
        //
        // List mode
        
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        // describe list

        tempaddress = (pAME_SCSI_REQUEST->NT_linkside_BAR2_lowaddress + pAME_SCSI_REQUEST->BDL_physicalStartAddress) 
                        | ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_highaddress << 32);

        
        pBDL_64->Addr_HighPart= (AME_U32)(tempaddress >> 32);
        pBDL_64->Addr_LowPart= (AME_U32)(tempaddress);

        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pBDLList) pAME_SCSI_REQUEST->BDL_virtualStartAddress;
        

    }


    for(i=0; i < TotalSeg; i++)
    {
        tempaddress = ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_lowaddress + pHostSG->Address) | 
                      ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_highaddress << 32);

        tempBufferLength = pHostSG->BufferLength;

        pHostSG->Address = 0;
        pHostSG->BufferLength = 0;
        pHostSG->Reserve = 0;
    
        pAME_RAIDSG->Addr_HighPart = ((AME_U32)(tempaddress >> 32));
        pAME_RAIDSG->Addr_LowPart = ((AME_U32)(tempaddress));
        pAME_RAIDSG->FlagsLength = AME_FLAGS_64_BIT_ADDRESSING << 24 | BDLDirection << 24 | AME_BDL_BASIC_ELEMENT << 24 | tempBufferLength ;

        pAME_RAIDSG++;
        pHostSG++;
    }

    return TRUE;
}

AME_U32 AME_341_NT_Send_SCSI_Command(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AME_U32     ReqType;
    AME_U32     returnstatus;
    
    returnstatus = AME_Queue_Command(pAMEData,pAME_SCSI_REQUEST->NT_RAID_System_Index,pAME_SCSI_REQUEST->NTRAIDType,pAME_SCSI_REQUEST->SCSIRequest_physicalStartAddress,NULL);
    
    return returnstatus;
}


AME_U32 AME_341_NT_PrepareSG(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AME_U32     returnstatus;

    returnstatus = AME_341_NT_PrepareSG_command(pAMEData,pAME_SCSI_REQUEST);

    return returnstatus;
}


AME_U32 AME_2208_NT_PrepareSG_SCSIReadWrite(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_NT       *SCSIRequest;
    pAME_RAID_SCSIRW_SG     pAME_RAIDSG;
    pAME_Host_SG            pHostSG;
    pBDLList                pBDL_64;
            
    AME_U32     TotalSeg = 0;       
    AME_U32     i;

    SCSIRequest=(pAMESCSIRequest_NT)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg=pAME_SCSI_REQUEST->SGCount;
    pHostSG=(pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    
    if (TotalSeg <= ((pAMEData->Message_SIZE-48-16)/sizeof(AME_RAID_SCSIRW_SG))) {

        // direct mode
        pAME_RAIDSG = (pAME_RAID_SCSIRW_SG) &SCSIRequest->BDL;
        
        pAME_RAIDSG->Address = 0;
        pAME_RAIDSG->BufferLength = 0;
        pAME_RAIDSG->ExtensionBit = 0;
        
        pAME_RAIDSG++;
    

    }
    else {
        //
        // List mode

        AME_U64     tempaddress;
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        // describe list 
        //pBDL_64->Address = AMEByteSwapHigh32_low32((AME_U64)(pAME_SCSI_REQUEST->BDL_physicalStartAddress));


        // Not 8 byte swap, high 4 byte swap and low 4 byte swap.
        tempaddress = ( (pAME_SCSI_REQUEST->NT_linkside_BAR2_lowaddress + pAME_SCSI_REQUEST->BDL_physicalStartAddress) 
                        | ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_highaddress << 32) );
        
        pBDL_64->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pBDL_64->Addr_LowPart= ((AME_U32)(tempaddress));

        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pAME_RAID_SCSIRW_SG) pAME_SCSI_REQUEST->BDL_virtualStartAddress;

    }


    for(i=0; i < TotalSeg; i++)
    {

        pAME_RAIDSG->Address = ( ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_lowaddress + pHostSG->Address) 
                        | ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_highaddress << 32) );  

        pAME_RAIDSG->BufferLength = ((AME_U32)(pHostSG->BufferLength));

        pAME_RAIDSG->ExtensionBit = 0;


        pAME_RAIDSG++;
        pHostSG++;
    
    }
    
    return TRUE;
}


AME_U32 AME_2208_NT_PrepareSG_Not_SCSIReadWrite(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_NT           *SCSIRequest;
    pBDLList                    pAME_RAIDSG;
    pAME_Host_SG                pHostSG;
    pBDLList                    pBDL_64;
            
    AME_U32     TotalSeg = 0;       
    AME_U32     i;
    AME_U32     DataControl;
    AME_U8      BDLDirection = 0;
    AME_U64     tempaddress;

    SCSIRequest=(pAMESCSIRequest_NT)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg=pAME_SCSI_REQUEST->SGCount;
    pHostSG=(pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    DataControl=SCSIRequest->Control;
    
    if (DataControl & AME_SCSI_CONTROL_READ)
        BDLDirection = AME_FLAGS_DIR_DTR_TO_HOST;
    else if (DataControl & AME_SCSI_CONTROL_WRITE)
        BDLDirection = AME_FLAGS_DIR_HOST_TO_DTR;
        
    if (TotalSeg==0) // none data command
        return TRUE;
    
    if (TotalSeg <= ((pAMEData->Message_SIZE-40)/sizeof(BDLList))) {

        // direct mode
        pAME_RAIDSG = (pBDLList) &SCSIRequest->BDL;

    }
    else {
        //
        // List mode

        
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        // describe list 

        tempaddress = ( (pAME_SCSI_REQUEST->NT_linkside_BAR2_lowaddress + pAME_SCSI_REQUEST->BDL_physicalStartAddress) 
                        | ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_highaddress << 32) );

        pBDL_64->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pBDL_64->Addr_LowPart= ((AME_U32)(tempaddress));

        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pBDLList) pAME_SCSI_REQUEST->BDL_virtualStartAddress;

    }


    for(i=0; i < TotalSeg; i++)
    {

        tempaddress = ( ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_lowaddress + pHostSG->Address) 
                        | ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_highaddress << 32) );

        pAME_RAIDSG->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pAME_RAIDSG->Addr_LowPart= ((AME_U32)(tempaddress));
        

        pAME_RAIDSG->FlagsLength=AME_FLAGS_64_BIT_ADDRESSING << 24 | BDLDirection << 24 | AME_BDL_BASIC_ELEMENT << 24 | pHostSG->BufferLength ;

        pAME_RAIDSG->FlagsLength = (pAME_RAIDSG->FlagsLength);


        pAME_RAIDSG++;
        pHostSG++;
    
    }

    
    return TRUE;
}


AME_U32 AME_3108_NT_PrepareSG_SCSIReadWrite(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_NT       *SCSIRequest;
    pAME_RAID_SCSIRW_SG     pAME_RAIDSG;
    pAME_Host_SG            pHostSG;
    pBDLList                pBDL_64;
            
    AME_U32     TotalSeg = 0;       
    AME_U32     i;

    SCSIRequest=(pAMESCSIRequest_NT)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg=pAME_SCSI_REQUEST->SGCount;
    pHostSG=(pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    
    if (TotalSeg <= ((pAMEData->Message_SIZE-48-16)/sizeof(AME_RAID_SCSIRW_SG))) {

        // direct mode
        pAME_RAIDSG = (pAME_RAID_SCSIRW_SG) &SCSIRequest->BDL;
        
        pAME_RAIDSG->Address = 0;
        pAME_RAIDSG->BufferLength = 0;
        pAME_RAIDSG->ExtensionBit = 0;
        
        pAME_RAIDSG++;
    

    }
    else {
        //
        // List mode

        AME_U64     tempaddress;
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        // describe list 
        //pBDL_64->Address = AMEByteSwapHigh32_low32((AME_U64)(pAME_SCSI_REQUEST->BDL_physicalStartAddress));


        // Not 8 byte swap, high 4 byte swap and low 4 byte swap.
        tempaddress = ( (pAME_SCSI_REQUEST->NT_linkside_BAR2_lowaddress + pAME_SCSI_REQUEST->BDL_physicalStartAddress) 
                        | ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_highaddress << 32) );
        
        pBDL_64->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pBDL_64->Addr_LowPart= ((AME_U32)(tempaddress));

        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pAME_RAID_SCSIRW_SG) pAME_SCSI_REQUEST->BDL_virtualStartAddress;

    }


    for(i=0; i < TotalSeg; i++)
    {

        pAME_RAIDSG->Address = ( ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_lowaddress + pHostSG->Address) 
                        | ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_highaddress << 32) );  

        pAME_RAIDSG->BufferLength = ((AME_U32)(pHostSG->BufferLength));

        pAME_RAIDSG->ExtensionBit = 0;


        pAME_RAIDSG++;
        pHostSG++;
    
    }
    
    return TRUE;
}


AME_U32 AME_3108_NT_PrepareSG_Not_SCSIReadWrite(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AMESCSIRequest_NT           *SCSIRequest;
    pBDLList                    pAME_RAIDSG;
    pAME_Host_SG                pHostSG;
    pBDLList                    pBDL_64;
            
    AME_U32     TotalSeg = 0;       
    AME_U32     i;
    AME_U32     DataControl;
    AME_U8      BDLDirection = 0;
    AME_U64     tempaddress;

    SCSIRequest=(pAMESCSIRequest_NT)pAME_SCSI_REQUEST->SCSIRequest;
    TotalSeg=pAME_SCSI_REQUEST->SGCount;
    pHostSG=(pAME_Host_SG)pAME_SCSI_REQUEST->pSG;
    DataControl=SCSIRequest->Control;
    
    if (DataControl & AME_SCSI_CONTROL_READ)
        BDLDirection = AME_FLAGS_DIR_DTR_TO_HOST;
    else if (DataControl & AME_SCSI_CONTROL_WRITE)
        BDLDirection = AME_FLAGS_DIR_HOST_TO_DTR;
        
    if (TotalSeg==0) // none data command
        return TRUE;
    
    if (TotalSeg <= ((pAMEData->Message_SIZE-40)/sizeof(BDLList))) {

        // direct mode
        pAME_RAIDSG = (pBDLList) &SCSIRequest->BDL;

    }
    else {
        //
        // List mode

        
        pBDL_64 = (pBDLList) &SCSIRequest->BDL;
        // describe list 

        tempaddress = ( (pAME_SCSI_REQUEST->NT_linkside_BAR2_lowaddress + pAME_SCSI_REQUEST->BDL_physicalStartAddress) 
                        | ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_highaddress << 32) );

        pBDL_64->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pBDL_64->Addr_LowPart= ((AME_U32)(tempaddress));

        pBDL_64->FlagsLength = ((AME_U32)(AME_FLAGS_64_BIT_ADDRESSING << 24 | AME_BDL_LIST_ELEMENT << 24 | TotalSeg));

        pAME_RAIDSG = (pBDLList) pAME_SCSI_REQUEST->BDL_virtualStartAddress;

    }


    for(i=0; i < TotalSeg; i++)
    {

        tempaddress = ( ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_lowaddress + pHostSG->Address) 
                        | ((AME_U64)pAME_SCSI_REQUEST->NT_linkside_BAR2_highaddress << 32) );

        pAME_RAIDSG->Addr_HighPart= ((AME_U32)(tempaddress >> 32));
        pAME_RAIDSG->Addr_LowPart= ((AME_U32)(tempaddress));
        

        pAME_RAIDSG->FlagsLength=AME_FLAGS_64_BIT_ADDRESSING << 24 | BDLDirection << 24 | AME_BDL_BASIC_ELEMENT << 24 | pHostSG->BufferLength ;

        pAME_RAIDSG->FlagsLength = (pAME_RAIDSG->FlagsLength);


        pAME_RAIDSG++;
        pHostSG++;
    
    }

    
    return TRUE;
}


// Retrun true: I2O I2O_Inbound FIFO feee, can issue command.
AME_U32 AME_2208_NT_Check_I2O_Inbound_Queue_Free(pAME_Data pAMEData,AME_U32 Raid_ID)
{
    AME_U32 count=0;
    AME_U32 NT_REQUEST_MSG_PORT = 0;
    AME_U32 Raid_Freequeue_index = 0;
    AME_U32 Free_1,Free_2;
    AME_U32 Post_1,Post_2;
    AME_U32 NT_Inbound_Free_List_PORT = 0,NT_Inbound_Post_List_PORT = 0;

    // Not 2208 NT
    if ( pAMEData->AME_RAIDData[Raid_ID].NTRAIDType != NTRAIDType_2208 ) {
        return TRUE;
    }
    
    NT_REQUEST_MSG_PORT = AME_REQUEST_MSG_PORT + 
                          Raid_ID*ACS2208_BAR_OFFSET +
                          pAMEData->AME_NTHost_data.NTBar_Offset;
    
    while (1)
    {
        Raid_Freequeue_index = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,NT_REQUEST_MSG_PORT);
        
        if (Raid_Freequeue_index != 0xFFFFFFFF) {
            break;
        } else {

            NT_Inbound_Free_List_PORT = AME_Inbound_Free_List_Consumer_Index + 
                                  Raid_ID*ACS2208_BAR_OFFSET +
                                  pAMEData->AME_NTHost_data.NTBar_Offset;

            NT_Inbound_Post_List_PORT = AME_Inbound_Post_List_Consumer_Index + 
                                  Raid_ID*ACS2208_BAR_OFFSET +
                                  pAMEData->AME_NTHost_data.NTBar_Offset;
            
            if (count == 0) {
                Free_1 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,NT_Inbound_Free_List_PORT);  
                Post_1 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,NT_Inbound_Post_List_PORT);
            }
            
            AME_IO_microseconds_Delay(1);
            if (++count >= 1000000) {
                Free_2 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,NT_Inbound_Free_List_PORT);
                Post_2 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,NT_Inbound_Post_List_PORT);
                AME_Msg_Err("(%d)AME_2208_NT_Check_I2O_Inbound_Queue_Free, Fail. NT Raid(%d), 0x60:[%x][%x], 0x64:[%x][%x] \n",pAMEData->AME_Serial_Number,Raid_ID,Free_1,Free_2,Post_1,Post_2);
                return FALSE;
            }
        }
    }
    
    return TRUE;
}


// Retrun true: I2O I2O_Inbound FIFO feee, can issue command.
AME_U32 AME_3108_NT_Check_I2O_Inbound_Queue_Free(pAME_Data pAMEData,AME_U32 Raid_ID)
{
    AME_U32 count=0;
    AME_U32 NT_REQUEST_MSG_PORT = 0;
    AME_U32 Raid_Freequeue_index = 0;
    AME_U32 Free_1,Free_2;
    AME_U32 Post_1,Post_2;
    AME_U32 NT_Inbound_Free_List_PORT = 0,NT_Inbound_Post_List_PORT = 0;

    // Not 2208 NT
    if ( pAMEData->AME_RAIDData[Raid_ID].NTRAIDType != NTRAIDType_3108 ) {
        return TRUE;
    }
    
    NT_REQUEST_MSG_PORT = AME_REQUEST_MSG_PORT + 
                          Raid_ID*ACS3108_BAR_OFFSET +
                          pAMEData->AME_NTHost_data.NTBar_Offset;
    
    while (1)
    {
        Raid_Freequeue_index = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,NT_REQUEST_MSG_PORT);
        
        if (Raid_Freequeue_index != 0xFFFFFFFF) {
            break;
        } else {

            NT_Inbound_Free_List_PORT = AME_Inbound_Free_List_Consumer_Index + 
                                  Raid_ID*ACS3108_BAR_OFFSET +
                                  pAMEData->AME_NTHost_data.NTBar_Offset;

            NT_Inbound_Post_List_PORT = AME_Inbound_Post_List_Consumer_Index + 
                                  Raid_ID*ACS3108_BAR_OFFSET +
                                  pAMEData->AME_NTHost_data.NTBar_Offset;
            
            if (count == 0) {
                Free_1 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,NT_Inbound_Free_List_PORT);  
                Post_1 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,NT_Inbound_Post_List_PORT);
            }
            
            AME_IO_microseconds_Delay(1);
            if (++count >= 1000000) {
                Free_2 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,NT_Inbound_Free_List_PORT);
                Post_2 = (AME_U32)AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,NT_Inbound_Post_List_PORT);
                AME_Msg_Err("(%d)AME_3108_NT_Check_I2O_Inbound_Queue_Free, Fail. NT Raid(%d), 0x60:[%x][%x], 0x64:[%x][%x] \n",pAMEData->AME_Serial_Number,Raid_ID,Free_1,Free_2,Post_1,Post_2);
                return FALSE;
            }
        }
    }
    
    return TRUE;
}


AME_U32 AME_NT_write_Host_Inbound_Queue_Port_0x40(pAME_Data pAMEData,AME_U32 Raid_ID,AME_U64 Request_Buffer_Phy_addr)
{
    AME_U32 PCISerialNumber = 0;
    AME_U32 NT_REQUEST_Phy_addr = 0;
    AME_U32 NT_REQUEST_MSG_PORT = pAMEData->AME_RAIDData[Raid_ID].NT_REQUEST_MSG_PORT;
    
    // Check Raid is exist.
    if ( !((pAMEData->AME_NTHost_data.Raid_All_Topology >> Raid_ID) & 0x01) ) {
        return FALSE;
    }
    
    PCISerialNumber = pAMEData->AME_NTHost_data.PCISerialNumber;
    
    NT_REQUEST_Phy_addr  = (AME_U32)Request_Buffer_Phy_addr +
                           pAMEData->AME_NTHost_data.NT_linkside_BAR2_lowaddress;
    
    switch (pAMEData->AME_RAIDData[Raid_ID].NTRAIDType)
    {
        case NTRAIDType_341:
            PCISerialNumber = ((PCISerialNumber - 1) << 2) | 0x02;
            NT_REQUEST_Phy_addr |= PCISerialNumber;
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,NT_REQUEST_MSG_PORT,NT_REQUEST_Phy_addr);
            break;
        
        case NTRAIDType_2208:
            PCISerialNumber = ((PCISerialNumber - 1) & 0x1F) << 3;
            PCISerialNumber |= 0x6;
            NT_REQUEST_Phy_addr |= PCISerialNumber;

            // 2208 NT need Check Free Queue
            /* Check Free Queue index */
            if (FALSE == AME_2208_NT_Check_I2O_Inbound_Queue_Free(pAMEData,Raid_ID)) {
                return FALSE;
            }
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,NT_REQUEST_MSG_PORT,NT_REQUEST_Phy_addr);
            break;
        
        case NTRAIDType_3108:
            PCISerialNumber = ((PCISerialNumber - 1) & 0x1F) << 3;
            PCISerialNumber |= 0x6;
            NT_REQUEST_Phy_addr |= PCISerialNumber;

            // 3108 NT need Check Free Queue
            /* Check Free Queue index */
            if (FALSE == AME_3108_NT_Check_I2O_Inbound_Queue_Free(pAMEData,Raid_ID)) {
                return FALSE;
            }
            AME_Address_Write_32(pAMEData,pAMEData->Raid_Base_Address,NT_REQUEST_MSG_PORT,NT_REQUEST_Phy_addr);
            break;
        
        default:
            AME_Msg_Err("(%d)Error:  Unknow index %d Raid type, can't send request cmd.\n",pAMEData->AME_Serial_Number,Raid_ID);
            return FALSE;
    }
    
    return TRUE;
}

AME_U32 AME_2208_NT_Send_SCSI_Command(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AME_U32     returnstatus;
    
    returnstatus = AME_Queue_Command(pAMEData,pAME_SCSI_REQUEST->NT_RAID_System_Index,pAME_SCSI_REQUEST->NTRAIDType,pAME_SCSI_REQUEST->SCSIRequest_physicalStartAddress,NULL);
    
    return returnstatus;
}

AME_U32 AME_3108_NT_Send_SCSI_Command(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    AME_U32     returnstatus;
    
    returnstatus = AME_Queue_Command(pAMEData,pAME_SCSI_REQUEST->NT_RAID_System_Index,pAME_SCSI_REQUEST->NTRAIDType,pAME_SCSI_REQUEST->SCSIRequest_physicalStartAddress,NULL);
    
    return returnstatus;
}

AME_U32 AME_2208_NT_PrepareSG(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{

    AME_U32             returnstatus;
    AMESCSIRequest_NT   *SCSIRequest;

    SCSIRequest = (pAMESCSIRequest_NT)pAME_SCSI_REQUEST->SCSIRequest;

    if (TRUE == AME_IS_RW_CMD(SCSIRequest->CDB[0])) {
        returnstatus = AME_2208_NT_PrepareSG_SCSIReadWrite(pAMEData,pAME_SCSI_REQUEST);
    } else {
        returnstatus = AME_2208_NT_PrepareSG_Not_SCSIReadWrite(pAMEData,pAME_SCSI_REQUEST);
    }

    return returnstatus;
}

AME_U32 AME_3108_NT_PrepareSG(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{

    AME_U32             returnstatus;
    AMESCSIRequest_NT   *SCSIRequest;

    SCSIRequest = (pAMESCSIRequest_NT)pAME_SCSI_REQUEST->SCSIRequest;

    if (TRUE == AME_IS_RW_CMD(SCSIRequest->CDB[0])) {
        returnstatus = AME_2208_NT_PrepareSG_SCSIReadWrite(pAMEData,pAME_SCSI_REQUEST);
    } else {
        returnstatus = AME_2208_NT_PrepareSG_Not_SCSIReadWrite(pAMEData,pAME_SCSI_REQUEST);
    }

    return returnstatus;
}


AME_U32 AME_NT_SCSI_PrepareSG(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSI_REQUEST)
{
    
    switch (pAME_SCSI_REQUEST->NTRAIDType)
    {
        case NTRAIDType_341:
            return (AME_341_NT_PrepareSG(pAMEData,pAME_SCSI_REQUEST));
            
        case NTRAIDType_2208:
            return (AME_2208_NT_PrepareSG(pAMEData,pAME_SCSI_REQUEST));
            
        case NTRAIDType_3108:
            return (AME_3108_NT_PrepareSG(pAMEData,pAME_SCSI_REQUEST));
            
        default:
            AME_Msg_Err("(%d)Error:  Unknow index %d Raid type, can't send request cmd.\n",pAMEData->AME_Serial_Number,pAME_SCSI_REQUEST->NT_RAID_System_Index);
            return FALSE;
    }
    
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_DAS_SCSI_Prepare(pAME_Data pAMEData,pAME_Data pAMEData_Path,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command,pAME_SCSI_REQUEST pAME_SCSIRequest,AME_U32 BufferID)
{
    pAMESCSIRequest_DAS         SCSIRequest;
    pAME_Host_SG                pHostSG;

    SCSIRequest = (pAMESCSIRequest_DAS)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
    pHostSG = (pAME_Host_SG)pAMEData->AMEBufferData[BufferID].Request_BDL_Vir_addr;

    AME_Memory_copy(pHostSG, pAME_ModuleSCSI_Command->AME_Host_Sg, (pAME_ModuleSCSI_Command->SGCount+1)*16);
    
    SCSIRequest->Reserved[0] = 0;                               /* 00h */
    SCSIRequest->Reserved[1] = 0;                               /* 01h */
    SCSIRequest->Lun = (AME_U8)pAME_ModuleSCSI_Command->Target_ID;  /* 02h */
    SCSIRequest->Function = AME_FUNCTION_SCSI_REQUEST;          /* 03h */
    SCSIRequest->MsgIdentifier = (BufferID << 2);               /* 04h */
    SCSIRequest->CDBLength  = pAME_ModuleSCSI_Command->CDBLength;   /* 08h */   
    SCSIRequest->SenseBufferLength = AME_SENSE_BUFFER_SIZE;     /* 09h */
    SCSIRequest->Reserved2[0] = 0;                              /* 0Ah */
    SCSIRequest->Reserved2[1] = 0;                              /* 0Bh */
    SCSIRequest->Control = pAME_ModuleSCSI_Command->Control;        /* 0Ch */
                                                                    /* 10h */
    AME_Memory_copy(SCSIRequest->CDB, pAME_ModuleSCSI_Command->CDB, pAME_ModuleSCSI_Command->CDBLength);
    SCSIRequest->DataLength = pAME_ModuleSCSI_Command->DataLength;  /* 20h */
                                                                /* 24h */
    SCSIRequest->SenseBufferLowAddr = (AME_U32)pAMEData->AMEBufferData[BufferID].Sense_Buffer_Phy_addr;
    //SCSIRequest->BDL                                          /* 28h */

    
    pAME_SCSIRequest->SCSIRequest = (PVOID)SCSIRequest;
    pAME_SCSIRequest->SCSIRequest_physicalStartAddress = (AME_U64)pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr;
    pAME_SCSIRequest->SGCount = pAME_ModuleSCSI_Command->SGCount;
    pAME_SCSIRequest->pSG = pHostSG;
    pAME_SCSIRequest->BDL_physicalStartAddress = (AME_U64)pAMEData->AMEBufferData[BufferID].Request_BDL_Phy_addr;
    pAME_SCSIRequest->BDL_virtualStartAddress = (AME_U64)pAMEData->AMEBufferData[BufferID].Request_BDL_Vir_addr;
    pAME_SCSIRequest->callback = pAME_ModuleSCSI_Command->callback;
    
    #if defined (Enable_AME_RAID_R0)
        pAME_SCSIRequest->NT_RAID_System_Index = 0;
    #endif

    pAMEData->AMEBufferData[BufferID].pRequestExtension = pAME_ModuleSCSI_Command->pRequestExtension;
    pAMEData->AMEBufferData[BufferID].pMPIORequestExtension = pAME_ModuleSCSI_Command->pMPIORequestExtension;
    pAMEData->AMEBufferData[BufferID].pAME_Raid_QueueBuffer = pAME_ModuleSCSI_Command->pAME_Raid_QueueBuffer;  // for RAID 0/1
    pAMEData->AMEBufferData[BufferID].callback = pAME_ModuleSCSI_Command->callback;
    
    pAMEData->AMEBufferData[BufferID].pAMEData_Path = pAMEData_Path;  // Raid Error handle for NT
    pAMEData->AMEBufferData[BufferID].Path_Raid_ID = pAME_ModuleSCSI_Command->Slave_Raid_ID;  // Raid Error handle for NT
    
    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_NT_SCSI_Prepare(pAME_Data pAMEData,pAME_Data pAMEData_Path,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command,pAME_SCSI_REQUEST pAME_SCSIRequest,AME_U32 BufferID)
{
    pAMESCSIRequest_NT          SCSIRequest;
    pAME_Host_SG                pHostSG;
    
    SCSIRequest = (pAMESCSIRequest_NT)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
    pHostSG = (pAME_Host_SG)pAMEData->AMEBufferData[BufferID].Request_BDL_Vir_addr;

    AME_Memory_copy(pHostSG, pAME_ModuleSCSI_Command->AME_Host_Sg, (pAME_ModuleSCSI_Command->SGCount+1)*16);
    
    SCSIRequest->Reserved[0] = 0;                                   /* 00h */
    SCSIRequest->Reserved[1] = 0;                                   /* 01h */
    SCSIRequest->Lun = (AME_U8)pAME_ModuleSCSI_Command->Target_ID;  /* 02h */
    SCSIRequest->Function = AME_FUNCTION_SCSI_EXT2_REQUEST;         /* 03h */
    
    if (pAMEData == pAMEData_Path){ // master, direct send cmd
        SCSIRequest->MsgIdentifier = (BufferID << 2);               /* 04h */
    } else {
        AME_Msg_Err("(%d)AME_NT_SCSI_Prepare error:  Why go to here ???\n",pAMEData->AME_Serial_Number);
        //SCSIRequest->MsgIdentifier = ((BufferID | MPIO_Master_Request_Bit) << 2);  /* 04h */
    }
    
    SCSIRequest->CDBLength  = pAME_ModuleSCSI_Command->CDBLength;   /* 08h */
    SCSIRequest->SenseBufferLength = AME_SENSE_BUFFER_SIZE;         /* 09h */
    SCSIRequest->Reserved2[0] = 0;                                  /* 0Ah */
    SCSIRequest->Reserved2[1] = 0;                                  /* 0Bh */
    SCSIRequest->Control = pAME_ModuleSCSI_Command->Control;        /* 0Ch */
                                                                    /* 10h */
    AME_Memory_copy(SCSIRequest->CDB, pAME_ModuleSCSI_Command->CDB, pAME_ModuleSCSI_Command->CDBLength);
    SCSIRequest->DataLength = pAME_ModuleSCSI_Command->DataLength;  /* 20h */
                                                                    /* 24h */
    SCSIRequest->SenseBufferLowAddr = (AME_U32)pAMEData->AMEBufferData[BufferID].Sense_Buffer_Phy_addr;
                                                                    /* 28h -- NT Port */
    SCSIRequest->PCISerialNumber    = pAMEData_Path->AME_NTHost_data.PCISerialNumber;
                                                                    /* 2Ch -- NT Port */
    SCSIRequest->ReplyAddressLow    = (AME_U32)pAMEData->AMEReplyBufferData[BufferID].Reply_Buffer_Phy_addr +
                                      pAMEData_Path->AME_NTHost_data.NT_linkside_BAR2_lowaddress;
    
    //SCSIRequest->BDL                                              /* 30h */
    pAME_SCSIRequest->SCSIRequest = (PVOID)SCSIRequest;
    pAME_SCSIRequest->SCSIRequest_physicalStartAddress = (AME_U64)pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr;
    pAME_SCSIRequest->SGCount = pAME_ModuleSCSI_Command->SGCount;
    pAME_SCSIRequest->pSG = pHostSG;
    pAME_SCSIRequest->BDL_physicalStartAddress = (AME_U64)pAMEData->AMEBufferData[BufferID].Request_BDL_Phy_addr;
    pAME_SCSIRequest->BDL_virtualStartAddress = (AME_U64)pAMEData->AMEBufferData[BufferID].Request_BDL_Vir_addr;
    pAME_SCSIRequest->callback = pAME_ModuleSCSI_Command->callback;
    
    pAME_SCSIRequest->NT_RAID_System_Index = pAME_ModuleSCSI_Command->Slave_Raid_ID;
    pAME_SCSIRequest->NT_linkside_BAR2_lowaddress = pAMEData_Path->AME_NTHost_data.NT_linkside_BAR2_lowaddress;
    pAME_SCSIRequest->NT_linkside_BAR2_highaddress = pAMEData_Path->AME_NTHost_data.NT_linkside_BAR2_highaddress;
    pAME_SCSIRequest->NTBar_Offset = pAMEData_Path->AME_NTHost_data.NTBar_Offset;
    pAME_SCSIRequest->NTRAIDType = pAMEData_Path->AME_RAIDData[pAME_ModuleSCSI_Command->Slave_Raid_ID].NTRAIDType;
    pAME_SCSIRequest->Request_BufferID = BufferID;
    
    pAMEData->AMEBufferData[BufferID].Raid_ID = pAME_ModuleSCSI_Command->Raid_ID;  // Raid Error handle for NT
    pAMEData->AMEBufferData[BufferID].pRequestExtension = pAME_ModuleSCSI_Command->pRequestExtension;
    pAMEData->AMEBufferData[BufferID].pMPIORequestExtension = pAME_ModuleSCSI_Command->pMPIORequestExtension;
    pAMEData->AMEBufferData[BufferID].pAME_Raid_QueueBuffer = pAME_ModuleSCSI_Command->pAME_Raid_QueueBuffer; // for RAID 0/1, but NT unused
    pAMEData->AMEBufferData[BufferID].callback = pAME_ModuleSCSI_Command->callback;
    
    pAMEData->AMEBufferData[BufferID].pAMEData_Path = pAMEData_Path;  // Raid Error handle for NT
    pAMEData->AMEBufferData[BufferID].Path_Raid_ID = pAME_ModuleSCSI_Command->Slave_Raid_ID;  // Raid Error handle for NT
    
    // Raid Error handling, stop cmd, return Busy
    if (pAMEData->AME_RAIDData[pAME_ModuleSCSI_Command->Raid_ID].Raid_Error_Handle_State == TRUE) {
        return FALSE;
    } 
    
    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_DAS_SOFT_RAID_SCSI_Prepare(pAME_Data pAMEData,pAME_Data pAMEData_Path,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command,pAME_SCSI_REQUEST pAME_SCSIRequest,AME_U32 BufferID)
{
    pAMESCSIRequest_NT          SCSIRequest;
    pAME_Host_SG                pHostSG;
    
    SCSIRequest = (pAMESCSIRequest_NT)pAMEData->AMEBufferData[BufferID].Request_Buffer_Vir_addr;
    pHostSG = (pAME_Host_SG)pAMEData->AMEBufferData[BufferID].Request_BDL_Vir_addr;

    AME_Memory_copy(pHostSG, pAME_ModuleSCSI_Command->AME_Host_Sg, (pAME_ModuleSCSI_Command->SGCount+1)*16);
    
    SCSIRequest->Reserved[0] = 0;                                   /* 00h */
    SCSIRequest->Reserved[1] = 0;                                   /* 01h */
    SCSIRequest->Lun = (AME_U8)pAME_ModuleSCSI_Command->Target_ID;  /* 02h */
    SCSIRequest->Function = AME_FUNCTION_SCSI_EXT2_REQUEST;         /* 03h */
    SCSIRequest->MsgIdentifier = (BufferID << 2);                   /* 04h */
    SCSIRequest->CDBLength  = pAME_ModuleSCSI_Command->CDBLength;   /* 08h */
    SCSIRequest->SenseBufferLength = AME_SENSE_BUFFER_SIZE;         /* 09h */
    SCSIRequest->Reserved2[0] = 0;                                  /* 0Ah */
    SCSIRequest->Reserved2[1] = 0;                                  /* 0Bh */
    SCSIRequest->Control = pAME_ModuleSCSI_Command->Control;        /* 0Ch */
                                                                    /* 10h */
    AME_Memory_copy(SCSIRequest->CDB, pAME_ModuleSCSI_Command->CDB, pAME_ModuleSCSI_Command->CDBLength);
    SCSIRequest->DataLength = pAME_ModuleSCSI_Command->DataLength;  /* 20h */
                                                                    /* 24h */
    SCSIRequest->SenseBufferLowAddr = (AME_U32)pAMEData->AMEBufferData[BufferID].Sense_Buffer_Phy_addr;
                                                                    /* 28h -- NT Port */
    SCSIRequest->PCISerialNumber    = pAMEData->AME_NTHost_data.PCISerialNumber;
                                                                    /* 2Ch -- NT Port */
    SCSIRequest->ReplyAddressLow    = (AME_U32)pAMEData->AMEReplyBufferData[BufferID].Reply_Buffer_Phy_addr;
    
    //SCSIRequest->BDL                                              /* 30h */
    pAME_SCSIRequest->SCSIRequest = (PVOID)SCSIRequest;
    pAME_SCSIRequest->SCSIRequest_physicalStartAddress = (AME_U64)pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr;
    pAME_SCSIRequest->SGCount = pAME_ModuleSCSI_Command->SGCount;
    pAME_SCSIRequest->pSG = pHostSG;
    pAME_SCSIRequest->BDL_physicalStartAddress = (AME_U64)pAMEData->AMEBufferData[BufferID].Request_BDL_Phy_addr;
    pAME_SCSIRequest->BDL_virtualStartAddress = (AME_U64)pAMEData->AMEBufferData[BufferID].Request_BDL_Vir_addr;
    pAME_SCSIRequest->callback = pAME_ModuleSCSI_Command->callback;
    
    pAME_SCSIRequest->NT_RAID_System_Index = 0;
    pAME_SCSIRequest->NT_linkside_BAR2_lowaddress = 0;
    pAME_SCSIRequest->NT_linkside_BAR2_highaddress = 0;
    pAME_SCSIRequest->NTBar_Offset = 0;
    pAME_SCSIRequest->NTRAIDType = NTRAIDType_SOFR_Raid;
    pAME_SCSIRequest->Request_BufferID = BufferID;
    
    pAMEData->AMEBufferData[BufferID].Raid_ID = pAME_ModuleSCSI_Command->Raid_ID;  // Raid Error handle for NT
    pAMEData->AMEBufferData[BufferID].pRequestExtension = pAME_ModuleSCSI_Command->pRequestExtension;
    pAMEData->AMEBufferData[BufferID].pMPIORequestExtension = pAME_ModuleSCSI_Command->pMPIORequestExtension;
    pAMEData->AMEBufferData[BufferID].pAME_Raid_QueueBuffer = pAME_ModuleSCSI_Command->pAME_Raid_QueueBuffer; // for RAID 0/1, but NT unused
    pAMEData->AMEBufferData[BufferID].callback = pAME_ModuleSCSI_Command->callback;
    
    pAMEData->AMEBufferData[BufferID].pAMEData_Path = pAMEData_Path;  // Raid Error handle for NT
    pAMEData->AMEBufferData[BufferID].Path_Raid_ID = pAME_ModuleSCSI_Command->Slave_Raid_ID;  // Raid Error handle for NT
    
    // Raid Error handling, stop cmd, return Busy
    if (pAMEData->AME_RAIDData[pAME_ModuleSCSI_Command->Raid_ID].Raid_Error_Handle_State == TRUE) {
        return FALSE;
    } 
    
    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_NT_Send_SCSI_Cmd(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSIRequest)
{
    AME_U32 ret = FALSE;
    
    switch (pAME_SCSIRequest->NTRAIDType)
    {
        case NTRAIDType_341:
            ret = AME_341_NT_Send_SCSI_Command(pAMEData,pAME_SCSIRequest);
            break;
            
        case NTRAIDType_2208:
            ret = AME_2208_NT_Send_SCSI_Command(pAMEData,pAME_SCSIRequest);
            if (ret == FALSE) {
                AME_NT_Raid_2208_Check_Bar_State(pAMEData,pAME_SCSIRequest->NT_RAID_System_Index);
            }
            break;
            
        case NTRAIDType_3108:
            ret = AME_3108_NT_Send_SCSI_Command(pAMEData,pAME_SCSIRequest);
            if (ret == FALSE) {
                AME_NT_Raid_3108_Check_Bar_State(pAMEData,pAME_SCSIRequest->NT_RAID_System_Index);
            }
            break;
            
        default:
            break;
    }
    
    return ret;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Build_SCSI_Cmd(pAME_Data pAMEData,pAME_Data pAMEData_Path,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command)
{
    AME_SCSI_REQUEST AME_SCSIRequest;
    AME_U32  BufferID = 0;
    
    if (AME_U32_NONE == (BufferID = AME_Request_Buffer_Queue_Allocate(pAMEData))) {
        return FALSE;
    }
    
    switch (pAMEData->Device_ID)
    {
        case AME_6101_DID_DAS:
        case AME_6201_DID_DAS:
            if (FALSE == AME_DAS_SCSI_Prepare(pAMEData,pAMEData_Path,pAME_ModuleSCSI_Command,&AME_SCSIRequest,BufferID)) {
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                return FALSE;
            }
            if (FALSE == AME_6201_6101_PrepareSG(pAMEData,&AME_SCSIRequest)) {
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                return FALSE;
            }
            if (FALSE == AME_6201_6201_Send_SCSI_Command(pAMEData,&AME_SCSIRequest)) {
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                return FALSE;
            }
            break;
        
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
            if (TRUE == AME_Raid_2208_3108_Bar_lost_Check(pAMEData)) {
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                return FALSE;
            }
            
            if (pAMEData == pAMEData_Path) {
                if (FALSE == AME_DAS_SCSI_Prepare(pAMEData,pAMEData_Path,pAME_ModuleSCSI_Command,&AME_SCSIRequest,BufferID)) {
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    return FALSE;
                }
                if (FALSE == AME_2208_PrepareSG(pAMEData,&AME_SCSIRequest)) {
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    return FALSE;
                }
                if (FALSE == AME_2208_3108_Send_SCSI_Command(pAMEData,&AME_SCSIRequest)) {
                    /* DAS 2208 Raid Link Error handle */
                    AME_Raid_2208_3108_Check_Bar_State(pAMEData);
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    return FALSE;
                }
            }
            else {
                //AME_Msg("(%d):(%d)AME_Buid_SCSI_CMD:  BufferID(%d)\n",pAMEData->AME_Serial_Number,pAMEData_Path->AME_Serial_Number,BufferID);
                if (FALSE == AME_DAS_SOFT_RAID_SCSI_Prepare(pAMEData,pAMEData_Path,pAME_ModuleSCSI_Command,&AME_SCSIRequest,BufferID)) {
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    return FALSE;
                }
                if (FALSE == AME_2208_Soft_Raid_PrepareSG(pAMEData,&AME_SCSIRequest)) {
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    return FALSE;
                }
                if (FALSE == AME_DAS_SOFT_RAID_Send_SCSI_Command(pAMEData,pAMEData_Path,&AME_SCSIRequest)) {
                    /* DAS 2208 Raid Link Error handle */
                    AME_Raid_2208_3108_Check_Bar_State(pAMEData);
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    return FALSE;
                }
            }
            
            break;
            
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            if (TRUE == AME_Raid_2208_3108_Bar_lost_Check(pAMEData)) {
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                return FALSE;
            }
            
            if (FALSE == AME_DAS_SCSI_Prepare(pAMEData,pAMEData_Path,pAME_ModuleSCSI_Command,&AME_SCSIRequest,BufferID)) {
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                return FALSE;
            }
            if (FALSE == AME_3108_PrepareSG(pAMEData,&AME_SCSIRequest)) {
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                return FALSE;
            }
            if (FALSE == AME_2208_3108_Send_SCSI_Command(pAMEData,&AME_SCSIRequest)) {
                /* DAS 2208 Raid Link Error handle */
                AME_Raid_2208_3108_Check_Bar_State(pAMEData);
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                return FALSE;
            }
            break;
            
        case AME_8508_DID_NT:
        case AME_8608_DID_NT:
        case AME_87B0_DID_NT:
            if (FALSE == AME_NT_SCSI_Prepare(pAMEData,pAMEData_Path,pAME_ModuleSCSI_Command,&AME_SCSIRequest,BufferID)) {
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                return FALSE;
            }
            if (FALSE == AME_NT_SCSI_PrepareSG(pAMEData,&AME_SCSIRequest)) {
                AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                return FALSE;
            }
            if (pAMEData == pAMEData_Path){ // master, direct send cmd
                if (FALSE == AME_NT_Send_SCSI_Cmd(pAMEData,&AME_SCSIRequest)) {
                    AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
                    return FALSE;
                }
            }
            //else { // put cmd to Slave Req Fifo
            //    if (FALSE == AME_Req_Fifo_put(pAMEData,pAMEData_Path,&AME_SCSIRequest)) {
            //        AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
            //        return FALSE;
            //    }
            //    AME_Queue_Command(pAMEData,pAME_ModuleSCSI_Command->Raid_ID,NTRAIDType_MPIO_Slave,AME_SCSIRequest.SCSIRequest_physicalStartAddress,NULL);
            //}
            break;
            
        default:
            AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
            AME_Msg_Err("(%d)AME_Build_SCSI_Cmd with unknow device \n",pAMEData->AME_Serial_Number);
            return FALSE;
            
    }
    
    return TRUE;
    
}

AME_U32 AME_Cmd_All_Timeout(pAME_Data pAMEData)
{
    AME_U32 BufferID;

    AME_Msg_Err("(%d)Enter AME_Cmd_All_Timeout \n",pAMEData->AME_Serial_Number);
    
    for (BufferID=1; BufferID<pAMEData->Total_Support_command; BufferID++) {
        
        if (pAMEData->AMEBufferData[BufferID].ReqInUse == FALSE) {
            continue;
        }

        AME_Queue_Abort_Command(pAMEData,pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr);
        
        pAMEData->AMEBufferData[BufferID].ReqTimeouted = TRUE;
        AME_Msg_Err("(%d)AME_Cmd_Timeout BufferID=%d\n",pAMEData->AME_Serial_Number, BufferID);
   
    }
    
    // set InBandFlag to INBANDCMD_IDLE.
    pAMEData->AMEInBandBufferData.InBandFlag = INBANDCMD_IDLE;
    
    AME_Msg_Err("(%d)Leave AME_Cmd_Timeout done.\n",pAMEData->AME_Serial_Number);
    return TRUE;
    
}


AME_U32 AME_Cmd_Timeout(pAME_Data pAMEData,PVOID pRequestExtension)
{
    AME_U32 BufferID;

    AME_Msg_Err("(%d)Enter AME_Cmd_Timeout[%p] \n",pAMEData->AME_Serial_Number,pRequestExtension);
    
    for (BufferID=1;BufferID<pAMEData->Total_Support_command;BufferID++) {
        
        if (pAMEData->AMEBufferData[BufferID].ReqInUse == FALSE) {
            continue;
        }

        if (pAMEData->AMEBufferData[BufferID].pRequestExtension == pRequestExtension) {

            AME_Queue_Abort_Command(pAMEData,pAMEData->AMEBufferData[BufferID].Request_Buffer_Phy_addr);
            
            pAMEData->AMEBufferData[BufferID].ReqTimeouted = TRUE;
            AME_Msg_Err("(%d)Leave AME_Cmd_Timeout done.BufferID=%d Raid ID = %d\n",pAMEData->AME_Serial_Number, BufferID,pAMEData->AMEBufferData[BufferID].Raid_ID);
            return TRUE;
        }
   
    }
    
    AME_Msg_Err("(%d)Leave AME_Cmd_Timeout: Not Found the Request \n",pAMEData->AME_Serial_Number);

    return FALSE;
}


AME_U32 AME_Cmd_Mark_Raid_Path_Remove(pAME_Data pAMEData,pAME_Data pAMEData_Path,AME_U32 Path_Raid_ID)
{
    AME_U32 BufferID;

    for (BufferID=1;BufferID<pAMEData->Total_Support_command;BufferID++) {
        
        if (pAMEData->AMEBufferData[BufferID].ReqInUse == FALSE) {
            continue;
        }
        
        if (pAMEData->AMEBufferData[BufferID].ReqTimeouted == TRUE) {
            continue;
        }
        
        if ((pAMEData->AMEBufferData[BufferID].pAMEData_Path == pAMEData_Path) &&
            (pAMEData->AMEBufferData[BufferID].Path_Raid_ID == Path_Raid_ID)) {
            pAMEData->AMEBufferData[BufferID].ReqIn_Path_Remove = TRUE;
            AME_Msg_Err("(%d)AME_Cmd_Mark_Raid_Path_Remove: Mark Request[%d] for Raid(%d:%d) path remove ...\n",pAMEData->AME_Serial_Number,BufferID,pAMEData_Path->AME_Serial_Number,Path_Raid_ID);
        }
        
    }

    return TRUE;
}

AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_CleanupPathRemoveRequest(pAME_Data pAMEData)
{
    AME_U32 BufferID;
    AME_REQUEST_CallBack    AMEREQUESTCallBack;

    AME_Msg_Err("(%d)Enter AME_CleanupPathRemoveRequest CMD count[%d] \n",pAMEData->AME_Serial_Number,pAMEData->AMEBufferissueCommandCount);
    
    for (BufferID=1;BufferID<pAMEData->Total_Support_command;BufferID++) {
        
        if (pAMEData->AMEBufferData[BufferID].ReqIn_Path_Remove == FALSE) {
            continue;
        }
        
        AMEREQUESTCallBack.pAMEData=pAMEData;
        AMEREQUESTCallBack.pRequestExtension = pAMEData->AMEBufferData[BufferID].pRequestExtension;
        AMEREQUESTCallBack.pMPIORequestExtension = pAMEData->AMEBufferData[BufferID].pMPIORequestExtension;
        AMEREQUESTCallBack.pAME_Raid_QueueBuffer = pAMEData->AMEBufferData[BufferID].pAME_Raid_QueueBuffer;  // for RAID 0/1, but NT unused
        AMEREQUESTCallBack.pRequest = 0x00;
        AMEREQUESTCallBack.pReplyFrame = 0x00;
        AMEREQUESTCallBack.pSCSISenseBuffer = 0x00;
        AMEREQUESTCallBack.Raid_ID = pAMEData->AMEBufferData[BufferID].Raid_ID;
        AMEREQUESTCallBack.TransferedLength = 0x00;
        AMEREQUESTCallBack.Retry_Cmd = TRUE;
        
        MPIO_Host_Error_handler_SCSI_REQUEST(&(AMEREQUESTCallBack));
        AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
        
    }

    AME_Msg_Err("(%d)Leave AME_CleanupPathRemoveRequest CMD count[%d] \n",pAMEData->AME_Serial_Number,pAMEData->AMEBufferissueCommandCount);

    return TRUE;
}


void 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_CleanupAllOutstandingRequest(pAME_Data pAMEData)
{
    AME_U32 BufferID;
    AME_REQUEST_CallBack    AMEREQUESTCallBack;

    AME_Msg_Err("(%d)Enter CleanupAllOutstandingRequestCommnad CMD count[%d] \n",pAMEData->AME_Serial_Number,pAMEData->AMEBufferissueCommandCount);
    
    AME_Queue_Data_ReInit(pAMEData);
    
    for (BufferID=1;BufferID<pAMEData->Total_Support_command;BufferID++) {
        
        if (pAMEData->AMEBufferData[BufferID].ReqInUse == FALSE) {
            continue;
        }
        
        if (pAMEData->AMEBufferData[BufferID].ReqTimeouted == TRUE) {
            AME_Msg_Err("(%d)BufferID[%d] request marked Timeout\n",pAMEData->AME_Serial_Number,BufferID);
            AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
            continue;
        }
        
        AMEREQUESTCallBack.pAMEData=pAMEData;
        AMEREQUESTCallBack.pRequestExtension = pAMEData->AMEBufferData[BufferID].pRequestExtension;
        AMEREQUESTCallBack.pMPIORequestExtension = pAMEData->AMEBufferData[BufferID].pMPIORequestExtension;
        AMEREQUESTCallBack.pAME_Raid_QueueBuffer = pAMEData->AMEBufferData[BufferID].pAME_Raid_QueueBuffer;  // for RAID 0/1, but NT unused
        AMEREQUESTCallBack.pRequest = 0x00;
        AMEREQUESTCallBack.pReplyFrame = 0x00;
        AMEREQUESTCallBack.pSCSISenseBuffer = 0x00;
        AMEREQUESTCallBack.Raid_ID = pAMEData->AMEBufferData[BufferID].Raid_ID;
        AMEREQUESTCallBack.TransferedLength = 0x00;
        
        MPIO_Host_Error_handler_SCSI_REQUEST(&(AMEREQUESTCallBack));
        AME_Request_Buffer_Queue_Release(pAMEData,BufferID);
        
    }

    // set InBandFlag to INBANDCMD_IDLE.
    pAMEData->AMEInBandBufferData.InBandFlag = INBANDCMD_IDLE;
    
    AME_Msg_Err("(%d)Leave CleanupAllOutstandingRequestCommnad CMD count[%d] \n",pAMEData->AME_Serial_Number,pAMEData->AMEBufferissueCommandCount);

    return;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_NT_Check_SendVir_Ready(pAME_Data pAMEData)
{
    AME_U32  Raid_ID;
    
    for (Raid_ID=0; Raid_ID<MAX_RAID_System_ID; Raid_ID++)
    {
        if ( (pAMEData->AME_NTHost_data.Raid_All_Topology >> Raid_ID) & 0x01 )
        {
            if (pAMEData->AME_RAIDData[Raid_ID].SendVir_Ready == FALSE) {
                return FALSE;
            }
            
            //if ((pAMEData->AME_RAIDData[Raid_ID].InBand_Loaded_Ready == FALSE)) {
            //    if (AME_InBand_Get_Page(pAMEData,Raid_ID,Lun_Mask_Page,AME_InBand_Loaded_Get_Lun_Mask_Page_callback) == FALSE) {
            //        continue;                
            //    }
            //    if (AME_InBand_Get_Page(pAMEData,Raid_ID,0,AME_InBand_Loaded_Get_Page_0_callback) == FALSE) {
            //        continue;                
            //    }
            //    return FALSE;
            //}
            
        }
    }
 
    return TRUE;
}


AME_U32 AME_NT_Kick_SendVir_Not_Ready_Raid_Out(pAME_Data pAMEData)
{
    AME_U32  Raid_ID;
    
    for (Raid_ID=0; Raid_ID<MAX_RAID_System_ID; Raid_ID++) 
    {
        if ( (pAMEData->AME_NTHost_data.Raid_All_Topology >> Raid_ID) & 0x01 ) 
        {
            if (pAMEData->AME_RAIDData[Raid_ID].SendVir_Ready == FALSE)
            {
                AME_Msg_Err("(%d)NT Raid ID %d SendVirtual failed, kick this Raid out.\n",pAMEData->AME_Serial_Number,Raid_ID);
                
                pAMEData->AME_NTHost_data.Raid_All_Topology -= (0x01 << Raid_ID);
                
                if ( (pAMEData->AME_NTHost_data.Raid_2208_3108_Topology >> Raid_ID) & 0x01 ) {
                    pAMEData->AME_NTHost_data.Raid_2208_3108_Topology -= (0x01 << Raid_ID);
                }
            }
        }
    }
 
    return TRUE;
}

AME_U32 AME_NT_Kick_InBand_Loaded_Not_Ready_Raid_Out(pAME_Data pAMEData)
{
    AME_U32  Raid_ID;
    
    for (Raid_ID=0; Raid_ID<MAX_RAID_System_ID; Raid_ID++) 
    {
        if ( (pAMEData->AME_NTHost_data.Raid_All_Topology >> Raid_ID) & 0x01 ) 
        {
            if (pAMEData->AME_RAIDData[Raid_ID].InBand_Loaded_Ready == FALSE)
            {
                AME_Msg_Err("(%d)NT Raid ID %d InBand_Loaded failed, kick this Raid out.\n",pAMEData->AME_Serial_Number,Raid_ID);
                
                pAMEData->AME_NTHost_data.Raid_All_Topology -= (0x01 << Raid_ID);
                
                if ( (pAMEData->AME_NTHost_data.Raid_2208_3108_Topology >> Raid_ID) & 0x01 ) {
                    pAMEData->AME_NTHost_data.Raid_2208_3108_Topology -= (0x01 << Raid_ID);
                }
                
                return TRUE;
            }
        }
    }
 
    return TRUE;
}

AME_U32 AME_NT_87B0_Workaround (pAME_Data pAMEData)
{
    /* PCIe link error (link phase lock) issue
     * The PCIe link between Host and Switch will no longer active after constant times hotplug in/out operation.
     * The problem is the link training phase lock from PLX link training state machine. 
     * An workaround method will be disable/enable corresponding downstream port.
     * The control register is Port Control Register of upstream port. Register offset is 208h[5:0].
     */
    
    if (pAMEData->AME_NTHost_data.Workaround_87B0_Done == TRUE) {
        return FALSE;
    }
    
    if (pAMEData->AME_NTHost_data.Port_Number == 1) {
            AME_Msg("(%d)AME_NT_87B0_Workaround, Port 1 Workaround done.\n",pAMEData->AME_Serial_Number);
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x208,2);
            AME_IO_milliseconds_Delay(1500);
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x208,0);
            AME_IO_milliseconds_Delay(500);
    } else {
            AME_Msg("(%d)AME_NT_87B0_Workaround, Port 2 Workaround done.\n",pAMEData->AME_Serial_Number);
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x208,4);
            AME_IO_milliseconds_Delay(1500);
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,0x208,0);
            AME_IO_milliseconds_Delay(500);
    }
    
    pAMEData->AME_NTHost_data.Workaround_87B0_Done = TRUE;
    
    return TRUE;
}

AME_U32 AME_NT_87B0_Workaround_Check (pAME_Data pAMEData)
{
    /* PCIe link error (link phase lock) issue
     * The PCIe link between Host and Switch will no longer active after constant times hotplug in/out operation.
     * The problem is the link training phase lock from PLX link training state machine. 
     * An workaround method will be disable/enable corresponding downstream port.
     * The control register is Port Control Register of upstream port. Register offset is 208h[5:0].
     */
    
    AME_U32  i,tmp_reg,Raid_ID;
    AME_U32  RAID_OFFSET = 0;
    
    if (pAMEData->Device_ID != AME_87B0_DID_NT) {
        return FALSE;
    }
    
    if (pAMEData->AME_NTHost_data.Raid_All_Topology == 0x00) {
        if (TRUE == AME_NT_87B0_Workaround(pAMEData)) {
            return TRUE;
        }
        return FALSE;
    }
    
    /* Check link lock mathod:
     * Find any exist Raid to read register if register data = MUEmpty
     * means link lock error, driver will reset port1 or port2 link.
     */
    for (i=0; i<MAX_RAID_System_ID; i++) {
        if ((pAMEData->AME_NTHost_data.Raid_All_Topology >> i) & 0x01 ) {
            Raid_ID = i;
            break;
        }
    }
    
    RAID_OFFSET = Raid_ID*ACS2208_BAR_OFFSET + pAMEData->AME_NTHost_data.NTBar_Offset;
    
    switch (pAMEData->AME_RAIDData[Raid_ID].NTRAIDType)
    {
        case NTRAIDType_341:
        case NTRAIDType_2208:
        case NTRAIDType_3108:
            tmp_reg = AME_Address_read_32(pAMEData,pAMEData->Raid_Base_Address,AME_HOST_INT_MASK_REGISTER + RAID_OFFSET);
            break;
        
        default:
            AME_Msg_Err("(%d)Error:  Unknow index %d Raid type, can't AME_NT_87B0_Workaround_Check.\n",pAMEData->AME_Serial_Number,Raid_ID);
            return FALSE;
    }
    
    /* Add 5 sec delay to re-check link lock error.
     * Because sometimes read register data = MUEmpty, maybe is Raid to SW link error.
     * In the time SW try to fixed link issue, so driver wait 5 sec avoid check error. 
     */
    if (tmp_reg == MUEmpty) {
        
        // Wait 5 sec to chek is Raid Bar lose issue or link lock issue.
        
        if (++pAMEData->AME_NTHost_data.Workaround_87B0_Wait_index > 5) {
            pAMEData->AME_NTHost_data.Workaround_87B0_Wait_index = 0;
            if (TRUE == AME_NT_87B0_Workaround(pAMEData)) {
                return TRUE;
            }
        } else {
            //AME_Msg_Err("(%d)Error:  AME_NT_87B0_Workaround_Check failed, wait to check is Bar lose issue (%d).\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.Workaround_87B0_Wait_index);
        }
        
    } else {
        pAMEData->AME_NTHost_data.Workaround_87B0_Wait_index = 0;
    }
    
    return FALSE;
}

AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_NT_Hotplug_Handler (pAME_Data pAMEData)
{
    AME_U32  i,NT_SW_Ready_State,Link_Side_Link_State;
    
    // Only NT Device need handle.
    if (FALSE == AME_Check_is_NT_Mode(pAMEData)) {
        return TRUE;
    }
    
    // Only support one NT device
    if (pAMEData->Is_First_NT_device == FALSE) {
        return TRUE;
    }
    
    // Workaround fixed HBA 8717 PCIe link phase lock issue
    if (TRUE == AME_NT_87B0_Workaround_Check(pAMEData)) {
        return TRUE;
    }
    
    NT_SW_Ready_State = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.SCRATCH_REG_4);
    Link_Side_Link_State = AME_Address_read_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_LINK_LINKSTATE_REG_OFFSET) & 0xF00000;
    //AME_Msg("(%d)NT_SW_Ready_State = 0x%x\n",pAMEData->AME_Serial_Number,NT_SW_Ready_State);
    
    /* NT Cable Plug out */
    if (Link_Side_Link_State == 0)
    {
        if ((NT_SW_Ready_State != 0) && 
            (pAMEData->AME_NTHost_data.Hotplug_State != NT_Hotplug_First_Load_Wait_SW_Ready))
        {
            AME_U32 Raid_All_Topology = pAMEData->AME_NTHost_data.Raid_All_Topology;
            
            AME_Msg("(%d)NT Cable plug out\n",pAMEData->AME_Serial_Number);
            
            pAMEData->AME_NTHost_data.Hotplug_State = NT_Hotplug_First_Load_Wait_SW_Ready;
            pAMEData->AME_NTHost_data.PCISerialNumber = 0;
            pAMEData->AME_NTHost_data.Raid_All_Topology = 0;
            pAMEData->AME_NTHost_data.Raid_2208_3108_Topology = 0;
            pAMEData->AME_NTHost_data.Error_Raid_bitmap = 0;
            pAMEData->AME_NTHost_data.Raid_Change_Topology = 0;
            
            /* Clear NT link/virtual DoorBell */
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG,0xFFFF);
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_Link_IRQ_REG,0xFFFF);
            
            /* Mask NT link/virtual DoorBell ISR */
            //AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_MASK_REG,0xFFFF);
            //AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_SET_Link_IRQ_MASK_REG,0xFFFF);
            
            // Clear NT_SW_Ready_State
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.SCRATCH_REG_4,0x00);

            // Clear Raid bitmap = 0x00
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.SCRATCH_REG_2,0x00);
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.SCRATCH_REG_5,0x00);
            
            // Remove Raid's Lun
            for (i=0; i<MAX_RAID_System_ID; i++) {
                if ((Raid_All_Topology >> i) & 0x01 ) {
                    AME_NT_Report_Raid_Remove(pAMEData,i);
                }
            }
            
            // Clear all Request command and report scsi busy to retry again
            //AME_CleanupAllOutstandingRequest(pAMEData);
            
        }
        pAMEData->Polling_init_Enable = FALSE;
        return TRUE;
    }
    
    /* Hotplug State
     * State 0 :  Wait SW Ready, do first negotaition to SW ...
     * State 1 :  Wait SW send SW info and send SendVir Request to Raid.
     * State 2 :  Check every exist Raid SendVir ready. If ready scan all Lun.
     * State 3 :  Handle Raid change.
     * State 4 :  Wait Plug in Raid, SendVir ready.
     */
    
    switch (pAMEData->AME_NTHost_data.Hotplug_State)
    {
        case NT_Hotplug_First_Load_Wait_SW_Ready:
            if (NT_SW_Ready_State != 0)
            {
                /* Try to change Link speed to Gen2 */
                AME_NT_8608_Change_Link_to_Gen2(pAMEData);
                
                /* Clear before ISR and enable NT link/virtual DoorBell */
                AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_REG,0xFFFF);
                AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_VIRTUAL_IRQ_MASK_REG,0xFFFF);
                AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_CLEAR_Link_IRQ_MASK_REG,0xFFFF);
                
                /* Trigger ISR 0x01 to SW, SW will return Raid infomation */
                AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_SET_Link_IRQ_REG,0x01);
                AME_Msg("(%d)Trigger ISR to SW, wait to receive SW INFO ...\n",pAMEData->AME_Serial_Number);
                
                pAMEData->AME_NTHost_data.Hotplug_State++;
            }
            
            break;
        
        case NT_Hotplug_First_Load_SendVirtual_Cmd:
            if ( (pAMEData->AME_NTHost_data.PCISerialNumber != 0) && (Link_Side_Link_State != 0) ) {
                
                for (i=0; i<MAX_RAID_System_ID; i++)
                {
                    if ( (pAMEData->AME_NTHost_data.Raid_All_Topology >> i) & 0x01 ) {
                        AME_Build_SendVirtual_Cmd(pAMEData,i);
                    }
                }
                pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index = 0;
                pAMEData->AME_NTHost_data.Hotplug_State++;
            }
            
            break;
            
        case NT_Hotplug_First_Load_Wait_Sendvir_Ready:
            // Scan all Lun ...
            if (AME_NT_Check_SendVir_Ready(pAMEData) == TRUE) {
                pAMEData->Polling_init_Enable = FALSE;
                AME_Msg("(%d)AME_NT SendVir Ready, Start to Scan all host lun\n",pAMEData->AME_Serial_Number);

                if (pAMEData->ReInit == TRUE) { // reinit done    
                    pAMEData->ReInit = FALSE;
                    
                    // for Win hotfix bug 7985
                    #if defined (AME_Windows)
                    AME_U32 Raid_ID;
                    
                    for (Raid_ID=0; Raid_ID<MAX_RAID_System_ID; Raid_ID++)
                    {
                        if ( (pAMEData->AME_NTHost_data.Raid_All_Topology >> Raid_ID) & 0x01 ) {
                            MPIO_Host_Scan_All_Lun(pAMEData,Raid_ID);
                        }
                    }
                    #endif
                } else {
                    // For mac can't call MPIO_Host_Scan_All_Lun in AME_InBand_Loaded_Get_Page_0_callback
                    // move function to here.
                    
                    AME_U32 Raid_ID;
                    
                    for (Raid_ID=0; Raid_ID<MAX_RAID_System_ID; Raid_ID++)
                    {
                        if ( (pAMEData->AME_NTHost_data.Raid_All_Topology >> Raid_ID) & 0x01 ) {
                            MPIO_Host_Scan_All_Lun(pAMEData,Raid_ID);
                        }
                    }
                }
                pAMEData->AME_NTHost_data.Hotplug_State++;
            } else {
                AME_U32 Waiting_time;
                
                // When Polling case timer is 1 milli-Sec, normal case is 1 Sec.
                // Finally Total Waiting time is 10 secs.
                if (pAMEData->Polling_init_Enable == TRUE) {
                    Waiting_time = 10000;
                } else {
                    Waiting_time = 10;
                }
                
                if (++pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index > Waiting_time) {
                    AME_NT_Kick_SendVir_Not_Ready_Raid_Out(pAMEData);
                    AME_NT_Kick_InBand_Loaded_Not_Ready_Raid_Out(pAMEData);
                    pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index = 0;
                } else {
                    if (pAMEData->Polling_init_Enable == TRUE) {
                        if ((pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index%1000) == 0) {
                            AME_Msg("(%d)AME_NT SendVir not Ready, wait Raid ready (%d Sec)...\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index/1000);
                        }
                    } else {
                        AME_Msg("(%d)AME_NT SendVir not Ready, wait Raid ready (%d Sec)...\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index);
                    }
                }
            }
            break;
        
        case NT_Hotplug_Raid_Change_Check:
            // Handle Raid change ...
            if (pAMEData->AME_NTHost_data.Raid_Change_Topology != 0)
            {
                for (i=0; i<MAX_RAID_System_ID; i++)
                {
                    if ( (pAMEData->AME_NTHost_data.Raid_Change_Topology >> i) & 0x01 ) {
                        
                        if ( (pAMEData->AME_NTHost_data.Raid_All_Topology >> i) & 0x01 ) {
                            
                            if (pAMEData->AME_RAIDData[i].Raid_Error_Handle_State != TRUE) {
                                AME_Msg("(%d)NT Raid (%d) plug in\n",pAMEData->AME_Serial_Number,i);
                            }
                            
                            AME_Build_SendVirtual_Cmd(pAMEData,i);
                        }
                        else {
                            AME_Msg("(%d)NT Raid (%d) plug out\n",pAMEData->AME_Serial_Number,i);

                            /* Clear Error Raid request */
                            AME_NT_Cleanup_Error_Raid_Request(pAMEData,i);
                            
                            /* Clear Reply Queue entry state */
                            AME_NT_Cleanup_Reply_Queue_EntryState(pAMEData,i);

                            AME_NT_Report_Raid_Remove(pAMEData,i);
                            
                            pAMEData->AME_NTHost_data.Raid_Change_Topology -= (0x01 << i);
                        } 
                    }
                }
                
                pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index = 0;
                pAMEData->AME_NTHost_data.Hotplug_State++;
            }
            break;
        
        case NT_Hotplug_Raid_Change_Wait_Sendvir_Ready:
            // ReScan all Lun ...
            if (AME_NT_Check_SendVir_Ready(pAMEData) == TRUE) {
                AME_Msg("(%d)AME_NT_Check_SendVir_Ready, Start to ReScan all host lun\n",pAMEData->AME_Serial_Number);
                AME_NT_Report_Raid_Add(pAMEData);
                pAMEData->AME_NTHost_data.Hotplug_State--;
            } else {
                if (++pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index > 10) {
                    AME_NT_Kick_SendVir_Not_Ready_Raid_Out(pAMEData);
                    AME_NT_Kick_InBand_Loaded_Not_Ready_Raid_Out(pAMEData);
                    pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index = 0;
                }
                AME_Msg("(%d)AME_NT SendVir not Ready, wait Raid ready (%d Sec)...\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.Check_SendVirtual_Ready_retry_index);
            }
            break;
            
        default:
            AME_Msg_Err("(%d)AME_NT_Hotplug_Handler Error:  Why go to here (Step 0x%x) ???\n",pAMEData->AME_Serial_Number,pAMEData->AME_NTHost_data.Hotplug_State);
            break;
        
    }
    
    // Error handle Trigger ISR to NT host by self, test ISR lose issue.
    // Make sure next ISR must be 1 sec latter.
    if (pAMEData->Next_NT_ISR_Check != FALSE) {
        if (pAMEData->Next_NT_ISR_Check == TRUE) {
            pAMEData->Next_NT_ISR_Check++;
        } else {
            AME_Address_Write_32(pAMEData,pAMEData->NT_Base_Address,pAMEData->AME_NTHost_data.NT_SET_VIRTUAL_IRQ_REG,0x01);
            pAMEData->Next_NT_ISR_Check = FALSE;
        }
    }
    
    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
    DRV_MAIN_CLASS_NAME::
#endif
AME_Timer(pAME_Data pAMEData)
{
   /*
    * For DAS mode timer init handle.
    */
    if (pAMEData->Polling_init_Enable == FALSE) {
        if (AME_Get_RAID_Ready(pAMEData) == FALSE) {
            if (AME_Check_RAID_Ready_Status(pAMEData) == TRUE) {
                AME_Host_RAID_Ready(pAMEData);
            }
        }
    }
    
    /* 
     * LED control:  Support Device 
     * AME_8608_DID_NT,AME_87B0_DID_NT,AME_2208_DID1_DAS
     * AME_2208_DID2_DAS,AME_2208_DID3_DAS,AME_2208_DID4_DAS
     */
    if (TRUE == AME_Check_is_DAS_Mode(pAMEData)) {
        if (pAMEData->AMEBufferissueCommandCount > 0) {
            AME_LED_ON(pAMEData);
        } else {
            AME_LED_OFF(pAMEData);
        }
    }
    else {
        if ((pAMEData->AME_NTHost_data.Raid_All_Topology != 0) &&
            (pAMEData->AMEBufferissueCommandCount > 0)) {
            AME_LED_ON(pAMEData);
        } else {
            AME_LED_OFF(pAMEData);
        }
    }
    
    /*
     * NT Handler Hotplug Function
     */
    AME_NT_Hotplug_Handler(pAMEData);
    
    /* 
     * Lun Change detect
     */
    AME_Lun_Change_Check(pAMEData);
    
    /* 
     * DAS 2208 Raid Link Error handle: Support Device
     * AME_2208_DID1_DAS,AME_2208_DID2_DAS
     * AME_2208_DID3_DAS,AME_2208_DID4_DAS 
     * AME_3108_DID0_DAS,AME_3108_DID1_DAS     
     */
    if (AME_Raid_2208_3108_Link_Error_Check(pAMEData) == TRUE) {
        
        static AME_U32 Retry_Done_index = 0;
        
        // Turn on Event SW
        pAMEData->AMEEventSwitchState = AME_EVENT_SWTICH_OFF;
        AME_Event_Turn_Switch(pAMEData,AME_EVENT_SWTICH_ON);
        AME_Msg("(%d)AME_Timer:  Send_EventSwitch Done\n",pAMEData->AME_Serial_Number);
        AME_Msg("(%d)AME_Timer:  Re-cover PCI-E Done, Retry %d times\n",pAMEData->AME_Serial_Number,++Retry_Done_index);
    }
    
    return TRUE;
}



