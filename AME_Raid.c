/*
 *  AME_Raid.c
 *  Acxxx
 *
 *  Copyright (c) 2004-2014 Accusys, Inc. All rights reserved.
 *
 */


#include "AME_module.h"
#include "AME_import.h"
#include "MPIO.h"
#include "ACS_MSG.h"
#include "AME_Raid.h"

#if defined (AME_MAC_DAS)
    #include "Acxxx.h"
#endif


void AME_Raid_Queue_add_tail(pAME_Raid_Queue_List pHead,pAME_Raid_Queue_List pNew)
{
    pAME_Raid_Queue_List pPrev = (pAME_Raid_Queue_List)pHead->pPrev;
    
    pHead->pPrev = pNew;
    pNew->pNext = pHead;
    pNew->pPrev = pPrev;
    pPrev->pNext = pNew;
    
    return;
}

void AME_Raid_Queue_del_entry(pAME_Raid_Queue_List pEntry)
{
    pAME_Raid_Queue_List pPrev = (pAME_Raid_Queue_List)pEntry->pPrev;
    pAME_Raid_Queue_List pNext = (pAME_Raid_Queue_List)pEntry->pNext;
    
    pNext->pPrev = pPrev;
    pPrev->pNext = pNext;
    
    return;
}

AME_U32 AME_Raid_Queue_empty(pAME_Raid_Queue_List pHead)
{
    if (pHead->pNext == pHead) {
        return TRUE;
    } else {
        return FALSE;
    }
    
}


void AME_Raid_Queue_Init(pAME_Raid_Queue_List pHead)
{
   pHead->pNext = pHead;
   pHead->pPrev = pHead;
   
   return;
}


void AME_Raid_Clean_Queue_Buffer(pAME_Raid_Queue_Buffer pAME_Raid_QueueBuffer)
{
    AME_U32 i;
    
    if (pAME_Raid_QueueBuffer == NULL) {
        return;
    }
    
    pAME_Raid_QueueBuffer->ReqInUse = FALSE;
    pAME_Raid_QueueBuffer->Target_Index = 1;
    pAME_Raid_QueueBuffer->Complete_Index = 0;
    
    for (i=0;i<Maximun_Raid;i++) {
        AME_Memory_zero(&pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[i],sizeof(AME_Raid_Host_SG));
    }
    pAME_Raid_QueueBuffer->pAMERaid_QueueDATA = NULL;
    
    return;
}


AME_U32 AME_Raid_Data_Init(PVOID DeviceExtension,pAME_Raid_Queue_DATA pAMERaid_QueueDATA)
{
    AME_U32 i;
    
    pAMERaid_QueueDATA->Raid_Path_Count = Soft_Raid_Target;
    pAMERaid_QueueDATA->pDeviceExtension = DeviceExtension;
    AME_Raid_Queue_Init(&pAMERaid_QueueDATA->AME_Raid_Queue_List_Free);
    
    #if defined (Enable_AME_RAID_R0)
        #if defined (Enable_AME_RAID_R1)
            AME_Msg("AME Software Raid 1 Function support, Sritpe size = %d KB\n",AME_Raid_LBA_Stripe_Size/2);
        #else
            AME_Msg("AME Software Raid 0 Function support, Sritpe size = %d KB\n",AME_Raid_LBA_Stripe_Size/2);
        #endif
    #endif
    
    for (i=0; i<MAX_Command_AME; i++) {
        AME_Raid_Clean_Queue_Buffer(&pAMERaid_QueueDATA->AME_Raid_QueueBuffer[i]);
        AME_Raid_Queue_add_tail(&pAMERaid_QueueDATA->AME_Raid_Queue_List_Free,&pAMERaid_QueueDATA->AME_Raid_QueueBuffer[i].AMEQueue_list_Buffer);
    }
        
    return TRUE;
}


pAME_Raid_Queue_Buffer AME_Raid_Data_Get_Free_Buffer(pAME_Raid_Queue_DATA pAMERaid_QueueDATA)
{
    pAME_Raid_Queue_List pHead,pNew;
    
    AME_Raid_spinlock(pAMERaid_QueueDATA);
    
    pHead = &pAMERaid_QueueDATA->AME_Raid_Queue_List_Free;
    pNew  = (pAME_Raid_Queue_List)pHead->pNext;
    
    if (TRUE == AME_Raid_Queue_empty(&pAMERaid_QueueDATA->AME_Raid_Queue_List_Free)) {
        AME_Raid_spinunlock(pAMERaid_QueueDATA);
        return FALSE;
    }
    
    AME_Raid_Queue_del_entry(pNew);
    
    AME_Raid_spinunlock(pAMERaid_QueueDATA);
    
    AME_Raid_Clean_Queue_Buffer((pAME_Raid_Queue_Buffer)pNew);
    
    return (pAME_Raid_Queue_Buffer)pNew;
}


