// AccuMessage Exchange
// MU/DMA Interface code
// Header file

#ifndef AME_H
#define AME_H

// ****************************************************************************
// Data type definition
// ****************************************************************************
	//typedef signed   char   S8;
	//typedef unsigned char   U8;
	//typedef signed   short  S16;
	//typedef unsigned short  U16;
	//
	//typedef signed   long  S32;
	//typedef unsigned long  U32;
	//
	//typedef struct _S64
	//{
	//    U32          Low;
	//    S32          High;
	//} S64;
	//
	//typedef struct _U64
	//{
	//    U32          Low;
	//    U32          High;
	//} U64;
	//
	//// Pointers
	//typedef S8      *PS8;
	//typedef U8      *PU8;
	//typedef S16     *PS16;
	//typedef U16     *PU16;
	//typedef S32     *PS32;
	//typedef U32     *PU32;
	//typedef S64     *PS64;
	//typedef U64     *PU64;

// ****************************************************************************
// Global Variable Setting
// ****************************************************************************
#define AME_REQUEST_MSG_SIZE				(512)
#define AME_REPLY_MSG_SIZE					(96)
#define AME_SENSE_DATA_SIZE					(64)
#define AME_QUEUE_DEPTH						(512)
#define AME_EVENT_DEPTH						(64)
#define AME_BDL_SIZE						(4800)	//400Entries * 12ElementSize

// ****************************************************************************
// PCI System Interface Registers
// ****************************************************************************
#define AME_HOST_MSG_REGISTER	            (0x00000010)
#define AME_CTRL_RDY_REGISTER	            (0x00000018)

#define AME_HOST_INT_STATUS_REGISTER        (0x00000030)
#define AME_HIS_REPLY_INTERRUPT     		(0x00000008)
#define AME_HIS_CRTLRDY_INTERRUPT     		(0x00000001)

#define AME_HOST_INT_MASK_REGISTER        	(0x00000034)
#define AME_HIM_RIM                         (0x00000008)
#define AME_HIM_CRTLRDY			     		(0x00000001)

#define AME_REQUEST_MSG_PORT	            (0x00000040)
#define AME_REPLY_MSG_PORT		            (0x00000044)

// ****************************************************************************
// Request Message Packet Handle 
// ****************************************************************************
#define AME_REQ_MSG_ADDRESS_MASK  			(0xFFFFFFFC)

#define AME_NORMAL_REPLY_N_BIT              (0x80000000)
#define AME_NORMAL_REPLY_ADDRESS_MASK       (0x7FFFFFFF)

#define AME_FAST_REPLY_N_BIT              	(0x80000000)
#define AME_FAST_REPLY_CONTEXT_MASK      	(0x7FFFFFFF)

// ****************************************************************************
// Message Function Variables
// ****************************************************************************
#define AME_FUNCTION_SCSI_REQUEST       	(0x00)
#define AME_FUNCTION_SCSI_TASK_MGMT     	(0x01)
#define AME_FUNCTION_EVENT_SWITCH       	(0x02)
#define AME_FUNCTION_IN_BAND            	(0x03)
#define AME_FUNCTION_SCSI_PASSTHGH      	(0x04)
#define AME_FUNCTION_ATA_PASSTHGH			(0x05)


// ****************************************************************************
// Buffer Descriptor Lists
// ****************************************************************************

//
// Basic Element Structure
//
typedef struct _BDL_BASIC32
{
    U32                     FlagsLength __attribute__ ((packed));
    U32                     Address __attribute__ ((packed));
} BDLBasic32_t, *pBDLBasic32_t;

typedef struct _BDL_BASIC64
{
    U32                     FlagsLength __attribute__ ((packed));
    U64                     Address __attribute__ ((packed));
} BDLBasic64_t, *pBDLBasic64_t;

typedef struct _BDL_BASIC_UNION
{
    U32                     FlagsLength __attribute__ ((packed));
    union
    {
        U32                 Address32 __attribute__ ((packed));
        U64                 Address64 __attribute__ ((packed));
    } u;
} BDLBasicUnion_t, *pBDLBasicUnion_t;

//
// List Element Structure
// 
typedef struct _BDL_LIST32
{
	//U8						BasicElementCount __attribute__ ((packed));
	//U8						Reserved[2] __attribute__ ((packed));
	//U8						Flags __attribute__ ((packed));
    U32                     FlagsLength __attribute__ ((packed));
    U32                     Address __attribute__ ((packed));
} BDLList32_t, *pBDLList32_t;

typedef struct _BDL_LIST64
{
	//U8						BasicElementCount __attribute__ ((packed));
	//U8						Reserved[2] __attribute__ ((packed));
	//U8						Flags __attribute__ ((packed));
    U32                     FlagsLength __attribute__ ((packed));
    U64                     Address __attribute__ ((packed));
} BDLList64_t, *pBDLList64_t;

typedef struct _BDL_LIST_UNION
{
	//U8						BasicElementCount __attribute__ ((packed));
	//U8						Reserved[2] __attribute__ ((packed));
	//U8						Flags __attribute__ ((packed));
    U32                     FlagsLength __attribute__ ((packed));
    union
    {
        U32                 Address32 __attribute__ ((packed));
        U64                 Address64 __attribute__ ((packed));
    } u;
} BDLListUnion_t, *pBDLListUnion_t;

