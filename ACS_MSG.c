/*
 *  message.c
 *  Acxxx
 *
 *  Copyright (c) 2004-2014 Accusys, Inc. All rights reserved.
 *
 */


#include "AME_module.h"
#include "AME_import.h"
#include "MPIO.h"
#include "ACS_MSG.h"


#if defined (AME_MAC_DAS)
#include "Acxxx.h"
#endif

// category : Driver_init		0
char Driver_init_msg[][Msg_size]={
	"Driver Initail Start...",		// Driver_Init_start				0
	"Driver Initail Success",		// Driver_Init_Done_Success		1
	""
};


// category : HBA_init		1
char Driver_HBA_init_msg[][Msg_size]={
	"HBA Initail Start...",					// HBA_Init_start					0
	"HBA Initail Success",					// HBA_Init_Done_Success			1
	"HBA Restart Start...",					// HBA_Restart_start					2
	"HBA Restart Success",					// HBA_Restart_Done_Success			3	
	"HBA Allocate Memory Success",			//HBA_Init_Allocate_Memory_Success	4
	"HBA VendorID/DeviceID",				//HBA_Init_VendorID_DeviceID			5
	"HBA ClassCode",						// HBA_Init_Classcode				6
	""
};


// category : HBA_Stop			2
	char HBA_Stop_msg[][Msg_size]={

	"HBA Stop Start...",		// HBA_Stop_start				0
	"HBA Stop Success",		// HBA_Stop_Done_Success		1
	"HBA Pause Start...",	// HBA_Pause_start				2
	"HBA Pause Success",		// HBA_Pause_Done_Success		3
	""
};






// category : MPIO_init		10
char MPIO_init_msg[][Msg_size]={
	"MPIO Module Initail Start...",		// MPIO_Init_start				0
	"MPIO Module Initail Success",		// MPIO_Init_Done_Success		1
	""
};

// category : AME_init		20
char AME_init_msg[][Msg_size]={
	"AME Module Initail Start...",		// AME_Init_start				0
	"AME Module Initail Success",		// AME_Init_Done_Success		1
	""
};



// category : Driver_Windows_init		30
char Driver_Windows_init_msg[][Msg_size]={
	"HBA Initail Start...(Hibernation_mode)",				// Init_start_Hibernation_mode			0
	"HBA Initail Success (Hibernation_mode)",				// Init_Done_Success_Hibernation_mode	1
	""
};


#define Message_log_start			0
#define Message_log_stop			1
#define Message_log_overrun 		2

// category : Message_log_msg		60
char Message_log_msg[][Msg_size]={
	"Message log start",				// Message_log_start			0
	"Message log stop",					// Message_log_stop			1
	"Message log overrun",				// Message_log_overrun	2
	""
};


/* ======================= Code ================================== 
    ============================================================== */

AME_U32 Global_Message_Serial_Number=0;


AME_U32 ACS_LogMemory_SWAP(pMSG_DATA pMSGDATA)
{
	pLog_header private_log_p;
	AME_U32		buffer_offset;

	private_log_p = (pLog_header)pMSGDATA->LogMemory_DATA.Log_virtualAddress;
	private_log_p->shared_pingpong ^= 1;
	buffer_offset = sizeof(Log_header)+ ACS_PRIVATE_LOG_BUF_SIZE*private_log_p->shared_pingpong + private_log_p->count_p[private_log_p->shared_pingpong];
	pMSGDATA->LogMemory_DATA.current_Log_Address =  (PVOID)((char *)pMSGDATA->LogMemory_DATA.Log_virtualAddress+buffer_offset);
	
	return TRUE;
}

AME_U32 ACS_LogMemory_clear_oldEvent(pMSG_DATA pMSGDATA)
{
	pLog_header private_log_p;
	unsigned int 	old_shared_pingpong;
	PVOID			old_Log_Address;


	private_log_p = (pLog_header)pMSGDATA->LogMemory_DATA.Log_virtualAddress;
	old_shared_pingpong = private_log_p->shared_pingpong ^ 1;

	private_log_p->count_p[old_shared_pingpong] = 0;
	old_Log_Address = (PVOID)((char *)pMSGDATA->LogMemory_DATA.Log_virtualAddress+sizeof(Log_header)+ACS_PRIVATE_LOG_BUF_SIZE*old_shared_pingpong);
	AME_Memory_zero (old_Log_Address,ACS_PRIVATE_LOG_BUF_SIZE);

	return TRUE;
}