AME_U32 AME_Raid_Data_Put_Free_Buffer(pAME_Raid_Queue_Buffer pAME_Raid_QueueBuffer)
{
    AME_U32 ret = FALSE;
    pAME_Raid_Queue_DATA pAMERaid_QueueDATA = (pAME_Raid_Queue_DATA)pAME_Raid_QueueBuffer->pAMERaid_QueueDATA;
    
    AME_Raid_spinlock(pAMERaid_QueueDATA);
    
    if (pAME_Raid_QueueBuffer->Target_Index == ++pAME_Raid_QueueBuffer->Complete_Index) {
        AME_Raid_Queue_add_tail(&pAMERaid_QueueDATA->AME_Raid_Queue_List_Free,(pAME_Raid_Queue_List)pAME_Raid_QueueBuffer);
        ret = TRUE;
    }
    
    AME_Raid_spinunlock(pAMERaid_QueueDATA);
    
    return ret;
}


AME_U64 AME_Get_LBA_Address(pAME_Module_SCSI_Command pAME_ModuleSCSI_Command)
{
    AME_U32 LBA_Address_16 = 0;
    AME_U32 LBA_Address_32 = 0;
    AME_U64 LBA_Address_64 = 0;
    
    switch (pAME_ModuleSCSI_Command->CDB[0])
    {
        case SCSIOP_READ6:
        case SCSIOP_WRITE6:
            LBA_Address_32 = *((AME_U32 *)&pAME_ModuleSCSI_Command->CDB[0]);
            LBA_Address_32 = AMEByteSwap32(LBA_Address_32 & 0x001FFFFF);
            LBA_Address_64 = (AME_U64)LBA_Address_32;
            AME_Msg("AME_Get_LBA_Address:  CDB6 LBA_Address_64 = 0x%160llx\n",LBA_Address_64);
            break;
        
        case SCSIOP_READ:
        case SCSIOP_WRITE:
        case SCSIOP_READ12:
        case SCSIOP_WRITE12:
            LBA_Address_32 = *((AME_U32 *)&pAME_ModuleSCSI_Command->CDB[2]);
            LBA_Address_32 = AMEByteSwap32(LBA_Address_32);
            LBA_Address_64 = (AME_U64)LBA_Address_32;
            break;
            
        case SCSIOP_READ16:
        case SCSIOP_WRITE16:
            LBA_Address_64 = *((AME_U64 *)&pAME_ModuleSCSI_Command->CDB[2]);
            LBA_Address_64 = AMEByteSwap64(LBA_Address_64);
            break;
        
        default:
            AME_Msg_Err("AME_Get_LBA_Address:  Unknow CDB Type.\n");
            break;
    }
    
    return  LBA_Address_64;
}


AME_U32 AME_Get_LBA_Length(pAME_Module_SCSI_Command pAME_ModuleSCSI_Command)
{
    AME_U8  LBA_Length_8 = 0;
    AME_U16 LBA_Length_16 = 0;
    AME_U32 LBA_Length_32 = 0;
    
    switch (pAME_ModuleSCSI_Command->CDB[0])
    {
        case SCSIOP_READ6:
        case SCSIOP_WRITE6:
            LBA_Length_8 = *((AME_U8 *)&pAME_ModuleSCSI_Command->CDB[4]);
            LBA_Length_32 = (AME_U32)(LBA_Length_8);
            break;
        
        case SCSIOP_READ:
        case SCSIOP_WRITE:
            LBA_Length_16 = *((AME_U16 *)&pAME_ModuleSCSI_Command->CDB[7]);
            LBA_Length_16 = AMEByteSwap16(LBA_Length_16);
            LBA_Length_32 = (AME_U32)(LBA_Length_16);
            break;
        
        case SCSIOP_READ12:
        case SCSIOP_WRITE12:
            LBA_Length_32 = *((AME_U32 *)&pAME_ModuleSCSI_Command->CDB[6]);
            LBA_Length_32 = AMEByteSwap32(LBA_Length_32);
            break;
            
        case SCSIOP_READ16:
        case SCSIOP_WRITE16:
            LBA_Length_32 = *((AME_U32 *)&pAME_ModuleSCSI_Command->CDB[10]);
            LBA_Length_32 = AMEByteSwap32(LBA_Length_32);
            break;
        
        default:
            AME_Msg_Err("AME_Get_LBA_Length:  Unknow CDB Type.\n");
            break;
    }
    
    return  LBA_Length_32;
}


