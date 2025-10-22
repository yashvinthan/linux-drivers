/*
 *  AME_module.h
 *  Acxxx
 *
 *  Copyright (c) 2004-2014 Accusys, Inc. All rights reserved.
 *
 */

#include "OS_Define.h"

#define AME_Module_version "2.1.0.16"
#define AME_Module_Modify_Time "2014/11/19"

#ifndef AME_MODULE_H
#define AME_MODULE_H

#if defined (Enable_AME_message)

	#if defined (AME_MAC)
	#include <IOKit/IOLib.h>
	#define AME_Msg(fmt...) IOLog("AME Module:"); IOLog(fmt);IODelay(1000*1000);
	#define AME_Msg_Err(fmt...) IOLog("AME Module:!!! "); IOLog(fmt);IODelay(1000*1000);
	#endif

	#if defined (AME_Linux)
	#include <linux/module.h>
	#define AME_Msg(fmt...) printk("AME Module:"); printk(fmt)
	#define AME_Msg_Err(fmt...) printk("AME Module:!!! "); printk(fmt)
	#endif

	#if defined (AME_Windows)
	#include <ntddk.h>
	#define AME_Msg DbgPrint("AME Module:"); DbgPrint
	#define AME_Msg_Err DbgPrint("AME Module:!!! "); DbgPrint
	#endif

#else

	#if defined (AME_MAC)
	#include <IOKit/IOLib.h>
	#define AME_Msg(fmt...) //IOLog("AME Module:"); IOLog(fmt);
	#define AME_Msg_Err(fmt...) //IOLog("AME Module:!!! "); IOLog(fmt);IODelay(100*1000);
	#endif

	#if defined (AME_Linux)
	#include <linux/module.h>
	#define AME_Msg(fmt...) //printk("AME Module:"); printk(fmt)
	#define AME_Msg_Err(fmt...) //printk("AME Module:!!! "); printk(fmt)
	#endif

	#if defined (AME_Windows)
	#include <ntddk.h>
	#define AME_Msg //DbgPrint("AME Module:"); DbgPrint
	#define AME_Msg_Err //DbgPrint("AME Module:!!! "); DbgPrint
	#endif

#endif //Enable_AME_message

#define AME_Module_NEW

/* Define lock number */
#define Inbound_lock 0x01;
#define Outbound_lock 0x02;
#define QueueCmd_lock 0x03;


#define Vendor_ACS              0x14D6
#define Vendor_PLX              0x10B5

#define AME_8508_DID_NT         0x8508
#define AME_8608_DID_NT         0x8608
#define AME_87B0_DID_NT         0x512D

#define AME_6101_DID_DAS        0x6101
#define AME_6201_DID_DAS        0x6201
#define AME_2208_DID1_DAS       0x6232
#define AME_2208_DID2_DAS       0x6300
#define AME_2208_DID3_DAS       0x6233
#define AME_2208_DID4_DAS       0x6235

#define AME_3108_DID0_DAS		0x6240
#define AME_3108_DID1_DAS		0x6262

//#define AME_8508_DID_VID_NT     0x850810B5
//#define AME_8608_DID_VID_NT     0x860810B5

//#define AME_2208_DID1_VID_DAS   0x623214d6
//#define AME_2208_DID2_VID_DAS   0x630014d6
//#define AME_2208_DID3_VID_DAS   0x623314d6
//#define AME_3108_DID0_VID_DAS	  0x624014d6

#define PLX_Brige_8508          0x850810B5
#define PLX_Brige_8608          0x860810B5
#define PLX_Brige_8518          0x851810B5
#define PLX_Brige_8618          0x861810B5
#define PLX_Brige_8624          0x862410B5
#define PLX_Brige_8717          0x871710B5

// EEPROM Type
#define Eeprom_type_Z2M_G3      0x0303
#define Eeprom_type_Z1M_NT      0x0304
#define Eeprom_type_Z2D_G3      0x0305
#define Eeprom_type_C1M_NT      0x0402

#define NT_Valid_DoorBell_bit   0x0000F00F

#define Other_Bridge_Class      0x068000


// must less than 4.8K by raid system, define in here to 4720
#define REQUEST_MSG_SIZE 512
#define REQUEST_MSG_Header_SIZE 64

typedef void                *PVOID;

#ifndef AME_U8
    #define AME_U8 unsigned char
#endif

#ifndef AME_U16
    #define AME_U16 unsigned short
#endif

#ifndef AME_U32
    #define AME_U32 unsigned int
#endif

#ifndef AME_U64
    #define AME_U64 unsigned long long
#endif

#ifndef AME_bit
    #define AME_bit(x)  (1 << x)
#endif


#ifndef TRUE
    #define TRUE    1
    #define FALSE   0
#endif

#define AME_U32_NONE 0xFFFFFFFF
#define AME_U64_NONE 0xFFFFFFFFFFFFFFFF
    
// ****************************************************************************
// PCI System Interface Registers
// ****************************************************************************
#define AME_HOST_MSG_REGISTER                   (0x00000010)
#define AME_CTRL_RDY_REGISTER                   (0x00000018)
                                                
#define AME_HOST_INT_STATUS_REGISTER            (0x00000030)
#define AME_HIS_REPLY_INTERRUPT                 (0x00000008)
#define AME_HIS_CRTLRDY_INTERRUPT               (0x00000001)
                                                
#define AME_HOST_INT_MASK_REGISTER              (0x00000034)
#define AME_HIM_RIM                             (0x00000008)
#define AME_HIM_CRTLRDY                         (0x00000001)
                                                
#define AME_Inbound_Doorbell_PORT               (0x00000020)
#define AME_Outbound_Doorbell_PORT              (0x0000009C)
#define AME_Outbound_Doorbell_Clear_PORT        (0x000000A0)
                                                
#define AME_REQUEST_MSG_PORT                    (0x00000040)
#define AME_REPLY_MSG_PORT                      (0x00000044)

#define AME_Inbound_Free_List_Consumer_Index    (0x00000060)
#define AME_Inbound_Free_List_Producer_Index    (0x00000062)

#define AME_Inbound_Post_List_Consumer_Index    (0x00000064)
#define AME_Inbound_Post_List_Producer_Index    (0x00000066)

#define AME_Outbound_Free_List_Consumer_Index   (0x00000068)
#define AME_Outbound_Free_List_Producer_Index   (0x0000006a)

#define AME_2208_3108_SCARTCHPAD1_REG_OFFSET    (0x000000B4)
#define AME_2208_3108_SCARTCHPAD2_REG_OFFSET    (0x000000B8)

#define AME_RaidType_REG_OFFSET                 (0x000000BC)

// ****************************************************************************
//  BDL field definition and masks
// ****************************************************************************

// General
#define AME_BDL_BASIC_ELEMENT           (0x00)
#define AME_BDL_LIST_ELEMENT            (0x40)
#define AME_BDL_LIST_DIRECTION          (0x20)
#define AME_BDL_LIST_ADDRESS_SIZE       (0x10)
                                        
// Direction                            
#define AME_FLAGS_DIR_DTR_TO_HOST       (0x00)
#define AME_FLAGS_DIR_HOST_TO_DTR       (0x20)
                                        
// Address Size                         
#define AME_FLAGS_32_BIT_ADDRESSING     (0x00)
#define AME_FLAGS_64_BIT_ADDRESSING     (0x10)

// SCSI definition ----------------------------------------------------------