void ACS_message_LogMemory(pMSG_DATA pMSGDATA,AME_U8 category,AME_U8 code,pMSG_Additional_Data pMSG_AdditionalData)
{

	AME_U32		i;

	unsigned int time_hi=0;
	unsigned int time_lo=0;
	unsigned int recent_log_buf_cnt;
	pPrivate_log header_p;
	pLog_header private_log_p;

	
#if 0	
	PTIME_FIELDS curTime_p=0;
	curTime_p= get_curTime();
 
	KdPrint ((" 	 [private_log] %02d:%02d:%02d.%03d cat=%d, cod=%03d \r\n",curTime_p->Hour, curTime_p->Minute, curTime_p->Second, curTime_p->Milliseconds, category, code));
	if (length > 0)
	{
		for (ui1=0; ui1<length; ui1++)
		{
			if ((ui1 % 4) == 0)
			{
				if (ui1 != 0)
					KdPrint(("\r\n"));
				KdPrint(("		"));
			}
			KdPrint(("	0x%08x", p_l_buf[ui1]));
		}
		KdPrint(("\r\n"));
	}
	goto exit;
#endif

	private_log_p = (pLog_header)pMSGDATA->LogMemory_DATA.Log_virtualAddress;

	// check if buffer overrun...
	if (private_log_p->count_p[private_log_p->shared_pingpong] > (ACS_PRIVATE_LOG_BUF_SIZE - (sizeof(Private_log_header)+sizeof(MSG_Additional_Data)))) {
		//overrun
		Msg("%s",Message_log_msg[Message_log_overrun]);
		return;
	}
	
#ifdef Support_LogMemory
	ACS_convert_current_time(&time_hi, &time_lo);
#endif

	//recent_log_buf_cnt = private_log_p->count_p[private_log_p->shared_pingpong];
	header_p = (pPrivate_log)pMSGDATA->LogMemory_DATA.current_Log_Address;

	header_p->Private_logheader.signature = log_signature;
	header_p->Private_logheader.time_lo = time_lo;
	header_p->Private_logheader.time_hi = time_hi;
	header_p->Private_logheader.category = category;
	header_p->Private_logheader.code = code;
	header_p->Private_logheader.reserved2=0;

	header_p->Private_logheader.count = pMSGDATA->MSG_log_memory_number;
	pMSGDATA->MSG_log_memory_number++;	
	
	if (pMSG_AdditionalData == NULL)
		header_p->Private_logheader.length_Additional = 0;
	else {

		header_p->Private_logheader.length_Additional = sizeof(MSG_Additional_Data);
		for (i=0;i<6;i++)
			pMSG_AdditionalData->reserved[i]=0;		
		
		AME_Memory_copy((PVOID)&(header_p->MSG_AdditionalData),pMSG_AdditionalData,sizeof(MSG_Additional_Data));
		
		#if 0
		switch (pMSG_AdditionalData->parameter_type) {
			case parameter_type_decimal_integer:
			case parameter_type_hexadecimal_integer:
				header_p->Private_logheader.length_Additional = 2 + pMSG_AdditionalData->parameter_length*sizeof(AME_U32);
				break;	
			case parameter_type_Character:
				header_p->Private_logheader.length_Additional = 2 + pMSG_AdditionalData->parameter_length*sizeof(AME_U8);
				break;	
		}
		AME_Memory_copy((PVOID)&(header_p->MSG_AdditionalData.parameter_data),pMSG_AdditionalData,header_p->Private_logheader.length_Additional);
		#endif
		
	}

	recent_log_buf_cnt = sizeof(Private_log_header)+header_p->Private_logheader.length_Additional;
	
	private_log_p->count_p[private_log_p->shared_pingpong] += recent_log_buf_cnt;
	pMSGDATA->LogMemory_DATA.current_Log_Address =  (PVOID)((char *)pMSGDATA->LogMemory_DATA.current_Log_Address+recent_log_buf_cnt);

	// check if buffer overrun...
	if (private_log_p->count_p[private_log_p->shared_pingpong] > (ACS_PRIVATE_LOG_BUF_SIZE - (sizeof(Private_log_header)+sizeof(MSG_Additional_Data)))) {
		//overrun
		Msg("%s",Message_log_msg[Message_log_overrun]);

		header_p = (pPrivate_log)pMSGDATA->LogMemory_DATA.current_Log_Address;

		header_p->Private_logheader.signature = log_signature;
		header_p->Private_logheader.time_lo = time_lo;
		header_p->Private_logheader.time_hi = time_hi;
		header_p->Private_logheader.category = Message_log;
		header_p->Private_logheader.code = Message_log_overrun;
		header_p->Private_logheader.reserved2=0;
		header_p->Private_logheader.count = pMSGDATA->MSG_log_memory_number;
		pMSGDATA->MSG_log_memory_number++;

		
		header_p->Private_logheader.length_Additional = 0;
		recent_log_buf_cnt = sizeof(Private_log_header)+header_p->Private_logheader.length_Additional;
		
		private_log_p->count_p[private_log_p->shared_pingpong] += recent_log_buf_cnt;
		pMSGDATA->LogMemory_DATA.current_Log_Address =	(PVOID)((char *)pMSGDATA->LogMemory_DATA.current_Log_Address+recent_log_buf_cnt);
	
		return;
	}	


	return;


}



