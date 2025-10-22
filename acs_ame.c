/**********************************************************************
*   ACS62000-08 Pci RAID card driver for linux
*
*   Copyright (C) 2006 - 2014 , Accusys Technology Inc. All rights reserved.
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
************************************************************************/
    
#include <linux/version.h>
#include <linux/kernel.h>   
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/slab.h>


#include <linux/pci_ids.h>
#include <linux/moduleparam.h>
#include <linux/blkdev.h>

#include <asm/io.h>         //ioremap
#include <linux/errno.h>    //for error 
#include <linux/fs.h>       //file_operations
#include <asm/uaccess.h>    //User-space access
#include <linux/delay.h>
#include <linux/workqueue.h>

// Use unsigned long as a pointer 
// cause of size(unsigend long) always equal to pointer size under all linux compatible platform
#include <linux/completion.h>   //init_completion  wait for a long time
#include <linux/spinlock.h> 

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
    #include <linux/aer.h>
    #include <linux/pci_regs.h>
#endif  

#include <scsi/scsi.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_tcq.h>
#include <scsi/scsi_device.h>
#include <scsi/scsicam.h>
#include <scsi/scsi_transport.h>
#include "modern_kernel_compat.h"

#include "acs_ame.h"

/* Debug used */
//#define ACS_DEBUG
#if defined (ACS_DEBUG)
    #define DPRINTK(fmt, args...) \
            printk(KERN_ALERT DRIVER_NAME "  %s:  " fmt, __func__, ## args)
#else
    #define DPRINTK(fmt, args...)
#endif

#define PRINTK(fmt, args...) \
        printk(KERN_ALERT DRIVER_NAME "  " fmt, ## args)

/* Global variable */
static  struct  All_Acs_Adapter gAcs_Adapter;
static  u8  gAcs_Adapter_Counter = 0 ;
static  struct class *ACS_class;

static void __iomem *RemapPCIMem(u32 base, u32 size);
static int acs_ame_get_free_inband (struct Acs_Adapter *acs_adt);
static int acs_ame_Queue_free_inband (struct Acs_Adapter *acs_adt);

/*******************************************************************
*  Support AME_module and AME_import
*******************************************************************/
void AME_Address_Write_32(pAME_Data pAMEData,PVOID Base_Address, U32 offset, U32 value)
{
    struct Acs_Adapter *acs_adt;
    acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    writel(value, Base_Address + offset);
    return;
}

AME_U32 AME_Address_read_32(pAME_Data pAMEData,PVOID Base_Address, U32 offset)
{
    return readl((void __iomem *)Base_Address + offset);
}

AME_U16 AME_Address_read_16(pAME_Data pAMEData,PVOID Base_Address, U32 offset)
{
    return readw((void __iomem *)Base_Address + offset);
}

void AME_spinlock(pAME_Data pAMEData)
{
    struct Acs_Adapter  *acs_adt = 0;
    acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    spin_lock_irqsave(&acs_adt->AME_module_lock,acs_adt->AME_module_processor_flags);
    return;
}

void AME_spinunlock(pAME_Data pAMEData)
{
    struct Acs_Adapter  *acs_adt = 0;
    acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    spin_unlock_irqrestore(&acs_adt->AME_module_lock,acs_adt->AME_module_processor_flags);
    return;
}

void MPIO_spinlock(pMPIO_DATA pMPIODATA)
{
    pAME_Data pAMEData = pMPIODATA->pAMEData;
    struct Acs_Adapter  *acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    spin_lock(&acs_adt->MPIO_lock);
    //spin_lock_irqsave(&acs_adt->MPIO_lock,acs_adt->MPIO_processor_flags);
    return;
}

void MPIO_spinunlock(pMPIO_DATA pMPIODATA)
{
    pAME_Data pAMEData = pMPIODATA->pAMEData;
    struct Acs_Adapter  *acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    spin_unlock(&acs_adt->MPIO_lock);
    //spin_unlock_irqrestore(&acs_adt->MPIO_lock,acs_adt->MPIO_processor_flags);
    return;
}

void AME_Raid_spinlock(pAME_Raid_Queue_DATA pAMERaid_QueueDATA)
{
    struct Acs_Adapter  *acs_adt = (struct Acs_Adapter *)pAMERaid_QueueDATA->pDeviceExtension;
    spin_lock(&acs_adt->AME_Raid_lock);
    //spin_lock_irqsave(&acs_adt->AME_Raid_lock,acs_adt->AME_Raid_processor_flags);
    return;
}

void AME_Raid_spinunlock(pAME_Raid_Queue_DATA pAMERaid_QueueDATA)
{
    struct Acs_Adapter  *acs_adt = (struct Acs_Adapter *)pAMERaid_QueueDATA->pDeviceExtension;
    spin_unlock(&acs_adt->AME_Raid_lock);
    //spin_unlock_irqrestore(&acs_adt->AME_Raid_lock,acs_adt->AME_Raid_processor_flags);
    return;
}

void AME_IO_milliseconds_Delay(U32 Delay)
{
    mdelay(Delay);
    return;
}

void AME_IO_microseconds_Delay(U32 Delay)
{
    udelay(Delay);
    return;
}

AME_U32 AME_Raid_PCI_Config_Read(pAME_Data pAMEData,AME_U32 offset)
{
	AME_U32 Temp_Reg = 0;
	struct Acs_Adapter *acs_adt;
    acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    pci_read_config_dword(acs_adt->pdev,offset,&Temp_Reg);
	return Temp_Reg;
}

void AME_Raid_PCI_Config_Write(pAME_Data pAMEData,AME_U32 offset,AME_U32 value)
{
	struct Acs_Adapter *acs_adt;
    acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    pci_write_config_dword(acs_adt->pdev,offset,value);
	return;
}	

PVOID AME_IOMapping_Bridge(pAME_Data pAMEData,AME_U32 bus,AME_U32 slot,AME_U32 func)
{
	u32  Bar0 = 0,Bar_Size = 0;
    struct pci_dev  *plx_Bridge = NULL;
    void __iomem    *register_base_Bridge;
    unsigned int    devfn = (unsigned int)((slot<<3)|func);
    AME_U16  DeviceID = pAMEData->Device_ID;
	 
	/* get Raid Bridge pci device */
    if ( (plx_Bridge = acs_pci_get_bus((unsigned int)bus,devfn)) == NULL ) {
        PRINTK("In %s Error:  Can't get bridge device.\n",__func__);
        return NULL;
    } 
	
	/* Sam Debug Message */
    //printk("Sam Debug:  DIDVID = 0x%x, Dev Bus = 0x%x, Pri Bus = 0x%x, Sec Bus = 0x%x, Sub Bus = 0x%x, slot = 0x%x, func = 0x%x\n",
    //                    (U32)(((U32)(plx_Bridge->device) <<16) | (U32)(plx_Bridge->vendor)),
    //                    (U32)(plx_Bridge->bus->number),
    //                    (U32)(plx_Bridge->bus->primary),
    //                    (U32)(plx_Bridge->bus->secondary),
    //                    (U32)(plx_Bridge->bus->subordinate),
    //                    (U32)(plx_Bridge->devfn>>3),
    //                    (U32)(plx_Bridge->devfn&0x7));
    
    /* get Raid Bridge Bar0 and Bar size */
    Bar0 = pci_resource_start(plx_Bridge, 0);
    Bar_Size = pci_resource_len(plx_Bridge, 0);
    
    if (Bar0 == (dma_addr_t)NULL) {
        PRINTK("In %s Error:  Can't get PLX(%X) bar0 address\n",__func__,plx_Bridge->device);
        return NULL;
    }
    
    register_base_Bridge = (void __iomem *)RemapPCIMem(Bar0, Bar_Size);
    if (register_base_Bridge == NULL) {
        PRINTK("In %s Error:  Can't remap PLX%X bar0\n",__func__,plx_Bridge->device);
        return NULL;
    }
	
    return register_base_Bridge;
}

AME_U32 AME_PCI_Config_Read_32(pAME_Data pAMEData,AME_U32 bus,AME_U32 slot,AME_U32 func,AME_U32 offset)
{
	AME_U32 Temp_Reg = 0;
	struct pci_dev  *plx_Bridge = NULL;
    unsigned int    devfn = (unsigned int)((slot<<3)|func);
	
	/* get Raid Bridge pci device */
    plx_Bridge = acs_pci_get_bus((unsigned int)bus,devfn);
    
    if (plx_Bridge == NULL) {
        //PRINTK("In %s Error:  Get Bridge Base address error\n",__func__);
        return FALSE;
    }
    
	pci_read_config_dword(plx_Bridge,offset,&Temp_Reg);
    return Temp_Reg;
}

AME_U32 AME_PCI_Config_Write_32(pAME_Data pAMEData,AME_U32 bus,AME_U32 slot,AME_U32 func,AME_U32 offset,AME_U32 value)
{
	AME_U32 Temp_Reg = 0;
	struct pci_dev  *plx_Bridge = NULL;
    unsigned int    devfn = (unsigned int)((slot<<3)|func);
	
	/* get Raid Bridge pci device */
    plx_Bridge = acs_pci_get_bus((unsigned int)bus,devfn);
    
    if (plx_Bridge == NULL) {
        //PRINTK("In %s Error:  Get Bridge Base address error\n",__func__);
        return FALSE;
    }
    
	pci_write_config_dword(plx_Bridge,offset,value);
	
    return Temp_Reg;
}

void AME_Memory_zero(PVOID Src, AME_U32 size)
{
	memset(Src, 0x00, size);
	return;
}

void AME_Memory_copy(PVOID Dest,PVOID Src, AME_U32 size)
{
	memcpy(Dest,Src,size);
	return;
}

AME_U32 AME_Memory_Compare(PVOID Source1,PVOID Source2, AME_U32 Length)
{
    if (0x00 == memcmp(Source1,Source2,Length)) {
        return TRUE;
    }
    
    return FALSE;
 }

/*******************************************************************
* Function:  Get_Upper_pci_device
*******************************************************************/
struct pci_dev *
Get_Upper_pci_device(struct pci_dev *pdev,U32 up_index)
{
    int i;
    
    for (i=0;i<up_index;i++) {
        if (pdev->bus->self) {
            pdev = pdev->bus->self;
        } else {
            pdev = NULL;
            break;
        }
    }
    
    return pdev;
}

/*******************************************************************
* Function :  SHOW_BUFFER_INFO
********************************************************************/
//static void SHOW_BUFFER_INFO(u8 *Buffer)
//{
//    int i;
//    
//    for (i=0;i<512;i=i+16) {
//        printk("SHOW_BUFFER_INFO [%02x] = %02x %02x %02x %02x | %02x %02x %02x %02x | %02x %02x %02x %02x | %02x %02x %02x %02x\n",
//               (i/16),Buffer[i],Buffer[i+1],Buffer[i+2],Buffer[i+3],
//               Buffer[i+4],Buffer[i+5],Buffer[i+6],Buffer[i+7],
//               Buffer[i+8],Buffer[i+9],Buffer[i+10],Buffer[i+11],
//               Buffer[i+12],Buffer[i+13],Buffer[i+15],Buffer[i+15]);
//    }
//    
//    return;
//}

/*******************************************************************
* Function:  Lun_change_handler_init
*******************************************************************/
static void Lun_change_handler_init(struct work_struct *work)
{
    int i,j,RAID_System_ID;
    struct Acs_Adapter  *acs_adt = NULL;
    struct scsi_device  *TargeID_device;
    pAME_Data pAMEData;
    
    if (NULL == (acs_adt = container_of(work, struct Acs_Adapter, Lun_change_init_work))) {
        PRINTK("In %s Error:  Find no acs_adt\n",__func__);
        return;
    }
    
    pAMEData = acs_adt->pAMEData;
    
    #if !defined (Enable_AME_RAID_R0)
        /* Scan all NT Lun */
        scsi_scan_host(acs_adt->host);
    
        /* sleep 2 sec to wait scsi_scan_host ready */
        ssleep(2);
    #else
        PRINTK("Sam Debug:  Stop scsi scan host. SN(%d)\n",pAMEData->AME_Serial_Number);
    #endif
    
    if (TRUE == AME_Check_is_NT_Mode(acs_adt->pAMEData)) {
        RAID_System_ID = MAX_RAID_System_ID;
    } else {
        RAID_System_ID = 1;
    }
    
    for (i=0; i<RAID_System_ID; i++)
    {
        for (j=0; j<MAX_RAID_LUN_ID; j++)
        {
            TargeID_device = NULL;
            TargeID_device = scsi_device_lookup(acs_adt->host, i, j+1, 0);
            
            if (TargeID_device)
            {
                acs_put_device(TargeID_device);
                
                /*
                 * Workaround:  sometimes scsi_device_lookup find LUN ,
                 * but this LUN not exist when this case sector_size be zero.
                 */
                
                //printk("Sam Debug:  ID = [%d][%d] sector_size = 0x%x \n",i,j,TargeID_device->sector_size);
                if (TargeID_device->sector_size == 0) {
                    acs_adt->dynamic_luntable[i][j].TargeID_device = NULL;
                } else {
                    acs_adt->dynamic_luntable[i][j].TargeID_device = TargeID_device;
                }
            }
            
            acs_adt->dynamic_luntable[i][j].Lun_Change_Flag = No_Lun_Change;
            
        }
    }
    
}

/*******************************************************************
* Function:  Lun_change_handler
*******************************************************************/
static void Lun_change_handler(struct work_struct *work)
{
    int i,j,RAID_System_ID,retry_index;
    struct Acs_Adapter  *acs_adt = NULL;
    struct scsi_device  *TargeID_device;
    pAME_Data pAMEData;
    
    if (NULL == (acs_adt = container_of(work, struct Acs_Adapter, Lun_change_work))) {
        PRINTK("In %s Error:  Find no acs_adt\n",__func__);
        return;
    }
    
    pAMEData = acs_adt->pAMEData;

    if (TRUE == AME_Check_is_NT_Mode(acs_adt->pAMEData)) {
        RAID_System_ID = MAX_RAID_System_ID;
    } else {
        RAID_System_ID = 1;
    }
    
    for (i=0; i<RAID_System_ID; i++)
    {
        for (j=0; j<MAX_RAID_LUN_ID; j++)
        {
            //TargeID_device = acs_adt->dynamic_luntable[i][j].TargeID_device;
            
            if (NULL != (TargeID_device = scsi_device_lookup(acs_adt->host, i, j+1, 0))) {
                acs_put_device(TargeID_device);
            }
            
            switch (acs_adt->dynamic_luntable[i][j].Lun_Change_Flag)
            {
                case Lun_Add:
                    //printk("Sam Debug:  Add Raid %d Lun ID %d [%p]:[%p]\n",i,j,TargeID_device,acs_adt->dynamic_luntable[i][j].TargeID_device);
                    
                    // Linux issue, Raid default setting no disk, add disk by gui , must call remove TargeID_device first and add again.
                    if ((TargeID_device != NULL) && (TargeID_device->sector_size == 0)) {
                        scsi_remove_device(TargeID_device);
                        TargeID_device = NULL;
                    }
                    
                    if (TargeID_device == NULL)
                    {
                        retry_index = 0;
                        
                        while (1)
                        {
                            TargeID_device = acs_add_device(acs_adt->host,i,j+1,0);
                            
                            if ((TargeID_device != ERR_PTR(-ENODEV)) &&
                                (TargeID_device != ERR_PTR(-ENOMEM))) {
                                acs_put_device(TargeID_device);
                                break;
                            }
                            
                            if (++retry_index>2) {
                                PRINTK("ACS Error:  scsi_add_device Raid %d LUN %d failed\n",i,j);
                                TargeID_device = NULL;
                                break;
                            }
                            
                            ssleep(1);
                            
                        }
                        
                    }
                    
                    acs_adt->dynamic_luntable[i][j].Lun_Change_Flag = No_Lun_Change;
                    acs_adt->dynamic_luntable[i][j].TargeID_device = TargeID_device;
                    break;
                    
                case Lun_Remove:
                    //printk("Sam Debug:  Remove Raid %d Lun ID %d [%p]:[%p]\n",i,j,TargeID_device,acs_adt->dynamic_luntable[i][j].TargeID_device);
                    if (TargeID_device) {
                        scsi_remove_device(TargeID_device);
                        TargeID_device = NULL;
                    }
                    
                    acs_adt->dynamic_luntable[i][j].Lun_Change_Flag = No_Lun_Change;
                    acs_adt->dynamic_luntable[i][j].TargeID_device = TargeID_device;
                    break;
                    
                case No_Lun_Change:
                default:
                    break;
            }
            
        }
    }
    
    return;
}
    
/******************************************************************
*  Function : RemapPCIMem 
*  base : physical addr
*  return : virtual addr
*******************************************************************/
static void __iomem *RemapPCIMem(u32 base, u32 size)
{
    u32 page_base,page_offs;
    void __iomem *page_remapped = NULL;

    page_base = ((u32)base) & PAGE_MASK;
    page_offs = ((u32)base) - page_base;
    page_remapped = ioremap(page_base, page_offs+size);

    return page_remapped;
}


/*******************************************************************
*  ACS_Timer_Func
*******************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
/* Modern timer API (kernel 4.15+) - callback takes struct timer_list * */
static void ACS_Timer_Func(struct timer_list *t)
{
    struct Acs_Adapter  *acs_adt = from_timer(acs_adt, t, ACS_Timer);

    if (acs_adt == NULL) {
         PRINTK("In %s Error:  acs_adt is null\n",__func__);
         return;
    }

    /* Call MPIO_Timer */
    MPIO_Timer(acs_adt->pMPIOData);

    /* Call AME_Timer */
    AME_Timer(acs_adt->pAMEData);

    // recall timer
    mod_timer(&acs_adt->ACS_Timer, jiffies + 1*HZ); // 1 sec

    return;
}
#else
/* Legacy timer API (kernel < 4.15) - callback takes unsigned long */
static void ACS_Timer_Func(unsigned long ACS_index)
{
    struct Acs_Adapter  *acs_adt = gAcs_Adapter.acs[ACS_index];

    if (acs_adt == NULL) {
         PRINTK("In %s Error:  ACS index %d acs_adt is null\n",__func__,(u32)ACS_index);
         return;
    }

    /* Call MPIO_Timer */
    MPIO_Timer(acs_adt->pMPIOData);

    /* Call AME_Timer */
    AME_Timer(acs_adt->pAMEData);

    // recall timer
    acs_adt->ACS_Timer.function = (void *)ACS_Timer_Func;
    acs_adt->ACS_Timer.data     = ACS_index;
    acs_adt->ACS_Timer.expires  = jiffies + 1*HZ; // 1 sec
    add_timer(&acs_adt->ACS_Timer);

    return;
}
#endif


/******************************************************************
*  Function : acs_ame_pci_unmap_dma 
*  Return   : Release mapping dma memory
*******************************************************************/
static void acs_ame_pci_unmap_dma(struct Acs_Adapter *acs_adt, struct scsi_cmnd *pCmd)
{
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
        scsi_dma_unmap(pCmd);
        return;
    #else       
        if (pCmd->use_sg) {
            struct scatterlist *sl;
            sl = (struct scatterlist *)pCmd->request_buffer;
            pci_unmap_sg(acs_adt->pdev, sl, pCmd->use_sg, pCmd->sc_data_direction);
        } else if (pCmd->request_bufflen) {
            pci_unmap_single(acs_adt->pdev, pCmd->SCp.dma_handle, pCmd->request_bufflen, pCmd->sc_data_direction);
        }
        return;
    #endif
}

/*******************************************************************
*  AME_Host_Error_handler_SCSI_REQUEST  
*******************************************************************/
AME_U32 AME_Host_Error_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAMEREQUESTCallBack)
{
    //pAME_Data pAMEData = pAMEREQUESTCallBack->pAMEData;
    //struct Acs_Adapter *acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    struct scsi_cmnd   *pCmd = (struct scsi_cmnd *)(pAMEREQUESTCallBack->pRequestExtension);
    
    if (pCmd == NULL) {
        PRINTK("In %s Error:  SCSI_CMD is null\n",__func__);
        return FALSE;
    }
    
    /* Return Scsi Cmd done */
#if 1
    pCmd->sense_buffer[0]  = 0x70;  // RESPONSE CODE
    pCmd->sense_buffer[2]  = 0x06;  // Sense Key
    pCmd->sense_buffer[7]  = 0x00;  // Additional sense bytes
    pCmd->sense_buffer[12] = 0x29;  // ADDITIONAL SENSE CODE
    pCmd->sense_buffer[13] = 0x03;  // ADDITIONAL SENSE CODE QUALIFIER
    pCmd->result = DID_OK << 16 | COMMAND_COMPLETE << 8 | SAM_STAT_CHECK_CONDITION;
#else
    pCmd->result = (DID_OK << 16) | (BUSY << 1);
#endif

    acs_scsi_done(pCmd);
    
    return TRUE;
}

/*******************************************************************
* Function:  AME_Host_Normal_handler_SCSI_REQUEST
********************************************************************/
AME_U32 AME_Host_Normal_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAMEREQUESTCallBack)
{
    pAME_Data pAMEData = pAMEREQUESTCallBack->pAMEData;
    struct Acs_Adapter *acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    struct scsi_cmnd   *pCmd = (struct scsi_cmnd *)(pAMEREQUESTCallBack->pRequestExtension);
    pAMESCSIErrorReply_t    pAMESCSIErrorReply  = (pAMESCSIErrorReply_t) pAMEREQUESTCallBack->pReplyFrame;
    pAMESCSISenseBuffer_t   pAMESCSISenseBuffer = (pAMESCSISenseBuffer_t) pAMEREQUESTCallBack->pSCSISenseBuffer;
    
    if (pCmd == NULL) {
        PRINTK("In %s Error:  SCSI_CMD is null\n",__func__);
        return FALSE;
    }
    
    
    /* Check SCSI Reply Status */
    switch (pAMESCSIErrorReply->ReplyStatus)
	{	
		case AME_REPLYSTATUS_SUCCESS:
			
			switch (pAMESCSIErrorReply->SCSIStatus)
			{
				case AME_SCSI_STATUS_SUCCESS:
				    pCmd->result = host_byte(DID_OK);
				    //PRINTK("AME_SCSI_STATUS_SUCCESS\n");
					break;
				case AME_SCSI_STATUS_CHECK_CONDITION:
				    pCmd->result = DID_OK << 16 | COMMAND_COMPLETE << 8 | SAM_STAT_CHECK_CONDITION;
				    PRINTK("AME_SCSI_STATUS_CHECK_CONDITION\n");
					break;
				case AME_SCSI_STATUS_BUSY:
				    pCmd->result = (DID_OK << 16) | (BUSY << 1);
				    PRINTK("SCSI ERROR:  AME_SCSI_STATUS_BUSY\n");
					break;
				case AME_SCSI_STATUS_TASK_SET_FULL:
				    pCmd->result = (DID_BAD_TARGET<<16);
				    PRINTK("SCSI ERROR:  AME_SCSI_STATUS_TASK_SET_FULL\n");
					break;
			    default:
			        pCmd->result = (DID_BAD_TARGET<<16);
			        PRINTK("SCSI ERROR:  Unknow AME SCSI STATUS %x\n",pAMESCSIErrorReply->SCSIStatus);
			        break;
			}
			break;
			 
		case AME_REPLYSTATUS_SCSI_RECOVERED_ERROR:
			pCmd->result = DID_OK << 16 | COMMAND_COMPLETE << 8 | SAM_STAT_CHECK_CONDITION;
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_SCSI_RECOVERED_ERROR \n");
			break;
		case AME_REPLYSTATUS_DATA_UNDEROVERRUN:
			pCmd->result = DID_OK << 16 | COMMAND_COMPLETE << 8 | SAM_STAT_CHECK_CONDITION;
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_DATA_UNDEROVERRUN \n");
			break;
		case AME_REPLYSTATUS_DATA_OVERRUN:
			pCmd->result = DID_OK << 16 | COMMAND_COMPLETE << 8 | SAM_STAT_CHECK_CONDITION;
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_DATA_OVERRUN\n");
			break;
		case AME_REPLYSTATUS_SCSI_DEVICE_NOT_HERE:
			pCmd->result = (DID_NO_CONNECT<<16);
			//PRINTK("SCSI ERROR:  AME_REPLYSTATUS_SCSI_DEVICE_NOT_HERE ID = %d \n",pCmd->device->id);
			break;
		case AME_REPLYSTATUS_BUSY:
			pCmd->result = (DID_OK << 16) | (BUSY << 1);
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_BUSY\n"); 			
			break;
		case AME_REPLYSTATUS_INVALID_FUNCTION :
			pCmd->result = (DID_BAD_TARGET<<16);
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_INVALID_FUNCTION\n"); 			
		    break;
		case AME_REPLYSTATUS_INSUFFICIENT_RESOURCES:
		    pCmd->result = (DID_BAD_TARGET<<16);
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_INSUFFICIENT_RESOURCES\n"); 			
		    break;
		case AME_REPLYSTATUS_INVALID_BDL:
		    pCmd->result = (DID_BAD_TARGET<<16);
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_INVALID_BDL\n"); 			
		    break;
		case AME_REPLYSTATUS_INTERNAL_ERROR:
		    pCmd->result = (DID_BAD_TARGET<<16);
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_INTERNAL_ERROR\n"); 			
		    break;
		case AME_REPLYSTATUS_SCSI_TASK_TERMINATED:
		    pCmd->result = (DID_BAD_TARGET<<16);
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_SCSI_TASK_TERMINATED\n"); 			
		    break;
		case AME_REPLYSTATUS_SCSI_TASLK_MGMT_FAILED:
		    pCmd->result = (DID_BAD_TARGET<<16);
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_SCSI_TASLK_MGMT_FAILED\n");
		    break;
		case AME_REPLYSTATUS_INVALID_FIELD:
		    pCmd->result = (DID_BAD_TARGET<<16);
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_INVALID_FIELD\n");
		    break;
		case AME_REPLYSTATUS_SCSI_DATA_ERROR:
		    pCmd->result = (DID_BAD_TARGET<<16);
			PRINTK("SCSI ERROR:  AME_REPLYSTATUS_SCSI_DATA_ERROR\n"); 			
		    break;
		default:
			pCmd->result = (DID_BAD_TARGET<<16);
			PRINTK("SCSI ERROR:  UnKnow AME Normal SCIS Reply status = %x\n",pAMESCSIErrorReply->SCSIStatus );
			break;
	}
	
	/* Copy Sense data */
	if (pAMESCSIErrorReply->SCSIState & AME_SCSI_STATE_AUTOSENSE_VALID) {
        
        if (pAMESCSISenseBuffer->FcpFlags & AME_SCSI_SENSE_FLAGS_RESID_UNDER) {
            //PRINTK("DATA UNDER RUN %d\n", pAMESCSISenseBuffer->FcpResid);
            #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
                scsi_set_resid(pCmd, pAMESCSISenseBuffer->FcpResid);
            #else 
                pCmd->resid= pAMESCSISenseBuffer->FcpResid;
            #endif 
        }
        
        if (pAMESCSISenseBuffer->FcpFlags & AME_SCSI_SENSE_FLAGS_RESID_OVER) {
            PRINTK("Error:  DATA UNDEROVER %d\n", pAMESCSISenseBuffer->FcpResid);
        }
        
        if (pAMESCSISenseBuffer->FcpFlags & AME_SCSI_SENSE_FLAGS_SNS_VALID) {
            
            if (pAMESCSISenseBuffer->FcpFlags & AME_SCSI_SENSE_FLAGS_RSP_VALID) {
                PRINTK("sense buffer with response buffer,FcpResponseLength = %d,FcpSenseLength = %d\n",pAMESCSISenseBuffer->FcpResponseLength,pAMESCSISenseBuffer->FcpSenseLength);
                memcpy(pCmd->sense_buffer,
                       &(pAMESCSISenseBuffer->FcpRespondSenseData[pAMESCSISenseBuffer->FcpResponseLength]),
                       pAMESCSISenseBuffer->FcpSenseLength );
            } else {
                //PRINTK("sense buffer only\n");
                memcpy(pCmd->sense_buffer,
                       pAMESCSISenseBuffer->FcpRespondSenseData,
                       pAMESCSISenseBuffer->FcpSenseLength );
            }
        }
        
    }
    
    /* Return Scsi Cmd done */
    acs_scsi_done(pCmd);
    
    /* Unmap Scsi Cmd DMA */
    acs_ame_pci_unmap_dma(acs_adt, pCmd);
    
    return TRUE;
}