// SCSI CDB operation codes
#define SCSIOP_TEST_UNIT_READY      0x00
#define SCSIOP_REZERO_UNIT          0x01
#define SCSIOP_REWIND               0x01
#define SCSIOP_REQUEST_BLOCK_ADDR   0x02
#define SCSIOP_REQUEST_SENSE        0x03
#define SCSIOP_FORMAT_UNIT          0x04
#define SCSIOP_READ_BLOCK_LIMITS    0x05
#define SCSIOP_REASSIGN_BLOCKS      0x07
#define SCSIOP_READ6                0x08
#define SCSIOP_RECEIVE              0x08
#define SCSIOP_WRITE6               0x0A
#define SCSIOP_PRINT                0x0A
#define SCSIOP_SEND                 0x0A
#define SCSIOP_SEEK6                0x0B
#define SCSIOP_TRACK_SELECT         0x0B
#define SCSIOP_SLEW_PRINT           0x0B
#define SCSIOP_SEEK_BLOCK           0x0C
#define SCSIOP_PARTITION            0x0D
#define SCSIOP_READ_REVERSE         0x0F
#define SCSIOP_WRITE_FILEMARKS      0x10
#define SCSIOP_FLUSH_BUFFER         0x10
#define SCSIOP_SPACE                0x11
#define SCSIOP_INQUIRY              0x12
#define SCSIOP_VERIFY6              0x13
#define SCSIOP_RECOVER_BUF_DATA     0x14
#define SCSIOP_MODE_SELECT6         0x15
#define SCSIOP_RESERVE_UNIT         0x16
#define SCSIOP_RELEASE_UNIT         0x17
#define SCSIOP_COPY                 0x18
#define SCSIOP_ERASE                0x19
#define SCSIOP_MODE_SENSE6          0x1A
#define SCSIOP_START_STOP_UNIT      0x1B
#define SCSIOP_STOP_PRINT           0x1B
#define SCSIOP_LOAD_UNLOAD          0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC   0x1C
#define SCSIOP_SEND_DIAGNOSTIC      0x1D
#define SCSIOP_MEDIUM_REMOVAL       0x1E
#define SCSIOP_READ_CAPACITY        0x25
#define SCSIOP_READ                 0x28
#define SCSIOP_WRITE                0x2A
#define SCSIOP_SEEK                 0x2B
#define SCSIOP_LOCATE               0x2B
#define SCSIOP_WRITE_VERIFY         0x2E
#define SCSIOP_VERIFY               0x2F
#define SCSIOP_SEARCH_DATA_HIGH     0x30
#define SCSIOP_SEARCH_DATA_EQUAL    0x31
#define SCSIOP_SEARCH_DATA_LOW      0x32
#define SCSIOP_SET_LIMITS           0x33
#define SCSIOP_READ_POSITION        0x34
#define SCSIOP_SYNCHRONIZE_CACHE    0x35
#define SCSIOP_COMPARE              0x39
#define SCSIOP_COPY_COMPARE         0x3A
#define SCSIOP_WRITE_DATA_BUFF      0x3B
#define SCSIOP_READ_DATA_BUFF       0x3C
#define SCSIOP_CHANGE_DEFINITION    0x40
#define SCSIOP_READ_SUB_CHANNEL     0x42
#define SCSIOP_READ_TOC             0x43
#define SCSIOP_READ_HEADER          0x44
#define SCSIOP_PLAY_AUDIO           0x45
#define SCSIOP_PLAY_AUDIO_MSF       0x47
#define SCSIOP_PLAY_TRACK_INDEX     0x48
#define SCSIOP_PLAY_TRACK_RELATIVE  0x49
#define SCSIOP_PAUSE_RESUME         0x4B
#define SCSIOP_LOG_SELECT           0x4C
#define SCSIOP_LOG_SENSE            0x4D
#define SCSIOP_MODE_SELECT10        0x55     // Modified by Morris
#define SCSIOP_MODE_SENSE10         0x5A
#define SCSIOP_REPORT_LUNS          0xA0     // Added by Morris.
#define SCSIOP_LOAD_UNLOAD_SLOT     0xA6
#define SCSIOP_MECHANISM_STATUS     0xBD
#define SCSIOP_READ_CD              0xBE
#define SCSIOP_ACS_FIBRE_TEST_CMD   0x26     // Added by Morris
#define SCSIOP_ACS_ALIVE_CMD        0x27     // ythuang: for tramsmit alive msg
#define SCSIOP_ACS_CTRL_CMD         0xDD
#define SCSIOP_ACS_IDE_CMD          0xDE
#define SCSIOP_ACS_EMU_CMD          0xDF
#define SCSIOP_RED                  0xC0     //Redundant CDB operation code

// For newly support command
#define SCSIOP_READ12               0xA8
#define SCSIOP_WRITE12              0xAA
#define SCSIOP_WRITE_VERIFY12       0xAE
#define SCSIOP_VERIFY12             0xAF

// For Capacity Over2T
#define SCSIOP_READ_CAPACITY16      0x9E
#define SCSIOP_SYNCHRONIZE_CACHE16  0x91
#define SCSIOP_READ16               0x88
#define SCSIOP_WRITE16              0x8A
#define SCSIOP_WRITE_VERIFY16       0x8E
#define SCSIOP_VERIFY16             0x8F

// Denon CD ROM operation codes
#define SCSIOP_DENON_EJECT_DISC     0xE6
#define SCSIOP_DENON_STOP_AUDIO     0xE7
#define SCSIOP_DENON_PLAY_AUDIO     0xE8
#define SCSIOP_DENON_READ_TOC       0xE9
#define SCSIOP_DENON_READ_SUBCODE   0xEB

// SCSI bus status codes.
#define SCSISTAT_GOOD                  0x00
#define SCSISTAT_CHECK_CONDITION       0x02
#define SCSISTAT_CONDITION_MET         0x04
#define SCSISTAT_BUSY                  0x08
#define SCSISTAT_INTERMEDIATE          0x10
#define SCSISTAT_INTERMEDIATE_COND_MET 0x14
#define SCSISTAT_RESERVATION_CONFLICT  0x18
#define SCSISTAT_COMMAND_TERMINATED    0x22
#define SCSISTAT_QUEUE_FULL            0x28



//can't work of these codes
/*
#define AMEByteSwap64(x)    ( \
                ( ( ( x<<56 ) & 0xFF00000000000000 ) | \
              ( ( x<<40 ) & 0x00FF00000000000000 ) | \
              ( ( x>>24 ) & 0x0000FF0000000000 ) | \
              ( ( x<<8  ) & 0x000000FF00000000 ) | \
              ( ( x>>8  ) & 0x00000000FF000000 ) | \              
              ( ( x<<24 ) & 0x0000000000FF0000 ) | \
              ( ( x>>40 ) & 0x000000000000FF00 ) | \
              ( ( x>>56 ) & 0x00000000000000FF ) );
*/


#define AMEByteSwap32(x)    ( ( ( x<<24 ) & 0xFF000000 ) | \
              ( ( x<<8  ) & 0x00FF0000 ) | \
              ( ( x>>8  ) & 0x0000FF00 ) | \
              ( ( x>>24 ) & 0x000000FF ) );

#define AMEByteSwap16(x)    ( ( ( x<<8 ) & 0xFF00 ) | \
              ( ( x>>8 ) & 0x00FF ) );


