#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
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

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif


static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x624c3e2c, "scsi_dma_map" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x9e7d6bd0, "__udelay" },
	{ 0x730430e3, "__class_create" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x195e512d, "pci_enable_device" },
	{ 0xb0e602eb, "memmove" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x3f348b0c, "scsi_remove_host" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x2b03b7a, "class_destroy" },
	{ 0x5833d26b, "__pci_register_driver" },
	{ 0xedc03953, "iounmap" },
	{ 0x6488d804, "pci_request_regions" },
	{ 0x69acdf38, "memcpy" },
	{ 0x4de4c8c3, "scsi_dma_unmap" },
	{ 0x37a0cba, "kfree" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0xe2964344, "__wake_up" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0x9332ccd7, "pci_unregister_driver" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x53e74d4f, "pci_read_config_dword" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x92997ed8, "_printk" },
	{ 0xc2c1f73c, "scsi_device_lookup" },
	{ 0x1000e51, "schedule" },
	{ 0xa19b956, "__stack_chk_fail" },
	{ 0x27d5719e, "scsi_device_put" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0xc8d5f15a, "scsi_host_put" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x5bad400, "device_create" },
	{ 0x32186161, "pci_get_domain_bus_and_slot" },
	{ 0x50b361c6, "dma_alloc_attrs" },
	{ 0xd580433b, "scsi_scan_host" },
	{ 0xde80cd09, "ioremap" },
	{ 0x449ad0a7, "memcmp" },
	{ 0x9ed12e20, "kmalloc_large" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0xfb578fc5, "memset" },
	{ 0x4c58a87c, "pci_set_master" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xf7afb466, "dma_set_coherent_mask" },
	{ 0x97c35814, "dma_free_attrs" },
	{ 0x999e8297, "vfree" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x4b48744b, "pci_release_regions" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0x87b8798d, "sg_next" },
	{ 0x87c56589, "__register_chrdev" },
	{ 0xd6f8cf3e, "device_destroy" },
	{ 0x56470118, "__warn_printk" },
	{ 0x372368c2, "pci_disable_device" },
	{ 0xe76deb02, "scsi_done" },
	{ 0xff95dfce, "dma_set_mask" },
	{ 0x52dd86bf, "kmalloc_trace" },
	{ 0x6b945da8, "scsi_remove_device" },
	{ 0xc0d3fb0a, "scsi_add_host_with_dma" },
	{ 0x44a1e47d, "scsi_host_alloc" },
	{ 0x68a1c358, "__scsi_add_device" },
	{ 0xb5b54b34, "_raw_spin_unlock" },
	{ 0xf9a482f9, "msleep" },
	{ 0xe2c17b5d, "__SCT__might_resched" },
	{ 0x52fb7da0, "pci_write_config_dword" },
	{ 0xc80d0404, "kmalloc_caches" },
	{ 0x2d3385d3, "system_wq" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x160c03af, "module_layout" },
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

MODULE_INFO(srcversion, "4D77E9B1B7AFDEEEAF9A61C");