/*******************************************************************
* Function:  AME_Host_Normal_handler_IN_Band
********************************************************************/
AME_U32 AME_Host_Normal_handler_IN_Band (pAME_REQUEST_CallBack pAMEREQUESTCallBack)
{
    pAME_Data pAMEData = pAMEREQUESTCallBack->pAMEData;
    struct Acs_Adapter *acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    
    if (acs_adt->InbandFlag != INBANDCMD_USING) {
        PRINTK("In %s Error:  Two InBand request Running\n",__func__);
        return FALSE;
    }
    
    acs_adt->InbandFlag = pAMEData->AMEInBandBufferData.InBandFlag;
    acs_adt->InBandErrorCode = pAMEData->AMEInBandBufferData.InBandErrorCode;
    
    /* Inband wake up */
    wake_up_interruptible(&acs_adt->inband_wait_lock);
        
    return TRUE;
}


/*******************************************************************
* Function:  AME_Host_Normal_handler_IN_Band
********************************************************************/
void AME_Host_RAID_Ready(pAME_Data pAMEData)
{
	struct Acs_Adapter *acs_adt;
    acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    
	/* AME module Start Function */
	MPIO_Start(acs_adt->pMPIOData);

	return;
}

/*******************************************************************
* Function:  AME_Host_Scan_All_Lun
* RAID_ID:  Zero base, from 0 ~ 31
* Lun_ID:   Zero base, from 0 ~ 63
* State: TRUE, means lun Add, FALSE, means lun remove
********************************************************************/
AME_U32 AME_Host_Scan_All_Lun (pAME_Data pAMEData,AME_U32 RAID_ID)
{
    U32 Lun_ID;
    pMPIO_DATA 	pPath_MPIODATA  = (pMPIO_DATA)pAMEData->pMPIOData;
    struct Acs_Adapter *acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    
    //printk("Sam Debug: in %s\n",__func__);
    
    if (AME_Check_is_NT_Mode(pAMEData) == TRUE)
    {
        if (FALSE == MPIO_NT_Raid_Alive_Check(pPath_MPIODATA,RAID_ID)) {
            return FALSE;
        }
		
		for (Lun_ID = 0; Lun_ID < MAX_RAID_LUN_ID; Lun_ID++) {
		    if (TRUE == MPIO_Lun_Alive_Check(pPath_MPIODATA,RAID_ID,Lun_ID)) {
                acs_adt->dynamic_luntable[RAID_ID][Lun_ID].Lun_Change_Flag = Lun_Add;
		    } else {
		        acs_adt->dynamic_luntable[RAID_ID][Lun_ID].Lun_Change_Flag = Lun_Remove;
		    }
		}
	
	} else {
		
		#if defined (Enable_AME_RAID_R0)
            /* Call system function to rescan lun */
            PRINTK("Das Software Raid Support, start scan lun.\n");
            
            for (Lun_ID = 0; Lun_ID < MAX_RAID_LUN_ID; Lun_ID++) {
		        if (TRUE == MPIO_Lun_Alive_Check(pPath_MPIODATA,RAID_ID,Lun_ID)) {
                    acs_adt->dynamic_luntable[RAID_ID][Lun_ID].Lun_Change_Flag = Lun_Add;
		        } else {
		            acs_adt->dynamic_luntable[RAID_ID][Lun_ID].Lun_Change_Flag = Lun_Remove;
		        }
		    }
        #else
            PRINTK("Why DAS call to here ???\n");
            return TRUE;
        #endif
	}
	
	/* Schedule Work to Lun_change_handler */
    schedule_work(&acs_adt->Lun_change_work);
    
    return TRUE;
}