//
// BDL IO Type Union, for IO BDL's
//
typedef struct _BDL_IO_UNION
{
    union
    {
        BDLBasicUnion_t     Basic; //__attribute__ ((packed));
        BDLListUnion_t      List; //__attribute__ ((packed));
    } u;
} BDLIOUnion_t, *pBDLIOUnion_t;

// ****************************************************************************
//  BDL field definition and masks
// ****************************************************************************

// General
#define AME_BDL_BASIC_ELEMENT              	(0x00)
#define AME_BDL_LIST_ELEMENT            	(0x40)
#define AME_BDL_LIST_DIRECTION            	(0x20)
#define AME_BDL_LIST_ADDRESS_SIZE          	(0x10)

// Direction
#define AME_FLAGS_DIR_DTR_TO_HOST           (0x00)
#define AME_FLAGS_DIR_HOST_TO_DTR           (0x20)

// Address Size 
#define AME_FLAGS_32_BIT_ADDRESSING         (0x00)
#define AME_FLAGS_64_BIT_ADDRESSING         (0x10)


// ****************************************************************************
//  Default Request and Reply Message Structures
// ****************************************************************************

//
// Default Request Header
//
typedef struct _AME_MSG_REQUEST_HEADER
{
    U8                      Reserved[2];
    U8                      FunctionFlags;
    U8                      Function;
    U32                     MsgIdentifier;
} AMEHeader_t, *pAMEHeader_t;

//
// Default Reply Header
//
typedef struct _AME_MSG_DEFAULT_REPLY
{
    U8                     	Flags;
	U8						ReplyStatus;
    U8                      ReplyLength;
    U8                      Function;
    U32                     MsgIdentifier;
} AMEDefaultReply_t, *pAMEDefaultReply_t;


// Flags definition
#define AME_REPLY_FLAGS_CONTINUE_REPLY         (0x80)


// ****************************************************************************
//  ReplyStatus Definition
// ****************************************************************************
#define AME_REPLYSTATUS_SUCCESS                 (0x00)
#define AME_REPLYSTATUS_INVALID_FUNCTION        (0x01)
#define AME_REPLYSTATUS_BUSY	                (0x02)
#define AME_REPLYSTATUS_INVALID_BDL             (0x03)
#define AME_REPLYSTATUS_INTERNAL_ERROR          (0x04)
#define AME_REPLYSTATUS_INSUFFICIENT_RESOURCES  (0x05)
#define AME_REPLYSTATUS_INVALID_FIELD           (0x06)
#define AME_REPLYSTATUS_DATA_OVERRUN	    	(0x23)
#define AME_REPLYSTATUS_DATA_UNDEROVERRUN	    (0x24)

#define AME_REPLYSTATUS_IN_BAND_ILLEGAL_CMD     (0x11)

#define AME_REPLYSTATUS_SCSI_RECOVERED_ERROR    (0x21)
#define AME_REPLYSTATUS_SCSI_DEVICE_NOT_HERE    (0x22)
#define AME_REPLYSTATUS_SCSI_DATA_ERROR    		(0x25)

#define AME_REPLYSTATUS_SCSI_TASK_TERMINATED    (0x27)
#define AME_REPLYSTATUS_SCSI_TASLK_MGMT_FAILED  (0x30)


// ****************************************************************************
//  SCSI Function
// ****************************************************************************
// Request Structure
typedef struct _AME_MSG_SCSI_REQUEST
{
    U8                      Reserved[2];        /* 00h */
    U8                      Lun;                /* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;      /* 04h */
    U8                      CDBLength;          /* 08h */
    U8                      SenseBufferLength;  /* 09h */
    U8                      Reserved2[2];       /* 0Ah */
    U32                     Control;            /* 0Ch */
    U8                      CDB[16];            /* 10h */
    U32                     DataLength;         /* 20h */
    U32                     SenseBufferLowAddr; /* 24h */
    BDLIOUnion_t            BDL;                /* 28h */
} AMESCSIRequest_t, *pAMESCSIRequest_t;

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

// Reply Structure
typedef struct _AME_MSG_SCSI_ERROR_REPLY
{
    U8                     	Flags;				/* 00h */
	U8						ReplyStatus;        /* 01h */
    U8                      ReplyLength;        /* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;      /* 04h */
    U8                      SCSIStatus;         /* 08h */
    U8                      SCSIState;          /* 09h */
    U8                      Reserved[2];        /* 0Ah */
    U32                     TransferCount;      /* 0Ch */
    U32                     SenseCount;         /* 10h */
} AMESCSIErrorReply_t, *pAMESCSIErrorReply_t;

// SCSIStatus definition
#define AME_SCSI_STATUS_SUCCESS                 (0x00)
#define AME_SCSI_STATUS_CHECK_CONDITION         (0x02)
#define AME_SCSI_STATUS_BUSY                    (0x08)
#define AME_SCSI_STATUS_TASK_SET_FULL           (0x28)
//#define AME_SCSI_STATUS_CONDITION_MET           (0x04)
//#define AME_SCSI_STATUS_INTERMEDIATE            (0x10)
//#define AME_SCSI_STATUS_INTERMEDIATE_CONDMET    (0x14)
//#define AME_SCSI_STATUS_RESERVATION_CONFLICT    (0x18)
//#define AME_SCSI_STATUS_COMMAND_TERMINATED      (0x22)
//#define AME_SCSI_STATUS_ACA_ACTIVE              (0x30)

