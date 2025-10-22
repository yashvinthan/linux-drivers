/************************************************************************
*   ACS62000-08 and ACS_61000_XX Pci RAID card driver for linux
*
*   Copyright (C) 2006 - 2014, Accusys Technology Inc. All rights reserved.
*   E-mail: Sam_Chuang@accusys.com.tw
*   
*   This program is free software; you can redistribute it and/or
*   modify it under the terms of the GNU General Public License
*   as published by the Free Software Foundation; either version 2
*   of the License, or (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
**************************************************************************/

/***************************************************************************
*   Data type definition
****************************************************************************/
//  Sam:  For ame.h ->
#define U8  u8
#define U16 u16
#define U32 u32
#define U64 u64
#define LONG long
// ~Sam:  For ame.h <-

#include "ame.h"
#include "AME_module.h"
#include "AME_import.h"
#include "MPIO.h"
#include "AME_Queue.h"
#include "ACS_MSG.h"
#include "AME_Raid.h"


#include <linux/version.h>

#ifndef KERNEL_VERSION
    #define KERNEL_VERSION(V, P, S) (((V) << 16) + ((P) << 8) + (S))
#endif

#ifndef TRUE
    #define TRUE 0x01
    #define FALSE 0x00
#endif

#ifndef __user
   #define __user
#endif

#ifndef __iomem
   #define __iomem          
#endif

#ifndef DMA_64BIT_MASK
    #define DMA_64BIT_MASK          0xffffffffffffffffULL
#endif

#ifndef DMA_32BIT_MASK
    #define DMA_32BIT_MASK          0x00000000ffffffffULL
#endif

/* All API compatibility macros moved to compat.h */


#define PCI_NT_DEVICE(vend,dev,svend,sdev) \
        .vendor = (vend), .device = (dev), \
        .subvendor = svend, .subdevice = sdev

/***************************************************************************
*   Vendor and Device Support definition
****************************************************************************/

/* Device Support */
#define TESTVID                 0xCAFE
#define TESTDID                 0xBABE
#define PLX_VID		            0x10B5
#define Accusys_VID             0x14D6	// Accusys  Inc.
#define Caldigit_VID            0x1AB6	// Caldigit Inc.

/* Vendor Support */
#define ACCUSYS_SIGNATURE 1
//#define UIT_SIGNATURE 1
//#define YANO_SIGNATURE 1
//#define CALDIGIT_SIGNATURE 1

#if defined (Enable_AME_RAID_R0)
    #define DRIVER_VERSION          "2.3.5 R0/R1 Raid Support, Version Beta 1."
#else
    #define DRIVER_VERSION          "3.2.2"
#endif

#if (defined(ACCUSYS_SIGNATURE) || defined(UIT_SIGNATURE) || defined(YANO_SIGNATURE))
    #define DRIVER_Title            "ACS"
    #define VENDERID                0x14D6  // Accusys Inc.
#endif 

#ifdef CALDIGIT_SIGNATURE
    #define DRIVER_Title            "HDPro"
    #define VENDERID                0x1AB6  // CalDigit Inc.
#endif 

#if (defined(ACCUSYS_SIGNATURE) || defined(UIT_SIGNATURE))
    #define DRIVER_NAME         "acs_ame"
    #define CHAR_DRIVER_NAME    "ACS_CDEV"
#endif

#ifdef YANO_SIGNATURE
    #define DRIVER_NAME         "acs_ame"
    #define CHAR_DRIVER_NAME    "YANO_CDEV"
#endif

#ifdef CALDIGIT_SIGNATURE
    #define DRIVER_NAME         "hdpro_ame"
    #define CHAR_DRIVER_NAME    "CalDigit_CDEV"
#endif


/***************************************************************************
*   Raid config definition
****************************************************************************/
#define MAX_ADAPTER     16      // MAX support Raid device
#define ACS_SUPPORT_CMD 0x1C0   // MAX support Cmd
#define No_Lun_Change   0xFF


struct All_Acs_Adapter 
{
    struct  Acs_Adapter *acs[MAX_ADAPTER];
};

/* Linux Support Maximum IO = 512K  
 * MAX SG-size = 512 / 4 = 128
 * if SG-size give more have no effect.
 */