/*******************************************************************
* Function:  AME_Host_Lun_Change
* RAID_ID:  Zero base, from 0 ~ 31
* Lun_ID:   Zero base, from 0 ~ 63
* State: TRUE, means lun Add, FALSE, means lun remove
********************************************************************/
AME_U32 AME_Host_Lun_Change (pAME_Data pAMEData,AME_U32 RAID_ID,AME_U32 lun_ID,AME_U32 State)
{
    struct Acs_Adapter *acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    
    //printk("Sam Debug:  in %s  Raid ID = %x , Lun ID = %x, state = %x\n",__func__,RAID_ID,lun_ID,State);
    
    /* alert Lun change */
    acs_adt->dynamic_luntable[RAID_ID][lun_ID].Lun_Change_Flag = State;
    
    /* Schedule Work to Lun_change_handler */
    schedule_work(&acs_adt->Lun_change_work);
    
    return TRUE;
}

/*******************************************************************
* Function : AME_Host_Fast_handler_SCSI_REQUEST
********************************************************************/
AME_U32 AME_Host_Fast_handler_SCSI_REQUEST (pAME_REQUEST_CallBack pAMEREQUESTCallBack)
{
    pAME_Data pAMEData = pAMEREQUESTCallBack->pAMEData;
    struct Acs_Adapter *acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    struct scsi_cmnd   *pCmd = (struct scsi_cmnd *)(pAMEREQUESTCallBack->pRequestExtension);
    
    if (pCmd == NULL) {
        PRINTK("In %s Error:  SCSI_CMD is null\n",__func__);
        return FALSE;
    }
    
    
    /* Unmap Scsi Cmd DMA */
    acs_ame_pci_unmap_dma(acs_adt, pCmd);
    
    /* Return Scsi Cmd done */
    pCmd->result = host_byte(DID_OK);
    acs_scsi_done(pCmd);
    
    return TRUE;
}

/*******************************************************************
* Function : acs_ame_interrupt
********************************************************************/
static irqreturn_t acs_ame_interrupt(int irq,void *dev_id,struct pt_regs *regs)
{
    struct Acs_Adapter  *acs_adt;
    U32                 IRQ_Handle_status;
    //unsigned long       processor_flags;
    
    acs_adt = (struct Acs_Adapter  *)dev_id;
  
    //spin_lock_irqsave(acs_adt->host->host_lock, processor_flags);
    
    IRQ_Handle_status = AME_ISR(acs_adt->pAMEData);
        
    //spin_unlock_irqrestore(acs_adt->host->host_lock, processor_flags);
    
    if (IRQ_Handle_status == TRUE) {
        return IRQ_HANDLED;
    } else {
        return IRQ_NONE;
    }
    
}

/*******************************************************************
*  Function : acs_ame_get_free_inband
********************************************************************/
static int acs_ame_get_free_inband (struct Acs_Adapter *acs_adt)
{
    /* Only support one Inband cmd at same time */
    spin_lock(&acs_adt->inband_lock);
    
    if (acs_adt->InbandFlag != INBANDCMD_IDLE) {
        spin_unlock(&acs_adt->inband_lock);
        PRINTK("IOCTL Error:  Inband CMD is using!\n");
        return FALSE;
    }
    
    acs_adt->InbandFlag = INBANDCMD_USING;
    acs_adt->InBandErrorCode = 0xFFFF;
    
    spin_unlock(&acs_adt->inband_lock);
    return TRUE;

}