#ifndef PACKED
    #if defined (AME_MAC) || defined (AME_Linux)            
        #define PACKED __attribute__((packed))
    #else  
        #define PACKED
    #endif
#endif


//LED Data Structure
typedef struct _AME_LED_Data
{
    AME_U32         LED_HBA_Type;               // HBA type EX.8717 or 8608
    AME_U32         LED_HBA_Port_Number;        // HBA LED Port number (EX.8717 port 1,2)
    PVOID           LED_HBA_Base_Address;       // HBA LED (EX.63200)
    PVOID           LED_Raid_Base_Address;      // Raid bridge board (EX.460 Raid)
} PACKED AME_LED_Data, *pAME_LED_Data;

#define MAX_Command_AME         512
#define MUEmpty                 0xFFFFFFFF  // Empty Context
#define AME_NORMAL_REPLY_N_BIT  (0x80000000)
#define MAX_NT_System_ID        4
#define MAX_RAID_System_ID      32
#define MAX_RAID_Target_ID      64
#define MAX_RAID_LUN_ID         64
#define MAX_NT_LUN_Event_Size   96
#define MAX_NT_Event_Elements   96
#define MAX_NT_Reply_Elements   513
#define MPIO_Master_Request_Bit 0x8000


//
// Default Reply Header
//
typedef struct _AMEDefaultReply
{
    AME_U8          Flags;
    AME_U8          ReplyStatus;
    AME_U8          ReplyLength;
    AME_U8          Function;
    AME_U32         MsgIdentifier;
} PACKED AMEDefaultReply, *pAMEDefaultReply;

//
// Default Request Header
//
#if defined (Enable_SW16_Debug_Check)
typedef struct _AMEDefault_Request
{
    AME_U8          Reserved[2];        /* 00h */
    AME_U8          Flags;              /* 02h */
    AME_U8          Function;           /* 03h */
    AME_U32         MsgIdentifier;      /* 04h */
} PACKED AMEDefault_Request, *pAMEDefault_Request;
#endif

typedef struct _AME_VIRTUAL_REPLY_QUEUE3_REPLY
{
    AME_U8         	Flags;				/* 00h */
	AME_U8			ReplyStatus;        /* 01h */
    AME_U8          ReplyLength;        /* 02h */
    AME_U8          Function;           /* 03h */
    AME_U32         MsgIdentifier;      /* 04h */
} AMEVirtualReplyQueue3Reply_t, *pAMEVirtualReplyQueue3Reply_t;


typedef struct _AME_Buffer_Size
{
    AME_U32         Request_frame_size;
    AME_U32         Reply_frame_size;
    AME_U32         Sense_buffer_size;
    AME_U32         NT_Reply_queue_size;
    AME_U32         NT_Event_queue_size;
    
} PACKED AME_Buffer_Size, *pAME_Buffer_Size;


typedef struct _AME_Buffer_Data
{
    AME_U64         Request_Buffer_Phy_addr;
    AME_U64         Request_Buffer_Vir_addr;
    AME_U64         Request_BDL_Phy_addr;
    AME_U64         Request_BDL_Vir_addr;
    AME_U64         Sense_Buffer_Phy_addr;
    AME_U64         Sense_Buffer_Vir_addr;
    AME_U32         BufferID;
    AME_U32         Raid_ID;            /* NT used only, for Raid Error handle */
    AME_U8          ReqInUse;
    AME_U8          ReqTimeouted;
    PVOID           pNext;
    PVOID           pRequestExtension;  
	PVOID           pMPIORequestExtension;
	PVOID           pAME_Raid_QueueBuffer;		// for RAID 0/1
	AME_U32         ReqIn_Path_Remove;
	PVOID           pAMEData_Path;
	AME_U32         Path_Raid_ID;
    void            (*callback)(PVOID); /* call back function */
    
} PACKED AME_Buffer_Data, *pAME_Buffer_Data;


typedef struct _AME_Reply_Buffer_Data
{
    AME_U64         Reply_Buffer_Phy_addr;
    AME_U64         Reply_Buffer_Vir_addr;
} PACKED AME_Reply_Buffer_Data, *pAME_Reply_Buffer_Data;


#define EVENT_LOG_QUEUE_SIZE        (64)
#define AME_EVENT_STRING_LENGTH     (32)
#define AME_EVENT_SWTICH_OFF        (0x00)
#define AME_EVENT_SWTICH_ON         (0x01)

typedef struct _AME_EventLog
{
    AME_U8          eventstring[AME_EVENT_STRING_LENGTH];
} PACKED AME_EventLog, *pAME_EventLog;

typedef struct _AME_EventLogQueue 
{
    AME_EventLog    eventlog[EVENT_LOG_QUEUE_SIZE];
    AME_U32         head;
    AME_U32         tail;
} PACKED AME_EventLogQueue, *pAME_EventLogQueue;

// Event Switch Request Structure
typedef struct _AMEEventSwitchRequest_DAS
{
    AME_U8          Reserved[2];        /* 00h */
    AME_U8          Switch;             /* 02h */
    AME_U8          Function;           /* 03h */
    AME_U32         MsgIdentifier;      /* 04h */
} PACKED AMEEventSwitchRequest_DAS, *pAMEEventSwitchRequest_DAS;

// Reply Structure
typedef struct _AMEEventSwitchReply
{
    AME_U8          Flags;              /* 00h */
    AME_U8          ReplyStatus;        /* 01h */
    AME_U8          ReplyLength;        /* 02h */
    AME_U8          Function;           /* 03h */
    AME_U32         MsgIdentifier;      /* 04h */   
    AME_U8          EDB[88];            /* 08h */
} PACKED AMEEventSwitchReply, *pAMEEventSwitchReply;


#define INBAND_CMD_LENGHT                       (32)
//#define INBAND_BUFFER_SIZE                      (5*1024)
#define INBAND_External_BUFFER_SIZE             (5*1024)
#define INBAND_Internal_BUFFER_SIZE             (1024)

#define INBANDCMD_IDLE                          (0)
#define INBANDCMD_USING                         (1)
#define INBANDCMD_COMPLETE                      (2)
#define INBANDCMD_FAILED                        (3)

#define AME_REPLYSTATUS_SUCCESS                 (0x00)

#define AME_INBAND_FLAGS_NODATATRANSFER         (0x00000000)
#define AME_INBAND_FLAGS_WRITE                  (0x00000001)
#define AME_INBAND_FLAGS_READ                   (0x00000002)
#define AME_INBAND_FLAGS_DATADIRECTION_MASK     (0x00000003)



typedef struct _AME_REQUEST_CallBack    PACKED AME_REQUEST_CallBack,*pAME_REQUEST_CallBack;


typedef struct _AME_InBand_Buffer_Data
{
    AME_U8          InBandFlag;
    AME_U8          InBandReply;
    AME_U16         InBandErrorCode;
    AME_U32         InBand_Length;
    PVOID           InBand_DataBuffer;
    AME_U64         InBand_External_Buffer_Phy_addr;
    AME_U64         InBand_External_Buffer_Vir_addr;
    AME_U64         InBand_Internal_Buffer_Phy_addr;
    AME_U64         InBand_Internal_Buffer_Vir_addr;
    AME_U32         (*callback)(pAME_REQUEST_CallBack); /* Internal call back function */
} PACKED AME_InBand_Buffer_Data, *pAME_InBand_Buffer_Data;

