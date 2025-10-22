#ifndef ACS_COMPAT_H
#define ACS_COMPAT_H

/*
 * Compatibility layer for Accusys ACS6x driver
 * Target kernels: 6.1 - 6.12 (Debian 12 / TrueNAS SCALE / Proxmox)
 * Minimal invasive patches to modernize legacy APIs
 */

#include <linux/version.h>
#include <linux/pci.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>

/***************************************************************************
 * PCI API Modernization
 ***************************************************************************/

/* pci_get_bus_and_slot() removed in 5.0, use pci_get_domain_bus_and_slot() */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
#define acs_pci_get_bus(bus, devfn) \
        pci_get_domain_bus_and_slot(0, bus, devfn)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
#define acs_pci_get_bus(bus, devfn) \
        pci_get_bus_and_slot(bus, devfn)
#else
#define acs_pci_get_bus(bus, devfn) \
        pci_find_slot(bus, devfn)
#endif

/***************************************************************************
 * DMA API Modernization
 ***************************************************************************/

/* DMA mask constants changed in 5.18 */
#ifndef DMA_BIT_MASK
#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#endif

#ifndef DMA_64BIT_MASK
#define DMA_64BIT_MASK DMA_BIT_MASK(64)
#endif

#ifndef DMA_32BIT_MASK
#define DMA_32BIT_MASK DMA_BIT_MASK(32)
#endif

/* PCI DMA direction constants removed in 5.18, use DMA_* instead */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0)
#ifndef PCI_DMA_BIDIRECTIONAL
#define PCI_DMA_BIDIRECTIONAL DMA_BIDIRECTIONAL
#endif
#ifndef PCI_DMA_TODEVICE
#define PCI_DMA_TODEVICE DMA_TO_DEVICE
#endif
#ifndef PCI_DMA_FROMDEVICE
#define PCI_DMA_FROMDEVICE DMA_FROM_DEVICE
#endif
#ifndef PCI_DMA_NONE
#define PCI_DMA_NONE DMA_NONE
#endif
#endif

/* pci_set_dma_mask() wrapper for modern dma_set_mask_and_coherent() */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
static inline int acs_set_dma_mask(struct pci_dev *pdev, u64 mask)
{
    int ret;
    ret = dma_set_mask(&pdev->dev, mask);
    if (ret == 0)
        ret = dma_set_coherent_mask(&pdev->dev, mask);
    return ret;
}
#define pci_set_dma_mask(pdev, mask) acs_set_dma_mask(pdev, mask)
#define pci_set_consistent_dma_mask(pdev, mask) \
    dma_set_coherent_mask(&(pdev)->dev, mask)
#endif

/***************************************************************************
 * SCSI Midlayer API Modernization
 ***************************************************************************/

/* SCSI status codes changed in 4.16+ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,16,0)
#ifndef BUSY
#define BUSY SAM_STAT_BUSY
#endif
#ifndef QUEUE_FULL
#define QUEUE_FULL SAM_STAT_TASK_SET_FULL
#endif
#ifndef COMMAND_TERMINATED
#define COMMAND_TERMINATED SAM_STAT_TASK_ABORTED
#endif
#endif

/* scsi_cmnd->scsi_done callback handling changed in 5.16+ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0)
/* Modern kernels: scsi_done() is passed in or called directly */
static inline void acs_scsi_done(struct scsi_cmnd *cmd)
{
    scsi_done(cmd);
}
/* No need to assign scsi_done callback in modern kernels */
#define acs_set_scsi_done(cmd, done) do { } while(0)
#else
/* Legacy kernels: use cmd->scsi_done field */
static inline void acs_scsi_done(struct scsi_cmnd *cmd)
{
    cmd->scsi_done(cmd);
}
/* Must assign the callback pointer in legacy kernels */
#define acs_set_scsi_done(cmd, done) do { cmd->scsi_done = done; } while(0)
#endif

/* SCSI resid field handling (sdb.resid removed in later kernels) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
#define acs_scsi_get_resid(cmd) scsi_get_resid(cmd)
#define acs_scsi_set_resid(cmd, resid) scsi_set_resid(cmd, resid)
#else
#define acs_scsi_get_resid(cmd) ((cmd)->sdb.resid)
#define acs_scsi_set_resid(cmd, resid) do { (cmd)->sdb.resid = resid; } while(0)
#endif

/***************************************************************************
 * Timer API Modernization
 ***************************************************************************/

/* timer_setup() introduced in 4.15, replaces init_timer + .data + .function */
/* The driver already handles this correctly in acs_ame.c, so no changes needed */

/***************************************************************************
 * Device Class API Modernization
 ***************************************************************************/

/* class_create() signature changed in 6.4+ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
/* Modern kernels: class_create(name) - no owner parameter */
#define acs_class_create(name) class_create(name)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
/* Legacy kernels: class_create(owner, name) */
#define acs_class_create(name) class_create(THIS_MODULE, name)
#else
/* Very old kernels: no class support */
#define acs_class_create(name) NULL
#endif

/***************************************************************************
 * SCSI Device Management
 ***************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,6)
#define acs_add_device(host, channel, target, lun) \
        __scsi_add_device(host, channel, target, lun, NULL)
#else
#define acs_add_device(host, channel, target, lun) \
        scsi_add_device(host, channel, target, lun)
#endif

#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,9)
#define acs_put_device(device) \
        put_device(device)
#else
#define acs_put_device(device) \
        scsi_device_put(device)
#endif

/***************************************************************************
 * Device Creation API
 ***************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#define acs_device_create(class,parent,devt,fmt,name,index) \
            device_create(class,parent,devt,NULL,fmt,name,index)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
#define acs_device_create(class,parent,devt,fmt,name,index) \
            device_create(class,parent,devt,fmt,name,index)
#else
#define acs_device_create(class,parent,devt,fmt,name,index) do {} while(0)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
#define acs_device_destroy(class,devt) \
            device_destroy(class,devt)
#else
#define acs_device_destroy(class,devt) do {} while(0)
#endif

/***************************************************************************
 * IRQ Request API
 ***************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
#define acs_request_irq(irq,handler,name,dev) \
            request_irq(irq,handler,IRQF_SHARED,name,dev)
#else
#define acs_request_irq(irq,handler,name,dev) \
            request_irq(irq,handler,SA_INTERRUPT|SA_SHIRQ,name,dev)
#endif

/***************************************************************************
 * Unlocked IOCTL
 ***************************************************************************/

/* unlocked_ioctl became standard in 2.6.36+ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
/* Prototype: long (*unlocked_ioctl)(struct file*, unsigned int, unsigned long) */
#define ACS_IOCTL_HANDLER unlocked_ioctl
#else
/* Prototype: int (*ioctl)(struct inode*, struct file*, unsigned int, unsigned long) */
#define ACS_IOCTL_HANDLER ioctl
#endif

/***************************************************************************
 * User-space access compatibility
 ***************************************************************************/

/* asm/uaccess.h moved to linux/uaccess.h in 4.11+ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
#include <linux/uaccess.h>
#else
#include <asm/uaccess.h>
#endif

#endif /* ACS_COMPAT_H */
