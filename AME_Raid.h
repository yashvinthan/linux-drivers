/*
 *  AME_Raid.h
 *  Acxxx
 *
 *  Copyright (c) 2004-2014 Accusys, Inc. All rights reserved.
 *
 */


#include "OS_Define.h"

#ifndef AME_Raid_H
#define AME_Raid_H

#define Block_Size              512
#define AME_Raid_SG_Size	    2080    // 16*130 Byte

#define Maximun_Raid  4   // Maximun support 4 Raid

//#define AME_Raid_Stripe_Size	0x10000 // 64 KB , LBA = 0x80
//#define AME_Raid_LBA_Stripe_Size	0x80

//#define AME_Raid_Stripe_Size	0x20000 // 128 KB, LBA = 0x100 
//#define AME_Raid_LBA_Stripe_Size	0x100

#define AME_Raid_Stripe_Size	    0x40000 // 256 KB, LBA = 0x200
#define AME_Raid_LBA_Stripe_Size	0x200

//#define AME_Raid_Stripe_Size	0x80000 // 512 KB, LBA = 0x400
//#define AME_Raid_LBA_Stripe_Size	0x400

#define Raid_Master 0x00
#define Raid_Slave  0x01


typedef struct _AME_Raid_Queue_List
{
    PVOID                   pPrev;
    PVOID                   pNext;
	
} PACKED AME_Raid_Queue_List, *pAME_Raid_Queue_List;

typedef struct _AME_Raid_Host_SG
{
    AME_U32                 SGCount;
    AME_U32                 Data_Size;
    AME_U64                 LBA_Address;
    AME_Host_SG             AME_Host_Sg[130];
	
} PACKED AME_Raid_Host_SG, *pAME_Raid_Host_SG;

typedef struct _AME_Raid_Queue_Buffer
{
    AME_Raid_Queue_List		AMEQueue_list_Buffer;
    AME_U8                  ReqInUse;
    AME_U32                 Target_Index;
    AME_U32                 Complete_Index;
    AME_Raid_Host_SG        AME_Raid_Host_Sg[Maximun_Raid];
    PVOID                   pAMERaid_QueueDATA;
   
} PACKED AME_Raid_Queue_Buffer, *pAME_Raid_Queue_Buffer;

typedef struct _AME_Raid_Queue_DATA
{
	AME_U32                 Raid_Path_Count;
	PVOID					pDeviceExtension;
	AME_Raid_Queue_List		AME_Raid_Queue_List_Free;
	AME_Raid_Queue_Buffer	AME_Raid_QueueBuffer[MAX_Command_AME];

} PACKED AME_Raid_Queue_DATA, *pAME_Raid_Queue_DATA;


extern  AME_U32 AME_Raid_Data_Init(PVOID DeviceExtension,pAME_Raid_Queue_DATA pAMERaid_QueueDATA);
extern  AME_U32 AME_Raid_Build_SCSI_Cmd(pAME_Raid_Queue_DATA pAMERaid_QueueDATA,pMPIO_DATA pHost_MPIODATA,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command);
extern  AME_U32 AME_Raid_Host_Normal_handler_SCSI_REQUEST(pAME_REQUEST_CallBack pAME_REQUESTCallBack);
extern  AME_U32 AME_Raid_Host_Fast_handler_SCSI_REQUEST(pAME_REQUEST_CallBack pAME_REQUESTCallBack);


extern 	void AME_Raid_spinlock(pAME_Raid_Queue_DATA pAMERaid_QueueDATA);
extern 	void AME_Raid_spinunlock(pAME_Raid_Queue_DATA pAMERaid_QueueDATA);

#endif //AME_Raid_H