/*******************************************************************
*  Function : acs_ame_Queue_free_inband
********************************************************************/
static int acs_ame_Queue_free_inband (struct Acs_Adapter *acs_adt)
{
    spin_lock(&acs_adt->inband_lock);
    acs_adt->InbandFlag = INBANDCMD_IDLE;
    spin_unlock(&acs_adt->inband_lock);
    return TRUE;
}


/*******************************************************************
* Function : acs_ame_get_event
********************************************************************/
static int acs_ame_get_event(struct Acs_Adapter *acs_adt, void *userarg)
{
    u8 Event_log[AME_EVENT_STRING_LENGTH];
    
    memset(&Event_log, 0x00, AME_EVENT_STRING_LENGTH);
    
    if (AME_Event_Get_log(acs_adt->pAMEData, (AME_U8 *)(&Event_log)) == FALSE) {
		//PRINTK("In %s Error:  AME_Event_Get_log Error\n",__func__);
		return FALSE;
	}
	
	if (copy_to_user((void *)userarg,
                     &Event_log,
                     AME_EVENT_STRING_LENGTH) != 0) {
        PRINTK("In %s Error:  Copy Read Buffer Fail\n",__func__);
        return FALSE;
    }
    
    return TRUE;
}

/*******************************************************************
* Function : acs_ame_build_inband_cmd
* Return value:  0x00 is successed
*                0x01 is Raid return error code
*                -EINVAL is send cmd to Raid failed
********************************************************************/
static int acs_ame_build_inband_cmd(struct Acs_Adapter *acs_adt, void *userarg,u32 InBand_Type)
{
    AME_Module_InBand_Command AME_ModuleInBand_Command;
    inband_package *InBand_Package = (inband_package *)&(acs_adt->InBand_Buffer);
    
    memset(InBand_Package, 0x00, INBAND_BUFFER_SIZE);
    
    if ( copy_from_user(&(InBand_Package->InBandCDB),
                        &(((inband_package *)userarg)->InBandCDB),
                        INBAND_CMD_LENGHT) != 0) {
        PRINTK("In %s Error:  Copy CDB Error\n",__func__);
        return -EINVAL;
    }
    
    if ( copy_from_user(&(InBand_Package->DataLength),
                        &(((inband_package *)userarg)->DataLength),
                        INBAND_DATA_LENGHT) != 0 ) {
        PRINTK("In %s Error:  Copy Data Length Error\n",__func__);
        return -EINVAL;
    }
    
    if (InBand_Type == INBAND_WRITE_DATA) {
        if ( copy_from_user(&(InBand_Package->buffer),
                            &(((inband_package *)userarg)->buffer), 
                            InBand_Package->DataLength) != 0 ) {
            PRINTK("In %s Error:  Copy Write Buffer Error\n",__func__);
            return -EINVAL;
        }
    }
    
	AME_ModuleInBand_Command.InBand_CDB = &(InBand_Package->InBandCDB);
	AME_ModuleInBand_Command.InBand_Buffer_addr = &(InBand_Package->buffer);
	AME_ModuleInBand_Command.pRequestExtension = userarg;
	
    //printk("Sam Debug:  Inband CDB [%x] [%x] [%x]\n",*((u8 *)userarg),*((u8 *)userarg+1),*((u8 *)userarg+2));
    
    if ( !MPIO_Build_InBand_Cmd(acs_adt->pMPIOData,&AME_ModuleInBand_Command) ) {
        PRINTK("In %s Error:  AME_Build_InBand_Cmd Fail\n",__func__);
        return -EINVAL;
    }
    
    /* Wait Raid return Inband interrupt */
    wait_event_interruptible(acs_adt->inband_wait_lock,((acs_adt->InbandFlag==INBANDCMD_COMPLETE)||(acs_adt->InbandFlag==INBANDCMD_FAILED)));
    
    switch (acs_adt->InbandFlag)
    {
        case INBANDCMD_COMPLETE:
            if ((InBand_Type == INBAND_READ_DATA)) {
                if (copy_to_user(&(((inband_package *)userarg)->buffer),
                                 &(InBand_Package->buffer),
                                 InBand_Package->DataLength) != 0) {
                    PRINTK("In %s Error:  Copy Read Buffer Fail\n",__func__);
                    return -EINVAL;
                }
            }
            
            //PRINTK("INBANDCMD_COMPLETE\n");
            return 0;
            
        case INBANDCMD_FAILED:
            // copy error code to GUI, error code size 2 byte
            if (copy_to_user(&(((inband_package *)userarg)->buffer),
                             &(InBand_Package->buffer),
                             2) != 0) {
                PRINTK("In %s Error:  Copy Read Buffer Fail\n",__func__);
                return -EINVAL;
            }
            
            PRINTK("In %s Error:  INBANDCMD_FAILED\n",__func__);
            return 1;
        
        default:
            //if (AME_Cmd_Timeout(acs_adt->pAMEData,userarg) == FALSE) {
		    //    PRINTK("In %s Error: abort InBand cmd not found\n",__func__);
	        //}
            //PRINTK("In %s Error:  INBANDCMD_TIMEOUT\n",__func__);
            PRINTK("In %s Error:  INBANDCMD_UnKnow_Flag = %x\n",__func__,acs_adt->InbandFlag);
            return -EINVAL;
    }
    
}

/*******************************************************************
*  Function : acs_ame_fops_ioctl
*  @inode - Our device inode
*  @filep - acs_adt
*  @cmd - ioctl command
*  @arg - user buffer
********************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
    static long acs_ame_fops_ioctl(struct file *filep, unsigned int ioctl_cmd, unsigned long arg)
#else
static int acs_ame_fops_ioctl(struct inode *inode, struct file *filep, unsigned int ioctl_cmd, unsigned long arg)
#endif
{
    int                 ret = 0;
    struct Acs_Adapter  *acs_adt;
                    
    acs_adt = (struct Acs_Adapter *)filep->private_data;
    
    if (acs_adt == NULL) {
        ret= -ENOTTY;
        PRINTK("IOCTL Error:  acs_adt is null\n");
        goto exit;
    }
    
    if (TRUE == AME_Check_is_NT_Mode(acs_adt->pAMEData)) {
        ret= -ENOTTY;
        PRINTK("IOCTL Error:  Not support NT mode.\n");
        goto exit;
    }
    
    if (acs_ame_get_free_inband(acs_adt) == FALSE) {
        ret = -EACCES;
        goto exit;
    }
        
    switch (ioctl_cmd)
    {
        case INBAND_NONT_DATA:      /* non data xfer */
            ret = acs_ame_build_inband_cmd(acs_adt,(void *)arg,INBAND_NONT_DATA);
            break;
            
        case INBAND_READ_DATA:      /* read from device */
        case INBAND_READ_DATA_2:    /* Fixed Bug 1768 */
            ret = acs_ame_build_inband_cmd(acs_adt,(void *)arg,INBAND_READ_DATA);
            //SHOW_BUFFER_INFO((u8 *)&(((inband_package *)arg)->buffer));
            break;
            
        case INBAND_WRITE_DATA:     /* write to device */
            ret = acs_ame_build_inband_cmd(acs_adt,(void *)arg,INBAND_WRITE_DATA);
            break;
            
        case INBAND_EVENTSWITCH_ON:
            if (AME_Event_Turn_Switch(acs_adt->pAMEData,AME_EVENT_SWTICH_ON) == FALSE) {
		        ret = -EINVAL;
		    }
            break;
    
        case INBAND_EVENTSWITCH_OFF:
            if (AME_Event_Turn_Switch(acs_adt->pAMEData,AME_EVENT_SWTICH_OFF) == FALSE) {
                ret = -EINVAL;
            }
            break;
    
        case INBAND_GET_EVENT:
            if (acs_ame_get_event(acs_adt,(void *)arg) == FALSE) {
                ret = -EINVAL;
            }
            break;
            
        case INBAND_RESCAN:
            //scsi_remove_host(acs_adt->host);
            //scsi_scan_host(acs_adt->host);
            //scsi_rescan_device(&(acs_adt->pdev->dev)); 
            //ret = 0;
            //break;
        
        case INBAND_Test: /* Fixed Bug 1768 */
            /* do nothing */
            ret = EINVAL;
            break;
    
        default:
            PRINTK("IOCTL Error:  not support this ioctl cmd = %x",ioctl_cmd);
            ret = EINVAL;
            break;
    }

    acs_ame_Queue_free_inband(acs_adt);
    
exit:
    return ret;
}

/*******************************************************************
*  Function : acs_ame_fops_open
********************************************************************/
static int acs_ame_fops_open(struct inode *inode, struct file *filep)
{
    int                 i,major;
    struct Acs_Adapter  *acs_adt;
                    
    acs_adt = NULL;
    major = MAJOR(inode->i_rdev);
    //printk("Sam DBG:  inode = 0x%x , inode->i_rdev = 0x%x ,major = 0x%d\n",inode,inode->i_rdev,major);
    
    for (i=0; i<MAX_ADAPTER; i++) {
        if (gAcs_Adapter.acs[i] && (gAcs_Adapter.acs[i]->cdev_major==major)) {
            acs_adt = gAcs_Adapter.acs[i];
            break;
        }
    }
    
    filep->private_data = acs_adt;
    return 0;       
}

/*******************************************************************
*  Function : acs_ame_fops_close
********************************************************************/
static int acs_ame_fops_close(struct inode *inode, struct file *filep)
{
    return 0;
}

/*****************************************************
*      Define CHAR Device template
******************************************************/ 
static struct file_operations acs_ame_fos = 
{
    .owner      = THIS_MODULE,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
    .unlocked_ioctl = acs_ame_fops_ioctl,
#else
    .ioctl      = acs_ame_fops_ioctl,
#endif  
    .open       = acs_ame_fops_open,
    .release    = acs_ame_fops_close
};

/*******************************************************************
*  Function : acs_ame_abort
********************************************************************/
int acs_ame_abort(struct scsi_cmnd *cmd)
{
    struct Scsi_Host    *host;
    struct Acs_Adapter  *acs_adt;

    host = cmd->device->host;
    acs_adt = (struct Acs_Adapter *)host->hostdata;
    
    if (MPIO_Cmd_Timeout(acs_adt->pMPIOData,(void *)cmd) == FALSE) {
                PRINTK("In %s Error: abort cmd %p not found\n",__func__,cmd);
                return FALSE;
        }
    
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
        cmd->result = (DID_OK << 16) | (QUEUE_FULL << 1);
        acs_scsi_done(cmd);
    #endif
    
    return SUCCESS;
}


/*******************************************************************
*  Function : acs_ame_build_sg
********************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
static u32 acs_ame_build_sg(struct Acs_Adapter *acs_adt,struct scsi_cmnd *cmd,pAME_Host_SG pHostSG)
{
    int nseg;
    U64 Addr_64;
    U32 i,sg_unit_size;
    U32 SGCount = 0;
    struct scatterlist *sl;
    
    nseg = scsi_dma_map(cmd);
    
    if ( (nseg > ACS_MAX_SGLIST) || (nseg < 0) ) {
        PRINTK("In %s error:  nseg(%d) error.\n",__func__,nseg);
        return AME_U32_NONE;
    }
    
    scsi_for_each_sg(cmd, sl, nseg, i)
    {
        sg_unit_size = sg_dma_len(sl);
        Addr_64 = sg_dma_address(sl);
        pHostSG->Address = Addr_64;
        pHostSG->BufferLength = sg_unit_size;
        pHostSG++;
        SGCount++;
    }
    
    return SGCount;
    
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
static u32 acs_ame_build_sg(struct Acs_Adapter *acs_adt,struct scsi_cmnd *cmd,pAME_Host_SG pHostSG)
{
    /* Support AME Module */
    U64 Addr_64;
    U32 SGCount = 0;
        
    if (! ((cmd->use_sg)||(cmd->request_bufflen)) ) {
        PRINTK("In %s error:  SCSI CDB cmd[0] = 0x%x , is TEST_UNIT_READY ???\n",__func__,cmd->cmnd[0]);
        return AME_U32_NONE;
    }
    
    if (cmd->use_sg) {
        
        int sg_counter;
        u32 i,sg_unit_size;
        struct scatterlist *sl;
        
        sl = (struct scatterlist *)cmd->request_buffer;
        sg_counter = pci_map_sg(acs_adt->pdev, sl, cmd->use_sg, cmd->sc_data_direction);
        
        for (i=0; i<sg_counter; i++) {
            sg_unit_size = sg_dma_len(sl);
            Addr_64 = sg_dma_address(sl);
            pHostSG->Address = Addr_64;
            pHostSG->BufferLength = sg_unit_size;
            pHostSG++;
            SGCount++;
            sl++;
        }
        
    } else {
        Addr_64 = (u64)pci_map_single(acs_adt->pdev, cmd->request_buffer, cmd->request_bufflen, cmd->sc_data_direction);
        pHostSG->Address = Addr_64;
        pHostSG->BufferLength = cmd->request_bufflen;
        SGCount++;
    }
    
    return SGCount;
    
}
#endif