void AME_Write_LBA_Address(pAME_Module_SCSI_Command pAME_ModuleSCSI_Command,AME_U64 LBA_Address_64)
{
    AME_U32 LBA_Address_32 = 0;
    AME_U32 LBA_Address_32_temp = 0;
    
    switch (pAME_ModuleSCSI_Command->CDB[0])
    {
        case SCSIOP_READ6:
        case SCSIOP_WRITE6:
            LBA_Address_32 = *((AME_U32 *)&pAME_ModuleSCSI_Command->CDB[0]);
            LBA_Address_32 = LBA_Address_32 & 0xFFE00000;
            LBA_Address_32_temp = (AME_U32)LBA_Address_64;
            LBA_Address_32_temp = AMEByteSwap32(LBA_Address_32_temp);
            if (LBA_Address_32_temp & 0xFFE00000) {
                AME_Msg_Err("CDB6 Type error.\n");
            }
            
            LBA_Address_32 = LBA_Address_32 | LBA_Address_32_temp;
            *((AME_U32 *)&pAME_ModuleSCSI_Command->CDB[0]) = LBA_Address_32;
            break;
        
        case SCSIOP_READ:
        case SCSIOP_WRITE:
        case SCSIOP_READ12:
        case SCSIOP_WRITE12:
            LBA_Address_32 = (AME_U32)LBA_Address_64;
            *((AME_U32 *)&pAME_ModuleSCSI_Command->CDB[2]) = AMEByteSwap32(LBA_Address_32);
            break;
        
        case SCSIOP_READ16:
        case SCSIOP_WRITE16:
            *((AME_U64 *)&pAME_ModuleSCSI_Command->CDB[2]) = AMEByteSwap64(LBA_Address_64);
            break;
        
        default:
            AME_Msg_Err("AME_Write_LBA_Address:  Unknow CDB Type.\n");
            break;
    }
    
    return;
}


void AME_Write_LBA_Length(pAME_Module_SCSI_Command pAME_ModuleSCSI_Command,AME_U32 LBA_Length_32)
{
    AME_U8  LBA_Length_8 = 0;
    AME_U16 LBA_Length_16 = 0;
    
    switch (pAME_ModuleSCSI_Command->CDB[0])
    {
        case SCSIOP_READ6:
        case SCSIOP_WRITE6:
            LBA_Length_8 = (AME_U8)LBA_Length_32;
            *((AME_U8 *)&pAME_ModuleSCSI_Command->CDB[4]) = LBA_Length_8;
            break;
        
        case SCSIOP_READ:
        case SCSIOP_WRITE:
            LBA_Length_16 = (AME_U16)LBA_Length_32;
            *((AME_U16 *)&pAME_ModuleSCSI_Command->CDB[7]) = AMEByteSwap16(LBA_Length_16);
            break;
        
        case SCSIOP_READ12:
        case SCSIOP_WRITE12:
            *((AME_U32 *)&pAME_ModuleSCSI_Command->CDB[6]) = AMEByteSwap32(LBA_Length_32);
            break;
            
        case SCSIOP_READ16:
        case SCSIOP_WRITE16:
            *((AME_U32 *)&pAME_ModuleSCSI_Command->CDB[10]) = AMEByteSwap32(LBA_Length_32);
            break;
        
        default:
            AME_Msg_Err("AME_Write_LBA_Length:  Unknow CDB Type.\n");
            break;
    }
    
    return;
}