// SCSIState definition
#define AME_SCSI_STATE_AUTOSENSE_VALID          (0x01)
#define AME_SCSI_STATE_AUTOSENSE_FAILED         (0x02)
#define AME_SCSI_STATE_NO_SCSI_STATUS           (0x04)
#define AME_SCSI_STATE_TERMINATED               (0x08)
//#define AME_SCSI_STATE_RESPONSE_INFO_VALID      (0x10)
//#define AME_SCSI_STATE_QUEUE_TAG_REJECTED       (0x20)

// SenseBuffer Format
// NOTE: FCP_RSP data is big-endian. When used on a little-endian system, this
// structure properly orders the bytes.
typedef struct _AME_SCSI_SENSE_BUFFER
{
    U8      Reserved[2];                                /* 00h */
    U8      FcpFlags;                                   /* 02h */
    U8      FcpStatus;                                  /* 03h */
    U32     FcpResid;                                   /* 04h */
    U32     FcpSenseLength;                             /* 08h */
    U32     FcpResponseLength;                          /* 0Ch */
    U8      FcpRespondSenseData[48];     				/* 10h */
} AMESCSISenseBuffer_t, *pAMESCSISenseBuffer_t;

#define AME_SCSI_SENSE_FLAGS_RSP_VALID                 (0x01)
#define AME_SCSI_SENSE_FLAGS_SNS_VALID                 (0x02)
#define AME_SCSI_SENSE_FLAGS_RESID_OVER                (0x04)
#define AME_SCSI_SENSE_FLAGS_RESID_UNDER               (0x08)

// ****************************************************************************
//  SCSI Task Management Function
// ****************************************************************************
// Request Structure
typedef struct _AME_MSG_SCSI_TASK_MGMT
{
	U8						Reserved[2];		/* 00h */	
    U8                      TaskType;           /* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;      /* 04h */
	U8						Lun;				/* 08h */
	U8						Reserved2[3];		/* 09h */
    U32                     TaskMsgIdentifier;	/* 0Ch */
} AMESCSITaskMgmt_t, *pAMESCSITaskMgmt_t;

// TaskType definition
#define AME_SCSITASKMGMT_TASKTYPE_ABORT_TASK            (0x01)
#define AME_SCSITASKMGMT_TASKTYPE_CLEAR_TASK_SET        (0x02)
#define AME_SCSITASKMGMT_TASKTYPE_LOGICAL_UNIT_RESET    (0x03)
#define AME_SCSITASKMGMT_TASKTYPE_TARGET_RESET          (0x04)
//#define AME_SCSITASKMGMT_TASKTYPE_ABRT_TASK_SET         (0x02)
//#define AME_SCSITASKMGMT_TASKTYPE_RESET_BUS             (0x04)

// Reply Structure
/* SCSI Task Management Reply */
typedef struct _AME_MSG_SCSI_TASK_MGMT_REPLY
{
    U8                     	Flags;				/* 00h */
	U8						ReplyStatus;        /* 01h */
    U8                      ReplyLength;        /* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;      /* 04h */
    U8                      TaskType;           /* 08h */
    U8                      Reserved[3];        /* 09h */
    U32                     TerminationCount;   /* 0Ch */
}  AMESCSITaskMgmtReply_t, *pAMESCSITaskMgmtReply_t;

// ****************************************************************************
//  Event Switch Function
// ****************************************************************************
// Request Structure
typedef struct _AME_MSG_EVENT_SWITCH_REQUEST
{
    U8                      Reserved[2];        /* 00h */
    U8                      Switch;             /* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;      /* 04h */
} AMEEventSwitchRequest_t, *pAMEEventSwitchRequest_t;

// Swtich definition
#define AME_EVENT_SWTICH_ON								(0x01)
#define AME_EVENT_SWTICH_OFF							(0x00)
#define AME_EVENT_SWTICH_USING							(0x55)
#define AME_EVENT_SWTICH_COMPLETE					    (0xAA)
#define AME_EVENT_SWTICH_Raid_Error					    (0xFF)

// Reply Structure
typedef struct _AME_MSG_EVNET_SWITCH_REPLY
{
    U8                     	Flags;				/* 00h */
	U8						ReplyStatus;        /* 01h */
    U8                      ReplyLength;        /* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;		/* 04h */	
    U8						EDB[88];            /* 08h */
} AMEEventSwitchReply_t, *pAMEEventSwitchReply_t;

// ****************************************************************************
//  InBand Function
// ****************************************************************************
// Request Structure
typedef struct _AME_MSG_IN_BAND_REQUEST
{
    U8                      Reserved[2];        /* 00h */
    U8						Flags;				/* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;      /* 04h */
    U8						InBandCDB[32];		/* 08h */
	U32						DataLength;			/* 28h */
	U32						Reserved2;			/* 2Ch */
    BDLIOUnion_t            BDL;                /* 30h */
} AMEInBandRequest_t, *pAMEInBandRequest_t;

#define AME_INBAND_FLAGS_DATADIRECTION_MASK    (0x00000003)
#define AME_INBAND_FLAGS_NODATATRANSFER	  	   (0x00000000)
#define AME_INBAND_FLAGS_WRITE   			   (0x00000001)
#define AME_INBAND_FLAGS_READ				   (0x00000002)

// Reply Structure
typedef struct _AME_MSG_IN_BAND_REPLY
{
    U8                     	Flags;				/* 00h */
	U8						ReplyStatus;        /* 01h */
    U8                      ReplyLength;        /* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;		/* 04h */	
    // InBand Error Code v1, JeffChang ->
    U16                     InBandErrCode;		/* 08h */	
    // InBand Error Code v1, JeffChang <-
} AMEInBandReply_t, *pAMEInBandReply_t;