#define ACS_MAX_SGLIST  128

/* Set MAX_SECTOR_SIZE can change MaxIO
 * If MAX_SECTOR_SIZE set 1024, (1024*512)/1024 = 512 K (Max_io 512K)
 * Set MAX_SECTOR_SIZE 128(Max_io 64K), 256(Max_io 128K), 512(Max_io 256��), 1024(Max_io 512��)
 */ 
#define MAX_SECTOR_SIZE  2*(ACS_MAX_SGLIST*4) // 1024(Max_io 512K)

#define GET_DMA_HI32(dma_addr)  (u32)((u64)dma_addr >> 32 )


/***************************************************************************
*   IOCTL definition
****************************************************************************/
//#define INBANDCMD_IDLE      0
//#define INBANDCMD_USING     1
//#define INBANDCMD_COMPLETE  2
//#define INBANDCMD_FAILED    3

//#define PCI_DMA_BIDIRECTIONAL   0
//#define PCI_DMA_TODEVICE        1
//#define PCI_DMA_FROMDEVICE      2
//#define PCI_DMA_NONE            3

#define INBAND_WRITE_DATA       1
#define INBAND_READ_DATA        2
#define INBAND_NONT_DATA        3 
#define INBAND_EVENTSWITCH_ON   4
#define INBAND_EVENTSWITCH_OFF  5
#define INBAND_GET_EVENT        6
#define INBAND_RESCAN           7
#define INBAND_READ_DATA_2      8   /* Fixed Bug 1768 */
#define INBAND_Test         0x100   /* Fixed Bug 1768 */


#define INBAND_DATA_LENGHT      4
#define INBAND_BUFFER_SIZE      (5*1024)

typedef struct _INBAND_PACKAGE
{
    U8      InBandCDB[INBAND_CMD_LENGHT];  //#define INBAND_CMD_LENGHT 32
    U32     DataLength;
    U32     PAD;
    U8      buffer[INBAND_BUFFER_SIZE];
} inband_package;



/***************************************************************************
*   Other struct definition
****************************************************************************/

//  Sam:  Support Lun change handler ->
typedef struct _Dynamic_luntable
{
    u8      Lun_Change_Flag;
    struct  scsi_device *TargeID_device;
} Dynamic_luntable;
// ~Sam:  Support Lun change handler <-


/***************************************************************************
*   Private Data definition
****************************************************************************/

struct Acs_Adapter 
{
    struct Scsi_Host        *host;
    struct pci_dev          *pdev;
    
    u8                      irq;
    u8                      blIs64bit;
    int                     cdev_major;
    u8                      InbandFlag;
    u16                     InBandErrorCode;
    
    void __iomem            *register_base;                 /* iomapped PCI memory space */
    void __iomem            *register_base_HBA;             /* iomapped PCI memory space */
    void __iomem            *register_base_Raid_Bridge;     /* iomapped PCI memory space */

    void                    *virtual_dma_coherent;
    dma_addr_t              physical_dma_coherent_handle;
    size_t                  dma_mem_All_size;

    spinlock_t              inband_lock;
    wait_queue_head_t       inband_wait_lock;
    spinlock_t              AME_module_lock;
    unsigned long           AME_module_processor_flags;
    spinlock_t              MPIO_lock;
    unsigned long           MPIO_processor_flags;
    spinlock_t              AME_Raid_lock;
    unsigned long           AME_Raid_processor_flags;
    
    //  Sam:    support merge new AME_module ->
    pAME_Data               pAMEData;
    pMSG_DATA		        pMSGData;
    pAME_Raid_Queue_DATA    pAMERaid_Queue_DATA;
    pMPIO_DATA		        pMPIOData;
	pAME_Queue_DATA         pAMEQueueData;
    struct  timer_list      ACS_Timer;
    // ~Sam:    support merge new AME_module <-

    //  Sam:  Support Lun change handler ->
    struct work_struct      Lun_change_work;
    struct work_struct      Lun_change_init_work;
    Dynamic_luntable        dynamic_luntable[MAX_RAID_System_ID][MAX_RAID_LUN_ID];
    // ~Sam:  Support Lun change handler <-
    
    u8                      InBand_Buffer[INBAND_BUFFER_SIZE];
    
};


