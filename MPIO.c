/*
 *  AME_MPIO.cpp
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

AME_U32		MPIO_HBA_Count=0;
AME_U32		MPIO_Inited=0;
MPIO_Global MPIOGlobal;


void MPIO_Request_Buffer_Queue_init(pMPIO_DATA pMPIODATA)
{

	pMPIO_Request_Buffer_QUEUE		pMPIOQueue = &pMPIODATA->MPIO_RequestBuffer_Queue;
	AME_U32							i;

	
	pMPIOQueue->MPIOfreequeuebufferCount=0;
	
	pMPIOQueue->MPIOfreequeuebuffer=(PVOID) (&pMPIOQueue->MPIOQueueBuffer[0]);
	pMPIOQueue->MPIOQueueBuffer[0].pNext=NULL;
	pMPIOQueue->MPIOfreequeuebufferCount++;
	
	for (i=0;i<MAX_MPIO_Queue_Depth-1;i++) {
		pMPIOQueue->MPIOQueueBuffer[i].pNext=(PVOID) (&pMPIOQueue->MPIOQueueBuffer[i+1]);
		pMPIOQueue->MPIOQueueBuffer[i+1].pNext=NULL;
		pMPIOQueue->MPIOfreequeuebufferCount++;
	}

	for (i=0;i<MAX_MPIO_Queue_Depth;i++) {
		pMPIOQueue->MPIOQueueBuffer[i].MPIO_QUEUEData.MPIO_BufferID=i;
	}

	return;
}


pMPIO_Request_QUEUE_Buffer MPIO_Request_Buffer_Queue_Allocate(pMPIO_DATA pMPIODATA)
{

	pMPIO_Request_Buffer_QUEUE		pMPIOQueue = &pMPIODATA->MPIO_RequestBuffer_Queue;
    pMPIO_Request_QUEUE_Buffer 		tempqueue;
	pAME_Data 						pAMEData = (pAME_Data)pMPIODATA->pAMEData;

    if (pMPIOQueue->MPIOfreequeuebufferCount == 0) // can't pending command, w/o pending buffer
    {
    	MPIO_Msg_Err("(%d)MPIO_Request_Buffer_Queue_Allocate, Fail by MPIOfreequeuebufferCount =0\n",pMPIODATA->MPIO_Serial_Number);
        return FALSE;
    }

	MPIO_spinlock(pMPIODATA);
    tempqueue = (pMPIO_Request_QUEUE_Buffer) pMPIOQueue->MPIOfreequeuebuffer;
	
    pMPIOQueue->MPIOfreequeuebuffer=tempqueue->pNext;
    pMPIOQueue->MPIOfreequeuebufferCount--;

    tempqueue->pNext=0;

	MPIO_spinunlock(pMPIODATA);
	
    return tempqueue;

}


AME_U32 MPIO_Request_Buffer_Queue_Release(pMPIO_DATA pMPIODATA, pMPIO_Request_QUEUE_Buffer tempqueue)
{
	pMPIO_Request_Buffer_QUEUE		pMPIOQueue = &pMPIODATA->MPIO_RequestBuffer_Queue;
	pAME_Data 						pAMEData = (pAME_Data)pMPIODATA->pAMEData;
	

	MPIO_spinlock(pMPIODATA);
	
	// insert to free buffer queue
    tempqueue->pNext=(PVOID)pMPIOQueue->MPIOfreequeuebuffer;
    pMPIOQueue->MPIOfreequeuebuffer=(PVOID)tempqueue;
    pMPIOQueue->MPIOfreequeuebufferCount++;

	MPIO_spinunlock(pMPIODATA);

    return TRUE;
}


AME_U32 MPIO_Init_Prepare(pMPIO_DATA pMPIODATA,pAME_Data pAMEData,pAME_Module_Prepare pAMEModulePrepare)
{

	return  AME_Init_Prepare(pAMEData,pAMEModulePrepare);
		
}


AME_U32 MPIO_GetPAGE0_Data_Add(pMPIO_DATA pMPIODATA,pMPIO_Get_PAGE_0_Data pMPIO_Get_PAGE0_Data ,AME_U32 Raid_ID)
{
	AME_U32		i;

	if (pMPIODATA->MPIO_RAIDDATA[Raid_ID].GetPAGE0_Data.Ready != TRUE) {
		pMPIODATA->MPIO_RAIDDATA[Raid_ID].GetPAGE0_Data.Ready = TRUE;
	    AME_Memory_copy((PVOID)(pMPIODATA->MPIO_RAIDDATA[Raid_ID].GetPAGE0_Data.Production_ID),(PVOID)(pMPIO_Get_PAGE0_Data->Production_ID), 0x10);
		AME_Memory_copy((PVOID)(pMPIODATA->MPIO_RAIDDATA[Raid_ID].GetPAGE0_Data.Model_Name),(PVOID)(pMPIO_Get_PAGE0_Data->Model_Name), 0x10);
		AME_Memory_copy((PVOID)(pMPIODATA->MPIO_RAIDDATA[Raid_ID].GetPAGE0_Data.Serial_Number),(PVOID)(pMPIO_Get_PAGE0_Data->Serial_Number), 0x10);

		MPIO_Msg("(%d)New MPIO Raid :%d\n",pMPIODATA->MPIO_Serial_Number,Raid_ID);

		return TRUE;
	} else {

		MPIO_Msg_Err("(%d)New MPIO Raid :%d, Had exist???\n",pMPIODATA->MPIO_Serial_Number,Raid_ID);
	}

	return FALSE;
}


/* Software Raid R0 R1 support.
 * Only Mac needed, becase class error.
 * Wait Slave Path ready to Build_Das_SendVirtual and scan all Lun.
 */
#if defined (AME_MAC)
AME_U32
DRV_MAIN_CLASS_NAME::MPIO_Mac_Path_Ready(pMPIO_DATA pHost_MPIODATA)
{
    static int Hostscanlun = FALSE;
    static int HostSendviture = FALSE;
    AME_U32	PATH_ID;
    pAME_Data pAMEData_host,pAMEData_path;
    
    pAMEData_host = (pAME_Data)pHost_MPIODATA->pAMEData;
    
    if ((Hostscanlun != TRUE) &&
        (pHost_MPIODATA->MPIO_RAIDDATA[0].PATHCount == Soft_Raid_Target)) {
        
        if (HostSendviture != TRUE) {
            HostSendviture = TRUE;
            for (PATH_ID=1; PATH_ID<Soft_Raid_Target; PATH_ID++) {
                pAMEData_path = (pAME_Data)((pMPIO_DATA)pHost_MPIODATA->MPIO_RAIDDATA[0].Path_RAIDData[PATH_ID].pMPIOData)->pAMEData;
                AME_Build_Das_SendVirtual_Cmd(pAMEData_host,pAMEData_path,PATH_ID-1);
            }
        } else {
            Hostscanlun = TRUE;
            AME_Host_Scan_All_Lun(&AMEData,0);
        }
    }
    
    return TRUE;
}
#endif