// ****************************************************************************
//  SCSI Passthrough Function
// ****************************************************************************
// Request Structure
typedef struct _AME_MSG_SCSI_PASSTHGH_REQUEST
{
    //U8                    Reserved;        	/* 00h */
	//U8					CDBLength;			/* 01h */
    U8                      CDBLength;        	/* 00h */
	U8						Reserved;			/* 01h */
    U8                      DiskNumber;         /* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;      /* 04h */
    U8                      CDB[16];            /* 08h */
    U32                     Control;         	/* 18h */
    U32                     DataLength;         /* 1Ch */
    BDLIOUnion_t            BDL;                /* 20h */
} AMESCSIPassThghRequest_t, *pAMESCSIPassThghRequest_t;

// Control definition 
#define AME_SCSI_PASSTHGH_CONTROL_DATADIRECTION_MASK   (0x00000003)
#define AME_SCSI_PASSTHGH_CONTROL_NODATATRANSFER       (0x00000000)
#define AME_SCSI_PASSTHGH_CONTROL_WRITE                (0x00000001)
#define AME_SCSI_PASSTHGH_CONTROL_READ                 (0x00000002)

// Reply Structure
typedef struct _AME_MSG_SCSI_PASSTHGH_REPLY
{
    U8                     	Flags;				/* 00h */
	U8						ReplyStatus;        /* 01h */
    U8                      ReplyLength;        /* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;      /* 04h */
    U8                      SCSIState;          /* 08h */
    U8                      Reserved[3];        /* 09h */
    U8                      ASCQ;         		/* 0Ch */
    U8                      ASC;		        /* 0Dh */
    U8                      SenseKey;  		    /* 0Eh */
    U8                      SCSIStatus;         /* 0Fh */
} AMESCSIPassThghReply_t, *pAMESCSIPassThghReply_t;

// SCSIStatus definition
#define AME_SCSI_PASSTHGH_STATUS_SUCCESS                 (0x00)
#define AME_SCSI_PASSTHGH_STATUS_CHECK_CONDITION         (0x02)
#define AME_SCSI_PASSTHGH_STATUS_CONDITION_MET           (0x04)
#define AME_SCSI_PASSTHGH_STATUS_BUSY                    (0x08)
#define AME_SCSI_PASSTHGH_STATUS_INTERMEDIATE            (0x10)
#define AME_SCSI_PASSTHGH_STATUS_INTERMEDIATE_CONDMET    (0x14)
#define AME_SCSI_PASSTHGH_STATUS_RESERVATION_CONFLICT    (0x18)
#define AME_SCSI_PASSTHGH_STATUS_COMMAND_TERMINATED      (0x22)
#define AME_SCSI_PASSTHGH_STATUS_TASK_SET_FULL           (0x28)
#define AME_SCSI_PASSTHGH_STATUS_ACA_ACTIVE              (0x30)

// SCSIState definition
#define AME_SCSI_PASSTHGH_STATE_AUTOSENSE_VALID          (0x01)
#define AME_SCSI_PASSTHGH_STATE_AUTOSENSE_FAILED         (0x02)
#define AME_SCSI_PASSTHGH_STATE_NO_SCSI_STATUS           (0x04)
#define AME_SCSI_PASSTHGH_STATE_TERMINATED               (0x08)
#define AME_SCSI_PASSTHGH_STATE_RESPONSE_INFO_VALID      (0x10)
#define AME_SCSI_PASSTHGH_STATE_QUEUE_TAG_REJECTED       (0x20)

// ****************************************************************************
//  ATA Passthrough Function
// ****************************************************************************
// Request Structure
typedef struct _AME_MSG_ATA_PASSTHGH_REQUEST
{
    U8                      Reserved[2];      	/* 00h */
    U8                      DiskNumber;         /* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;      /* 04h */
    U8                      CommandFIS[20];     /* 08h */
    U32                     Control;         	/* 1Ch */
    U32                     DataLength;         /* 20h */
    BDLIOUnion_t            BDL;                /* 2Ch */
} AMEATAPassThghRequest_t, *pAMEATAPassThghRequest_t;

// Control definition 
#define AME_ATA_PASSTHGH_CONTROL_DATADIRECTION_MASK   (0x00000003)
#define AME_ATA_PASSTHGH_CONTROL_NODATATRANSFER       (0x00000000)
#define AME_ATA_PASSTHGH_CONTROL_WRITE                (0x00000001)
#define AME_ATA_PASSTHGH_CONTROL_READ                 (0x00000002)
#define AME_ATA_PASSTHGH_CONTROL_XFERTYPE_MASK   	  (0x00000004)
#define AME_ATA_PASSTHGH_CONTROL_XFERTYPE_PIO   	  (0x00000000)
#define AME_ATA_PASSTHGH_CONTROL_XFERTYPE_DMA   	  (0x00000004)

// Reply Structure
typedef struct _AME_MSG_ATA_PASSTHGH_REPLY
{
    U8                     	Flags;				/* 00h */
	U8						ReplyStatus;        /* 01h */
    U8                      ReplyLength;        /* 02h */
    U8                      Function;           /* 03h */
    U32                     MsgIdentifier;      /* 04h */
    U8                      StatusFIS[20];      /* 08h */
} AMEATAPassThghReply_t, *pAMEATAPassThghReply_t;