typedef struct _AMEInBand_BDLUnion
{
    AME_U32         FlagsLength;
    AME_U32         Address_Low;
    AME_U32         Address_High;

} PACKED AMEInBand_BDLUnion, *pAMEInBand_BDLUnion;

// Request Structure
typedef struct _AMEInBandRequest_DAS
{
    AME_U8                      Reserved[2];        /* 00h */
    AME_U8                      Flags;              /* 02h */
    AME_U8                      Function;           /* 03h */
    AME_U32                     MsgIdentifier;      /* 04h */
    AME_U8                      InBandCDB[32];      /* 08h */
    AME_U32                     DataLength;         /* 28h */
    AME_U32                     Reserved2;          /* 2Ch */
    AMEInBand_BDLUnion          BDL;                /* 30h */
} PACKED AMEInBandRequest_DAS, *pAMEInBandRequest_DAS;

typedef struct _AMEInBandRequest_NT
{
    AME_U8                      Reserved[2];        /* 00h */
    AME_U8						Flags;				/* 02h */
    AME_U8                      Function;           /* 03h */
    AME_U32                     MsgIdentifier;      /* 04h */
    AME_U8						InBandCDB[32];		/* 08h */
	AME_U32						DataLength;			/* 28h */
	AME_U32						Reserved2;			/* 2Ch */
	AME_U32						PCISerialNumber;	/* 30h -- NT Port */
	AME_U32						ReplyAddressLow;	/* 34h -- NT Port */
    AMEInBand_BDLUnion          BDL;                /* 38h */
} PACKED AMEInBandRequest_NT, *pAMEInBandRequest_NT;

// Reply Structure
typedef struct _AMEInBandReply
{
    AME_U8                      Flags;              /* 00h */
    AME_U8                      ReplyStatus;        /* 01h */
    AME_U8                      ReplyLength;        /* 02h */
    AME_U8                      Function;           /* 03h */
    AME_U32                     MsgIdentifier;      /* 04h */   

    // Inband ErrorCode Implement
    AME_U16                     InBandErrorCode;    /* 08h */
    AME_U16                     Reserved;           /* 10h */
    // ~Inband ErrorCode Implement

} PACKED AMEInBandReply, *pAMEInBandReply;


//Lun_Change
#define Lun_Change_NONE                 0
#define Lun_Change_Add                  AME_bit(1)
#define Lun_Change_Remove               AME_bit(2)

//Lun_Mask
#define Mask  1
#define UnMask  0

//Lun_Data
#define Lun_Not_Exist  0xffff

//#define of Lun_Change state
#define Lun_Remove      0
#define Lun_Add         1

// Lun_Change_State
#define Lun_Change_State_NONE           0
#define Lun_Change_State_Need_check     AME_bit(1)
#define Lun_Change_State_Checking       AME_bit(2)
#define Lun_Mask_Need_check AME_bit(3)

//Inband page no.
#define Lun_Map_Page    0x00
#define Lun_Mask_Page 0x33


//Das Raid ID
#define Das_Raid_ID 0

// ****************************************************************************
// NT Register definition
// ****************************************************************************

// Should use 1MB
// Using 1MB to support 32 ACS62s
#define ACS62_BAR_OFFSET                0x100000 // 1M
#define ACS2208_BAR_OFFSET              0x100000 // 1M
#define ACS3108_BAR_OFFSET              0x100000 // 1M

// NT Link Side Register
#define	NT_LINK_LINKSTATE_REG_OFFSET_8508		            0x11078
#define	NT_LINK_LINKSTATE_REG_OFFSET_8608		            0x11078
#define	NT_LINK_LINKSTATE_REG_OFFSET_87B0_P0	            0x3F078
#define	NT_LINK_LINKSTATE_REG_OFFSET_87B0_P1	            0x3D078

//#define NT_LINK_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET			0x11C3C
//#define NT_LINK_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET_upper		0x11C40
//#define NT_LINK_LUT_REG_OFFSET_BASE							0x11DB4

// NT Virtual Side Register
#define	NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET_8508	 0x10C3C
#define	NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET_8608	 0x10C3C
#define	NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET_87B0_P0 0x3eC3C
#define	NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET_87B0_P1 0x3cC3C

// NT Virtual Side DoorBell Register for PLX 8508
#define	NT_VIRTUAL_SET_VIRTUAL_IRQ_OFFSET_8508			    0x10090
#define	NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_OFFSET_8508			0x10094
#define	NT_VIRTUAL_SET_VIRTUAL_IRQ_MASK_OFFSET_8508			0x10098
#define	NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_MASK_OFFSET_8508		0x1009C
#define	NT_VIRTUAL_SET_Link_IRQ_OFFSET_8508					0x100A0    
#define	NT_VIRTUAL_CLEAR_Link_IRQ_OFFSET_8508				0x100A4
#define	NT_VIRTUAL_SET_Link_IRQ_MASK_OFFSET_8508			0x100A8    
#define	NT_VIRTUAL_CLEAR_Link_IRQ_MASK_OFFSET_8508			0x100AC

// NT Virtual Side Scratch Regitser for PLX 8508
#define	NT_VIRTUAL_SCARTCHPAD0_REG_OFFSET_8508			    0x100B0
#define	NT_VIRTUAL_SCARTCHPAD1_REG_OFFSET_8508			    0x100B4
#define	NT_VIRTUAL_SCARTCHPAD2_REG_OFFSET_8508			    0x100B8
#define	NT_VIRTUAL_SCARTCHPAD3_REG_OFFSET_8508			    0x100BC
#define	NT_VIRTUAL_SCARTCHPAD4_REG_OFFSET_8508			    0x100C0
#define	NT_VIRTUAL_SCARTCHPAD5_REG_OFFSET_8508			    0x100C4
#define	NT_VIRTUAL_SCARTCHPAD6_REG_OFFSET_8508			    0x100C8
#define	NT_VIRTUAL_SCARTCHPAD7_REG_OFFSET_8508			    0x100CC

// NT Virtual Side DoorBell Register for PLX 8608
#define	NT_VIRTUAL_SET_VIRTUAL_IRQ_OFFSET_8608			    0x10c4c
#define	NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_OFFSET_8608		    0x10c50
#define	NT_VIRTUAL_SET_VIRTUAL_IRQ_MASK_OFFSET_8608		    0x10c54
#define	NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_MASK_OFFSET_8608	    0x10c58
#define	NT_VIRTUAL_SET_Link_IRQ_OFFSET_8608					0x10c5c
#define	NT_VIRTUAL_CLEAR_Link_IRQ_OFFSET_8608				0x10c60
#define	NT_VIRTUAL_SET_Link_IRQ_MASK_OFFSET_8608			0x10c64
#define	NT_VIRTUAL_CLEAR_Link_IRQ_MASK_OFFSET_8608			0x10c68

// NT Virtual Side Scratch Regitser for PLX 8608
#define	NT_VIRTUAL_SCARTCHPAD0_REG_OFFSET_8608			    0x10c6c
#define	NT_VIRTUAL_SCARTCHPAD1_REG_OFFSET_8608			    0x10c70
#define	NT_VIRTUAL_SCARTCHPAD2_REG_OFFSET_8608			    0x10c74
#define	NT_VIRTUAL_SCARTCHPAD3_REG_OFFSET_8608			    0x10c78
#define	NT_VIRTUAL_SCARTCHPAD4_REG_OFFSET_8608			    0x10C7c
#define	NT_VIRTUAL_SCARTCHPAD5_REG_OFFSET_8608			    0x10C80
#define	NT_VIRTUAL_SCARTCHPAD6_REG_OFFSET_8608			    0x10C84
#define	NT_VIRTUAL_SCARTCHPAD7_REG_OFFSET_8608			    0x10C88

