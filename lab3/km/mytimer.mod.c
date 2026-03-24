#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
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
__used
__attribute__((section("__versions"))) = {
	{ 0xa16cf51b, "module_layout" },
	{ 0xf48ec189, "single_release" },
	{ 0xc8b8082b, "seq_read" },
	{ 0xc79cde0d, "seq_lseek" },
	{ 0xb4d05a35, "remove_proc_entry" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x5f754e5a, "memset" },
	{ 0x40993e6d, "proc_create" },
	{ 0x1f6e82a4, "__register_chrdev" },
	{ 0xbc64391d, "seq_printf" },
	{ 0x18b697c2, "single_open" },
	{ 0xf4fa543b, "arm_copy_to_user" },
	{ 0x91715312, "sprintf" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0x3f95a91f, "__get_task_comm" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x9d669763, "memcpy" },
	{ 0xd9ce8f0c, "strnlen" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x526c3a6c, "jiffies" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x97934ecf, "del_timer_sync" },
	{ 0x20c55ae0, "sscanf" },
	{ 0x28cc25db, "arm_copy_from_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x8373a021, "kill_fasync" },
	{ 0x10942ff7, "fasync_helper" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