// ****************************************************************************
//  MU and DMA parts
// ****************************************************************************

#define	ROUND64(x)		(x) = (((x) + 0x3F) & 0xFFFFFFC0)
#define	ROUND32(x)		(x) = (((x) + 0x1F) & 0xFFFFFFE0)
#define	ROUND16(x)		(x) = (((x) + 0x0F) & 0xFFFFFFF0)
#define	ROUND8(x)		(x) = (((x) + 0x07) & 0xFFFFFFF8)
#define	CUT16(x)		(x) = ((x) & 0xFFFFFFF0)
#define	CUT256(x)		(x) = ((x) & 0xFFFFFF00)
#define	CUT512(x)		(x) = ((x) & 0xFFFFFE00)
#define	CUT1024(x)		(x) = ((x) & 0xFFFFFC00)
#define	U32V 			volatile LONG

// MU definition ----------------------------------------------------------
#define QUEUEDEPTH	4096		// 4K Entries
#define MMIOSize	4096		// 4K MMIO
#define CQEnable	0x01		// Enable Circular Queue
#define CQ4K		0x02		// 4K Entries setting
#define CQ8K		0x04		// 8K Entries setting
#define CQ16K		0x08		// 16K Entries setting
#define CQ32K		0x10		// 32K Entries setting
#define CQ64K		0x20		// 64K Entries setting
#define MUEmpty		0xFFFFFFFF	// Empty Context
#define CQ4KMask	0x3FFF
#define CQ4KOff		0x4000
#define CQ8KOff		0x8000
#define CQ12KOff	0xC000
// ~MU definition ----------------------------------------------------------

// MU Structure ----------------------------------------------------------
#define QUEUEDEPTH	4096		// 4K Entries
#define CQEnable	0x01		// Enable Circular Queue
#define CQ4K		0x02		// 4K Entries setting
#define CQ8K		0x04		// 8K Entries setting
#define CQ16K		0x08		// 16K Entries setting
#define CQ32K		0x10		// 32K Entries setting
#define CQ64K		0x20		// 64K Entries setting
// ~MU Structure ----------------------------------------------------------


// DMA definition ----------------------------------------------------------
#define MAX_DMA_CHAN	2
#define DMARead         0x0001
#define DMAWrite        0x0002
#define DMAVerify       0x0003
#define DMASuspend      0x0004
#define DMAChainResume  0x0005

// Define State for Queue
#define NO_ERROR_REASON		0
#define ALLOC_BUF_ERROR_REASON	1
#define DMA_ERROR_REASON	2
#define FLUSH_ERROR_REASON	3

#define	ALLOC_BUF_STATE		0
#define	XFER_DATA_STATE		1
#define	FLUSH_DATA_STATE	2
#define	REPLY_STATE		3
#define	SCSI_REPLY_STATE	4
#define	FLUSH_WAIT_STATE	5
// ~Define State for Queue
// ~DMA definition ----------------------------------------------------------


// DMA Structure ----------------------------------------------------------
typedef struct _DMA_CHAIN_DESCRIPTOR
{
	LONG	NextDescriptorAddress;  /* NDA  */
	LONG	LowerPCIAddress;        /* PAD  */
	LONG	UpperPCIAddress;        /* UPAD */
	LONG	LocalAddress;           /* LAD  */
	LONG	ByteCounts;             /* BC   */
	LONG	DescriptorControl;      /* DC   */
	LONG	CrcAddress;				/* CAD: locate in memory, not register */
	LONG	State;					/* static link pointer to NDA */
} DMA_Chain_Descriptor, *PDMA_Chain_Descriptor;

#define DMADescriptorSize	(32)					//32Bytes
#define MAX_BDE_PER_CMD		(1024)					// Max descriptors allowed in one command
#define DMADescriptorNo		(AME_QUEUE_DEPTH*MAX_BDE_PER_CMD)	// 512(request)*256(descriptors)

typedef struct  _DMA_REGISTER
{
	volatile LONG	CCR;		/* Channel Control Register */
	volatile LONG	CSR;		/* Channel Status Register */
	volatile LONG	Reserved;	/* Not used */
	volatile LONG	DAR;		/* Descriptor Address Register */
	volatile LONG	NDAR;		/* Next Descriptor Address Regoister */
	volatile LONG	PADR;		/* PCI Address Register */
	volatile LONG	PUADR;		/* PCI Upper Address Register */
	volatile LONG	LADR;		/* Local Address Register */
	volatile LONG	BCR;		/* Byte Count Register */
	volatile LONG	DCR;		/* Descriptor Control Register */
} DMA_Register, *PDMA_Register;

// CCR definition
#define ChainResume				0x00000002
#define ChannelEnable			0x00000001

// CSR definition
#define ChannelActive			0x00000400
#define EndofTransferInterrupt	0x00000200
#define EndofChainInterrupt		0x00000100
//#define MemoryFaultError		0x00000040
#define BusFaultError			0x00000020
#define MasterAbort				0x00000008
#define TargetAbort				0x00000004
#define PCIXSplitError			0x00000002

// DCR definition
#define CRCTransferComplete		0x80000000
#define CRCSeedDisable			0x00000200
#define CRCGenerateEnable		0x00000100
#define MemoryToMemoryEnable	0x00000040
#define DualAddressCycleEnable	0x00000020
#define InterruptEnable			0x00000010