/*******************************************************************
*  Function : AME_Host_preapre_SCSI_SG
*  Description: AME_Host_preapre_SCSI_SG call back function 
*  Return:  BDL SGCount
********************************************************************/
AME_U32 AME_Host_preapre_SCSI_SG(pAME_Data pAMEData,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command,pAME_Host_SG pHostSG)
{
    struct Acs_Adapter *acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    struct scsi_cmnd *cmd = (struct scsi_cmnd *)pAME_ModuleSCSI_Command->pRequestExtension;

    pAME_ModuleSCSI_Command->SGCount = acs_ame_build_sg(acs_adt,cmd,pHostSG);
    
    if (AME_U32_NONE == pAME_ModuleSCSI_Command->SGCount) {
        return FALSE;
    }
    
    return TRUE;
}

void TestCallBack(PVOID pAMERequestCallBack)
{
	return;
}

//AME_U32 MPIO_Host_Rebuild_SCSI_REQUEST(pAME_REQUEST_CallBack pAME_REQUESTCallBack,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command, pMPIO_CMD_Data pMPIO_CMDData)
AME_U32 MPIO_Host_Rebuild_SCSI_REQUEST(pAME_REQUEST_CallBack pAME_REQUESTCallBack,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command)
{
    pAME_Data pAMEData = pAME_REQUESTCallBack->pAMEData;
    struct Acs_Adapter *acs_adt = (struct Acs_Adapter *)pAMEData->pDeviceExtension;
    struct scsi_cmnd   *pCmd = (struct scsi_cmnd *)(pAME_REQUESTCallBack->pRequestExtension);
    
	pAME_ModuleSCSI_Command->pRequestExtension=(PVOID)pCmd;
	pAME_ModuleSCSI_Command->callback=TestCallBack;
	
	//pMPIO_CMDData->Raid_ID = pCmd->device->channel;
	//pMPIO_CMDData->CDB_0 = pCmd->cmnd[0];

	return TRUE;
}


/*******************************************************************
*  Function : acs_ame_prepare_AME_SCSI_cmd
********************************************************************/
//AME_U32 acs_ame_prepare_AME_SCSI_cmd(struct Acs_Adapter *acs_adt,struct scsi_cmnd *cmd,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command,pMPIO_CMD_Data pMPIO_CMDData)
AME_U32 acs_ame_prepare_AME_SCSI_cmd(struct Acs_Adapter *acs_adt,struct scsi_cmnd *cmd,pAME_Module_SCSI_Command pAME_ModuleSCSI_Command)
{
    u32 ScsiRequestControl = 0;

    //pMPIO_CMDData->Raid_ID = (AME_U8)cmd->device->channel;
    //pMPIO_CMDData->CDB_0   = (AME_U8)cmd->cmnd[0];
    
    pAME_ModuleSCSI_Command->Raid_ID   = cmd->device->channel;
    pAME_ModuleSCSI_Command->Slave_Raid_ID = pAME_ModuleSCSI_Command->Raid_ID;
    pAME_ModuleSCSI_Command->Target_ID = (u8)(cmd->device->id - 1);
    pAME_ModuleSCSI_Command->Lun_ID    = (u8)(cmd->device->lun);
    pAME_ModuleSCSI_Command->CDBLength = (u8)cmd->cmd_len;
    pAME_ModuleSCSI_Command->pRequestExtension = (PVOID)cmd;
    pAME_ModuleSCSI_Command->callback = TestCallBack;
    pAME_ModuleSCSI_Command->MPIO_Which = 0; // default Master for Raid R0/R1
    pAME_ModuleSCSI_Command->pMPIORequestExtension = NULL;
    
    memcpy(pAME_ModuleSCSI_Command->CDB, cmd->cmnd, cmd->cmd_len);
    
    /* 2.6.25 upper kernel */
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
        pAME_ModuleSCSI_Command->DataLength = cmd->sdb.length;
    #else  /* 2.6.24 down kernel */
        pAME_ModuleSCSI_Command->DataLength = cmd->request_bufflen;
    #endif  

    /* Setting data direction */
    switch (cmd->sc_data_direction)
    {
        case PCI_DMA_FROMDEVICE:
            ScsiRequestControl = AME_SCSI_CONTROL_READ | AME_SCSI_CONTROL_SENSEBUF_TAG_UNTAG;
            break;
            
        case PCI_DMA_TODEVICE:
            ScsiRequestControl = AME_SCSI_CONTROL_WRITE | AME_SCSI_CONTROL_SENSEBUF_TAG_UNTAG;
            break;
            
        case PCI_DMA_NONE:
            ScsiRequestControl = AME_SCSI_CONTROL_NODATATRANSFER | AME_SCSI_CONTROL_SENSEBUF_TAG_UNTAG;
            break;
            
        default:
            ScsiRequestControl = 0x00;
            PRINTK("In %s error:  Unknown DataDirection.\n",__func__);
            return FALSE;
    }    
    
    if (acs_adt->blIs64bit) {
        pAME_ModuleSCSI_Command->Control = ScsiRequestControl | AME_SCSI_CONTROL_SENSEBUF_ADDR_64;
    } else {
        pAME_ModuleSCSI_Command->Control = ScsiRequestControl | AME_SCSI_CONTROL_SENSEBUF_ADDR_32;
    }
    
    //pAME_ModuleSCSI_Command->SGCount = AME_Host_preapre_SCSI_SG(acs_adt->pAMEData,pAME_ModuleSCSI_Command,pAME_ModuleSCSI_Command->AME_Host_Sg);
    if (FALSE == AME_Host_preapre_SCSI_SG(acs_adt->pAMEData,pAME_ModuleSCSI_Command,pAME_ModuleSCSI_Command->AME_Host_Sg)) {
        return FALSE;
    }
    
    return TRUE;
    
}


/*******************************************************************
* Function : acs_ame_queue_command
* Return value:
*   If queuecommand returns 0, then the HBA has accepted the command.
********************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0)
    int acs_ame_queue_command_lck(struct Scsi_Host *shost, struct scsi_cmnd *cmd)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
#else
int acs_ame_queue_command(struct scsi_cmnd *cmd,void (* done)(struct scsi_cmnd *))
#endif
{
    struct Acs_Adapter  *acs_adt;
    //MPIO_CMD_Data		MPIO_CMDData;
    AME_Module_SCSI_Command AME_ModuleSCSI_Command;
    
    acs_adt = (struct Acs_Adapter *)cmd->device->host->hostdata;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,16,0)
    acs_set_scsi_done(cmd, done);
#endif
    //printk("Sam Debug:  in %s  cmd=[%X] lun=%d\n",__func__, cmd->cmnd[0],cmd->device->lun);
    
    if (TRUE == AME_Check_is_NT_Mode(acs_adt->pAMEData))
    {
        if (TRUE == AME_NT_Raid_Bar_lost_Check(acs_adt->pAMEData,cmd->device->channel)) {
            cmd->result = (DID_OK << 16) | (QUEUE_FULL << 1);
            acs_scsi_done(cmd);
            return 0;
        }
       
       /* Check NT Raid alive ?
        * If not alive don't send any cmd to Raid.
        */
        if (MPIO_NT_Raid_Alive_Check(acs_adt->pMPIOData,cmd->device->channel) == FALSE) {
            cmd->result = (DID_NO_CONNECT << 16);
            acs_scsi_done(cmd);
            return 0;
        }
    } else {
        /* 
         * For Raid 2208 3108 Error handle and check Das Raid init ready
         */
        if ((FALSE == AME_Get_RAID_Ready(acs_adt->pAMEData)) ||
            (TRUE == AME_Raid_2208_3108_Bar_lost_Check(acs_adt->pAMEData))) {
	        cmd->result = (DID_OK << 16) | (QUEUE_FULL << 1);
            acs_scsi_done(cmd);
            mdelay(500);
            return 0;
	    }
	    
	    #if defined (Enable_AME_RAID_R0)
            if (acs_adt->pdev->device == AME_6201_DID_DAS) {
                
                pMPIO_DATA pHost_MPIODATA = acs_adt->pMPIOData;
                
                // Wait Slave Path ready for Raid R0/R1
                if (pHost_MPIODATA->MPIO_RAIDDATA[cmd->device->channel].PATHCount < Soft_Raid_Target) {
                    cmd->result = (DID_NO_CONNECT << 16);
                    acs_scsi_done(cmd);
                    return 0;
                }
            }
        #endif
    }
    
    /* 
     * SYNCHRONIZE_CACHE and TEST_UNIT_READY, return Success.
     */
    if ((cmd->cmnd[0] == TEST_UNIT_READY) ||
        (cmd->cmnd[0] == SYNCHRONIZE_CACHE))  {
        acs_scsi_done(cmd);
        return 0;
    }
	
    //if (FALSE == acs_ame_prepare_AME_SCSI_cmd(acs_adt,cmd,&AME_ModuleSCSI_Command,&MPIO_CMDData)) {
    if (FALSE == acs_ame_prepare_AME_SCSI_cmd(acs_adt,cmd,&AME_ModuleSCSI_Command)) {
        cmd->result = (DID_OK << 16) | (COMMAND_TERMINATED  << 1);
        acs_scsi_done(cmd);
        return 0;
    }
    
    #if defined (Enable_AME_RAID_R0)
        //if (FALSE == AME_Raid_Build_SCSI_Cmd(acs_adt->pAMERaid_Queue_DATA,acs_adt->pMPIOData,&AME_ModuleSCSI_Command,&MPIO_CMDData)) {
        if (FALSE == AME_Raid_Build_SCSI_Cmd(acs_adt->pAMERaid_Queue_DATA,acs_adt->pMPIOData,&AME_ModuleSCSI_Command)) {
            cmd->result = (DID_OK << 16) | (QUEUE_FULL << 1);
            acs_scsi_done(cmd);
            return 0;
        }
    #else
        //if (FALSE == MPIO_Build_SCSI_Cmd(acs_adt->pMPIOData,&AME_ModuleSCSI_Command,&MPIO_CMDData)) {
        if (FALSE == MPIO_Build_SCSI_Cmd(acs_adt->pMPIOData,&AME_ModuleSCSI_Command)) {
            cmd->result = (DID_OK << 16) | (QUEUE_FULL << 1);
            acs_scsi_done(cmd);
            return 0;
        }
    #endif
    
    
    return 0;
       
}