// NT Virtual Side DoorBell Register for PLX 87B0 Port0
#define	NT_VIRTUAL_SET_VIRTUAL_IRQ_OFFSET_87B0_P0		    0x3ec4c
#define	NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_OFFSET_87B0_P0		    0x3ec50
#define	NT_VIRTUAL_SET_VIRTUAL_IRQ_MASK_OFFSET_87B0_P0		0x3ec54
#define	NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_MASK_OFFSET_87B0_P0	0x3ec58
#define	NT_VIRTUAL_SET_Link_IRQ_OFFSET_87B0_P0				0x3ec5c
#define	NT_VIRTUAL_CLEAR_Link_IRQ_OFFSET_87B0_P0			0x3ec60
#define	NT_VIRTUAL_SET_Link_IRQ_MASK_OFFSET_87B0_P0			0x3ec64
#define	NT_VIRTUAL_CLEAR_Link_IRQ_MASK_OFFSET_87B0_P0		0x3ec68

// NT Virtual Side Scratch Regitser for PLX 87B0 Port0
#define	NT_VIRTUAL_SCARTCHPAD0_REG_OFFSET_87B0_P0		    0x3ec6c
#define	NT_VIRTUAL_SCARTCHPAD1_REG_OFFSET_87B0_P0		    0x3ec70
#define	NT_VIRTUAL_SCARTCHPAD2_REG_OFFSET_87B0_P0		    0x3ec74
#define	NT_VIRTUAL_SCARTCHPAD3_REG_OFFSET_87B0_P0		    0x3ec78
#define	NT_VIRTUAL_SCARTCHPAD4_REG_OFFSET_87B0_P0		    0x3eC7c
#define	NT_VIRTUAL_SCARTCHPAD5_REG_OFFSET_87B0_P0		    0x3eC80
#define	NT_VIRTUAL_SCARTCHPAD6_REG_OFFSET_87B0_P0		    0x3eC84
#define	NT_VIRTUAL_SCARTCHPAD7_REG_OFFSET_87B0_P0		    0x3eC88

// NT Virtual Side DoorBell Register for PLX 87B0 Port1
#define	NT_VIRTUAL_SET_VIRTUAL_IRQ_OFFSET_87B0_P1		    0x3cc4c
#define	NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_OFFSET_87B0_P1		    0x3cc50
#define	NT_VIRTUAL_SET_VIRTUAL_IRQ_MASK_OFFSET_87B0_P1		0x3cc54
#define	NT_VIRTUAL_CLEAR_VIRTUAL_IRQ_MASK_OFFSET_87B0_P1	0x3cc58
#define	NT_VIRTUAL_SET_Link_IRQ_OFFSET_87B0_P1				0x3cc5c
#define	NT_VIRTUAL_CLEAR_Link_IRQ_OFFSET_87B0_P1			0x3cc60
#define	NT_VIRTUAL_SET_Link_IRQ_MASK_OFFSET_87B0_P1			0x3cc64
#define	NT_VIRTUAL_CLEAR_Link_IRQ_MASK_OFFSET_87B0_P1		0x3cc68

// NT Virtual Side Scratch Regitser for PLX 87B0 Port1
#define	NT_VIRTUAL_SCARTCHPAD0_REG_OFFSET_87B0_P1		    0x3cc6c
#define	NT_VIRTUAL_SCARTCHPAD1_REG_OFFSET_87B0_P1		    0x3cc70
#define	NT_VIRTUAL_SCARTCHPAD2_REG_OFFSET_87B0_P1		    0x3cc74
#define	NT_VIRTUAL_SCARTCHPAD3_REG_OFFSET_87B0_P1		    0x3cc78
#define	NT_VIRTUAL_SCARTCHPAD4_REG_OFFSET_87B0_P1		    0x3cC7c
#define	NT_VIRTUAL_SCARTCHPAD5_REG_OFFSET_87B0_P1		    0x3cC80
#define	NT_VIRTUAL_SCARTCHPAD6_REG_OFFSET_87B0_P1		    0x3cC84
#define	NT_VIRTUAL_SCARTCHPAD7_REG_OFFSET_87B0_P1		    0x3cC88

// NT Hotplug state define
#define NT_Hotplug_First_Load_Wait_SW_Ready         0
#define NT_Hotplug_First_Load_SendVirtual_Cmd       1
#define NT_Hotplug_First_Load_Wait_Sendvir_Ready    2
#define NT_Hotplug_Raid_Change_Check                3
#define NT_Hotplug_Raid_Change_Wait_Sendvir_Ready   4

typedef struct _AME_MPIO_Req_FIFO_Data
{
	AME_U32 Buffer_InUsed;
	AME_U32 Raid_ID;
	AME_U64 Request_Buffer_Phy_addr;
	
} PACKED AME_MPIO_Req_FIFO_Data, *pAME_MPIO_Req_FIFO_Data;

typedef struct _AME_MPIO_Reply_FIFO_Data
{
	AME_U32 Buffer_InUsed;
	AME_U32 Raid_ID;
	AME_U32 Reply_Msg;
	
} PACKED AME_MPIO_Reply_FIFO_Data, *pAME_MPIO_Reply_FIFO_Data;


typedef struct _AME_RAID_Data
{
    AME_U16     Lun_Data[MAX_RAID_LUN_ID];
    AME_U16     Lun_Change[MAX_RAID_LUN_ID];
    AME_U8		Lun_Mask[MAX_RAID_LUN_ID];
    AME_U32     Lun_Change_State;
    
    //  DAS 2208 Raid Link Error handle ->
    AME_U32     AME_Raid_Bar_Reg_0x10;
    AME_U32     AME_Raid_Bar_Reg_0x14;
    AME_U32     AME_ReInit_State;
    AME_U32     Error_handle_State;
    AME_U32     Bar_address_lost;
    //  DAS 2208 Raid Link Error handle <-
    
    //  NT enviroment used only ->
    AME_U32     Read_Index;     //for Software Raid.
    AME_U32     NTRAIDType;
    AME_U32     SendVir_Ready;
    AME_U32     InBand_Loaded_Ready;
    AME_U32     Raid_Error_Handle_State;
    
    AME_U32     NT_REQUEST_MSG_PORT;
    
    AME_U64     NT_Raid_Reply_queue_Start_Phy_addr;
    AME_U64     NT_Raid_Reply_queue_Start_Vir_addr;
    AME_U64     NT_Raid_Event_queue_Start_Phy_addr;
    AME_U64     NT_Raid_Event_queue_Start_Vir_addr;
    //  NT enviroment used only <-
    
} PACKED AME_RAID_Data, *pAME_RAID_Data;