AME_U32 MPIO_Path_Add(pMPIO_DATA pMPIODATA_host,pMPIO_DATA pMPIODATA_path,AME_U32 host_RaidID ,AME_U32 path_RaidID)
{
	AME_U32	PATH_ID;

	// avoid host add the same Raid path again.
	for (PATH_ID=0; PATH_ID<pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount; PATH_ID++) {
	    if ((path_RaidID == host_RaidID) &&
	        (pMPIODATA_path->MPIO_Serial_Number == pMPIODATA_host->MPIO_Serial_Number)) {
            MPIO_Msg_Err("MPIO_Path_Add Fail, it's exist path. Host:%d RAID_ID:%d\n",pMPIODATA_host->MPIO_Serial_Number,host_RaidID);
            return FALSE;
	    }
	}
	
	if (pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount > MPIO_MAX_support_PATH) {
		MPIO_Msg_Err("MPIO_Path_Add Fail. Host/Path/Host RAID_ID/Path RAID_ID:%d/%d/%d/%d.\n",pMPIODATA_host->MPIO_Serial_Number,pMPIODATA_path->MPIO_Serial_Number,host_RaidID,path_RaidID);
		MPIO_Msg_Err("MPIO_Path_Add Fail: Host:%d, RAID:%d add to MPIO Host:%d, RAID:%d. Path count > supported:%d\n",pMPIODATA_path->MPIO_Serial_Number,path_RaidID,pMPIODATA_host->MPIO_Serial_Number,host_RaidID,MPIO_MAX_support_PATH);
		return FALSE;
	}

	pMPIODATA_path->RAIDDATA[path_RaidID].Path_State = MPIO_Path_Built;
	pMPIODATA_path->RAIDDATA[path_RaidID].pMPIOData_host= pMPIODATA_host;
	pMPIODATA_path->RAIDDATA[path_RaidID].host_RaidID = host_RaidID;
	pMPIODATA_path->RAIDDATA[path_RaidID].Path_Number = pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount;

	PATH_ID = pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount;
	
	pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].Path_RAIDData[PATH_ID].Path_ID_of_Host_RAID = PATH_ID;
	pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].Path_RAIDData[PATH_ID].pMPIOData = pMPIODATA_path;
	pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].Path_RAIDData[PATH_ID].AME_RAID_ID = path_RaidID;
	pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PathCMDCount[PATH_ID]=0;
	

	pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount++;
	MPIO_Msg("MPIO_Path_Add : Host:%d, RAID:%d add to MPIO Host:%d, RAID:%d. Path count=%d\n",pMPIODATA_path->MPIO_Serial_Number,path_RaidID,pMPIODATA_host->MPIO_Serial_Number,host_RaidID,pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount);
	
	#if defined (Enable_AME_RAID_R0)
	    if (pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount == Soft_Raid_Target)
	    {
	        pAME_Data pAMEData_host,pAMEData_path;
	        
	        #if !defined (AME_MAC)  // Not support Mac, because Mac class error issue.
	            pAMEData_host = (pAME_Data)pMPIODATA_host->pAMEData;
	            for (PATH_ID=1; PATH_ID<Soft_Raid_Target; PATH_ID++) {
                    pAMEData_path = (pAME_Data)((pMPIO_DATA)pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].Path_RAIDData[PATH_ID].pMPIOData)->pAMEData;
                    AME_Build_Das_SendVirtual_Cmd(pAMEData_host,pAMEData_path,PATH_ID-1);
                }
            
	            MPIO_Msg("MPIO_Host_Scan_All_Lun,Host:%d, Raid:%d \n",pMPIODATA_host->MPIO_Serial_Number,host_RaidID);
                AME_Host_Scan_All_Lun((pAME_Data)pMPIODATA_host->pAMEData,host_RaidID);
            #endif
        }
    #endif

	return TRUE;
	

}


// host_data_keep : TRUE, for resume, path will be remove first, and then when AME ready, can add to same host.
AME_U32 MPIO_Path_Remove(pMPIO_DATA pMPIODATA_host,pMPIO_DATA pMPIODATA_path,AME_U32 host_RaidID ,AME_U32 path_RaidID, AME_U32 host_data_keep)
{
	AME_U32	i,j,PATHCount;


	pMPIODATA_path->RAIDDATA[path_RaidID].Path_State |= MPIO_Path_Disabled;
	pMPIODATA_path->RAIDDATA[path_RaidID].pMPIOData_host = NULL;
	//pMPIODATA_path->RAIDDATA[path_RaidID].host_RaidID = host_RaidID;

	PATHCount = pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount;
	for (i=0;i<PATHCount;i++) {
		if (pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].Path_RAIDData[i].pMPIOData == pMPIODATA_path) {
			pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount--;
			
			if (pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount == 0) {
				if (host_data_keep == FALSE) {
					pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].GetPAGE0_Data.Ready = FALSE;
				}
			}
			
			for (j=(i+1);j<PATHCount;j++) {
				pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].Path_RAIDData[j-1].Path_ID_of_Host_RAID = j-1;
				pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].Path_RAIDData[j-1].pMPIOData = pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].Path_RAIDData[j].pMPIOData;
				pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].Path_RAIDData[j-1].AME_RAID_ID = pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].Path_RAIDData[j].AME_RAID_ID;

			}
			MPIO_Msg("MPIO_Path_Remove : Host:%d, RAID:%d remove from MPIO Host:%d, RAID:%d. Path count=%d\n",pMPIODATA_path->MPIO_Serial_Number,path_RaidID,pMPIODATA_host->MPIO_Serial_Number,host_RaidID,pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount);
			return TRUE;
		}
	}
	
	MPIO_Msg_Err("MPIO_Path_Remove Fail. : Host/Path:%d/%d, path count=%d\n",pMPIODATA_host->MPIO_Serial_Number,pMPIODATA_path->MPIO_Serial_Number,pMPIODATA_host->MPIO_RAIDDATA[host_RaidID].PATHCount);	
	return FALSE;	

}


AME_U32 MPIO_Data_Init(pMPIO_DATA pMPIODATA, AME_U32 page_data_keep)
{
	AME_U32				i,j;
	pMPIO_RAID_DATA		pMPIO_RAIDDATA;

	for (i=0;i<MAX_RAID_System_ID;i++) {
		pMPIO_RAIDDATA = &pMPIODATA->MPIO_RAIDDATA[i];

		pMPIO_RAIDDATA->ConcurrentCMDCount = 0;
		pMPIO_RAIDDATA->TotalIssue_CMDCount = 0;
		pMPIO_RAIDDATA->PATHCount = 0;

		if (page_data_keep !=TRUE)
			pMPIO_RAIDDATA->GetPAGE0_Data.Ready = FALSE;
		
		for (j=0;j<MPIO_MAX_support_PATH;j++) {
			pMPIO_RAIDDATA->PathCMDCount[j]=0;
			pMPIO_RAIDDATA->Path_RAIDData[j].pMPIOData=NULL;
			
		}
	}

	for (i=0;i<MAX_RAID_System_ID;i++) {
		pMPIODATA->RAIDDATA[i].Path_State = MPIO_Path_stand_alone;
		pMPIODATA->RAIDDATA[i].pMPIOData_original= pMPIODATA;
		pMPIODATA->RAIDDATA[i].pMPIOData_host= NULL;
	}

	MPIO_Request_Buffer_Queue_init(pMPIODATA);
	
	return TRUE;
}