/*******************************************************************
* Function:  Allocate_DMA_memory
* Description:  allocate DMA memory, if can't allocated retry five
* times and make sure memory range in 4G boundary.
* Return:  Success return TRUE, Fail return FALSE.
*******************************************************************/
static int Allocate_DMA_memory(struct Acs_Adapter *acs_adt)
{
    int         i=0;
    void        *virtual_dma_address;
    dma_addr_t  physical_dma_address;
    
    /* AME module memory allocate */
    size_t AME_Total_memory_size;
    AME_Module_Prepare AMEModulePrepare;
    
    AMEModulePrepare.DeviceID = acs_adt->pdev->device;
    AMEModulePrepare.SG_per_command = ACS_MAX_SGLIST + 1;
	AMEModulePrepare.Total_Support_command = ACS_SUPPORT_CMD;
	AME_Total_memory_size = (size_t)MPIO_Init_Prepare(acs_adt->pMPIOData,acs_adt->pAMEData,&AMEModulePrepare);
	
    acs_adt->dma_mem_All_size = AME_Total_memory_size;
    
    /* allocate DMA memory , retry 5 times to get memory */
    while ( i++ < 5 )
    {
        /* mapping memory for dma */
        virtual_dma_address = dma_alloc_coherent( &(acs_adt->pdev->dev),
                                                  AME_Total_memory_size,
                                                  &physical_dma_address,GFP_KERNEL);
    
        if (virtual_dma_address) {
            
            /* make sure in 4G boundary */
            if ( ((u32)physical_dma_address) < (0xFFFFFFFF - AME_Total_memory_size) ) {
                PRINTK("allocate Total DMA memory %d KB \n",(u32)(AME_Total_memory_size/1024));
                break;
            }
            
            /* memory over 4G boundary ,free memory and allocate again */
            dma_free_coherent( &(acs_adt->pdev->dev),
                               AME_Total_memory_size,
                               virtual_dma_address,physical_dma_address);
            
        }
        
        ssleep(2);
        PRINTK("dma_alloc_coherent retry %d times\n",i);
        
    }
    
    if (! virtual_dma_address) {
        PRINTK("In %s error:  Allocate DMA memory fail.\n",__func__);
        return FALSE;
    }
    
    //PRINTK("ACS MSG:  Allocate DMA memory OK!\n");
    {
        MSG_Additional_Data MSG_AdditionalData;
		MSG_AdditionalData.parameter_type = parameter_type_decimal_integer;
		MSG_AdditionalData.parameter_length = 1;
		MSG_AdditionalData.parameter_data.INT_data[0]=AME_Total_memory_size;			
		
        ACS_message(acs_adt->pMSGData,HBA_init,HBA_Init_Allocate_Memory_Success,&MSG_AdditionalData);
    }
    
    /* set 0x00 in vir dma memory */
    memset(virtual_dma_address, 0x00, AME_Total_memory_size);
            
    /* set vir dma memory and phy dma memory*/
    acs_adt->virtual_dma_coherent = virtual_dma_address;
    acs_adt->physical_dma_coherent_handle = physical_dma_address;
    
    return TRUE;
}


/*******************************************************************
*  Function:  Erratum_8518
*  Description:  PLX Erratum#13-Credit Overflow when Cut-Thru 
*  is Enabled (refer attachment)
*  Return:  None
*******************************************************************/
static void Erratum_8518(struct Acs_Adapter *acs_adt)
{
    u32 bar0,Bar_Size,tmpreg;
    void __iomem    *register_base;
    struct pci_dev  *plx_8518 = NULL;
    
    plx_8518 = acs_adt->pdev->bus->self;
    pci_read_config_dword(plx_8518, 0x00, &tmpreg);
    //PRINTK("Erratum_8518 : device = 0x%x\n",tmpreg);
    
    if (tmpreg == 0x851810b5)
    {
        plx_8518 = acs_adt->pdev->bus->self->bus->self;
        pci_read_config_dword(plx_8518, 0x00, &tmpreg);
        //PRINTK("Erratum_8518 : device = 0x%x\n",tmpreg);
    }
    
    if (tmpreg == 0x851810b5)
    {
        //PLX Erratum#13-Credit Overflow when Cut-Thru is Enabled (refer attachment)
        //(a) short the strapping pin (EEPROM enable)
        //(b) mount EEPROM with PLX8518 EEPROM data 
        //(c) Clear offset 0x1dc bit 21
        PRINTK("Fixed PLX8518 Erratum:  Credit Overflow when Cut-Thru is Enabled\n");
        
        /* get NT card upstream Bar0 and Bar size */
        bar0 = pci_resource_start(plx_8518, 0);
        Bar_Size = pci_resource_len (plx_8518, 0);
        
        /* set vir memory 128k (0x20000)to phy memory */
        register_base = (void __iomem *)RemapPCIMem(bar0, Bar_Size);
        
        //PRINTK("PLX 8518 uppstream bar0=0x%8x, acs_adt->bar0=0x%x get mmio size 128k\n", (u32)bar0, (u32)register_base);
        
        if (register_base == 0)     
        {   
            PRINTK("Can not remap PLX8518 bar0");
            iounmap(register_base);
            return;
        }
        
        tmpreg = readl(register_base+0x1DC);
        //PRINTK("plx_8518 : 1DC : 0x%x\n",tmpreg);
              
        writel((u32)(tmpreg & 0xFFDFFFFF), register_base + 0x1DC);
        tmpreg = readl(register_base+0x1DC);
        //PRINTK("plx_8518 : 1DC : 0x%x\n",tmpreg);
        
        // release memory
        iounmap(register_base);
    }
    
    return;
}


/**********************************************************
                  2.6 kernel inite
 **********************************************************/
static struct pci_device_id acs_ame_ids[] = 
{
    { PCI_DEVICE(TESTVID, TESTDID), },
    { PCI_DEVICE(VENDERID, AME_6101_DID_DAS), },
    { PCI_DEVICE(VENDERID, AME_6201_DID_DAS), },
    { PCI_DEVICE(VENDERID, AME_2208_DID1_DAS), },
    { PCI_DEVICE(VENDERID, AME_2208_DID2_DAS), },
    { PCI_DEVICE(VENDERID, AME_2208_DID3_DAS), },
    { PCI_DEVICE(VENDERID, AME_2208_DID4_DAS), },
    { PCI_DEVICE(VENDERID, AME_3108_DID0_DAS), },
    { PCI_DEVICE(VENDERID, AME_3108_DID1_DAS), },
    { PCI_DEVICE(PLX_VID, AME_8508_DID_NT), },
    { PCI_DEVICE(PLX_VID, AME_8608_DID_NT), },
    { PCI_NT_DEVICE(PLX_VID, AME_8608_DID_NT,Accusys_VID,AME_8608_DID_NT), },
    { PCI_NT_DEVICE(PLX_VID, AME_87B0_DID_NT,Accusys_VID,AME_87B0_DID_NT), },
    { PCI_NT_DEVICE(Caldigit_VID, AME_8608_DID_NT,Caldigit_VID,AME_8608_DID_NT), },
    { 0, }
};
MODULE_DEVICE_TABLE(pci, acs_ame_ids);

/*****************************************************
*      2.6 kernel scsi_host_template
******************************************************/ 
static struct scsi_host_template acs_ame_scsi_host_template = 
{
    .module                 = THIS_MODULE,
#if (defined(ACCUSYS_SIGNATURE) || defined(UIT_SIGNATURE) || defined(YANO_SIGNATURE))
    .proc_name              = "ACS6x",
    .name                   = "ACS6x",
#else //(defined CALDIGIT_SIGNATURE)
    .proc_name              = "HDPro_ame",
    .name                   = "hdpro_ame(ACS62000-08)",
#endif 
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0)
    .queuecommand           = acs_ame_queue_command_lck,
#else
    .queuecommand           = acs_ame_queue_command,
#endif
    .eh_abort_handler       = acs_ame_abort,
    .can_queue              = ACS_SUPPORT_CMD,
    .cmd_per_lun            = ACS_SUPPORT_CMD,
    .sg_tablesize           = ACS_MAX_SGLIST,
    .max_sectors            = MAX_SECTOR_SIZE,
    //  .proc_info              =
    //  .info                   =
    //  .ioctl                  =
    //  .eh_strategy_handler    = 
    //  .eh_device_reset_handler= 
    //  .eh_bus_reset_handler   = 
    //  .eh_host_reset_handler  = 
    //  .bios_param             =
    //  .this_id                =
    //  .unchecked_isa_dma      =
    //  .use_clustering         = 
};

