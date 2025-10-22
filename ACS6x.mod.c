#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb0e602eb, "memmove" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x74b98d38, "scsi_remove_host" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x73ca058d, "class_destroy" },
	{ 0x3aa393f9, "__pci_register_driver" },
	{ 0xedc03953, "iounmap" },
	{ 0x44bf4a6b, "pci_request_regions" },
	{ 0x69acdf38, "memcpy" },
	{ 0x1ea80863, "scsi_dma_unmap" },
	{ 0x37a0cba, "kfree" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0xe2964344, "__wake_up" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0x6f857798, "pci_unregister_driver" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x3f110e4, "pci_read_config_dword" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x122c3a7e, "_printk" },
	{ 0x6870d8c2, "scsi_device_lookup" },
	{ 0x1000e51, "schedule" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x441d0de9, "__kmalloc_large_noprof" },
	{ 0xa2ac3cab, "scsi_device_put" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0xd4ed03c0, "scsi_host_put" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x4b873931, "device_create" },
	{ 0x69b07fa1, "class_create" },
	{ 0xd9f8c44d, "pci_get_domain_bus_and_slot" },
	{ 0x7b549f2d, "dma_alloc_attrs" },
	{ 0x3b30f771, "scsi_scan_host" },
	{ 0xde80cd09, "ioremap" },
	{ 0x449ad0a7, "memcmp" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0xfb578fc5, "memset" },
	{ 0xba9a7a85, "pci_set_master" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xbafcbfe5, "dma_set_coherent_mask" },
	{ 0x280f46e6, "dma_free_attrs" },
	{ 0x999e8297, "vfree" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xd0a6c742, "pci_release_regions" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0xfbe215e4, "sg_next" },
	{ 0xa5cca762, "__register_chrdev" },
	{ 0x3afad9df, "device_destroy" },
	{ 0xd8ba0fc0, "__kmalloc_cache_noprof" },
	{ 0x56470118, "__warn_printk" },
	{ 0x1e157ece, "pci_disable_device" },
	{ 0x5ce00e6a, "scsi_done" },
	{ 0x873aa76c, "dma_set_mask" },
	{ 0xd2487cec, "scsi_remove_device" },
	{ 0x396c5d1f, "scsi_add_host_with_dma" },
	{ 0x4c2e99e7, "scsi_host_alloc" },
	{ 0xfde38f2e, "__scsi_add_device" },
	{ 0xb5b54b34, "_raw_spin_unlock" },
	{ 0xf9a482f9, "msleep" },
	{ 0xe2c17b5d, "__SCT__might_resched" },
	{ 0xec751900, "pci_write_config_dword" },
	{ 0xf18a3a1b, "kmalloc_caches" },
	{ 0x2d3385d3, "system_wq" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xa660cc0e, "scsi_dma_map" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x9e7d6bd0, "__udelay" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xae4c7830, "pci_enable_device" },
	{ 0xbf1981cb, "module_layout" },
};

MODULE_INFO(depends, "scsi_mod");

MODULE_ALIAS("pci:v0000CAFEd0000BABEsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000014D6d00006101sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000014D6d00006201sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000014D6d00006232sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000014D6d00006300sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000014D6d00006233sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000014D6d00006235sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000014D6d00006240sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000014D6d00006262sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00008508sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00008608sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00008608sv000014D6sd00008608bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d0000512Dsv000014D6sd0000512Dbc*sc*i*");
MODULE_ALIAS("pci:v00001AB6d00008608sv00001AB6sd00008608bc*sc*i*");

MODULE_INFO(srcversion, "67CD1D279AF1DCCC6699B58");