AME_U32 MPIO_Init_Setup(pMPIO_DATA pMPIODATA) 
{
	AME_U32 	i,temp;
	AME_U32		number_used;
	
	// all of MPIO_Serial_Number and AME_Serial_Number can't with same value.
	if (!MPIO_Inited) {	// system boot
		pMPIODATA->MPIO_Serial_Number=MPIO_HBA_Count;
	}
	else { // get the unsed number (from 0 ~~)
		for (temp=0;temp<MPIO_MAX_support_HBA;temp++) {
			number_used = FALSE;
			for (i=0;i<MPIO_MAX_support_HBA;i++) {
				if (MPIOGlobal.pMPIOData[i] !=NULL) {
					if (MPIOGlobal.pMPIOData[i]->MPIO_Serial_Number==temp) { //number used
						number_used= TRUE;
						break;	
					}
				}
			}
			if (number_used == FALSE)
				break;
		}
		
		pMPIODATA->MPIO_Serial_Number=temp;
	}

	MPIO_HBA_Count++;
	return TRUE;
}


AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
MPIO_ReInit(pMPIO_DATA pMPIODATA,pAME_Data pAMEData,pAME_Module_Init_Prepare pAMEModule_Init_Prepare)
{

	pMPIO_DATA	pTemp_MPIODATA;
	int	i;

	MPIO_Msg("MPIO_ReInit : Host: [%d], Total Host: [%d]\n", pMPIODATA->MPIO_Serial_Number,MPIO_HBA_Count);
	
	if (FALSE == MPIO_Support_Check(pMPIODATA)) {
		return AME_ReInit(pAMEData,pAMEModule_Init_Prepare);
    }
	
	MPIO_Data_Init(pMPIODATA, TRUE);

	return AME_ReInit(pAMEData,pAMEModule_Init_Prepare);
}


AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
MPIO_Init(pMPIO_DATA pMPIODATA,pAME_Data pAMEData,pAME_Module_Init_Prepare pAMEModule_Init_Prepare)
{
	AME_U32 	i;

	pMPIODATA->pAMEData = (PVOID)pAMEData;
	pAMEData->pMPIOData = (PVOID)pMPIODATA;
	pMPIODATA->Vendor_ID = pAMEModule_Init_Prepare->Vendor_ID;
	pMPIODATA->Device_ID = pAMEModule_Init_Prepare->Device_ID;

  #ifdef Enable_MPIO_message
	ACS_message((pMSG_DATA)(pMPIODATA->pMSGDATA),MPIO_init,MPIO_Init_start,NULL);
  #endif
	
	if (MPIO_HBA_Count > MPIO_MAX_support_HBA) {
		MPIO_Msg_Err("MPIO_Init fail: Host_Count:%d >= supported Host:%d\n", pMPIODATA->MPIO_Serial_Number,MPIO_MAX_support_HBA);
		return FALSE;
	}

	if (!MPIO_Inited) { // system boot
		if (pMPIODATA->MPIO_Serial_Number == 0) { // first in
			#ifndef Disable_MPIO
			    MPIO_Msg("MPIO_Init : Enabled.\n");
			#else
			    MPIO_Msg("MPIO_Init : Disabled !!!.\n");
			#endif

			for (i=0;i<MPIO_MAX_support_HBA;i++) {
				MPIOGlobal.pMPIOData[i]=NULL;
			}
			MPIO_Inited = TRUE;
		}
		else {
			if (!MPIO_Inited) {
				for (i=0;i<20;i++) {
					AME_IO_milliseconds_Delay(100);
					//MPIO_Msg_Err("MPIO_Init [%d] ...\n", i);
					if (MPIO_Inited)
						break;
				}
		
				if (!MPIO_Inited) {
					MPIO_Msg_Err("MPIO_Init Fail !!! Host: [%d] ...\n", pMPIODATA->MPIO_Serial_Number);
					return FALSE;
				}
			}
		}
	}

	for (i=0;i<MPIO_MAX_support_HBA;i++) {
		if (MPIOGlobal.pMPIOData[i] ==NULL) {
			MPIOGlobal.pMPIOData[i] = pMPIODATA;
			break;
		}
	}

	MPIO_Msg("MPIO_Init :(%d), Total Host: [%d] ...\n", pMPIODATA->MPIO_Serial_Number,MPIO_HBA_Count);
	
	MPIO_Data_Init(pMPIODATA,FALSE);

	pAMEData->AME_Serial_Number = pMPIODATA->MPIO_Serial_Number;

  #ifdef Enable_MPIO_message
	ACS_message((pMSG_DATA)(pMPIODATA->pMSGDATA),MPIO_init,MPIO_Init_Done_Success,NULL);
  #endif
	
	return AME_Init(pAMEData,pAMEModule_Init_Prepare);
   
}


AME_U32 
MPIO_Single_NT_Check(pAME_Data pAMEData)
{
    AME_U32 i;
    
    for (i=0;i<MPIO_MAX_support_HBA;i++) {
		
		if (MPIOGlobal.pMPIOData[i] != NULL) {
			
			if ((MPIOGlobal.pMPIOData[i]->Device_ID == AME_8508_DID_NT) ||
			    (MPIOGlobal.pMPIOData[i]->Device_ID == AME_8608_DID_NT) ||
			    (MPIOGlobal.pMPIOData[i]->Device_ID == AME_87B0_DID_NT)) {
			    
			    if (pAMEData->pMPIOData == (PVOID)MPIOGlobal.pMPIOData[i]) {
			        return TRUE;
			    }
			    
			    return FALSE;
			}
		}
		
	}
    
    return FALSE;
}


AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
MPIO_ISR(pMPIO_DATA pMPIODATA)
{
	return AME_ISR((pAME_Data)pMPIODATA->pAMEData);
}


AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
MPIO_Start(pMPIO_DATA pMPIODATA)
{
	//MPIO_Msg("MPIO_Start : %d\n",pMPIODATA->MPIO_Serial_Number);
    return AME_Start((pAME_Data)pMPIODATA->pAMEData);
}


AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
MPIO_Stop(pMPIO_DATA pMPIODATA)
{
	AME_U32		i,j;
	AME_U32		path_RAID_ID;
	pMPIO_DATA 	path_MPIODATA,pMPIODATA_temp;

	
	if (MPIO_HBA_Count == 0) {
		MPIO_Msg_Err("MPIO_Stop (%d) fail: Host_Count = 0\n",pMPIODATA->MPIO_Serial_Number);
		return FALSE;		
	} else {
		MPIO_HBA_Count--;
    }

	// remove the MPIOGlobal.pMPIOData
	for (i=0;i<MPIO_MAX_support_HBA;i++) {
		if (MPIOGlobal.pMPIOData[i] == pMPIODATA) {
			MPIO_Msg("MPIO_Stop: (%d)\n",pMPIODATA->MPIO_Serial_Number);
			MPIOGlobal.pMPIOData[i] = NULL;
			for (j=i;j<(MPIO_MAX_support_HBA-1);j++) {
				MPIOGlobal.pMPIOData[j] = MPIOGlobal.pMPIOData[j+1];
			}
			break;
		}
	}

	if (FALSE == MPIO_Support_Check(pMPIODATA)) {
		return AME_Stop((pAME_Data)pMPIODATA->pAMEData);
    }


#ifdef MPIO_Stop_Report_To_Another_Host // this only can for windows.
	// 1. remove the the physical RAID of this Host.
	// 2. if still have raid of this MPIO host, report to another host.

	// 1. remove the the physical RAID of this Host.
	for (i=0;i<MAX_RAID_System_ID;i++) {
		if (pMPIODATA->RAIDDATA[i].Path_State == MPIO_Path_Built ) {
			MPIO_Path_Remove(pMPIODATA->RAIDDATA[i].pMPIOData_host,pMPIODATA,pMPIODATA->RAIDDATA[i].host_RaidID ,i,FALSE);
		}
	}

	// 2. if still have another Host raid of this MPIO host, report to another host.
	for (i=0;i<MAX_RAID_System_ID;i++) {
		if (pMPIODATA->MPIO_RAIDDATA[i].PATHCount > 0 ) {
			for (j=0;j<pMPIODATA->MPIO_RAIDDATA[i].PATHCount;j++) {
				path_MPIODATA = pMPIODATA->MPIO_RAIDDATA[i].Path_RAIDData[j].pMPIOData;
				path_RAID_ID = pMPIODATA->MPIO_RAIDDATA[i].Path_RAIDData[j].AME_RAID_ID;
				
				MPIO_Check_Multiple_Path(path_MPIODATA,&(path_MPIODATA->RAIDDATA[path_RAID_ID].GetPAGE0_Data),path_RAID_ID);
			}
		}
	}

	// This only can for Windows
	for (i=0;i<MPIO_MAX_support_HBA;i++) {
		if (MPIOGlobal.pMPIOData[i] != NULL) {
			pMPIODATA_temp = MPIOGlobal.pMPIOData[i];
			for (j=0;j<MAX_RAID_System_ID;j++) {
				AME_Host_Scan_All_Lun((pAME_Data)pMPIODATA_temp->pAMEData,j);
		    }
		}
	}

	MPIO_Data_Init(pMPIODATA,FALSE);

	return AME_Stop((pAME_Data)pMPIODATA->pAMEData);

#else
	MPIO_Data_Init(pMPIODATA,FALSE);

	return AME_Stop((pAME_Data)pMPIODATA->pAMEData);
#endif

}



AME_U32 MPIO_Support_Check(pMPIO_DATA pMPIODATA)
{

#ifdef Disable_MPIO
    return FALSE;
#endif

    switch (pMPIODATA->Device_ID)
    {
        case AME_8608_DID_NT:
		case AME_87B0_DID_NT:
			return TRUE;
			break;
			
	#if defined (Enable_AME_RAID_R0)
		case AME_2208_DID1_DAS:
		case AME_2208_DID3_DAS:
		    return TRUE;
			break;
    #endif
		default:
		    
			break;
    }
	return FALSE;

}



AME_U32 MPIO_Lun_Alive_Check (pMPIO_DATA pHost_MPIODATA,AME_U32 Raid_ID,AME_U32 Lun_ID)
{

	pMPIO_DATA	pPath_MPIODATA;

	if (FALSE == MPIO_Support_Check(pHost_MPIODATA)) {
		return AME_Lun_Alive_Check((pAME_Data)pHost_MPIODATA->pAMEData,Raid_ID,Lun_ID);
    }
	

	if (pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].PATHCount == 0) {
		return FALSE;
    }
		
	pPath_MPIODATA=pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].Path_RAIDData[0].pMPIOData;

	return AME_Lun_Alive_Check((pAME_Data)pPath_MPIODATA->pAMEData,Raid_ID,Lun_ID);

}


AME_U32 MPIO_NT_Raid_Alive_Check (pMPIO_DATA pHost_MPIODATA,AME_U32 Raid_ID)
{
	pMPIO_DATA	pPath_MPIODATA;

	if (FALSE == MPIO_Support_Check(pHost_MPIODATA)) {
		return AME_NT_Raid_Alive_Check((pAME_Data)pHost_MPIODATA->pAMEData,Raid_ID);
    }

	if (pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].PATHCount == 0) {
		return FALSE;
    }
	
	pPath_MPIODATA = pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].Path_RAIDData[0].pMPIOData;
	return AME_NT_Raid_Alive_Check((pAME_Data)pPath_MPIODATA->pAMEData,Raid_ID);

}

AME_U32 MPIO_Check_Multiple_Path(pMPIO_DATA pMPIODATA,pMPIO_Get_PAGE_0_Data pMPIO_Get_PAGE0_Data ,AME_U32 Raid_ID)
{

	// 1. check RAID path can been added into MPIO path, 
	// 2. else, create the RAID ID MPIO path

	AME_U32		i,j;
	pMPIO_DATA 	pMPIODATA_temp;

	// 1. check RAID path can been added into MPIO path,
	for (i=0;i<MPIO_MAX_support_HBA;i++) {
		
		if (MPIOGlobal.pMPIOData[i] == NULL) {
			continue;
		}
		
		pMPIODATA_temp = (pMPIO_DATA)MPIOGlobal.pMPIOData[i];

		for (j=0;j<MAX_RAID_System_ID;j++) {

			// Serial_Number check
			// Model_Name check
			// Production_ID check
			if ((TRUE == pMPIODATA_temp->MPIO_RAIDDATA[j].GetPAGE0_Data.Ready) &&
			#if !defined (Enable_AME_RAID_R0)  //Das software Raid, don't compare Raid Serial_Number
			    (TRUE == AME_Memory_Compare(pMPIODATA_temp->MPIO_RAIDDATA[j].GetPAGE0_Data.Serial_Number,pMPIO_Get_PAGE0_Data->Serial_Number,0x10)) && 
			    (TRUE == AME_Memory_Compare(pMPIODATA_temp->MPIO_RAIDDATA[j].GetPAGE0_Data.Model_Name,pMPIO_Get_PAGE0_Data->Model_Name,0x10)) &&
			#endif
			    (TRUE == AME_Memory_Compare(pMPIODATA_temp->MPIO_RAIDDATA[j].GetPAGE0_Data.Production_ID,pMPIO_Get_PAGE0_Data->Production_ID,0x10))) {
                
                MPIO_Path_Add(pMPIODATA_temp,pMPIODATA,j,Raid_ID);
                return TRUE;
			}
			
		}
		
	}

	
	// 2. else, create the RAID ID MPIO path
	MPIO_GetPAGE0_Data_Add(pMPIODATA,pMPIO_Get_PAGE0_Data,Raid_ID);
	MPIO_Path_Add(pMPIODATA,pMPIODATA,Raid_ID,Raid_ID);

	return TRUE;
	
}