typedef struct _AME_NT_Host_data
{
    AME_U32     Port_Number;            // Only PLX 8717(NT 87B0) used, support Port0 and Port1
    AME_U32     NT_SET_VIRTUAL_IRQ_REG;
	AME_U32     NT_CLEAR_VIRTUAL_IRQ_REG;
	AME_U32     NT_SET_VIRTUAL_IRQ_MASK_REG;
	AME_U32     NT_CLEAR_VIRTUAL_IRQ_MASK_REG;
	
	AME_U32     NT_SET_Link_IRQ_REG;
	AME_U32     NT_CLEAR_Link_IRQ_REG;
	AME_U32     NT_SET_Link_IRQ_MASK_REG;
	AME_U32     NT_CLEAR_Link_IRQ_MASK_REG;
	
	AME_U32     NT_LINK_LINKSTATE_REG_OFFSET;
	AME_U32     NT_VIRTUAL_MEM_BAR2_ADDRESS_TRANS_REG_OFFSET;
	
	AME_U32  	Read_Index;
	AME_U32  	Hotplug_State;
    AME_U32  	Check_SendVirtual_Ready_retry_index;	
    
    AME_U32     SCRATCH_REG_0;                          // Scratch regitser 0
    AME_U32     SCRATCH_REG_1;                          // Scratch register 1
    AME_U32     SCRATCH_REG_2;                          // Scratch register 2
    AME_U32     SCRATCH_REG_3;                          // Scratch register 3
    AME_U32     SCRATCH_REG_4;                          // Scratch register 4
    AME_U32     SCRATCH_REG_5;                          // Scratch register 5
    AME_U32     SCRATCH_REG_6;                          // Scratch register 6
    AME_U32     SCRATCH_REG_7;                          // Scratch register 7
    
    AME_U32     NT_SW_Ready_State;                      // From Scratch register 4
    AME_U32     PCISerialNumber;                        // From Scratch register 0
    AME_U32     NT_linkside_BAR2_lowaddress;            // From Scratch register 1
    AME_U32     NT_linkside_BAR2_highaddress;           // From Scratch register 3
    AME_U32     Raid_All_Topology;                      // From Scratch register 2
    AME_U32     Raid_2208_3108_Topology;                     // From Scratch register 5
    AME_U32     Error_Raid_bitmap;                      // From Scratch register 6
    AME_U32     Raid_Change_Topology;                   // Raid Change status
    AME_U32     NTBar_Offset;                           // Error handle for SW Raid not base on (0x80000000)
    
    AME_U32     Workaround_87B0_Done;
    AME_U32     Workaround_87B0_Wait_index;
    
} PACKED AME_NT_Host_data, *pAME_NT_Host_data;


typedef struct _AME_Restore_data
{
    AME_U32     NT_Bar2_0x18;
    
} PACKED AME_Restore_data, *pAME_Restore_data;


typedef struct _AME_Data
{
	AME_U32                 AME_Serial_Number;
    AME_U32                 Device_BusNumber;
    AME_U16                 Vendor_ID;
    AME_U16                 Device_ID;
    PVOID                   Raid_Base_Address;
    PVOID                   NT_Base_Address;            // for NT used only
    AME_U32                 Glo_High_Address;           // for Raid init used only
    PVOID                   pDeviceExtension;
    AME_LED_Data            AME_LED_data;               // for LED used only
    AME_U32                 AME_Raid_Link_Status;       // for InBand get page 0 used only
    AME_U32                 AME_RAID_Ready;	            // for DAS timer init
    AME_U32                 Polling_init_Enable;        // for AME_Init Polling
    AME_U32                 Init_Memory_by_AME_module;  // for AME Data buffer init by self or ame module.
        
    AME_U32                 Total_Support_command;
    AME_U32                 SG_per_command;
    AME_U64                 AME_Buffer_physicalStartAddress;
    AME_U64                 AME_Buffer_virtualStartAddress; 

    // internal of AME
    AME_U32                 ReInit;
    AME_Restore_data        AME_RestoreData;
    AME_U32                 Message_SIZE;
    AME_U32                 AMEBufferSize_PerCommand;
    AME_U32                 Request_frame_size_PerCommand;
    AME_U32                 Reply_frame_size_PerCommand;
    AME_U32                 Sense_buffer_size_PerCommand;
    AME_U32                 NT_Reply_queue_size_PerRaid;
    AME_U32                 NT_Event_queue_size_PerRaid;
    PVOID                   AMEBuffer_Head;
    PVOID                   AMEBuffer_Tail;
    AME_U32                 AMEBufferissueCommandCount;
    AME_Buffer_Data         AMEBufferData[MAX_Command_AME];
    AME_Reply_Buffer_Data   AMEReplyBufferData[MAX_Command_AME];
  
    // Only support one NT device.
    AME_U32                 Is_First_NT_device;
    
    // Fixed NT lose ISR issue
    AME_U32                 Next_NT_ISR_Check;
    
    // Event SW state
    AME_U8                  AMEEventSwitchState;
    
    // QueueEventlog
    AME_EventLogQueue       AMEEventlogqueue;
  
    // InBand Buffer
    AME_InBand_Buffer_Data  AMEInBandBufferData;
    
    AME_RAID_Data           AME_RAIDData[MAX_RAID_System_ID];
    AME_NT_Host_data        AME_NTHost_data;
    
	PVOID                   pMPIOData;
	PVOID                   pAMEQueueData;
	PVOID					pMSGDATA;
    
} PACKED AME_Data, *pAME_Data;


typedef struct _AME_Host_SG
{
    AME_U64           Address;
    AME_U32           BufferLength;  
    AME_U32           Reserve;
} PACKED AME_Host_SG, *pAME_Host_SG;


typedef struct _AME_RAID_SCSIRW_SG
{
    AME_U64                     Address;
    AME_U32                     BufferLength;
    AME_U32                     ExtensionBit;
} PACKED AME_RAID_SCSIRW_SG, *pAME_RAID_SCSIRW_SG;


typedef struct _BDL_LIST
{
    AME_U32                     FlagsLength;
    AME_U32                     Addr_LowPart;   
    AME_U32                     Addr_HighPart;  
    //AME_U64                     Address;
} PACKED BDLList, *pBDLList;


// Control definition 
#define AME_SCSI_CONTROL_DATADIRECTION_MASK   (0x00000003)
#define AME_SCSI_CONTROL_NODATATRANSFER       (0x00000000)
#define AME_SCSI_CONTROL_WRITE                (0x00000001)
#define AME_SCSI_CONTROL_READ                 (0x00000002)

#define AME_SCSI_CONTROL_SENSEBUF_ADDR_MASK   (0x00000004)
#define AME_SCSI_CONTROL_SENSEBUF_ADDR_32     (0x00000000)
#define AME_SCSI_CONTROL_SENSEBUF_ADDR_64     (0x00000004)

#define AME_SCSI_CONTROL_SENSEBUF_TAG_MASK    (0x00000038)
#define AME_SCSI_CONTROL_SENSEBUF_TAG_SIMPLE  (0x00000000)
#define AME_SCSI_CONTROL_SENSEBUF_TAG_HEAD    (0x00000008)
#define AME_SCSI_CONTROL_SENSEBUF_TAG_ORDER   (0x00000010)
#define AME_SCSI_CONTROL_SENSEBUF_TAG_ACA     (0x00000020)
#define AME_SCSI_CONTROL_SENSEBUF_TAG_UNTAG   (0x00000028)
#define AME_SCSI_CONTROL_SENSEBUF_TAG_NODIS   (0x00000038)