static int acs_ame_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int    i,err;
    u32    bar0,Bar_Size;
    struct Scsi_Host    *host = NULL;
    struct Acs_Adapter  *acs_adt = NULL;
    AME_Module_Init_Prepare     AMEModule_Init_Prepare;
    AME_Module_Support_Check    AMEModule_Support_Check;
    
    /* Show driver version */
    PRINTK("/*******************************************************\n");
    PRINTK(" ***  %s driver version %s\n",DRIVER_Title,DRIVER_VERSION);
    PRINTK(" ***  %s driver Devices 0x%x\n",DRIVER_Title,pdev->device);
    PRINTK(" *******************************************************/\n");
    
    /* 
     * Confirm pci type = type 0 
     * type 0 is pcie end-device, not bridge device 
     * make sure driver not load error.
     */
    if (pdev->hdr_type != PCI_HEADER_TYPE_NORMAL) {
        PRINTK("pci_type_error : THis type is Bridge type. \n");
        err = -ENODEV;
        goto fail_pci_type;
        return err;
    }
    
    /* AME_module Support device check */
    pci_read_config_dword(pdev,0x00,&AMEModule_Support_Check.Device_Reg0x00);
    pci_read_config_dword(pdev,0x08,&AMEModule_Support_Check.Device_Reg0x08);
    pci_read_config_dword(pdev,0x2C,&AMEModule_Support_Check.Device_Reg0x2C);
    if (FALSE == AME_Device_Support_Check(&AMEModule_Support_Check)) {
        err = -ENODEV;
        goto fail_pci_type;
        return err;
    }
    
    /* Enable device I/O and memory, and Set device DMA master */
    if (! pci_enable_device(pdev)) {
        pci_set_master(pdev);
    } else {
        PRINTK("In %s error:  pci_enable_device fail.\n",__func__);
        err = -ENODEV;
        goto fail_pci_enable;
    }
    
    /* Set DMA Mask */
    if (! pci_set_dma_mask(pdev, DMA_64BIT_MASK)) {
        pci_set_consistent_dma_mask(pdev, DMA_64BIT_MASK);
    } else if (! pci_set_dma_mask(pdev, DMA_32BIT_MASK)) {
        pci_set_consistent_dma_mask(pdev, DMA_32BIT_MASK);
    } else {
        PRINTK("In %s error:  set_dma_mask fail.\n",__func__);
        err = -ENOMEM;
        goto fail_set_dma_mask;
    }
    
    /* Register a scsi host adapter instance */
    if ((host = scsi_host_alloc(&acs_ame_scsi_host_template,sizeof(struct Acs_Adapter))))
    {
        /* SCSI Host Struct Setting */
        if ( (pdev->device == AME_87B0_DID_NT) ||
             (pdev->device == AME_8608_DID_NT) ||
             (pdev->device == AME_8508_DID_NT) )
        {
            host->max_channel = 31;            /* max_channel: 32 Raid ,zero base: from 0 ~ 31 */
        }
         
        host->max_id        = 1 + MAX_RAID_LUN_ID; /* Multiple ID : Single LUN */
        host->max_lun       = 1;                   /* Multiple ID : Single LUN */
        host->unique_id     = (pdev->bus->number << 8) | (pdev->devfn);
        host->max_cmd_len   = 16;              /* This is issue of 64bit LBA ,over 2T byte */
        host->irq           = pdev->irq;
        
    } else {
        PRINTK("In %s error:  scsi_host_alloc fail. Can't get memory 0x%x bytes for struct Acs_Adapter\n",__func__,(u32)sizeof(struct Acs_Adapter));
        err = -ENOMEM;
        goto fail_scsi_host_alloc;
    }
    
    /* set PCI device data to acs_adt */
    acs_adt = (struct Acs_Adapter *)host->hostdata;
    memset(acs_adt, 0x00, sizeof(struct Acs_Adapter));
    pci_set_drvdata(pdev, acs_adt);
    
    // allocate AME_DATA data memory
    acs_adt->pAMEData = (pAME_Data)kmalloc(sizeof(struct _AME_Data),GFP_KERNEL);
    if (NULL == acs_adt->pAMEData) {
        PRINTK("In %s error:  kmalloc get AME DATA memory 0x%x bytes failed\n",__func__,(u32)sizeof(struct _AME_Data));
        err = -ENOMEM;
        goto fail_other_memory_alloc;
    } else {
        memset(acs_adt->pAMEData, 0x00, sizeof(struct _AME_Data));
    }
    
    // allocate MSG_DATA data memory
    acs_adt->pMSGData = (pMSG_DATA)kmalloc(sizeof(struct _MSG_DATA),GFP_KERNEL);
    if (NULL == acs_adt->pMSGData) {
        PRINTK("In %s error:  kmalloc get MSG DATA memory 0x%x bytes failed\n",__func__,(u32)sizeof(struct _MSG_DATA));
        err = -ENOMEM;
        goto fail_other_memory_alloc;
    } else {
        memset(acs_adt->pMSGData, 0x00, sizeof(struct _MSG_DATA));
    }
    
    // allocate AME Raid Queue data memory
    #if defined (Enable_AME_RAID_R0)
        acs_adt->pAMERaid_Queue_DATA = (pAME_Raid_Queue_DATA)vmalloc(sizeof(struct _AME_Raid_Queue_DATA));
        if (NULL == acs_adt->pAMERaid_Queue_DATA) {
            PRINTK("In %s error:  kmalloc get Raid Queue DATA memory 0x%x bytes failed\n",__func__,(u32)sizeof(struct _AME_Raid_Queue_DATA));
            err = -ENOMEM;
            goto fail_other_memory_alloc;
        } else {
            memset(acs_adt->pAMERaid_Queue_DATA, 0x00, sizeof(struct _AME_Raid_Queue_DATA));
        }
    #endif
    
    // allocate MPIO data memory
    acs_adt->pMPIOData = (pMPIO_DATA)kmalloc(sizeof(struct _MPIO_DATA),GFP_KERNEL);
    if (NULL == acs_adt->pMPIOData) {
        PRINTK("In %s error:  kmalloc get MPIO DATA memory 0x%x bytes failed\n",__func__,(u32)sizeof(struct _MPIO_DATA));
        err = -ENOMEM;
        goto fail_other_memory_alloc;
    } else {
        memset(acs_adt->pMPIOData, 0x00, sizeof(struct _MPIO_DATA));
    }
    
    // allocate AME Queue data memory
    acs_adt->pAMEQueueData = (pAME_Queue_DATA)kmalloc(sizeof(struct _AME_Queue_DATA),GFP_KERNEL);
    if (NULL == acs_adt->pAMEQueueData) {
        PRINTK("In %s error:  kmalloc get AME Queue DATA memory 0x%x bytes failed\n",__func__,(u32)sizeof(struct _AME_Queue_DATA));
        err = -ENOMEM;
        goto fail_other_memory_alloc;
    } else {
        memset(acs_adt->pAMEQueueData, 0x00, sizeof(struct _AME_Queue_DATA));
    }
    
    /* Register multiple Accusys Raid Card instance address */
    if (gAcs_Adapter_Counter < MAX_ADAPTER) {
        for (i=0; i<MAX_ADAPTER; i++ ) {
            if (gAcs_Adapter.acs[i] == NULL) {
                gAcs_Adapter.acs[i] = acs_adt;
                gAcs_Adapter_Counter++;
                PRINTK("ACS MSG:  Find Raid index %d.\n",gAcs_Adapter_Counter);
                break;
            }
        }
    } else {
        PRINTK("In %s error:  Max_adapter support %d card's, over support.\n",__func__,MAX_ADAPTER);
        err = -EMLINK;
        goto fail_over_support;
    }

    /* private date:  acs_adt init and setting */
    acs_adt->irq= pdev->irq;
    acs_adt->pdev = pdev;
    acs_adt->host = host;
    acs_adt->cdev_major = -1;
    acs_adt->InbandFlag = INBANDCMD_IDLE;
    
    /* check 32 or 64 bit and set flage */
    if ( sizeof(dma_addr_t) == 0x08 ) {
        acs_adt->blIs64bit = TRUE;
    }
    
    spin_lock_init(&acs_adt->inband_lock);
    spin_lock_init(&acs_adt->AME_module_lock);
    spin_lock_init(&acs_adt->MPIO_lock);
    spin_lock_init(&acs_adt->AME_Raid_lock);
    init_waitqueue_head(&acs_adt->inband_wait_lock);
    
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
        INIT_WORK(&acs_adt->Lun_change_work, (void *)Lun_change_handler);
        INIT_WORK(&acs_adt->Lun_change_init_work, (void *)Lun_change_handler_init);
    #else
        INIT_WORK(&acs_adt->Lun_change_work, (void *)Lun_change_handler, (void *)&acs_adt->Lun_change_work);
        INIT_WORK(&acs_adt->Lun_change_init_work, (void *)Lun_change_handler_init, (void *)&acs_adt->Lun_change_init_work);
    #endif
   
    /* 
     * Mark all PCI regions associated with PCI device pdev as being reserved by owner
     * res_name. Do not access any address inside the PCI regions unless this call 
     * returns successfully.
     */
     
    if ((err = pci_request_regions(pdev, DRIVER_NAME)) < 0)
    {
        PRINTK("In %s error:  pci_request_regions fail.\n",__func__);
        err = -ENOMEM;
        goto fail_pci_request_regions;
    }
    
    /* 
     * Get ACS Raid Card Bar0 address 
     * IOP 2208  register IO mapping on Bar1 1M(16K)
     * NT device register IO mapping on Bar2 32M
     */
    
    switch (acs_adt->pdev->device)
    {
        case AME_2208_DID1_DAS:
        case AME_2208_DID2_DAS:
        case AME_2208_DID3_DAS:
        case AME_2208_DID4_DAS:
        case AME_3108_DID0_DAS:
        case AME_3108_DID1_DAS:
            bar0 = pci_resource_start(pdev,1);
            Bar_Size = pci_resource_len(pdev,1);
            break;
            
        case AME_8508_DID_NT:
        case AME_8608_DID_NT:
        case AME_87B0_DID_NT:
            bar0 = pci_resource_start(pdev,2);
            Bar_Size = pci_resource_len(pdev,2);
            break;
            
        default:
            bar0 = pci_resource_start(pdev,0);
            Bar_Size = pci_resource_len(pdev,0);
            break;
    }
    
    if (! bar0) {
        PRINTK("In %s error:  Can not get bar0 address.\n",__func__);
        err = -ENXIO;
        goto fail_RemapPCIMem;
    }

    /* Mapping Bar0 */
    acs_adt->register_base = (void __iomem *)RemapPCIMem(bar0, Bar_Size);
    if (acs_adt->register_base == NULL) {
        PRINTK("In %s error:  Can not remap bar0 IO memory.\n",__func__);
        err = -ENXIO;
        goto fail_RemapPCIMem;
    }
    
    /* Init ACS Message */
    MSG_Init(acs_adt->pMSGData,acs_adt->pMPIOData,acs_adt->pAMEData,NULL);
    ACS_message(acs_adt->pMSGData,HBA_init,HBA_Init_start,NULL);
    
    {
		MSG_Additional_Data MSG_AdditionalData;
		MSG_AdditionalData.parameter_type = parameter_type_hexadecimal_integer;
		MSG_AdditionalData.parameter_length = 1;
		MSG_AdditionalData.parameter_data.INT_data[0]=(U32)(((U32)(pdev->vendor) <<16) | (U32)(pdev->device));
		
		ACS_message(acs_adt->pMSGData,HBA_init,HBA_Init_VendorID_DeviceID,&MSG_AdditionalData);
	}
    
    /* DMA Memory allocate and init */
    if (Allocate_DMA_memory(acs_adt) == FALSE) {
        err = -ENOMEM;
        goto fail_dma_alloc_coherent;
    }
    
    /* Bug Fixed:  Fixed hdpro2 Data error issue. */
    //Erratum_8518(acs_adt);
    
    /* Setting AME_module Data */
	  AMEModule_Init_Prepare.Device_BusNumber    = (U32)(pdev->bus->number);
	  AMEModule_Init_Prepare.Vendor_ID           = pdev->vendor;
    AMEModule_Init_Prepare.Device_ID           = pdev->device;
	  AMEModule_Init_Prepare.Raid_Base_Address   = (PVOID)acs_adt->register_base;
    AMEModule_Init_Prepare.pDeviceExtension    = (PVOID)acs_adt;
    AMEModule_Init_Prepare.Glo_High_Address    = GET_DMA_HI32(acs_adt->physical_dma_coherent_handle);
    switch (acs_adt->pdev->device)
    {
        case AME_8508_DID_NT:
        case AME_8608_DID_NT:
        case AME_87B0_DID_NT:
            AMEModule_Init_Prepare.Polling_init_Enable = FALSE;
            break;
            
        default:
            AMEModule_Init_Prepare.Polling_init_Enable = TRUE;
            break;
    }
    AMEModule_Init_Prepare.Init_Memory_by_AME_module = TRUE;
    AMEModule_Init_Prepare.AME_Buffer_physicalStartAddress = (AME_U64)(acs_adt->physical_dma_coherent_handle);
  	AMEModule_Init_Prepare.AME_Buffer_virtualStartAddress  = (AME_U64)(acs_adt->virtual_dma_coherent);
        
    /* AME_Raid_Data_Init(), driver MUST call this before call MPIO_Init function */
    #if defined (Enable_AME_RAID_R0)
        AME_Raid_Data_Init((PVOID)acs_adt,acs_adt->pAMERaid_Queue_DATA);
    #endif
    
    /* MPIO_Init_Setup(), driver MUST call this before call MPIO_Init function */
    MPIO_Init_Setup(acs_adt->pMPIOData);
    
    /* AME_Queue_Data_Init(), driver MUST call this before call MPIO_Init function */
    AME_Queue_Data_Init(acs_adt->pAMEData,acs_adt->pAMEQueueData);
    
    /* Init AME_module: Init AME data,Raid Init and LED Init */
    if (TRUE == MPIO_Init(acs_adt->pMPIOData,acs_adt->pAMEData,&AMEModule_Init_Prepare)) {
        // keep other mapping IO space address for remove to free resource
        acs_adt->register_base_HBA = acs_adt->pAMEData->AME_LED_data.LED_HBA_Base_Address;
        acs_adt->register_base_Raid_Bridge = acs_adt->pAMEData->AME_LED_data.LED_Raid_Base_Address;
    } else {
        PRINTK("In %s error:  DTR_HandShake fail.\n",__func__);
        err = -ENODEV;
        goto fail_AME_Raid_Init;
    }
    
    /* Register interrupt handler with acs_ame_interrupt */
    if (acs_request_irq(pdev->irq,(void *)acs_ame_interrupt,DRIVER_NAME,acs_adt)) {
        PRINTK("In %s error:  request_irq fail.\n",__func__);
        err = -EINTR;
        goto fail_request_irq;
    }
    
    /* Register host data to scsi device */
    if (scsi_add_host(host,&pdev->dev)) {
        PRINTK("In %s error:  scsi_add_host fail.\n",__func__);
        err = -ENODEV;
        goto fail_scsi_add_host;
    }
    
    /* Schedule Work to scan scsi host and Lun_change_init */
    schedule_work(&acs_adt->Lun_change_init_work);
    
    /* AME module Start Function */
    //MPIO_Start(acs_adt->pMPIOData);
    
    /* Enable HBA LED Access Timer Function */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
    timer_setup(&acs_adt->ACS_Timer, ACS_Timer_Func, 0);
    PRINTK("ACS MSG:  Start ACS_Timer_Func.\n");
    mod_timer(&acs_adt->ACS_Timer, jiffies + 1*HZ);