AME_U32 AME_Raid_Show_pAME_Module_SCSI_Command(pAME_Module_SCSI_Command pAME_ModuleSCSI_Command,AME_U32 Cmd_type)
{
    AME_U32 i;
    
    AME_Msg("\n");
    AME_Msg("Sam Debug[%d]:  CDB_0[0x%x], Raid_ID = %d, Target_ID = %d\n",Cmd_type,pAME_ModuleSCSI_Command->CDB[0],pAME_ModuleSCSI_Command->Raid_ID,pAME_ModuleSCSI_Command->Target_ID);
    AME_Msg("Sam Debug[%d]:  Lun = %d, CDBLength = 0x%x, Control = 0x%x\n",Cmd_type,pAME_ModuleSCSI_Command->Lun_ID,pAME_ModuleSCSI_Command->CDBLength,pAME_ModuleSCSI_Command->Control);
    AME_Msg("Sam Debug[%d]:  DataLength = 0x%x, SGCount = %d, Slave_Raid_ID = 0x%x\n",Cmd_type,pAME_ModuleSCSI_Command->DataLength,pAME_ModuleSCSI_Command->SGCount,pAME_ModuleSCSI_Command->Slave_Raid_ID);
    AME_Msg("Sam Debug[%d]:  pRequestExtension = %p, MPIO_Which = 0x%x, pAME_Raid_QueueBuffer = %p\n",Cmd_type,pAME_ModuleSCSI_Command->pRequestExtension,pAME_ModuleSCSI_Command->MPIO_Which,pAME_ModuleSCSI_Command->pAME_Raid_QueueBuffer);
    
    for (i=0;i<pAME_ModuleSCSI_Command->CDBLength;i++) {
        AME_Msg("Sam Debug[%d]:  CDB[%X] = 0x%x\n",Cmd_type,i,pAME_ModuleSCSI_Command->CDB[i]);
    }
    
    AME_Msg("SG data  -------------------------------------------------------------------->\n");
    for (i=0; i<pAME_ModuleSCSI_Command->SGCount; i++) {
        AME_Msg("Sam Debug[%d]:  SG[%d] address = 0x%016llx,  BufferLength = 0x%x\n",Cmd_type,i,pAME_ModuleSCSI_Command->AME_Host_Sg[i].Address,pAME_ModuleSCSI_Command->AME_Host_Sg[i].BufferLength);
    }
    AME_Msg("SG data  <--------------------------------------------------------------------\n");
    
    return TRUE;
}


AME_U32 AME_Raid_R0_SG_algorithm(pAME_Module_SCSI_Command pAME_ModuleSCSI_Command)
{
    AME_U32 i=0,MPIO_Number=0,MPIO_Used=0,LBA_Length=0;
    AME_U64 address_Start=0,address_Limit=0,temp_length=0;
    pAME_Raid_Queue_Buffer pAME_Raid_QueueBuffer = (pAME_Raid_Queue_Buffer)pAME_ModuleSCSI_Command->pAME_Raid_QueueBuffer;
    pAME_Raid_Queue_DATA pAMERaid_QueueDATA = (pAME_Raid_Queue_DATA)pAME_Raid_QueueBuffer->pAMERaid_QueueDATA;
    
    address_Start = (AME_Get_LBA_Address(pAME_ModuleSCSI_Command)*Block_Size);
    // Bug:  In 32 bit OS can't devide 64 bit value.  in linux 32 bit OS error --->
    #if !defined (AME_Linux)
        address_Limit = ((address_Start/AME_Raid_Stripe_Size)+1)*AME_Raid_Stripe_Size;
        MPIO_Number = (address_Start/AME_Raid_Stripe_Size)%pAMERaid_QueueDATA->Raid_Path_Count; // Master = 0, Slave = 1
    #endif
    // Bug:  In 32 bit OS can't devide 64 bit value.  in linux 32 bit OS error <---
    
    for (i=0;i<pAME_ModuleSCSI_Command->SGCount;i++) {
        
        if ((address_Start + pAME_ModuleSCSI_Command->AME_Host_Sg[i].BufferLength) > address_Limit) {
            
            temp_length = address_Limit - address_Start;
            
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].AME_Host_Sg[pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount].Address = pAME_ModuleSCSI_Command->AME_Host_Sg[i].Address;
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].AME_Host_Sg[pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount].BufferLength = (AME_U32)temp_length;
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].Data_Size +=  pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].AME_Host_Sg[pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount].BufferLength;
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount++;
            
            if (++MPIO_Number == pAMERaid_QueueDATA->Raid_Path_Count) {
                MPIO_Number = 0;
            }
            
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].AME_Host_Sg[pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount].Address = pAME_ModuleSCSI_Command->AME_Host_Sg[i].Address + temp_length;
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].AME_Host_Sg[pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount].BufferLength = pAME_ModuleSCSI_Command->AME_Host_Sg[i].BufferLength - (AME_U32)temp_length;
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].Data_Size  += pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].AME_Host_Sg[pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount].BufferLength;
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount++;
            
            address_Limit += AME_Raid_Stripe_Size;
            
        } else {
            
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].AME_Host_Sg[pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount].Address = pAME_ModuleSCSI_Command->AME_Host_Sg[i].Address;
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].AME_Host_Sg[pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount].BufferLength = pAME_ModuleSCSI_Command->AME_Host_Sg[i].BufferLength;
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].Data_Size  += pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].AME_Host_Sg[pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount].BufferLength;
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[MPIO_Number].SGCount++;
            
            if ((address_Start + pAME_ModuleSCSI_Command->AME_Host_Sg[i].BufferLength) == address_Limit) {
                address_Limit += AME_Raid_Stripe_Size;
                if (++MPIO_Number == pAMERaid_QueueDATA->Raid_Path_Count) {
                    MPIO_Number = 0;
                }
            }
        }
        
        address_Start += pAME_ModuleSCSI_Command->AME_Host_Sg[i].BufferLength;
    }
    
    // Error check, check total LBA size is correct.
    LBA_Length = AME_Get_LBA_Length(pAME_ModuleSCSI_Command);
    
    if (LBA_Length != ((pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[0].Data_Size /Block_Size) +
                       (pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[1].Data_Size /Block_Size) + 
                       (pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[2].Data_Size /Block_Size) + 
                       (pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[3].Data_Size /Block_Size))) {
        AME_Msg_Err("Build Scsi R0 cmd, LBA length not match ???\n");
        return FALSE;
    }
    
    if ((LBA_Length*Block_Size) != (pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[0].Data_Size +
                                    pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[1].Data_Size +
                                    pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[2].Data_Size + 
                                    pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[3].Data_Size )) {
        AME_Msg_Err("Build Scsi R0 cmd, LBA length not match ???\n");
        return FALSE;
    }
    
    return TRUE;
}