#define CRCADDR					0
#define DefaultDMAControl		0
#define PCIXCmdMRB				0x0000000E
#define PCIXCmdMRB1				0x0000000C
#define PCIXCmdMRB2				0x00000006
#define PCIXCmdMWB				0x0000000F
#define PCIXCmdMWB1				0x00000007

#define MemoryRead				0x00000006
#define MemoryWrite				0x00000007
#define ConfigurationRead		0x0000000A
#define ConfigurationWrite		0x0000000B
#define MemoryReadMultiple		0x0000000C
#define MemoryReadLine			0x0000000E
#define MemoryWriteInvalidate	0x0000000F

// ~DMA Structure ----------------------------------------------------------

// SCSI definition ----------------------------------------------------------

// SCSI CDB operation codes
#define SCSIOP_TEST_UNIT_READY     0x00
#define SCSIOP_REZERO_UNIT         0x01
#define SCSIOP_REWIND              0x01
#define SCSIOP_REQUEST_BLOCK_ADDR  0x02
#define SCSIOP_REQUEST_SENSE       0x03
#define SCSIOP_FORMAT_UNIT         0x04
#define SCSIOP_READ_BLOCK_LIMITS   0x05
#define SCSIOP_REASSIGN_BLOCKS     0x07
#define SCSIOP_READ6               0x08
#define SCSIOP_RECEIVE             0x08
#define SCSIOP_WRITE6              0x0A
#define SCSIOP_PRINT               0x0A
#define SCSIOP_SEND                0x0A
#define SCSIOP_SEEK6               0x0B
#define SCSIOP_TRACK_SELECT        0x0B
#define SCSIOP_SLEW_PRINT          0x0B
#define SCSIOP_SEEK_BLOCK          0x0C
#define SCSIOP_PARTITION           0x0D
#define SCSIOP_READ_REVERSE        0x0F
#define SCSIOP_WRITE_FILEMARKS     0x10
#define SCSIOP_FLUSH_BUFFER        0x10
#define SCSIOP_SPACE               0x11
#define SCSIOP_INQUIRY             0x12
#define SCSIOP_VERIFY6             0x13
#define SCSIOP_RECOVER_BUF_DATA    0x14
#define SCSIOP_MODE_SELECT6        0x15
#define SCSIOP_RESERVE_UNIT        0x16
#define SCSIOP_RELEASE_UNIT        0x17
#define SCSIOP_COPY                0x18
#define SCSIOP_ERASE               0x19
#define SCSIOP_MODE_SENSE6         0x1A
#define SCSIOP_START_STOP_UNIT     0x1B
#define SCSIOP_STOP_PRINT          0x1B
#define SCSIOP_LOAD_UNLOAD         0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC  0x1C
#define SCSIOP_SEND_DIAGNOSTIC     0x1D
#define SCSIOP_MEDIUM_REMOVAL      0x1E
#define SCSIOP_READ_CAPACITY       0x25
#define SCSIOP_READ                0x28
#define SCSIOP_WRITE               0x2A
#define SCSIOP_SEEK                0x2B
#define SCSIOP_LOCATE              0x2B
#define SCSIOP_WRITE_VERIFY        0x2E
#define SCSIOP_VERIFY              0x2F
#define SCSIOP_SEARCH_DATA_HIGH    0x30
#define SCSIOP_SEARCH_DATA_EQUAL   0x31
#define SCSIOP_SEARCH_DATA_LOW     0x32
#define SCSIOP_SET_LIMITS          0x33
#define SCSIOP_READ_POSITION       0x34
#define SCSIOP_SYNCHRONIZE_CACHE   0x35
#define SCSIOP_COMPARE             0x39
#define SCSIOP_COPY_COMPARE        0x3A
#define SCSIOP_WRITE_DATA_BUFF     0x3B
#define SCSIOP_READ_DATA_BUFF      0x3C
#define SCSIOP_CHANGE_DEFINITION   0x40
#define SCSIOP_READ_SUB_CHANNEL    0x42
#define SCSIOP_READ_TOC            0x43
#define SCSIOP_READ_HEADER         0x44
#define SCSIOP_PLAY_AUDIO          0x45
#define SCSIOP_PLAY_AUDIO_MSF      0x47
#define SCSIOP_PLAY_TRACK_INDEX    0x48
#define SCSIOP_PLAY_TRACK_RELATIVE 0x49
#define SCSIOP_PAUSE_RESUME        0x4B
#define SCSIOP_LOG_SELECT          0x4C
#define SCSIOP_LOG_SENSE           0x4D
#define SCSIOP_MODE_SELECT10       0x55     // Modified by Morris
#define SCSIOP_MODE_SENSE10        0x5A
#define SCSIOP_REPORT_LUNS         0xA0     // Added by Morris.
#define SCSIOP_LOAD_UNLOAD_SLOT    0xA6
#define SCSIOP_MECHANISM_STATUS    0xBD
#define SCSIOP_READ_CD             0xBE
#define SCSIOP_ACS_FIBRE_TEST_CMD   0x26    // Added by Morris
#define SCSIOP_ACS_ALIVE_CMD        0x27    // ythuang: for tramsmit alive msg
#define SCSIOP_ACS_CTRL_CMD     0xDD
#define SCSIOP_ACS_IDE_CMD      0xDE
#define SCSIOP_ACS_EMU_CMD      0xDF
#define SCSIOP_RED              0xC0        //Redundant CDB operation code

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
#define SCSIOP_DENON_EJECT_DISC    0xE6
#define SCSIOP_DENON_STOP_AUDIO    0xE7
#define SCSIOP_DENON_PLAY_AUDIO    0xE8
#define SCSIOP_DENON_READ_TOC      0xE9
#define SCSIOP_DENON_READ_SUBCODE  0xEB

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