// After Get RAID ready, I just can confirm the path is exist.
AME_U32 MPIO_AME_RAID_Ready(pAME_Data pAMEData,AME_U32 Raid_ID, AME_U64 Get_Page0_data)
{
	pMPIO_DATA pMPIODATA = (pMPIO_DATA)pAMEData->pMPIOData;

	if (FALSE == MPIO_Support_Check(pMPIODATA)) {
		return TRUE;
    }
    
    MPIO_Msg("Host :%d, Raid:%d Ready\n",pMPIODATA->MPIO_Serial_Number,Raid_ID);
	AME_Memory_copy((PVOID)(pMPIODATA->RAIDDATA[Raid_ID].GetPAGE0_Data.Production_ID), (PVOID)(Get_Page0_data+0x00), 0x10);
	AME_Memory_copy((PVOID)(pMPIODATA->RAIDDATA[Raid_ID].GetPAGE0_Data.Model_Name), (PVOID)(Get_Page0_data+0x10), 0x10);
	AME_Memory_copy((PVOID)(pMPIODATA->RAIDDATA[Raid_ID].GetPAGE0_Data.Serial_Number), (PVOID)(Get_Page0_data+0x20), 0x10);
	
	MPIO_Check_Multiple_Path(pMPIODATA,&(pMPIODATA->RAIDDATA[Raid_ID].GetPAGE0_Data),Raid_ID);
	
	return TRUE;
}

AME_U32 MPIO_AME_RAID_Out(pAME_Data pAMEData,AME_U32 Raid_ID)
{
	pAME_Data pAMEData_Host;
	pMPIO_DATA 	pMPIODATA_path = (pMPIO_DATA)pAMEData->pMPIOData;
	pMPIO_DATA 	pHost_MPIODATA = pMPIODATA_path->RAIDDATA[Raid_ID].pMPIOData_host;
	
	if (FALSE == MPIO_Support_Check(pMPIODATA_path)) {
		return TRUE;
    }

	MPIO_Msg("Host :%d, Raid:%d Out\n",pMPIODATA_path->MPIO_Serial_Number,Raid_ID);
	
	if (pHost_MPIODATA == NULL) {
	    MPIO_Msg_Err("(%d)MPIO_AME_RAID_Out Error:  pHost_MPIODATA = NULL\n",pMPIODATA_path->MPIO_Serial_Number);
	    return FALSE;
	}
	
	pAMEData_Host = (pAME_Data)pHost_MPIODATA->pAMEData;

	if (pMPIODATA_path->RAIDDATA[Raid_ID].Path_State == MPIO_Path_Built) {
		MPIO_Path_Remove(pMPIODATA_path->RAIDDATA[Raid_ID].pMPIOData_host,pMPIODATA_path,pMPIODATA_path->RAIDDATA[Raid_ID].host_RaidID ,Raid_ID,FALSE);
	} else {	// ??? check before driver loaded and do the raid plug out, may get an raid out but not built MPIO.
		MPIO_Msg("Host :%d, Raid:%d ,Not in MPIO path\n",pMPIODATA_path->MPIO_Serial_Number,Raid_ID);
	}
	
    MPIO_Msg("(%d)AME_NT_Notify_Master_Raid_path_remove!\n",pMPIODATA_path->MPIO_Serial_Number);
    AME_NT_Notify_Master_Raid_path_remove((pAME_Data)pHost_MPIODATA->pAMEData);

	return TRUE;
}


AME_U32 MPIO_Build_InBand_Cmd(pMPIO_DATA pMPIODATA, pAME_Module_InBand_Command pAME_ModuleInBand_Command)
{
	return (AME_Build_InBand_Cmd((pAME_Data)pMPIODATA->pAMEData,pAME_ModuleInBand_Command) ); 
}


AME_U32 
pin_pon_queue_algorithm(pMPIO_DATA pMPIODATA, AME_U32 Raid_ID, pPath_RAID_DATA output_pPath_RAIDData)
{
	AME_U32		i;
	pMPIO_DATA 	pPath_MPIODATA;
		
	// pin-pon queue
	i= pMPIODATA->MPIO_RAIDDATA[Raid_ID].TotalIssue_CMDCount % pMPIODATA->MPIO_RAIDDATA[Raid_ID].PATHCount;
	
	pPath_MPIODATA = pMPIODATA->MPIO_RAIDDATA[Raid_ID].Path_RAIDData[i].pMPIOData;

	AME_Memory_copy((PVOID)output_pPath_RAIDData, (PVOID)(&pMPIODATA->MPIO_RAIDDATA[Raid_ID].Path_RAIDData[i]), sizeof(Path_RAID_DATA));

	return TRUE;
}


AME_U32 
issue_command_8608_8717_NT(pMPIO_DATA pMPIODATA, AME_U32 Raid_ID, pPath_RAID_DATA output_pPath_RAIDData)
{
	return pin_pon_queue_algorithm(pMPIODATA, Raid_ID, output_pPath_RAIDData);
}


AME_U32 
issue_command_Das_Soft_Raid(pMPIO_DATA pMPIODATA,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command, AME_U32 Raid_ID, pPath_RAID_DATA	output_pPath_RAIDData)
{
	AME_U32		i;
	i = pAME_ModuleSCSI_Command->MPIO_Which;
	AME_Memory_copy((PVOID)output_pPath_RAIDData, (PVOID)(&pMPIODATA->MPIO_RAIDDATA[Raid_ID].Path_RAIDData[i]), sizeof(Path_RAID_DATA));

	return TRUE;
}


AME_U32 
#if defined (AME_MAC)
	DRV_MAIN_CLASS_NAME::