AME_U32
#if defined (AME_MAC)
	DRV_MAIN_CLASS_NAME::
#endif
AME_Raid_Build_SCSI_R0_Cmd(pMPIO_DATA pHost_MPIODATA,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command)
{
    AME_U64 LBA_Start_Address,temp_Address;
    AME_U32 i,LBA_Length,MPIO_Number;
    pAME_Raid_Queue_Buffer pAME_Raid_QueueBuffer = (pAME_Raid_Queue_Buffer)pAME_ModuleSCSI_Command->pAME_Raid_QueueBuffer;
    pAME_Raid_Queue_DATA pAMERaid_QueueDATA = (pAME_Raid_Queue_DATA)pAME_Raid_QueueBuffer->pAMERaid_QueueDATA;
    
    LBA_Start_Address = AME_Get_LBA_Address(pAME_ModuleSCSI_Command);
    LBA_Length = AME_Get_LBA_Length(pAME_ModuleSCSI_Command);
    
    if (FALSE == AME_Raid_R0_SG_algorithm(pAME_ModuleSCSI_Command)) {
        AME_Msg("***************************************************************\n");
        AME_Msg("Sam Error Debug:  CDB = 0x%x LBA_Start_Address = 0x%016llx, Size = %d Kb\n",pAME_ModuleSCSI_Command->CDB[0],LBA_Start_Address,LBA_Length/2);
        AME_Msg("Sam Error Debug:  Master length = 0x%x, Slave length = 0x%x\n",pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[0].Data_Size,pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[1].Data_Size);
        AME_Msg("***************************************************************\n");
        if (FALSE == MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command)) {
            return FALSE;
        }
        return TRUE;
    }
    
    // Check Target index ...
    pAME_Raid_QueueBuffer->Target_Index = 0;
    for (i=0;i<Maximun_Raid;i++) {
        if (pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[i].Data_Size > 0) {
            pAME_Raid_QueueBuffer->Target_Index++;
        }
    }
    
    // Set Path LBA Address
    // Bug:  In 32 bit OS can't devide 64 bit value.  in linux 32 bit OS error --->
    #if !defined (AME_Linux)
        MPIO_Number = (LBA_Start_Address/AME_Raid_LBA_Stripe_Size)%pAMERaid_QueueDATA->Raid_Path_Count;
        temp_Address = (LBA_Start_Address/(AME_Raid_LBA_Stripe_Size*pAMERaid_QueueDATA->Raid_Path_Count))*AME_Raid_LBA_Stripe_Size;
    #endif
    // Bug:  In 32 bit OS can't devide 64 bit value.  in linux 32 bit OS error <---
    
    for (i=0;i<Maximun_Raid;i++) {
        if (i < MPIO_Number) {
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[i].LBA_Address = temp_Address + AME_Raid_LBA_Stripe_Size;
        }
        else if (i == MPIO_Number) {
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[i].LBA_Address = temp_Address + LBA_Start_Address%AME_Raid_LBA_Stripe_Size;
        }
        else if (i > MPIO_Number) {
            pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[i].LBA_Address = temp_Address;
        }
    }
    
    //AME_Msg("\n");
    //AME_Msg("Sam Debug:  CDB_0[0x%x], LBA_Start_Address = 0x%016llx, MPIO_Number = %d, Target_Index = %d\n",pAME_ModuleSCSI_Command->CDB[0],LBA_Start_Address,MPIO_Number,pAME_Raid_QueueBuffer->Target_Index);
    //AME_Msg("Sam Debug:  Master LBA Start = 0x%016llx, Slave LBA Start = 0x%016llx\n",pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[0].LBA_Address,pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[1].LBA_Address);
    //AME_Msg("Sam Debug:  Source LBA = 0x%x, Master LBA = 0x%x, Slave LBA = 0x%x\n",LBA_Length,pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[0].Data_Size/Block_Size,pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[1].Data_Size/Block_Size);
    
    // Dispatch scsi cmd
    for (i=0;i<Maximun_Raid;i++) {
        
        if (pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[i].Data_Size > 0) {
            
            AME_Memory_copy((PVOID)pAME_ModuleSCSI_Command->AME_Host_Sg,(PVOID)&pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[i].AME_Host_Sg[0],AME_Raid_SG_Size);
            pAME_ModuleSCSI_Command->MPIO_Which = i;
            pAME_ModuleSCSI_Command->SGCount = pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[i].SGCount;
            pAME_ModuleSCSI_Command->DataLength = pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[i].Data_Size;
            
            AME_Write_LBA_Address(pAME_ModuleSCSI_Command,pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[i].LBA_Address);
            AME_Write_LBA_Length(pAME_ModuleSCSI_Command,(pAME_Raid_QueueBuffer->AME_Raid_Host_Sg[i].Data_Size/Block_Size));
            
            if (pAME_ModuleSCSI_Command->DataLength == 0) {
                AME_Msg("Sam Debug:  Path[%d] Raid DataLength Error, CDB = 0x%x LBA_Start_Address = 0x%llx LBA_Length = 0x%x ???\n",i,pAME_ModuleSCSI_Command->CDB[0],LBA_Start_Address,LBA_Length);
                return FALSE;
            }
            
            if (FALSE == MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command)) {
                AME_Msg("Sam Debug:  Path[%d] Raid cmd failed ???\n",i);
                return FALSE;
            }
            
        }
        
    }
    
    return TRUE;
}


