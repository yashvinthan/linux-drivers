#ifndef MODERN_KERNEL_COMPAT_H
#define MODERN_KERNEL_COMPAT_H

#include <linux/version.h>

/* PCI bus slot API changed in 5.0 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
#define pci_get_bus_and_slot(domain, busdevfn) pci_get_domain_bus_and_slot(0, domain, busdevfn)
#endif

/* PCI DMA direction constants removed in 5.18 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0)
#define PCI_DMA_TODEVICE DMA_TO_DEVICE
#define PCI_DMA_FROMDEVICE DMA_FROM_DEVICE
#define PCI_DMA_NONE DMA_NONE
#endif

/* SCSI status constants changed in 4.16 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,16,0)
#define BUSY SAM_STAT_BUSY
#define QUEUE_FULL SAM_STAT_TASK_SET_FULL
#define COMMAND_TERMINATED SAM_STAT_COMMAND_TERMINATED
#endif

/* scsi_cmnd->scsi_done callback handling changed in 5.16+ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0)
static inline void acs_scsi_done(struct scsi_cmnd *cmd)
{
    scsi_done(cmd);
}
/* In kernel 5.16+, scsi_done is called directly, no assignment needed */
#define acs_set_scsi_done(cmd, done) do { } while(0)
#else
static inline void acs_scsi_done(struct scsi_cmnd *cmd)
{
    cmd->scsi_done(cmd);
}
/* In older kernels, we need to save the callback */
#define acs_set_scsi_done(cmd, done) do { cmd->scsi_done = done; } while(0)
#endif

/* PCI DMA mask API changed in 5.0 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
#define pci_set_dma_mask(pdev, mask) dma_set_mask(&(pdev)->dev, mask)
#define pci_set_consistent_dma_mask(pdev, mask) dma_set_coherent_mask(&(pdev)->dev, mask)
#endif

#endif