#endif 
MPIO_Build_SCSI_Cmd(pMPIO_DATA pMPIODATA, pAME_Module_SCSI_Command pAME_ModuleSCSI_Command)
{
	AME_U32			Raid_ID = pAME_ModuleSCSI_Command->Raid_ID;
	pMPIO_DATA		pPath_MPIODATA = pMPIODATA;
	Path_RAID_DATA	Path_RAIDData;
	pMPIO_Request_QUEUE_Buffer  pMPIO_RequestQUEUE_Buffer;
	
	if (FALSE == MPIO_Support_Check(pMPIODATA)) {
		if (FALSE == AME_Build_SCSI_Cmd((pAME_Data)pMPIODATA->pAMEData,(pAME_Data)pMPIODATA->pAMEData,pAME_ModuleSCSI_Command)) {
			MPIO_Msg_Err("(%d)MPIO_Build_SCSI_Cmd Fail\n",pMPIODATA->MPIO_Serial_Number);
			return FALSE;
		}
		return TRUE;
	}

	if (pMPIODATA->MPIO_RAIDDATA[Raid_ID].PATHCount == 0) {
		MPIO_Msg_Err("(%d)MPIO_Build_SCSI_Cmd : PATHCount = 0, ???\n",pMPIODATA->MPIO_Serial_Number);
		return FALSE;
	}

	if (FALSE == (pMPIO_RequestQUEUE_Buffer = MPIO_Request_Buffer_Queue_Allocate(pMPIODATA))) {
		MPIO_Msg_Err("(%d)pMPIO_RequestQUEUE_Buffer : Fail\n",pMPIODATA->MPIO_Serial_Number);
		return FALSE;
	}
		
	pPath_MPIODATA = pMPIODATA->MPIO_RAIDDATA[Raid_ID].Path_RAIDData[0].pMPIOData;
	
	// 8608NT/8717NT or Das soft Raid R0/R1
	#if defined (Enable_AME_RAID_R0)
        issue_command_Das_Soft_Raid(pMPIODATA,pAME_ModuleSCSI_Command,Raid_ID,&Path_RAIDData);
	#else
	    issue_command_8608_8717_NT(pMPIODATA,Raid_ID,&Path_RAIDData);
    #endif

	pPath_MPIODATA = Path_RAIDData.pMPIOData;

	pMPIO_RequestQUEUE_Buffer->MPIO_QUEUEData.pHostMPIOData = pMPIODATA;
	pMPIO_RequestQUEUE_Buffer->MPIO_QUEUEData.AME_RAID_ID = Path_RAIDData.AME_RAID_ID;
	pMPIO_RequestQUEUE_Buffer->MPIO_QUEUEData.Host_RAID_ID = Raid_ID;
	pMPIO_RequestQUEUE_Buffer->MPIO_QUEUEData.Path_ID_of_Host_RAID = Path_RAIDData.Path_ID_of_Host_RAID;
	
	pAME_ModuleSCSI_Command->Slave_Raid_ID = Path_RAIDData.AME_RAID_ID;
	pAME_ModuleSCSI_Command->pMPIORequestExtension = (PVOID)pMPIO_RequestQUEUE_Buffer;

	if (FALSE == AME_Build_SCSI_Cmd((pAME_Data)pMPIODATA->pAMEData,(pAME_Data)pPath_MPIODATA->pAMEData,pAME_ModuleSCSI_Command)) {
		MPIO_Msg_Err("(%d)MPIO_Build_SCSI_Cmd Fail\n",pMPIODATA->MPIO_Serial_Number);
		MPIO_Msg_Err("error count (%d)\n",pMPIODATA->MPIO_RAIDDATA[Raid_ID].TotalIssue_CMDCount);
		return FALSE;
	}
	
	MPIO_spinlock(pMPIODATA);
	pMPIODATA->MPIO_RAIDDATA[Raid_ID].PathCMDCount[Path_RAIDData.Path_ID_of_Host_RAID]++;
	pMPIODATA->MPIO_RAIDDATA[Raid_ID].ConcurrentCMDCount++;
	pMPIODATA->MPIO_RAIDDATA[Raid_ID].TotalIssue_CMDCount++;
	MPIO_spinunlock(pMPIODATA);
	
	return TRUE;
}


AME_U32 MPIO_Cmd_Timeout(pMPIO_DATA pMPIODATA,PVOID pRequestExtension)
{
	pAME_REQUEST_CallBack 	pAME_REQUESTCallBack;

	MPIO_Msg_Err("(%d)MPIO_Cmd_Timeout : request:%x\n",pMPIODATA->MPIO_Serial_Number,pRequestExtension);
	
	if (FALSE == MPIO_Support_Check(pMPIODATA)) {
		return (AME_Cmd_Timeout((pAME_Data)pMPIODATA->pAMEData,pRequestExtension));
    }
    
    //
    // need a new function to call master or slave do timeout cmd.
    //

	return (AME_Cmd_Timeout((pAME_Data)pMPIODATA->pAMEData,pRequestExtension));
}

AME_U32 MPIO_Cmd_All_Timeout(pMPIO_DATA pMPIODATA)
{


	MPIO_Msg_Err("(%d)MPIO_Cmd_All_Timeout\n",pMPIODATA->MPIO_Serial_Number);

	if (FALSE == MPIO_Support_Check(pMPIODATA)) {
		return (AME_Cmd_All_Timeout((pAME_Data)pMPIODATA->pAMEData));
    }
    
    //
    // need a new function to call master or slave do timeout cmd.
    //

	return (AME_Cmd_All_Timeout((pAME_Data)pMPIODATA->pAMEData));
}


AME_U32 MPIO_Cmd_Mark_Path_remove(pMPIO_DATA pHost_MPIODATA)
{
    AME_U32 i,j;
    pMPIO_DATA 	pPath_MPIODATA;
    
    for (i=0;i<MPIO_MAX_support_HBA;i++) {
		
		if (MPIOGlobal.pMPIOData[i] == NULL) {
			continue;
		}
		
		pPath_MPIODATA = (pMPIO_DATA)MPIOGlobal.pMPIOData[i];
		
		for (j=0;j<MAX_RAID_System_ID;j++) {
		    if (pPath_MPIODATA->RAIDDATA[j].Path_State & MPIO_Path_Disabled) {
		        AME_Cmd_Mark_Raid_Path_Remove((pAME_Data)pHost_MPIODATA->pAMEData,(pAME_Data)pPath_MPIODATA->pAMEData,j);
		    }
	    }
		
	}

    return TRUE;
}


AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
MPIO_Host_Fast_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAME_REQUESTCallBack)
{
	pAME_Data 						pAMEData = pAME_REQUESTCallBack->pAMEData;
	pMPIO_DATA 						pHost_MPIODATA = (pMPIO_DATA)pAMEData->pMPIOData;
	pMPIO_Request_QUEUE_Buffer		pMPIO_RequestQUEUE_Buffer;
	AME_U32         				Raid_ID = pAME_REQUESTCallBack->Raid_ID;

	if (FALSE == MPIO_Support_Check(pHost_MPIODATA)) {
		#if defined (Enable_AME_RAID_R0)
		    return AME_Raid_Host_Fast_handler_SCSI_REQUEST(pAME_REQUESTCallBack);
		#else
		    return AME_Host_Fast_handler_SCSI_REQUEST(pAME_REQUESTCallBack);
		#endif
    }

	pMPIO_RequestQUEUE_Buffer = (pMPIO_Request_QUEUE_Buffer)pAME_REQUESTCallBack->pMPIORequestExtension;
	pAME_REQUESTCallBack->Raid_ID = pMPIO_RequestQUEUE_Buffer->MPIO_QUEUEData.Host_RAID_ID;

	MPIO_spinlock(pHost_MPIODATA);

	pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].PathCMDCount[pMPIO_RequestQUEUE_Buffer->MPIO_QUEUEData.Path_ID_of_Host_RAID]--;
	pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].ConcurrentCMDCount--;

	MPIO_spinunlock(pHost_MPIODATA);

	MPIO_Request_Buffer_Queue_Release(pHost_MPIODATA,pMPIO_RequestQUEUE_Buffer);
	#if defined (Enable_AME_RAID_R0)
	    return AME_Raid_Host_Fast_handler_SCSI_REQUEST(pAME_REQUESTCallBack);
	#else
	    return AME_Host_Fast_handler_SCSI_REQUEST(pAME_REQUESTCallBack);
	#endif
}


AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
MPIO_Host_Normal_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAME_REQUESTCallBack)
{

	pAME_Data 						pAMEData = pAME_REQUESTCallBack->pAMEData;
	pMPIO_DATA 						pHost_MPIODATA = (pMPIO_DATA)pAMEData->pMPIOData;
	pMPIO_Request_QUEUE_Buffer		pMPIO_RequestQUEUE_Buffer;
	AME_U32         				Raid_ID = pAME_REQUESTCallBack->Raid_ID;

	if (FALSE == MPIO_Support_Check(pHost_MPIODATA)) {
		#if defined (Enable_AME_RAID_R0)
		    return AME_Raid_Host_Normal_handler_SCSI_REQUEST(pAME_REQUESTCallBack);
		#else
		    return AME_Host_Normal_handler_SCSI_REQUEST(pAME_REQUESTCallBack);
		#endif
    }


	pMPIO_RequestQUEUE_Buffer = (pMPIO_Request_QUEUE_Buffer)pAME_REQUESTCallBack->pMPIORequestExtension;
	pAME_REQUESTCallBack->Raid_ID = pMPIO_RequestQUEUE_Buffer->MPIO_QUEUEData.Host_RAID_ID;

	pAME_REQUESTCallBack->Raid_ID = pMPIO_RequestQUEUE_Buffer->MPIO_QUEUEData.Host_RAID_ID;

	MPIO_spinlock(pHost_MPIODATA);

	pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].PathCMDCount[pMPIO_RequestQUEUE_Buffer->MPIO_QUEUEData.Path_ID_of_Host_RAID]--;
	pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].ConcurrentCMDCount--;

	MPIO_spinunlock(pHost_MPIODATA);

	MPIO_Request_Buffer_Queue_Release(pHost_MPIODATA,pMPIO_RequestQUEUE_Buffer);
	
	#if defined (Enable_AME_RAID_R0)
	    return AME_Raid_Host_Normal_handler_SCSI_REQUEST(pAME_REQUESTCallBack);
	#else
	    return AME_Host_Normal_handler_SCSI_REQUEST(pAME_REQUESTCallBack);
	#endif

}


AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
MPIO_Host_Error_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAME_REQUESTCallBack)
{
	AME_U32         				Raid_ID = pAME_REQUESTCallBack->Raid_ID;
	pAME_Data 						pAMEData = pAME_REQUESTCallBack->pAMEData;
	pMPIO_DATA 						pHost_MPIODATA = (pMPIO_DATA)pAMEData->pMPIOData;
	pMPIO_Request_QUEUE_Buffer		pMPIO_RequestQUEUE_Buffer = (pMPIO_Request_QUEUE_Buffer)pAME_REQUESTCallBack->pMPIORequestExtension;
	
	if (FALSE == MPIO_Support_Check(pHost_MPIODATA)) {
		return AME_Host_Error_handler_SCSI_REQUEST(pAME_REQUESTCallBack);
    }

    if ((pAME_REQUESTCallBack->Retry_Cmd == TRUE) &&
        (pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].PATHCount > 0)) {
       
        AME_Module_SCSI_Command AME_ModuleSCSI_Command;
	   
	    MPIO_spinlock(pHost_MPIODATA);
	    pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].PathCMDCount[pMPIO_RequestQUEUE_Buffer->MPIO_QUEUEData.Path_ID_of_Host_RAID]--;
	    pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].ConcurrentCMDCount--;
	    MPIO_spinunlock(pHost_MPIODATA);
	    
	    pAME_REQUESTCallBack->pAMEData = (pAME_Data)pHost_MPIODATA->pAMEData;
	    MPIO_Host_Rebuild_SCSI_REQUEST(pAME_REQUESTCallBack,&AME_ModuleSCSI_Command);
	    
	    if (TRUE == MPIO_Build_SCSI_Cmd(pHost_MPIODATA,&AME_ModuleSCSI_Command)) {
			MPIO_Msg("Retry command to another path...\n");
			return TRUE;
		} else {
		    MPIO_Msg("Return error to host driver.\n");
		}
	}
	
	MPIO_spinlock(pHost_MPIODATA);
	pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].PathCMDCount[pMPIO_RequestQUEUE_Buffer->MPIO_QUEUEData.Path_ID_of_Host_RAID]--;
	pHost_MPIODATA->MPIO_RAIDDATA[Raid_ID].ConcurrentCMDCount--;
	MPIO_spinunlock(pHost_MPIODATA);

	MPIO_Request_Buffer_Queue_Release(pHost_MPIODATA,pMPIO_RequestQUEUE_Buffer);
	AME_Host_Error_handler_SCSI_REQUEST(pAME_REQUESTCallBack);

	return TRUE;		
	
}


AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
MPIO_Host_Scan_All_Lun (pAME_Data pAMEData,AME_U32 RAID_ID)
{
	pMPIO_DATA 	pPath_MPIODATA = (pMPIO_DATA)pAMEData->pMPIOData;
	pMPIO_DATA	pMPIODATA_temp = NULL;
	AME_U32		i,j,k;
	AME_U32		PATHCount;
	
	if (FALSE == MPIO_Support_Check(pPath_MPIODATA)) {
		return (AME_Host_Scan_All_Lun(pAMEData,RAID_ID));
    }
	
	for (i=0; i<MPIO_MAX_support_HBA; i++)
	{
		if (MPIOGlobal.pMPIOData[i] == NULL) {
			continue;
        }
		
		pMPIODATA_temp = (pMPIO_DATA)MPIOGlobal.pMPIOData[i];

		for (j=0; j<MAX_RAID_System_ID; j++)
        {
			PATHCount = pMPIODATA_temp->MPIO_RAIDDATA[j].PATHCount;
			
			for (k=0; k<PATHCount; k++)
            {
				if ((pMPIODATA_temp->MPIO_RAIDDATA[j].Path_RAIDData[k].pMPIOData == pPath_MPIODATA) &&
					(pMPIODATA_temp->MPIO_RAIDDATA[j].Path_RAIDData[k].AME_RAID_ID == RAID_ID)) {
					    
                    // When single NT cable plug out case, pHostDATA->pPathData[0] remove already.
                    // Driver must be return FALSE,and don't call AME_Host_Lun_Change Function.
                    // In this case, PATHCount = 0, so we add a new condition to avoid error.
                    //if ((pPath_MPIODATA != pHostDATA->pPathData[0]) &&
                        //(pHostDATA->PATHCount != 0)) {
                    	//return FALSE;
                    //}
                    if (k!=0)	// not first path
                    	return FALSE;
                    else {
                    	RAID_ID = j;
                    	MPIO_Msg("MPIO_Host_Scan_All_Lun,Host:%d, Raid:%d \n",pMPIODATA_temp->MPIO_Serial_Number,RAID_ID);
                    	return (AME_Host_Scan_All_Lun((pAME_Data)pMPIODATA_temp->pAMEData,RAID_ID));
                    }
					
				}
			}
		}
	}

	// need to check single NT cable plug out case, ???
	if (pMPIODATA_temp != NULL) {
	    MPIO_Msg("MPIO_Host_Scan_All_Lun,Host:%d, Raid:%d, pathcount=0 \n",pMPIODATA_temp->MPIO_Serial_Number,RAID_ID);
	}
	return (AME_Host_Scan_All_Lun(pAMEData,RAID_ID));

}


AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
MPIO_Host_Lun_Change (pAME_Data pAMEData,AME_U32 RAID_ID,AME_U32 lun_ID,AME_U32 State)
{
	pMPIO_DATA 	pPath_MPIODATA = (pMPIO_DATA)pAMEData->pMPIOData;
	pMPIO_DATA	pMPIODATA_temp;
	AME_U32		i,j,k;
	AME_U32		PATHCount;

	if (FALSE == MPIO_Support_Check(pPath_MPIODATA)) {
		return (AME_Host_Lun_Change(pAMEData,RAID_ID,lun_ID,State));
    }

	// 1. check RAID path can been added into MPIO path, 
	for (i=0;i<MPIO_MAX_support_HBA;i++) {
		
		if (MPIOGlobal.pMPIOData[i] == NULL) {
			continue;
	    }
		
		pMPIODATA_temp = (pMPIO_DATA)MPIOGlobal.pMPIOData[i];

		for (j=0;j<MAX_RAID_System_ID;j++) {
			
			PATHCount = pMPIODATA_temp->MPIO_RAIDDATA[j].PATHCount;
			
			for (k=0;k<PATHCount;k++) {
			    if ((pMPIODATA_temp->MPIO_RAIDDATA[j].Path_RAIDData[k].pMPIOData == pPath_MPIODATA) &&
			        (pMPIODATA_temp->MPIO_RAIDDATA[j].Path_RAIDData[k].AME_RAID_ID == RAID_ID)) {
		            
		            if (PATHCount == 1) {
		                MPIO_Msg("AME_Host_Lun_Change,PATHCount:%d, Host:%d, Raid:%d, Lun:%d, state:%d \n",PATHCount,pMPIODATA_temp->MPIO_Serial_Number,RAID_ID,lun_ID,State);
			            return (AME_Host_Lun_Change((pAME_Data)pMPIODATA_temp->pAMEData,RAID_ID,lun_ID,State));
		            }
		            
		            if ((PATHCount > 1) &&
		                (k == 0)) {
                        MPIO_Msg("AME_Host_Lun_Change,PATHCount:%d, Host:%d, Raid:%d, Lun:%d, state:%d \n",PATHCount,pMPIODATA_temp->MPIO_Serial_Number,RAID_ID,lun_ID,State);
			            return (AME_Host_Lun_Change((pAME_Data)pMPIODATA_temp->pAMEData,RAID_ID,lun_ID,State));
		            }
		            
		            //MPIO_Msg("AME_Host_Lun_Change Stop, because have other path,PATHCount:%d, Host:%d, Raid:%d, Lun:%d, state:%d \n",PATHCount,pMPIODATA_temp->MPIO_Serial_Number,RAID_ID,lun_ID,State);
		            return FALSE;
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
MPIO_Host_Normal_handler_IN_Band (pAME_REQUEST_CallBack pAMEREQUESTCallBack)
{
	return (AME_Host_Normal_handler_IN_Band(pAMEREQUESTCallBack));
}



AME_U32 
#if defined (AME_MAC)
		DRV_MAIN_CLASS_NAME::
#endif
MPIO_Timer (pMPIO_DATA pMPIODATA)
{
	return TRUE;
}


pAME_Data MPIO_Get_Master_AME_Data (pMPIO_DATA pPath_MPIODATA,AME_U32 RAID_ID)
{
	pMPIO_DATA 	pHost_MPIODATA;
	pAME_Data 	pAMEData_Host = NULL;

	if (FALSE == MPIO_Support_Check(pPath_MPIODATA)) {
		return FALSE;
    }

	pHost_MPIODATA = pPath_MPIODATA->RAIDDATA[RAID_ID].pMPIOData_host;
	
	if (pHost_MPIODATA != NULL) {
	    pAMEData_Host = (pAME_Data)pHost_MPIODATA->pAMEData;
	}
	
    return pAMEData_Host;
}


AME_U32 MPIO_Slave_Notify_Master_toget_Reply(pMPIO_DATA pPath_MPIODATA,AME_U32 RAID_ID)
{
	pAME_Data 	pAMEData;
	pMPIO_DATA 	pHost_MPIODATA;

	if (FALSE == MPIO_Support_Check(pPath_MPIODATA)) {
		return FALSE;
    }

	pHost_MPIODATA = pPath_MPIODATA->RAIDDATA[RAID_ID].pMPIOData_host;
	
	if (pHost_MPIODATA == NULL) {
	    MPIO_Msg_Err("MPIO_Slave_Notify_Master_toget_Reply Error:  pHost_MPIODATA = NULL\n");
	    return FALSE;
	}
	
	if (pHost_MPIODATA->pAMEData == NULL) {
	    MPIO_Msg_Err("MPIO_Slave_Notify_Master_toget_Reply Error:  pHost_MPIODATA->pAMEData = NULL\n");
	    return FALSE;
	}
	
	if (pHost_MPIODATA == pPath_MPIODATA) {
	    MPIO_Msg("MPIO_Slave_Notify_Master_toget_Reply failed, it's Master path\n");
	    return FALSE;
	}

	AME_NT_Notify_Master_toget_Reply_Fifo((pAME_Data)pHost_MPIODATA->pAMEData);
	
	return TRUE;
}