#else
    init_timer(&acs_adt->ACS_Timer);
    for (i=0; i<MAX_ADAPTER; i++ ) {
        if (gAcs_Adapter.acs[i] == acs_adt) {
            PRINTK("ACS MSG:  Start ACS_Timer_Func for ACS index %d.\n",i+1);
            ACS_Timer_Func(i);
            break;
        }
    }
#endif
    
    /* Register CHAR Device */
    if (acs_adt->cdev_major == -1)
    {
        if ((acs_adt->cdev_major = register_chrdev(0,CHAR_DRIVER_NAME,&acs_ame_fos)) >= 0) {
            for (i=0; i<MAX_ADAPTER; i++ ) {
                if (gAcs_Adapter.acs[i] == acs_adt) {
                    acs_device_create(ACS_class, NULL, MKDEV(acs_adt->cdev_major,0),"%s%d",CHAR_DRIVER_NAME,i);
                }
            }
        } else {
            PRINTK("In %s error:  Register_chrdev fail.\n",__func__);
            err = -ENODEV;
            goto fail_register_chrdev;
        }
    }
    
    ACS_message(acs_adt->pMSGData,HBA_init,HBA_Init_Done_Success,NULL);
    
    return SUCCESS;
    //PRINTK("**********ACS probe end**********\n");
    
    fail_register_chrdev:
        scsi_remove_host(host);
        del_timer_sync(&acs_adt->ACS_Timer);

    fail_scsi_add_host:
        free_irq(pdev->irq, acs_adt);
        
    fail_request_irq:
    fail_AME_Raid_Init:
        dma_free_coherent(&(pdev->dev),
                acs_adt->dma_mem_All_size,
                acs_adt->virtual_dma_coherent,
                acs_adt->physical_dma_coherent_handle);
    
    fail_dma_alloc_coherent:
        iounmap(acs_adt->register_base);
        if (acs_adt->register_base_HBA) {
            iounmap(acs_adt->register_base_HBA);
        }
        if (acs_adt->register_base_Raid_Bridge) {
            iounmap(acs_adt->register_base_Raid_Bridge);
        }
    
    fail_RemapPCIMem:
        pci_release_regions(pdev);
        
    fail_pci_request_regions:
    fail_over_support:
        pci_set_drvdata(pdev, NULL);
        scsi_host_put(host);
        
    fail_other_memory_alloc:
        if (acs_adt->pAMEData) {
            kfree(acs_adt->pAMEData);
        }
        if (acs_adt->pMSGData) {
            kfree(acs_adt->pMSGData);
        }
        if (acs_adt->pAMERaid_Queue_DATA) {
            vfree(acs_adt->pAMERaid_Queue_DATA);
        }
        if (acs_adt->pMPIOData) {
            kfree(acs_adt->pMPIOData);
        }
        if (acs_adt->pAMEQueueData) {
            kfree(acs_adt->pAMEQueueData);
        }
        
    fail_scsi_host_alloc:
    fail_set_dma_mask:
        pci_disable_device(pdev);
    fail_pci_enable:
    fail_pci_type:
        
        PRINTK("*****Probe ERROR !!\n");
        PRINTK("In %s error:  ****** Probe ERROR. ******\n",__func__);
    
    return err;
}

/**********************************************************
* Function : acs_ame_remove
***********************************************************/
static void acs_ame_remove(struct pci_dev *pdev)
{
    u8                  i;
    struct Acs_Adapter  *acs_adt;
    
    /* clean up any allocated resources and stuff here.
     * like call release_region();                        
     */
     
    //DPRINTK("**********ACS remove**********\n");

    acs_adt = (struct Acs_Adapter *)pci_get_drvdata(pdev);
    
    /* Remove Chrdev */
    acs_device_destroy(ACS_class, MKDEV(acs_adt->cdev_major,0));
    
    #if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22)
        if (acs_adt->cdev_major != -1 )
        {   
            unregister_chrdev(acs_adt->cdev_major, CHAR_DRIVER_NAME);
            PRINTK("ACS MSG:  Unregister_chrdev SUCCESS (%d)!!\n", acs_adt->cdev_major);
        }
    #else
        if (acs_adt->cdev_major != -1) 
        {
            if ( (unregister_chrdev(acs_adt->cdev_major, CHAR_DRIVER_NAME) < 0) ) 
                PRINTK("In %s error:  Unregister_chrdev fail!!\n",__func__);
            else
                PRINTK("ACS MSG:  Unregister_chrdev SUCCESS (%d)!!\n", acs_adt->cdev_major);
        } 
    #endif
        
    
    // Remove ACS timer
    del_timer_sync(&acs_adt->ACS_Timer);
    
    /* AME module Stop Function */
    MPIO_Stop(acs_adt->pMPIOData);
    
    free_irq(acs_adt->pdev->irq, acs_adt);
    dma_free_coherent(&(acs_adt->pdev->dev),
            acs_adt->dma_mem_All_size,
            acs_adt->virtual_dma_coherent,
            acs_adt->physical_dma_coherent_handle);
    iounmap(acs_adt->register_base);
    if (acs_adt->register_base_HBA) {
        iounmap(acs_adt->register_base_HBA);
    }
    if (acs_adt->register_base_Raid_Bridge) {
        iounmap(acs_adt->register_base_Raid_Bridge);
    }
    if (acs_adt->pAMEData) {
        kfree(acs_adt->pAMEData);
    }
    if (acs_adt->pMSGData) {
        kfree(acs_adt->pMSGData);
    }
    if (acs_adt->pAMERaid_Queue_DATA) {
        vfree(acs_adt->pAMERaid_Queue_DATA);
    }
    if (acs_adt->pMPIOData) {
        kfree(acs_adt->pMPIOData);
    }
    if (acs_adt->pAMEQueueData) {
        kfree(acs_adt->pAMEQueueData);
    }
    pci_release_regions(acs_adt->pdev);
    pci_disable_device(acs_adt->pdev);
    scsi_remove_host(acs_adt->host);
    
    for (i=0; i<MAX_ADAPTER; i++) 
    {
        if (gAcs_Adapter.acs[i]== acs_adt) 
        {
            gAcs_Adapter.acs[i]= NULL;
            gAcs_Adapter_Counter--;
            pci_set_drvdata(acs_adt->pdev, NULL);
            return;
        }
    }
    PRINTK("In %s error:  No match acs_adt adapter found!! \n",__func__);
}

/**********************************************************
* Function : acs_ame_suspend
***********************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
static int acs_ame_suspend(struct pci_dev *pdev, pm_message_t state)    
#else
    static int acs_ame_suspend(struct pci_dev *pdev, u32 state)
#endif
{
    struct Acs_Adapter  *acs_adt = (struct Acs_Adapter *)pci_get_drvdata(pdev);
    
    PRINTK(" ****** IN acs_ame_suspend Start ******\n");
    ACS_message(acs_adt->pMSGData,HBA_Stop,HBA_Stop_Done_Success,NULL);
    
    // Remove ACS timer
    del_timer_sync(&acs_adt->ACS_Timer);
    
    PRINTK(" ****** IN acs_ame_suspend End   ******\n");
    
    return 0;
}

/**********************************************************
* Function : acs_ame_resume
* Description : Default resume method for devices that have 
* no driver provided resume,or not even a driver at all.
***********************************************************/
static int acs_ame_resume(struct pci_dev *pdev)
{
    int i;
    struct Acs_Adapter  *acs_adt = (struct Acs_Adapter *)pci_get_drvdata(pdev);
    AME_Module_Init_Prepare     AMEModule_Init_Prepare;
    
    ACS_message(acs_adt->pMSGData,HBA_init,HBA_Restart_start,NULL);
    
    PRINTK(" ****** IN acs_ame_resume Start ******\n");
    
    /* Setting AME_module Data */
	AMEModule_Init_Prepare.Device_BusNumber    = (U32)(pdev->bus->number);
	AMEModule_Init_Prepare.Vendor_ID          = pdev->vendor;
    AMEModule_Init_Prepare.Device_ID          = pdev->device;
	AMEModule_Init_Prepare.Raid_Base_Address   = (PVOID)acs_adt->register_base;
    AMEModule_Init_Prepare.pDeviceExtension    = (PVOID)acs_adt;
    AMEModule_Init_Prepare.Glo_High_Address    = GET_DMA_HI32(acs_adt->physical_dma_coherent_handle);
    AMEModule_Init_Prepare.Polling_init_Enable = TRUE;
    AMEModule_Init_Prepare.Init_Memory_by_AME_module = TRUE;
    AMEModule_Init_Prepare.AME_Buffer_physicalStartAddress = (AME_U64)(acs_adt->physical_dma_coherent_handle);
	AMEModule_Init_Prepare.AME_Buffer_virtualStartAddress  = (AME_U64)(acs_adt->virtual_dma_coherent);

    /* AME_Queue_Data_Init(), driver MUST call this before call MPIO_Init function */
	AME_Queue_Data_Init(acs_adt->pAMEData,acs_adt->pAMEQueueData);

    /* Init AME_module: ReInit AME data,Raid Init and LED Init */
    if (TRUE == MPIO_ReInit(acs_adt->pMPIOData,acs_adt->pAMEData,&AMEModule_Init_Prepare)) {
        // keep other mapping IO space address for remove to free resource
        acs_adt->register_base_HBA = acs_adt->pAMEData->AME_LED_data.LED_HBA_Base_Address;
        acs_adt->register_base_Raid_Bridge = acs_adt->pAMEData->AME_LED_data.LED_Raid_Base_Address;
    } else {
        PRINTK("In %s error:  DTR_HandShake fail.\n",__func__);
    }
    
    /* AME module Start Function */
    //MPIO_Start(acs_adt->pMPIOData);
    
    /* Enable HBA LED Access Timer Function */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
    timer_setup(&acs_adt->ACS_Timer, ACS_Timer_Func, 0);
    PRINTK("ACS MSG:  Start ACS_Timer_Func.\n");
    mod_timer(&acs_adt->ACS_Timer, jiffies + 1*HZ);
#else
    init_timer(&acs_adt->ACS_Timer);
    for (i=0; i<MAX_ADAPTER; i++ ) {
        if (gAcs_Adapter.acs[i] == acs_adt) {
            PRINTK("ACS MSG:  Start ACS_Timer_Func for ACS index %d.\n",i+1);
            ACS_Timer_Func(i);
            break;
        }
    }
#endif
    
    PRINTK(" ****** IN acs_ame_resume End   ******\n");
    
    return 0;
}

/**********************************************************
              set pci_driver
 ***********************************************************/
static struct pci_driver pci_driver = 
{
    .name = DRIVER_NAME,
    .id_table = acs_ame_ids,
    .probe = acs_ame_probe,
    .remove = acs_ame_remove,
    //.shutdown = acs_ame_remove,
    .suspend = acs_ame_suspend,
    .resume = acs_ame_resume,
};

static int __init acs_ame_init(void)
{
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
        ACS_class = class_create(THIS_MODULE, CHAR_DRIVER_NAME);
    #endif
    return pci_register_driver(&pci_driver);
}

static void __exit acs_ame_exit(void)
{
    pci_unregister_driver(&pci_driver);
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
        class_destroy(ACS_class);
    #endif
}


MODULE_AUTHOR("Sam Chuang");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRIVER_VERSION);
#ifdef ACCUSYS_SIGNATURE
    MODULE_DESCRIPTION("ACS SAS/SATA RAID HBA");
#endif
#ifdef UIT_SIGNATURE
    MODULE_DESCRIPTION("UIT SAS/SATA RAID HBA");
#endif
#ifdef YANO_SIGNATURE
    MODULE_DESCRIPTION("YANO SAS/SATA RAID HBA");
#endif
#ifdef CALDIGIT_SIGNATURE
    MODULE_DESCRIPTION("HDPro SAS/SATA RAID HBA");
#endif


module_init(acs_ame_init);
module_exit(acs_ame_exit);


