/*
 *  AME_import.h
 *  Acxxx
 *
 *  Copyright (c) 2004-2014 Accusys, Inc. All rights reserved.
 *
 */

#include "OS_Define.h"


#if defined (AME_MAC)
	#define EXTERNC "C"
	#define AME_MAC_DAS
	//#define AME_MAC_NT
#else  
	#define EXTERNC
#endif


extern 	EXTERNC void AME_Address_Write_32(pAME_Data pAMEData,PVOID Base_Address, AME_U32 offset, AME_U32 value);
extern	EXTERNC	AME_U32 AME_Address_read_32(pAME_Data pAMEData,PVOID Base_Address, AME_U32 offset);
extern	EXTERNC	AME_U16 AME_Address_read_16(pAME_Data pAMEData,PVOID Base_Address, AME_U32 offset);
extern 	EXTERNC	void AME_spinlock(pAME_Data pAMEData);
extern 	EXTERNC	void AME_spinunlock(pAME_Data pAMEData);
extern 	EXTERNC	void AME_IO_milliseconds_Delay(AME_U32 Delay);
extern 	EXTERNC	void AME_IO_microseconds_Delay(AME_U32 Delay);

extern  EXTERNC AME_U32 AME_Host_Fast_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAME_REQUESTCallBack);
extern  EXTERNC AME_U32 AME_Host_Error_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAME_REQUESTCallBack);
extern  EXTERNC AME_U32 AME_Host_Normal_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAME_REQUESTCallBack);
extern	EXTERNC AME_U32 AME_Host_Scan_All_Lun (pAME_Data pAMEData,AME_U32 RAID_ID);
extern	EXTERNC AME_U32 AME_Host_Lun_Change (pAME_Data pAMEData,AME_U32 RAID_ID,AME_U32 lun_ID,AME_U32 State); 	// State: TRUE, means lun Add, FALSE, means lun remove
extern  EXTERNC AME_U32 AME_Host_Normal_handler_IN_Band (pAME_REQUEST_CallBack pAMEREQUESTCallBack);
extern  EXTERNC void AME_Memory_copy(PVOID Dest, PVOID Src, AME_U32 size);
extern  EXTERNC AME_U32 AME_Memory_Compare(PVOID Source1,PVOID Source2, AME_U32 Length);
extern  EXTERNC void AME_Memory_zero(PVOID Src, AME_U32 size);
extern  EXTERNC void AME_CleanupAllOutstandingRequest(pAME_Data pAMEData);
extern 	EXTERNC	AME_U32 AME_Raid_PCI_Config_Read(pAME_Data pAMEData,AME_U32 offset);
extern 	EXTERNC	void AME_Raid_PCI_Config_Write(pAME_Data pAMEData,AME_U32 offset,AME_U32 value);
extern 	EXTERNC AME_U32 AME_PCI_Config_Read_32(pAME_Data pAMEData,AME_U32 bus,AME_U32 slot,AME_U32 func,AME_U32 offset);
extern 	EXTERNC AME_U32 AME_PCI_Config_Write_32(pAME_Data pAMEData,AME_U32 bus,AME_U32 slot,AME_U32 func,AME_U32 offset,AME_U32 value);
extern 	EXTERNC PVOID AME_IOMapping_Bridge(pAME_Data pAMEData,AME_U32 bus,AME_U32 slot,AME_U32 func);
extern 	EXTERNC void AME_Host_RAID_Ready(pAME_Data pAMEData);