AME_U32
#if defined (AME_MAC)
	DRV_MAIN_CLASS_NAME::
#endif
AME_Raid_Build_SCSI_R1_Cmd(pMPIO_DATA pHost_MPIODATA,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command)
{
    static AME_U32 MPIO_Number=0;
    AME_U32 i,LBA_Length;
    AME_U64 LBA_Start_Address;
    pAME_Raid_Queue_Buffer pAME_Raid_QueueBuffer = (pAME_Raid_Queue_Buffer)pAME_ModuleSCSI_Command->pAME_Raid_QueueBuffer;
    pAME_Raid_Queue_DATA pAMERaid_QueueDATA = (pAME_Raid_Queue_DATA)pAME_Raid_QueueBuffer->pAMERaid_QueueDATA;
    
    LBA_Start_Address = AME_Get_LBA_Address(pAME_ModuleSCSI_Command);
    LBA_Length = AME_Get_LBA_Length(pAME_ModuleSCSI_Command);
    
    if ((SCSIOP_WRITE   == pAME_ModuleSCSI_Command->CDB[0]) ||
        (SCSIOP_WRITE6  == pAME_ModuleSCSI_Command->CDB[0]) ||
        (SCSIOP_WRITE12 == pAME_ModuleSCSI_Command->CDB[0]) ||
        (SCSIOP_WRITE16 == pAME_ModuleSCSI_Command->CDB[0])) {
        
        pAME_Raid_QueueBuffer->Target_Index = pAMERaid_QueueDATA->Raid_Path_Count;
        
        for (i=0;i<pAMERaid_QueueDATA->Raid_Path_Count;i++) {
            pAME_ModuleSCSI_Command->MPIO_Which = i;
            if (FALSE == MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command)) {
                AME_Msg("Sam Debug:  Path[%d] Raid Build cmd failed ???\n",i);
                return FALSE;
            }
        }
        
    } else {
        
        //MPIO_Number = (LBA_Start_Address/(AME_Raid_LBA_Stripe_Size*4))%2; // Master = 0, Slave = 1
        
        pAME_ModuleSCSI_Command->MPIO_Which = MPIO_Number;
        
        if (++MPIO_Number == pAMERaid_QueueDATA->Raid_Path_Count) {
            MPIO_Number = 0;
        }
        
        //AME_Msg("\n");
        //AME_Msg("Sam Debug:  CDB_0[0x%x], LBA_Start_Address = 0x%016llx\n",pAME_ModuleSCSI_Command->CDB[0],LBA_Start_Address);
        //AME_Msg("Sam Debug:  LBA Start = 0x%016llx, LBA_Length = 0x%x, , MPIO_Number %d\n",LBA_Start_Address,LBA_Length,MPIO_Number);
        
        if (FALSE == MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command)) {
            AME_Msg("Sam Debug:   Path[%d] Raid Build cmd failed ???\n",MPIO_Number);
            return FALSE;
        }
        
    }
    
    return TRUE;
}


