#include <linux/module.h>
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
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
__used __section(__versions) = {
	{ 0x3abb055f, "module_layout" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x6f7b80a9, "class_destroy" },
	{ 0xfc10a0a3, "platform_driver_unregister" },
	{ 0x73797020, "cdev_del" },
	{ 0x547839f9, "device_destroy" },
	{ 0x1469d0e5, "__platform_driver_register" },
	{ 0xf6456db9, "__raw_spin_lock_init" },
	{ 0xf6d1dfa, "device_create" },
	{ 0x451fbe65, "cdev_add" },
	{ 0xe19c3845, "cdev_init" },
	{ 0xaa5a681d, "__class_create" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0xacbb36ba, "kmem_cache_alloc_trace" },
	{ 0x48ec6627, "kmalloc_caches" },
	{ 0xfb578fc5, "memset" },
	{ 0x4c002e66, "wake_up_process" },
	{ 0xcbc270a1, "kthread_create_on_node" },
	{ 0x1000e51, "schedule" },
	{ 0xb8b9f817, "kmalloc_order_trace" },
	{ 0x37a0cba, "kfree" },
	{ 0x95e1b5f7, "_raw_spin_lock" },
	{ 0xc5850110, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "CC31F14067CE6A310A44775");