/* 
	for pMSGDATA 
		if the MSG_DATA not ready before call ACS_message(), such as in windows DriverEntry(), just set NULL of pMSGDATA. 

	for pMSG_AdditionalData 
		if no pMSG_AdditionalData. just set to NULL 
*/

void ACS_message(pMSG_DATA pMSGDATA,AME_U8 category,AME_U8 code,pMSG_Additional_Data pMSG_AdditionalData)
{
	AME_U32		i;




	if (pMSGDATA == NULL) {
		Msg_Err_title("MSG_Init un-inited");
		return;
	}

#ifdef Support_LogMemory
	ACS_message_LogMemory(pMSGDATA,category,code,pMSG_AdditionalData);
#endif


#ifdef AME_Windows
	{
		PTIME_FIELDS curTime_p=0;
		curTime_p= get_curTime();
	
		Msg ("%04d/%02d/%02d %02d:%02d:%02d.%03d:",curTime_p->Year,curTime_p->Month,curTime_p->Day,curTime_p->Hour, curTime_p->Minute, curTime_p->Second, curTime_p->Milliseconds);
	}
#endif

	Msg_title("%d-%d: ",pMSGDATA->MSG_Serial_Number,pMSGDATA->MSG_number);
	pMSGDATA->MSG_number++;
	
	switch (category) {
		case Driver_init:
			Msg("%s",Driver_init_msg[code]);
			break;
		case HBA_init:
			Msg("%s",Driver_HBA_init_msg[code]);
			break;	
		case HBA_Stop:
			Msg("%s",HBA_Stop_msg[code]);
			break;					
		case MPIO_init:
			Msg("%s",MPIO_init_msg[code]);
			break;	
		case AME_init:
			Msg("%s",AME_init_msg[code]);
			break;	
		case Driver_Windows_init:
			Msg("%s",Driver_Windows_init_msg[code]);
			break;
		case Message_log:
			Msg("%s",Message_log_msg[code]);
			break;			
		default:
			Msg("!!!!! unsupport:category:[%d],code:[%d]",category,code);
			Msg("\n");

			return;			
			

	}

	if (pMSG_AdditionalData != NULL) {
		Msg(": ");
		switch (pMSG_AdditionalData->parameter_type) {
			case parameter_type_decimal_integer:
				for (i=0;i<pMSG_AdditionalData->parameter_length;i++) {
					Msg("%d  ",pMSG_AdditionalData->parameter_data.INT_data[i]);
				}
				break;	
			case parameter_type_hexadecimal_integer:
				Msg("0x");
				for (i=0;i<pMSG_AdditionalData->parameter_length;i++) {
					Msg("%x  ",pMSG_AdditionalData->parameter_data.INT_data[i]);
				}				
				break;	
			case parameter_type_Character:
				for (i=0;i<pMSG_AdditionalData->parameter_length;i++) {
					Msg("%c",pMSG_AdditionalData->parameter_data.Char_data[i]);
				}				
				break;					
				break;	
		default:
			Msg("!!!!! unsupport:parameter_type[%d]",pMSG_AdditionalData->parameter_type);
			break;				
		}
		
	}

	Msg("\n");

	return;
}


AME_U32 MSG_Init(pMSG_DATA pMSGDATA,pMPIO_DATA pMPIODATA,pAME_Data pAMEData, PVOID Log_virtualAddress)
{
	Global_Message_Serial_Number++;
	pMSGDATA->MSG_Serial_Number = Global_Message_Serial_Number;
	pMSGDATA->MSG_number = 1;
	pMSGDATA->MSG_log_memory_number = 1;
	

#ifdef Support_LogMemory
	pMSGDATA->LogMemory_DATA.Log_virtualAddress =  Log_virtualAddress;
	{
		pLog_header private_log_p = pMSGDATA->LogMemory_DATA.Log_virtualAddress;

		private_log_p->signature = log_signature;
		private_log_p->shared_pingpong = 0;
		private_log_p->count_p[0]=0;
		private_log_p->count_p[1]=0;

		pMSGDATA->LogMemory_DATA.current_Log_Address =  (PVOID)((char *)Log_virtualAddress+sizeof(Log_header));
	}	

#endif
	Msg("\n");
	
	pMPIODATA->pMSGDATA = (PVOID)pMSGDATA;
	pAMEData->pMSGDATA = (PVOID)pMSGDATA;
	
	return TRUE;
}