// Sam for Develop R0/R1
AME_U32
#if defined (AME_MAC)
	DRV_MAIN_CLASS_NAME::
#endif
Analyze_SCSI_cmd(pMPIO_DATA pHost_MPIODATA,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command)
{
    AME_U32 i,ret=FALSE;
    
    switch (pAME_ModuleSCSI_Command->CDB[0])
    {
        case SCSIOP_TEST_UNIT_READY:
            //AME_Msg("TargetId = %02d, Lun_ID = %02d, SCSIOP_TEST_UNIT_READY\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID);
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
        
        case SCSIOP_REPORT_LUNS:
            //AME_Msg("TargetId = %02d, Lun_ID = %02d, SCSIOP_REPORT_LUNS\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID);
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
            
        case SCSIOP_INQUIRY:
            //AME_Msg("TargetId = %02d, Lun_ID = %02d, SCSIOP_INQUIRY\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID);
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
            
        case SCSIOP_MODE_SENSE6:
            //AME_Msg("TargetId = %02d, Lun_ID = %02d, SCSIOP_MODE_SENSE6\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID);
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
            
        case SCSIOP_MODE_SENSE10:
            //AME_Msg("TargetId = %02d, Lun_ID = %02d, SCSIOP_MODE_SENSE10\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID);
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
            
        case SCSIOP_READ_CAPACITY:
            //AME_Msg("TargetId = %02d, Lun_ID = %02d, SCSIOP_READ_CAPACITY10\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID);
            //for (i=0;i<pAME_ModuleSCSI_Command->CDBLength;i++) {
            //    AME_Msg("CDB[%X] = 0x%x\n",i,pAME_ModuleSCSI_Command->CDB[i]);
            //}
            //for (i=0;i<pAME_ModuleSCSI_Command->SGCount;i++) {
            //    AME_Msg("Sam Debug:  Source SG[%d] address = 0x%016llx,  BufferLength = 0x%x\n",i,pAME_ModuleSCSI_Command->AME_Host_Sg[i].Address,pAME_ModuleSCSI_Command->AME_Host_Sg[i].BufferLength);
            //}
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
            
        case SCSIOP_READ_CAPACITY16:
            //AME_Msg("TargetId = %02d, Lun_ID = %02d, SCSIOP_READ_CAPACITY16\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID);
            //for (i=0;i<pAME_ModuleSCSI_Command->CDBLength;i++) {
            //    AME_Msg("CDB[%X] = 0x%x\n",i,pAME_ModuleSCSI_Command->CDB[i]);
            //}
            //for (i=0;i<pAME_ModuleSCSI_Command->SGCount;i++) {
            //    AME_Msg("Sam Debug:  Source SG[%d] address = 0x%016llx,  BufferLength = 0x%x\n",i,pAME_ModuleSCSI_Command->AME_Host_Sg[i].Address,pAME_ModuleSCSI_Command->AME_Host_Sg[i].BufferLength);
            //}
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
        
        case SCSIOP_VERIFY:
            //AME_Msg("TargetId = %02d, Lun_ID = %02d, SCSIOP_VERIFY\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID);
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
            
        case SCSIOP_START_STOP_UNIT:
            //AME_Msg("TargetId = %02d, Lun_ID = %02d, SCSIOP_START_STOP_UNIT\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID);
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
        
        case SCSIOP_SYNCHRONIZE_CACHE:
            //AME_Msg("TargetId = %02d, Lun_ID = %02d, SCSIOP_SYNCHRONIZE_CACHE10\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID);
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
            
        case SCSIOP_SYNCHRONIZE_CACHE16:
            //AME_Msg("TargetId = %02d, Lun_ID = %02d, SCSIOP_SYNCHRONIZE_CACHE16\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID);
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
            
        case SCSIOP_READ:
        case SCSIOP_READ6:
        case SCSIOP_READ12:
        case SCSIOP_READ16:
        case SCSIOP_WRITE:
        case SCSIOP_WRITE6:
        case SCSIOP_WRITE12:
        case SCSIOP_WRITE16:
            //AME_Msg("TargetId = %2d, Lun_ID = %2d, SCSI R/W CDB [0x%x]\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID,pAME_ModuleSCSI_Command->CDB[0]);
            #if defined (Enable_AME_RAID_R1)
                ret = AME_Raid_Build_SCSI_R1_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            #else
                ret = AME_Raid_Build_SCSI_R0_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            #endif
            break;
        
        default:
            AME_Msg("TargetId = %2d, Lun_ID = %2d, unknow CDB [0x%x]\n",pAME_ModuleSCSI_Command->Target_ID,pAME_ModuleSCSI_Command->Lun_ID,pAME_ModuleSCSI_Command->CDB[0]);
            for (i=0;i<pAME_ModuleSCSI_Command->CDBLength;i++) {
                AME_Msg("CDB[%X] = 0x%x\n",i,pAME_ModuleSCSI_Command->CDB[i]);
            }
            ret = MPIO_Build_SCSI_Cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command);
            break;
    }
    
    return ret;
}