// pBuf->seqType
#define FCP_DATA        1       // FCP_CMND R/W CDB need to transfer data and no status
#define FCP_DATA_RSP    2       // FCP_CMND Read CDB data, last transfer and auto status
#define FCP_RSP         3       // FCP_CMND nonRead CDB need to transfer and auto status

// pBuf->entryType
#define NONRW_CMD       1           // Non R/W CDB
#define RW_CMD          2           // R/W CDB
#define ERR_CMD         3           // CDB with error sense

// FCP_RSP IU payload definition, FCP pp.46
#define FCP_RSP_LEN_VALID       0x0100
#define FCP_SNS_LEN_VALID       0x0200
#define FCP_RESID_OVER          0x0400
#define FCP_RESID_UNDER         0x0800

typedef struct _CMD_TRACKER
{
	LONG				HostRequestAddr;		// Address fo Request in Host from MUIPQ
	LONG				HostListAddr;			// Address of List in Host from MUIPQ
	LONG				RequestIndex;			// Index of Request Frame in local memory
	LONG				RequestFrame;			// Address of Request Frame in Local

	LONG				ListFrame;				// Address of List Frame in Local
	LONG				SingleChainBufPtr;		// Single DMA Descriptor
	LONG				MultiChainBufPtr;		// Multiple DMA Descriptors for RW Command
	LONG				SGLCount;				// Number of SGL

	LONG				BDLIndex;				// Index to Current DMA wanted BDE
	LONG				BDLOffset;				// Offset in bytes from start of Current DMA wanted BDE
	LONG				XferLen;				// Already transferred DATA length
	LONG				XferCount;				// SCSIRequest TransferCount

    LONG                Handle;                 // points to pBuf struct
	LONG				EventData;				// Address of Event Data
	LONG				UseMultipleChain;		// Indicate which Chain Buffer to use 0:RaidNo   1:FlagsExt
	LONG				InBandBuf;				// Address of InBand Data buffer

	LONG				FastReplyContext;		// ScsiRequest->MsgIdentifier, for Fast Reply use
	LONG				ReplyStatus;			// The AME_REPLY_STATUS_XXX to return host
	LONG				HostReplyAddr;			// Address of Reply in Host from MUOPQ
	LONG				ReplyFrame;				// Address of Reply Frame in local memory

    LONG                scsiStatus;             // Current SCSI status
    U32                 transferLength;         // Current transfer length 
    U32                 dataBuffer;             // Current NonRW buffer location 
	LONG				DeltaTime;				// Performance measurment variable


    U8                 	responseLength;         // Current Respones length (for FCP_RSP, error report)
    U8					Reserved;
    U8                  Direction;              // Direction: 0:ReceiveData Buffer,  1:SendData Buffer
    U8                  Residue;                // Residue: 0:No Residue  1:Residuer Under, 2:Residue Over

    U32                 residueLength;          // Residue Length

	U32					scsi_proc;				// Address of SCSI_PROC
    AMESCSISenseBuffer_t senseData;          	// Current Sense data (for FCP_RSP, error report)
} CMD_TRACKER, *pCMD_TRACKER;


//#define ANEXT_STATE_SCSI(IOCmd) NEXT_STATE_SCSI(IOCmd)

#define ANEXT_STATE_SCSI(IOCmd) RUN_NEXT_STATE(IOCmd)

/*
#define ANEXT_STATE_SCSI(IOCmd)\
{\
	if ( (DMA_Active[0]==FALSE) || (DMA_Active[1]==FALSE) )\
	{\
		RUN_NEXT_STATE(IOCmd);\
	}\
	else\
	{\
		NEXT_STATE_SCSI(IOCmd);\
	}\
}
*/

#define ClearCmdTracker(pCmdTracker)\
{\
    pCmdTracker->Direction = 0;\
    pCmdTracker->Residue = 0;\
    pCmdTracker->residueLength = 0;\
    pCmdTracker->BDLIndex = 0;\
    pCmdTracker->BDLOffset = 0;\
    pCmdTracker->XferLen = 0;\
}
//Ian
//typedef struct _ResIndex
//{
//	LONG 				Index;
//	LONG				InUsed;
//	IOCmdStruct 		*IOCmd;
//	struct BUF			*pBuf;
//	struct _ResIndex 	*Next;
//} ResIndex;

#define AllocRequestIndex(ResIndex)\
{\
	/*DISABLE_IRQ();*/\
	AllocRequest++;\
	if (RequestHead==NULL) PANIC("Request FULL");\
	ResIndex = RequestHead;\
	ResIndex->InUsed = TRUE;\
	RequestHead = RequestHead->Next;\
	/*ENABLE_IRQ();*/\
}


#define ReleaseRequestIndex(ResIndex) \
{\
	DISABLE_IRQ();\
	AllocRequest--;\
	if (AllocRequest<0) \
	{\
		ENABLE_IRQ();\
		PANIC("AllocRequest Error");\
	}\
	ResIndex->InUsed = FALSE;\
	ResIndex->Next = NULL;\
	RequestTail->Next = ResIndex;\
	RequestTail = ResIndex;\
	ENABLE_IRQ();\
}

#define ReleaseEventIndex(ResIndex) \
{\
	DISABLE_IRQ();\
	ResIndex->Next = NULL;\
	EventTail->Next = ResIndex;\
	EventTail = ResIndex;\
	ENABLE_IRQ();\
}