// ****************************************************************************
// Message Function Variables
// ****************************************************************************
#define AME_FUNCTION_SCSI_REQUEST           (0x00)
#define AME_FUNCTION_SCSI_TASK_MGMT         (0x01)
#define AME_FUNCTION_EVENT_SWITCH           (0x02)
#define AME_FUNCTION_IN_BAND                (0x03)
#define AME_FUNCTION_SCSI_PASSTHGH          (0x04)
#define AME_FUNCTION_ATA_PASSTHGH           (0x05)

// NT Function
#define AME_FUNCTION_SCSI_EXT2_REQUEST		(0x09)
#define AME_FUNCTION_VIRTUAL_REPLY_QUEUE3	(0x0A)
#define AME_FUNCTION_SCSI_TASK_MGMT_EXT2	(0x0B)
#define AME_FUNCTION_IN_BAND_EXT           	(0x0C)
#define AME_FUNCTION_EVENT_EXT_SWITCH		(0x0D)


// Request Structure
typedef struct _AME_MSG_SCSI_REQUEST_FRAME_DAS
{
    AME_U8                      Reserved[2];        /* 00h */
    AME_U8                      Lun;                /* 02h */   // Host need prepare this
    AME_U8                      Function;           /* 03h */   
    AME_U32                     MsgIdentifier;      /* 04h */   
    AME_U8                      CDBLength;          /* 08h */   // Host need prepare this
    AME_U8                      SenseBufferLength;  /* 09h */
    AME_U8                      Reserved2[2];       /* 0Ah */   
    AME_U32                     Control;            /* 0Ch */   // Host need prepare this
    AME_U8                      CDB[16];            /* 10h */   // Host need prepare this
    AME_U32                     DataLength;         /* 20h */   // Host need prepare this
    AME_U32                     SenseBufferLowAddr; /* 24h */
    AME_U32                     BDL;                /* 28h */
} PACKED AMESCSIRequest_DAS, *pAMESCSIRequest_DAS;


typedef struct _AME_MSG_SCSI_REQUEST_FRAME_NT
{
    AME_U8                      Reserved[2];        /* 00h */
    AME_U8                      Lun;                /* 02h */
    AME_U8                      Function;           /* 03h */
    AME_U32                     MsgIdentifier;      /* 04h */
    AME_U8                      CDBLength;          /* 08h */
    AME_U8                      SenseBufferLength;  /* 09h */
    AME_U8                      Reserved2[2];       /* 0Ah */
    AME_U32                     Control;            /* 0Ch */
    AME_U8                      CDB[16];            /* 10h */
    AME_U32                     DataLength;         /* 20h */
    AME_U32                     SenseBufferLowAddr; /* 24h */
    AME_U32                     PCISerialNumber;    /* 28h -- NT Port */
    AME_U32                     ReplyAddressLow;    /* 2Ch -- NT Port */
    AME_U32                     BDL;                /* 30h */   
} PACKED AMESCSIRequest_NT, *pAMESCSIRequest_NT;


typedef struct _AME_VIRTUAL_REPLY_QUEUE3_REQUEST_NT
{
    AME_U8                      Reserved[3];        	/* 00h */
    AME_U8                      Function;           	/* 03h */
    AME_U32                     MsgIdentifier;      	/* 04h */
	AME_U32						PCISerialNumber;		/* 08h */
	AME_U32						ReplyAddressLow;		/* 0Ch */
	AME_U32						ReplyQueueLow;			/* 10h */
	AME_U32						RemoteGlobalHigh;		/* 14h */
	AME_U32						EventReplyStartAddrLow;	/* 18h */
} PACKED AME_VIRTUAL_REPLY_QUEUE3_REQUEST_NT, *pAME_VIRTUAL_REPLY_QUEUE3_REQUEST_NT;


// NT Raid type define
// must match AME_Queue.h Buffer Queue type define
// Buffer Queue type
// #define ReqType_NT_SCSI_341            0x01
// #define ReqType_NT_SCSI_2208           0x02
#define NTRAIDType_341          0x01
#define NTRAIDType_2208         0x02
#define NTRAIDType_3108         0x03
#define NTRAIDType_MPIO_Slave   0x04
#define NTRAIDType_SOFR_Raid    0x05


struct _AME_REQUEST_CallBack {
        pAME_Data       pAMEData;
        AME_U32         Raid_ID;
        AME_U32         BufferID;
        PVOID           pRequestExtension;
		PVOID			pMPIORequestExtension;
		PVOID           pAME_Raid_QueueBuffer;		// for RAID 0/1
        PVOID           pRequest;
        PVOID           pReplyFrame;
        PVOID           pSCSISenseBuffer;
		AME_U32         TransferedLength;
		AME_U32         Retry_Cmd;
};


typedef struct _AME_Module_SCSI_Command {
        AME_U32                     Raid_ID;            // for NT used only
        AME_U32                     Target_ID;
        AME_U32                     Lun_ID;             /* 02h */   // Host need prepare this
        AME_U8                      CDBLength;          /* 08h */   // Host need prepare this
        AME_U32                     Control;            /* 0Ch */   // Host need prepare this
        AME_U8                      CDB[16];            /* 10h */   // Host need prepare this
        AME_U32                     DataLength;         /* 20h */   // Host need prepare this
        AME_U32                     SGCount;            // for R0/R1
        AME_Host_SG                 AME_Host_Sg[130];   // for R0/R1
        AME_U32                     Slave_Raid_ID;      /* MPIO used */
        PVOID                       pRequestExtension;
		PVOID                       pMPIORequestExtension;		// for MPIO
		PVOID                       pAME_Raid_QueueBuffer;		// for RAID 0/1
		AME_U32                     MPIO_Which;		// for RAID 0/1
        void                        (*callback)(PVOID); /* call back function */
} PACKED AME_Module_SCSI_Command, *pAME_Module_SCSI_Command;


typedef struct _AME_Module_InBand_Command {
        PVOID                       pRequestExtension;
        PVOID                       InBand_CDB;
        PVOID                       InBand_Buffer_addr;
        AME_U32                     (*callback)(pAME_REQUEST_CallBack); /* call back function */
} PACKED AME_Module_InBand_Command, *pAME_Module_InBand_Command;


typedef struct _AME_SCSI_REQUEST {

        PVOID                       SCSIRequest;
        AME_U64                     SCSIRequest_physicalStartAddress;
        AME_U32                     SGCount;
        void                        *pSG;
        AME_U64                     BDL_physicalStartAddress;
        AME_U64                     BDL_virtualStartAddress;  
        PVOID                       pDriverExtension;
        AME_U32                     NT_RAID_System_Index;           // for NT used only
        AME_U32                     NT_linkside_BAR2_lowaddress;    // for NT used only
        AME_U32                     NT_linkside_BAR2_highaddress;   // for NT used only
        AME_U32                     NTBar_Offset;                   // for NT used only
        AME_U32                     NTRAIDType;                     // for NT used only
        AME_U32                     Request_BufferID;
        void                        (*callback)(PVOID); /* call back function */
        
} PACKED AME_SCSI_REQUEST, *pAME_SCSI_REQUEST;


#define AME_REPLY_Message_SIZE                  96
#define AME_SENSE_BUFFER_SIZE                   64


typedef struct _AME_Module_Prepare {
        AME_U16                     DeviceID;
        AME_U32                     Total_Support_command;
        AME_U32                     SG_per_command;

} PACKED AME_Module_Prepare, *pAME_Module_Prepare;