AME_U32
#if defined (AME_MAC)
	DRV_MAIN_CLASS_NAME::
#endif
AME_Raid_Build_SCSI_Cmd(pAME_Raid_Queue_DATA pAMERaid_QueueDATA, pMPIO_DATA pHost_MPIODATA, pAME_Module_SCSI_Command pAME_ModuleSCSI_Command)
{
    pAME_Raid_Queue_Buffer pAME_Raid_QueueBuffer;
    
    pAME_Raid_QueueBuffer = AME_Raid_Data_Get_Free_Buffer(pAMERaid_QueueDATA);
    
    if (pAME_Raid_QueueBuffer == NULL) {
        AME_Msg("AME_RAID:  AME_Raid_Data_Get_Free_Buffer failed ???\n");
        return FALSE;
    }
    
    //AME_Msg("AME_RAID:  get Buffer %p ------>\n",pAME_Raid_QueueBuffer);
    
    pAME_Raid_QueueBuffer->ReqInUse = TRUE;
    pAME_Raid_QueueBuffer->pAMERaid_QueueDATA = (PVOID)pAMERaid_QueueDATA;
    pAME_ModuleSCSI_Command->pAME_Raid_QueueBuffer = (PVOID)pAME_Raid_QueueBuffer;
    
    if (FALSE == Analyze_SCSI_cmd(pHost_MPIODATA,pAME_ModuleSCSI_Command)) {
        AME_Raid_Data_Put_Free_Buffer(pAME_Raid_QueueBuffer);
        AME_Msg("AME_RAID:  Analyze_SCSI_cmd failed ???\n");
        return FALSE;
    }
    
    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
AME_Raid_Host_Fast_handler_SCSI_REQUEST(pAME_REQUEST_CallBack pAME_REQUESTCallBack)
{
    pAME_Raid_Queue_Buffer pAME_Raid_QueueBuffer = (pAME_Raid_Queue_Buffer)pAME_REQUESTCallBack->pAME_Raid_QueueBuffer;
    
    if (pAME_Raid_QueueBuffer == NULL) {
        AME_Msg("Get Fast reply :  pAME_Raid_QueueBuffer is NULL ???\n");
        return FALSE;
    }
    
    if (TRUE == AME_Raid_Data_Put_Free_Buffer(pAME_Raid_QueueBuffer)) {
        AME_Host_Fast_handler_SCSI_REQUEST(pAME_REQUESTCallBack);
    }
    
    //AME_Msg("Get Fast reply :  Buffer %p Target_Index = %d , Complete_Index = %d\n",pAME_Raid_QueueBuffer,pAME_Raid_QueueBuffer->Target_Index,pAME_Raid_QueueBuffer->Complete_Index);
    return TRUE;
}


AME_U32
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
AME_Raid_Host_Normal_handler_SCSI_REQUEST(pAME_REQUEST_CallBack pAME_REQUESTCallBack)
{
    pAME_Raid_Queue_Buffer pAME_Raid_QueueBuffer = (pAME_Raid_Queue_Buffer)pAME_REQUESTCallBack->pAME_Raid_QueueBuffer;
    
    if (pAME_Raid_QueueBuffer == NULL) {
        AME_Msg("Get Normal reply :  pAME_Raid_QueueBuffer is NULL ???\n");
        return FALSE;
    }
    
    if (TRUE == AME_Raid_Data_Put_Free_Buffer(pAME_Raid_QueueBuffer)) {
        AME_Host_Normal_handler_SCSI_REQUEST(pAME_REQUESTCallBack);
    }
    
    //AME_Msg("Get Normal reply :  Buffer %p Target_Index = %d , Complete_Index = %d\n",pAME_Raid_QueueBuffer,pAME_Raid_QueueBuffer->Target_Index,pAME_Raid_QueueBuffer->Complete_Index);
    return TRUE;
}