#define DCmdTracker(pCmdTracker) 
#define Dbp(pBuf) 
#define DallCmd() 
#define DChainPtr(ChainPtr) 
#define DC(IOCmd) 
#define dbgmsg(arg) 
#define AMEMSG(arg) 
#define DEBUG80(arg) 

/*
#define DEBUG80(arg) OUT80(arg)

#define dbgmsg(arg)\
{\
	if (AMEDebugMsg) printf arg ;\
}

#define AMEMSG(arg) \
{\
	if (AMEDebugMsg) printf arg ;\
}

#define DCmdTracker(pCmdTracker)\
{\
	if (AMEDebugMsg>2)\
	{\
		printf("HostRequestAddr	=%X\n",(int)(pCmdTracker->HostRequestAddr));\
		printf("EventData=%X\n",(int)(pCmdTracker->EventData));\
		printf("HostReplyAddr=%X\n",(int)(pCmdTracker->HostReplyAddr));\
		printf("RequestFrame=%X\n",(int)(pCmdTracker->RequestFrame));\
		printf("ListFrame=%X\n",(int)(pCmdTracker->ListFrame));\
		printf("SGLCount=%X\n",(int)(pCmdTracker->SGLCount));\
		printf("SingleChainBufPtr=%X\n",(int)(pCmdTracker->SingleChainBufPtr));\
		printf("MultiChainBufPtr=%X\n",(int)(pCmdTracker->MultiChainBufPtr));\
		printf("ReplyStatus=%X\n",(int)(pCmdTracker->ReplyStatus));\
		printf("Handle=%X\n",(int)(pCmdTracker->Handle));\
		printf("UseMultipleChain=%X\n",(int)(pCmdTracker->UseMultipleChain));\
		printf("FastReplyContext=%X\n",(int)(pCmdTracker->FastReplyContext));\
		printf("XferCount=%X\n",(int)(pCmdTracker->XferCount));\
		printf("InBandBuf=%X\n",(int)(pCmdTracker->InBandBuf));\
		printf("RequestIndex=%X\n",(int)(pCmdTracker->RequestIndex));\
		printf("ReplyFrame=%X\n",(int)(pCmdTracker->ReplyFrame));\
		printf("BDLIndex=%X\n",(int)(pCmdTracker->BDLIndex));\
		printf("BDLOffset=%X\n",(int)(pCmdTracker->BDLOffset));\
		printf("XferLen=%X\n",(int)(pCmdTracker->XferLen));\
	}\
}


#define Dbp(pBuf)\
{\
	if(AMEDebugMsg>3)\
	{\
		printf("ToScsiProc=%X\n",(int)(pBuf->ToScsiProc));\
		printf("Status=%X\n",(int)(pBuf->Status));\
		printf("CmdCode=%X\n",(int)(pBuf->CmdCode));\
		printf("RaidID =%X\n",(int)(pBuf->RaidID));\
		printf("Flag=%X\n",(int)(pBuf->Flag));\
		printf("Lba=%X\n",(int)(pBuf->Lba));\
		printf("Len=%X\n",(int)(pBuf->Len));\
		printf("CurLba=%X\n",(int)(pBuf->CurLba));\
		printf("CurLen=%X\n",(int)(pBuf->CurLen));\
		printf("ScsiResideLen=%X\n",(int)(pBuf->ScsiResideLen));\
		printf("RaidResideLen=%X\n",(int)(pBuf->RaidResideLen));\
		printf("ReadBufID=%X\n",(int)(pBuf->ReadBufID));\
		printf("OffBlk=%X\n",(int)(pBuf->OffBlk));\
		printf("BufHD=%X\n",(int)(pBuf->BufHD));\
		printf("BufCnt=%X\n",(int)(pBuf->BufCnt));\
		printf("seqType=%X\n",(int)(pBuf->seqType));\
		printf("entryType=%X\n",(int)(pBuf->entryType));\
		printf("pDevExt=%X\n",(int)(pBuf->pDevExt));\
		printf("relativeOffset=%X\n",(int)(pBuf->relativeOffset));\
		printf("pCmdTracker=%X\n",(int)(pBuf->pCmdTracker));\
	}\
}

#define DallCmd()\
{\
	if (AMEDebugMsg)\
	{\
		LONG iii;\
		DISABLE_IRQ();\
		for (iii=0;iii<512;iii++)\
		{\
			if (RequestResource[iii].IOCmd!=NULL) printf("RegisterIOCmd=%X\n",(int)(RequestResource[iii].IOCmd));\
		}\
		ENABLE_IRQ();\
	}\
}

#define DChainPtr(ChainPtr)\
{\
	if (AMEDebugMsg>1)\
	{\
		printf("NdaLpciUpciLocalBc %X %X %X %X %X\n",(int)(ChainPtr->NextDescriptorAddress),(int)(ChainPtr->LowerPCIAddress),(int)(ChainPtr->UpperPCIAddress),(int)(ChainPtr->LocalAddress),(int)(ChainPtr->ByteCounts));\
	}\
}


#define DC(IOCmd)\
{\
	if (AMEDebugMsg>1)\
	{\
		printf("CtDqMq=%X %X %X\n",(int)(IOCmd->CmdTracker),(int)(IOCmd->DmaQptr),(int)(IOCmd->MuQptr));	\
	}\
}

*/

#endif	//#ifndef AME_H