typedef struct _AME_Module_Init_Prepare {
        AME_U32                 Device_BusNumber;
        AME_U16                 Vendor_ID;
        AME_U16                 Device_ID;
        PVOID                   Raid_Base_Address;
        PVOID                   pDeviceExtension;
        AME_U32                 Glo_High_Address;           // for Raid init used only
        AME_U32                 Polling_init_Enable;        // for AME_Init Polling
        AME_U32                 Init_Memory_by_AME_module;  // for AME Data buffer init by self or ame module.
        AME_U64                 AME_Buffer_physicalStartAddress;
        AME_U64                 AME_Buffer_virtualStartAddress; 

} PACKED AME_Module_Init_Prepare, *pAME_Module_Init_Prepare;

typedef struct _AME_Module_Support_Check {
        AME_U32                 Device_Reg0x00;
        AME_U32                 Device_Reg0x08;
        AME_U32                 Device_Reg0x2C;

} PACKED AME_Module_Support_Check, *pAME_Module_Support_Check;


extern      AME_U32 AME_Device_Support_Check(pAME_Module_Support_Check AMEModule_Support_Check);
extern      AME_U32 AME_Init_Prepare(pAME_Data pAMEData,pAME_Module_Prepare pAMEModulePrepare);
extern      AME_U32 AME_ISR(pAME_Data pAMEData);
extern      AME_U32 AME_Timer(pAME_Data pAMEData);
extern      AME_U32 AME_Init(pAME_Data pAMEData,pAME_Module_Init_Prepare pAMEModule_Init_Prepare);
extern      AME_U32 AME_ReInit(pAME_Data pAMEData,pAME_Module_Init_Prepare pAMEModule_Init_Prepare);
extern      AME_U32 AME_Start(pAME_Data pAMEData);
extern      AME_U32 AME_Stop(pAME_Data pAMEData);
extern      AME_U32 AME_Cmd_Mark_Raid_Path_Remove(pAME_Data pAMEData,pAME_Data pAMEData_Path,AME_U32 Path_Raid_ID);
extern      AME_U32 AME_CleanupPathRemoveRequest(pAME_Data pAMEData);
extern      AME_U32 AME_Cmd_Timeout(pAME_Data pAMEData,PVOID pRequestExtension);
extern      AME_U32 AME_Cmd_All_Timeout(pAME_Data pAMEData);
extern      AME_U32 AME_Build_SCSI_Cmd(pAME_Data pAMEData,pAME_Data pAMEData_Path,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command);
extern      AME_U32 AME_NT_Send_SCSI_Cmd(pAME_Data pAMEData,pAME_SCSI_REQUEST pAME_SCSIRequest);
extern      AME_U64 AME_Prepare_SCSI_Cmd(pAME_Data pAMEData,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command);

extern		AME_U32 AME_IS_RW_CMD(AME_U8 scsi_cmd);
extern      AME_U32 AME_NT_Raid_Alive_Check (pAME_Data pAMEData,AME_U32 Raid_ID);
extern      AME_U32 AME_Lun_Alive_Check(pAME_Data pAMEData,AME_U32 Raid_ID,AME_U32 Lun_ID);
extern      AME_U32 AME_NT_Raid_Bar_lost_Check(pAME_Data pAMEData,AME_U32 Raid_ID);
extern      AME_U32 AME_NT_Raid_2208_Check_Bar_State(pAME_Data pAMEData,AME_U32 Raid_ID);
extern      AME_U32 AME_Raid_2208_3108_Bar_lost_Check(pAME_Data pAMEData);
extern      AME_U32 AME_Event_Get_log(pAME_Data pAMEData, AME_U8 *string);
extern      AME_U32 AME_Event_Turn_Switch(pAME_Data pAMEData, AME_U8 SWitchState);
extern      AME_U32 AME_Build_InBand_Cmd(pAME_Data pAMEData, pAME_Module_InBand_Command pAME_ModuleInBand_Command);
extern      AME_U32 AME_Check_is_NT_Mode(pAME_Data pAMEData);
extern      AME_U32 AME_Check_is_DAS_Mode(pAME_Data pAMEData);

extern      AME_U32 AME_NT_Notify_Master_toget_Reply_Fifo(pAME_Data pAMEData);  // Support MPIO
extern      AME_U32 AME_NT_Notify_Master_Raid_path_remove(pAME_Data pAMEData);  // Support MPIO

extern		AME_U32 AME_Reply_Fifo_put(pAME_Data pAMEData,AME_U32 Raid_ID,AME_U32 ReplyMsg); // AME_Module used
extern		AME_U32 AME_NT_Notify_Slave_toget_Req_Fifo(pAME_Data pAMEData); // DoorBell 0x2000

extern      AME_U32 AME_write_Host_Inbound_Queue_Port_0x40(pAME_Data pAMEData,  AME_U32 value);    // Support AME Queue
extern      AME_U32 AME_NT_write_Host_Inbound_Queue_Port_0x40(pAME_Data pAMEData,AME_U32 Raid_ID,AME_U64 Request_Buffer_Phy_addr);  // Support AME Queue

extern      AME_U32 AME_Build_Das_SendVirtual_Cmd (pAME_Data pAMEData,pAME_Data pAMEData_path,AME_U32 Raid_ID);


// Internal AME used
extern      void AME_NT_Soft_FIFO_Init(pAME_Data pAMEData,AME_U32 Raid_ID);
extern      void AME_Reply_Buffer_Init(pAME_Data pAMEData);
extern      AME_U32 AME_HBA_Init(pAME_Data pAMEData);
extern      AME_U32 AME_Raid_Init(pAME_Data pAMEData);
extern      AME_U32 AME_Raid_2208_3108_Link_Error_Check(pAME_Data pAMEData);
extern      AME_U32 AME_write_Host_outbound_Queue_Port_0x44(pAME_Data pAMEData,  AME_U32 value);
extern      void AME_LED_Start(pAME_Data pAMEData);
extern      void AME_LED_ON(pAME_Data pAMEData);
extern      void AME_LED_OFF(pAME_Data pAMEData);
extern      void AME_LED_Stop(pAME_Data pAMEData);
extern      void AME_HBA_Link_Error_check(pAME_Data pAMEData);
extern      AME_U32 AME_NT_87B0_Workaround(pAME_Data pAMEData);
extern      AME_U32 AME_NT_Error_handle_FreeQ(pAME_Data pAMEData, AME_U32 RAID_System_Index);
extern      AME_U32 AME_Event_Put_log(pAME_Data pAMEData, AME_U8 *string);
extern      AME_U32 AME_Check_RAID_Ready_Status(pAME_Data pAMEData);
extern		AME_U32 AME_Get_RAID_Ready(pAME_Data pAMEData);
extern      AME_U64 AMEByteSwap64(AME_U64 x);
extern      AME_U32 AME_NT_Check_Is_Event_Reply(pAME_Data pAMEData,AME_U32 AME_Buffer_PhyAddress_Low,AME_U64 *pNT_Event_Reply_address);
extern      AME_U32 AME_InBand_Loaded_Get_Page_0_callback(pAME_REQUEST_CallBack pAMERequestCallBack);
extern      AME_U32 AME_InBand_Loaded_Get_Lun_Mask_Page_callback(pAME_REQUEST_CallBack pAMERequestCallBack);
extern      AME_U32 AME_Find_Upstream_Bridge(pAME_Data pAMEData,AME_U32 Target_Bus,AME_U32 *pUpstreamt_Bus,AME_U32 *pUpstream_Slot);

#endif //AME_MODULE_H


