/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: menu.c,v 1.2 2003/10/06 15:15:03 iceblink Exp $
 */
/*
 * NOTE: 
 * With the menu stuff, you'll see some bizarre C syntax.
 * This strange syntax is supported properly by GCC, as well as the new
 * C99 standard.  They're called "compound literals".  
 *
 * The union field-specific initializer is a GCC specific feature.
 */

#include <printf.h>
#include <string.h>

#include "common/rom.h"
#include "common/menu.h"

#include "menu.h"
#include "boot.h"
#include "cache.h"
#include "cmos.h"
#include "eeprom.h"
#include "fs.h"
#include "gdb.h"
#include "i2c.h"
#include "ide.h"
#include "lcd.h"
#include "memory.h"
#include "pci.h"
#include "rtc.h"
#include "eth.h"
#include "microcode.h"
#include "smp.h"

/* to keep compiler happy */
menu_item main_menu; 
menu_item boot_menu; 
menu_item eeprom_menu; 
menu_item pci_menu;
menu_item io_menu;
menu_item cmos_menu;
menu_item mem_menu;
menu_item lowlevel_menu; 
menu_item test_menu;
menu_item lcd_cfg_menu;
menu_item lcdboot_menu;
menu_item lcd_bfd_menu;
menu_item smp_menu;

/* a separating item - does nothing */
menu_item separator = {
	"------------------------------------------------------", NULL,
	NULL, MENU_TYPE_NULL, {func: NULL}, NULL
};

/* global commands */
menu_item bfr_cmd = {
	"bfr", NULL, 
	"Load kernel image from romfs and boot", 
	MENU_TYPE_CMD, {func: bfr_func}, NULL
};

menu_item bfd_cmd = {
	"bfd", NULL,
	"Load kernel image from disk and boot", 
	MENU_TYPE_CMD, {func: bfd_func}, NULL
};

menu_item bfn_cmd = {
	"bfn", NULL, 
	"Boot from network",
	MENU_TYPE_CMD, {func: bfn_func}, NULL
};

menu_item bfX_cmd = {
	"bfx", "(d|r|n|x) (d|n|x)", 
	"Load kernel from <x> with root fs <y>",
	MENU_TYPE_CMD, {func: bfX_func}, NULL
};

menu_item reboot_cmd = {
	"reboot", NULL, 
	"Reboot the system now", 
	MENU_TYPE_CMD, {func: reboot_func}, NULL
};

menu_item halt_cmd = {
	"halt", NULL,
	"Halt the system now",
	MENU_TYPE_CMD, {func: halt_func}, NULL
};

menu_item quit_cmd = {
	"quit", NULL, 
	"Quit and boot default method",
	MENU_TYPE_CMD, {func: boot_default_func}, NULL
};

#ifdef REMOTE_GDB
menu_item debug_cmd = {
	"debug", NULL, 
	"Enter remote debugging",
	MENU_TYPE_CMD, {func: debug_func}, NULL
};
#endif


/*
 *  this defines the Main Menu
 */
menu_item *main_menu_items[] = {
	&boot_menu,
	&eeprom_menu,
	&lowlevel_menu,
	&test_menu,
	&bfd_cmd,
	&bfr_cmd,
	&bfn_cmd,
	&bfX_cmd,
#ifdef REMOTE_GDB
	&debug_cmd,
#endif
	&reboot_cmd,
	&halt_cmd,
	&quit_cmd,
	NULL
};
menu_item main_menu = {
	"main",				/* cmd */
	"Main Menu",			/* title */
	"Back to main menu",		/* help */
	MENU_TYPE_MENU,			/* type */
	{items: main_menu_items},	/* action */
	&main_menu			/* parent */
};


/*
 *  this defines the Boot Menu
 */
menu_item *boot_menu_items[] = {
	&(menu_item){ 
		"load_kernel", NULL,
		"Load/Decompress/Boot kernel",
		MENU_TYPE_CMD, {func: load_kernel_func}, NULL 
	},
	&(menu_item){ 
		"dl_kernel", NULL,
		"Load kernel from serial port into RAM", 	
		MENU_TYPE_CMD, {func: dl_kernel_func}, NULL 
	},
	&(menu_item){ 
		"bunzip2_kernel", NULL,
		"Decompress the kernel in RAM",
		MENU_TYPE_CMD, {func: bunzip2_kernel_func}, NULL 
	},
	&(menu_item){ 
		"boot_kernel", NULL,
		"Boot the kernel from RAM",
		MENU_TYPE_CMD, {func: boot_kernel_func}, NULL 
	},
	&(menu_item){ 
		"net_kernel", NULL,
		"Net boot the kernel in RAM",
		MENU_TYPE_CMD, {func: net_kernel_func}, NULL 
	},
	&bfr_cmd,
	&bfd_cmd,
	&bfn_cmd,
	&bfX_cmd,
	&(menu_item){ 
		"load", "file",
		"Load a file from romfs",
		MENU_TYPE_CMD, {func: load_func}, NULL 
	},
	&(menu_item){ 
		"set_params", "parameters",
		"Set parameters to pass to the boot kernel",
		MENU_TYPE_CMD, {func: set_params_func}, NULL 
	},
	&(menu_item){ 
		"set_lparams", "parameters",
		"Set parameters to pass to the load kernel",
		MENU_TYPE_CMD, {func: set_load_params_func}, NULL 
	},
	&(menu_item){ 
		"read_boot_type", NULL,
		"Read default boot method",
		MENU_TYPE_CMD, {func: read_boot_method_func}, NULL 
	},
	&(menu_item){ 
		"set_boot_type", "(disk|rom|net|netkernel|other)",
		"Configure default boot method",
		MENU_TYPE_CMD, {func: set_boot_method_func}, NULL 
	},
	&(menu_item){ 
		"read_root_dev", NULL,
		"Read default root device for disk boot",
		MENU_TYPE_CMD, {func: read_root_dev_func}, NULL 
	},
	&(menu_item){ 
		"set_root_dev", "device",
		"Set default root device for disk boot (e.g.: hda1)",
		MENU_TYPE_CMD, {func: set_root_dev_func}, NULL 
	},
	&(menu_item){ 
		"read_boot_dev", NULL,
		"Read default root device for ROM kernel",
		MENU_TYPE_CMD, {func: read_boot_dev_func}, NULL 
	},
	&(menu_item){ 
		"set_boot_dev", "device",
		"Set default root device for ROM kernel (e.g.: hda1)",
		MENU_TYPE_CMD, {func: set_boot_dev_func}, NULL 
	},
	&(menu_item){ 
		"set_mem_limit", "megabytes",
		"Set memory limit in MB for linux",
		MENU_TYPE_CMD, {func: set_mem_limit_func}, NULL 
	},
	&(menu_item){ 
		"netbsd", "image",
		"Boot NetBSD",
		MENU_TYPE_CMD, {func: boot_netbsd_func}, NULL 
	},
	&reboot_cmd,
	&halt_cmd,
	&separator,
	&main_menu,
	NULL
};
menu_item boot_menu = {
	"boot",				/* cmd */
	"Boot Menu",			/* title */
	"Loading and booting",		/* help */
	MENU_TYPE_MENU,			/* type */
	{items: boot_menu_items},	/* action */
	&main_menu			/* parent */
};


/*
 *  this defines the EEPROM Menu
 */
menu_item *eeprom_menu_items[] = {
	&(menu_item){ 
		"display_romid", NULL,
		"Display the EEPROM id",
		MENU_TYPE_CMD, {func: display_romid_func}, NULL 
	},
	&(menu_item){ 
		"set_bank", "bank page",
		"Set EEPROM bank and page",
		MENU_TYPE_CMD, {func: set_bank_func}, NULL 
	},
	&(menu_item){ 
		"erase_eeprom", "bank",
		"Erase EEPROM in bank",
		MENU_TYPE_CMD, {func: erase_eeprom_func}, NULL 
	},
	&(menu_item){ 
		"write_eeprom", "[bank] [len]",
		" 1. load image from serial port\n"
		" 2. erase EEPROM bank (default 0)\n"
		" 3. write image to EEPROM",
		MENU_TYPE_CMD, {func: write_eeprom_func}, NULL 
	},
	&(menu_item){ 
		"read_eeprom", "bank [len]",
		"Read data from the EEPROM in bank into RAM",
		MENU_TYPE_CMD, {func: read_eeprom_func}, NULL 
	},
	&(menu_item){ 
		"ls", "dir",
		"List contents of romfs dir",
		MENU_TYPE_CMD, {func: ls_func}, NULL 
	},
	&(menu_item){ 
		"reinit_eeprom", NULL,
		"Re-detect and initialize EEPROMs",
		MENU_TYPE_CMD, {func: reinit_eeprom_func}, NULL 
	},
	&separator,
	&main_menu,
	NULL
};
menu_item eeprom_menu = {
	"eeprom",			/* cmd */
	"EEPROM Menu",			/* title */
	"EEPROM commands",		/* help */
	MENU_TYPE_MENU,			/* type */
	{items: eeprom_menu_items},	/* action */
	&main_menu			/* parent */
};


/*
 *  this defines the Lowlevel -> Memory Menu
 */
menu_item *mem_menu_items[] = {
	&(menu_item){ 
		"rb", "address",
		"Read byte from memory",
		MENU_TYPE_CMD, {func: read_mem_func}, NULL
	},
	&(menu_item){ 
		"rw", "address",
		"Read 16 bit word from memory",
		MENU_TYPE_CMD, {func: read_mem_func}, NULL
	},
	&(menu_item){
		"rl", "address",
		"Read 32 bit word from memory", 
		MENU_TYPE_CMD, {func: read_mem_func}, NULL
	},
	&(menu_item){
		"wb", "address data",
		"Write byte to memory", 		
		MENU_TYPE_CMD, {func: write_mem_func}, NULL
	},
	&(menu_item){
		"ww", "address data",
		"Write 16 bit word to memory", 		
		MENU_TYPE_CMD, {func: write_mem_func}, NULL
	},
	&(menu_item){
		"wl", "address data",
		"Write 32 bit word to memory", 		
		MENU_TYPE_CMD, {func: write_mem_func}, NULL
	},
	&(menu_item){
		"wm", "address data count",
		"Write 32-bit data count time to memory, starting at address",
		MENU_TYPE_CMD, {func: write_mem_func}, NULL
	},
	&(menu_item){
		"dm", "address count",
		"Display count bytes of memory",		
		MENU_TYPE_CMD, {func: disp_mem_func}, NULL
	},
	&(menu_item){
		"cp", "dest src count",
		"Copy count bytes of memory from src to dest",		
		MENU_TYPE_CMD, {func: copy_mem_func}, NULL
	},
	&(menu_item){
		"mv", "dest src count",
		"Move count bytes of memory from src to dest",		
		MENU_TYPE_CMD, {func: move_mem_func}, NULL
	},
	&(menu_item){
		"disp_mem_eeprom", "row",
		"Display eeprom of DIMM in the row", 
		MENU_TYPE_CMD, {func: disp_mem_eeprom_func}, NULL
	},
	&(menu_item){
		"enl1", NULL,
		"Turn on L1 cache now",
		MENU_TYPE_CMD, {func: enl1_func}, NULL
	},
	&(menu_item){
		"enl2", NULL,
		"Turn on L2 cache now", 
		MENU_TYPE_CMD, {func: enl2_func}, NULL
	},
	&(menu_item){
		"set_caches", "(0|1) (0|1)",
		"Flag L1 and L2 caches for being enabled next boot", 
		MENU_TYPE_CMD, {func: set_caches_func}, NULL
	},
	&separator,
	&lowlevel_menu,
	&main_menu,
	NULL
};
menu_item mem_menu = {
	"mem",				/* cmd */
	"Memory Menu",			/* title */
	"Memory commands",		/* help */
	MENU_TYPE_MENU,			/* action */
	{items: mem_menu_items},	/* action */
	&lowlevel_menu			/* parent */
};


/*
 *  this defines the Lowlevel -> I/O Menu
 */
menu_item *io_menu_items[] = {
	&(menu_item){ 
		"inb", "port",
		"Read byte from IO port",
		MENU_TYPE_CMD, {func: inbwl_func}, NULL 
	},
	&(menu_item){ 
		"inw", "port",
		"Read 16 bit word from IO port",
		MENU_TYPE_CMD, {func: inbwl_func}, NULL 
	},
	&(menu_item){ 
		"inl", "port",
		"Read 32 bit word from IO port",
		MENU_TYPE_CMD, {func: inbwl_func}, NULL 
	},
	&(menu_item){ 
		"outb", "port data",
		"Write byte to IO port",
		MENU_TYPE_CMD, {func: outbwl_func}, NULL 
	},
	&(menu_item){ 
		"outw", "port data",
		"Write 16 bit word to IO port",
		MENU_TYPE_CMD, {func: outbwl_func}, NULL 
	},
	&(menu_item){ 
		"outl", "port data",
		"Write 32 bit word to IO port",
		MENU_TYPE_CMD, {func: outbwl_func}, NULL 
	},
	&(menu_item){ 
		"read_i2c", "dev index len",
		"Read from i2c device dev at index",
		MENU_TYPE_CMD, {func: read_i2c_func}, NULL 
	},
	&(menu_item){ 
		"write_i2c", "dev index len val",
		"Write to i2c device at index",
		MENU_TYPE_CMD, {func: write_i2c_func}, NULL 
	},
	&separator,
	&lowlevel_menu,
	&main_menu,
	NULL
};
menu_item io_menu = {
	"io",				/* cmd */
	"I/O Port Menu",		/* title */
	"I/O Port commands",		/* help */
	MENU_TYPE_MENU,			/* action */
	{items: io_menu_items},		/* action */
	&lowlevel_menu			/* parent */
};


/*
 *  this defines the Lowlevel -> CMOS Menu
 */
menu_item *cmos_menu_items[] = {
	&(menu_item){ 
		"reset", NULL,
		"Reset CMOS defaults",
		MENU_TYPE_CMD, {func: cmos_reset_func}, NULL 
	},
	&(menu_item){ 
		"read_cmos", "bytenum",
		"Read CMOS byte bytenum",
		MENU_TYPE_CMD, {func: read_cmos_func}, NULL 
	},
	&(menu_item){ 
		"write_cmos", "bytenum val",
		"Write value to CMOS byte",
		MENU_TYPE_CMD, {func: write_cmos_func}, NULL 
	},
	&(menu_item){ 
		"dump_cmos", NULL,
		"Dump the contents of CMOS",
		MENU_TYPE_CMD, {func: dump_cmos_func}, NULL 
	},
	&(menu_item){ 
		"check_cmos_cks", NULL,
		"Check the validity of the CMOS checksum",
		MENU_TYPE_CMD, {func: check_cmos_cks_func}, NULL 
	},
	&(menu_item){ 
		"update_cmos_cks", NULL,
		"Update CMOS checksum",
		MENU_TYPE_CMD, {func: update_cmos_cks_func}, NULL 
	},
	&(menu_item){ 
		"cmos_ver", NULL,
		"Print CMOS version",
		MENU_TYPE_CMD, {func: cmos_ver_func}, NULL 
	},
	&(menu_item){ 
		"bto_code", "<code>",
		"Query/set BTO code",
		MENU_TYPE_CMD, {func: bto_code_func}, NULL 
	},
	&(menu_item){ 
		"btoip", "oct1 oct2 oct3 oct4",
		"Set the bto IP by octest",
		MENU_TYPE_CMD, {func: bto_ip_func}, NULL 
	},
	&(menu_item){ 
		"cmos_flags", "<flag> [on|off]",
		"Query/Set CMOS flags, w/o args will list flags",
		MENU_TYPE_CMD, {func: cmos_flags_func}, NULL 
	},
	&separator,
	&lowlevel_menu,
	&main_menu,
	NULL
};
menu_item cmos_menu = {
	"cmos",				/* cmd */
	"CMOS Menu",			/* title */
	"CMOS commands",		/* help */
	MENU_TYPE_MENU,			/* action */
	{items: cmos_menu_items},	/* action */
	&lowlevel_menu			/* parent */
};


/*
 *  this defines the Lowlevel -> PCI Menu
 */
menu_item *pci_menu_items[] = {
	&(menu_item){ 
		"lspci", NULL,
		"List PCI devices",	
		MENU_TYPE_CMD, {func: lspci_func}, NULL
	},
	&(menu_item){ 
		"read_cfg", "bus dev fn reg (1|2|4)",
		"Read 1,2,4 bytes from PCI config space",	
		MENU_TYPE_CMD, {func: read_pci_cfg_func}, NULL
	},
	&(menu_item){ 
		"write_cfg", "bus dev fn reg val (1|2|4)",
		"Write 1,2,4 bytes to PCI config space",
		MENU_TYPE_CMD, {func: write_pci_cfg_func}, NULL
	},
	&(menu_item){ 
		"print_hdr", "bus dev fn",
		"Print formatted PCI config header",
		MENU_TYPE_CMD, {func: print_pci_header_func}, NULL
	},
	&(menu_item){ 
		"dump", "bus dev fn",
		"Dump PCI config space", 	
		MENU_TYPE_CMD, {func: dump_pci_func}, NULL
	},
	&(menu_item){ 
		"dump_all", NULL,
		"Dump PCI config space for all devs", 
		MENU_TYPE_CMD, {func: dump_all_pci_func}, NULL
	},
	&separator,
	&lowlevel_menu,
	&main_menu,
	NULL
};
menu_item pci_menu = {
	"pci",				/* cmd */
	"PCI Menu",			/* title */
	"PCI commands",			/* help */
	MENU_TYPE_MENU,			/* action */
	{items: pci_menu_items},	/* action */
	&lowlevel_menu			/* parent */
};


/*
 *  this defines the Lowlevel -> SMP Menu
 */
menu_item *smp_menu_items[] = {
	&(menu_item){ 
		"lapic_get", "reg",
		"Get Local APIC register",
		MENU_TYPE_CMD, {func: lapic_get_func}, NULL
	},
	&(menu_item){ 
		"lapic_set", "reg val",
		"Set Local APIC register to val",
		MENU_TYPE_CMD, {func: lapic_set_func}, NULL
	},
	&(menu_item){ 
		"lapic_on", NULL,
		"Enable the Local APIC",
		MENU_TYPE_CMD, {func: lapic_on_func}, NULL
	},
	&(menu_item){ 
		"lapic_off", NULL,
		"Disable the Local APIC",
		MENU_TYPE_CMD, {func: lapic_off_func}, NULL
	},
	&(menu_item){ 
		"send_ipi", "dest mode vector trigger level",
		"Send an interprocessor interrupt to APIC ID",
		MENU_TYPE_CMD, {func: lapic_send_ipi_func}, NULL
	},
	&(menu_item){ 
		"broadcast_ipi", "dest mode vector trigger level",
		"Broadcast an interprocessor interrupt",
		MENU_TYPE_CMD, {func: lapic_cast_ipi_func}, NULL
	},
	&(menu_item){ 
		"ioapic_get", "num reg",
		"Get I/O APIC register",
		MENU_TYPE_CMD, {func: ioapic_get_func}, NULL
	},
	&(menu_item){ 
		"ioapic_set", "num reg val",
		"Set I/O APIC register to val",
		MENU_TYPE_CMD, {func: ioapic_set_func}, NULL
	},
	&separator,
	&lowlevel_menu,
	&main_menu,
	NULL
};
menu_item smp_menu = {
	"smp",				/* cmd */
	"SMP Menu",			/* title */
	"SMP commands",			/* help */
	MENU_TYPE_MENU,			/* action */
	{items: smp_menu_items},	/* action */
	&lowlevel_menu			/* parent */
};


/*
 *  this defines the Lowlevel Menu
 */
menu_item *lowlevel_menu_items[] = {
	&(menu_item){
		"read_voltage", NULL,
		"Read the processor voltage",
		MENU_TYPE_CMD, {func: read_voltage_func}, NULL
	},
	&(menu_item){
		"read_temp", NULL,
		"Read the processor temperature",
		MENU_TYPE_CMD, {func: read_temp_func}, NULL
	},
	&(menu_item){
		"read_thresh", NULL,
		"Read the temperature alert thresholds",
		MENU_TYPE_CMD, {func: read_temp_thresh_func}, NULL
	},
	&(menu_item){
		"write_thresh", "(hyst|low|high|crit) val",
		"Write the temperature alert thresholds",
		MENU_TYPE_CMD, {func: write_temp_thresh_func}, NULL
	},
	&(menu_item){
		"read_rtc", NULL,
		"Read value of the RTC",
		MENU_TYPE_CMD, {func: read_rtc_func}, NULL
	},
	&(menu_item){
		"write_rtc", "YYMMDDWHHmmss",
		"Write value to the RTC",
		MENU_TYPE_CMD, {func: write_rtc_func}, NULL
	},
	&(menu_item){
		"lcd", "(on|off|init|reset|clear)",
		"Control the system LCD",
		MENU_TYPE_CMD, {func: lcd_func}, NULL
	},
	&(menu_item){
		"read_mac", NULL,
		"Read hardware ethernet address",
		MENU_TYPE_CMD, {func: read_mac_func}, NULL
	},
	&(menu_item){
		"write_mac", "port d1 d2 d3 d4 d5 d6",
		"Write hardware ethernet address",
		MENU_TYPE_CMD, {func: write_mac_func}, NULL
	},
	&(menu_item){
		"update_microcode", NULL,
		"Update the CPU microcode",
		MENU_TYPE_CMD, {func: microcode_func}, NULL
	},
	&(menu_item){
		"cpuid", "level",
		"Execute CPUID query with specified level",
		MENU_TYPE_CMD, {func: cpuid_func}, NULL
	},
	&(menu_item){
		"read_msr", "index",
		"Reads a model specific register",
		MENU_TYPE_CMD, {func: read_msr_func}, NULL
	},
	&(menu_item){
		"write_msr", "index hi_word lo_word",
		"Writes a model specific register",
		MENU_TYPE_CMD, {func: write_msr_func}, NULL
	},
	&(menu_item){
	    	"enable_nmi", NULL,
		"Enable parity error NMI",
		MENU_TYPE_CMD, {func: nmi_enable_func}, NULL
	},
	&(menu_item){
	    	"disable_nmi", NULL,
		"Disable parity error NMI",
		MENU_TYPE_CMD, {func: nmi_disable_func}, NULL
	},
	&(menu_item){ 
		"hlt", NULL,
		"Put the CPU into sleep mode (halt system)",
		MENU_TYPE_CMD, {func: hlt_func}, NULL 
	},
	&separator,
	&smp_menu,
	&pci_menu,
	&mem_menu,
	&io_menu,
	&cmos_menu,
	&main_menu,
	NULL
};
menu_item lowlevel_menu = {
	"lowlevel",			/* cmd */
	"Lowlevel Hardware Menu",	/* title */
	"Lowlevel hardware commands",	/* help */
	MENU_TYPE_MENU,			/* type */
	{items: lowlevel_menu_items},	/* action */
	&main_menu			/* parent */
};


/*
 *  this defines the test menu
 */
menu_item *test_menu_items[] = {
	&(menu_item){
		"read_test", "(1|2|4) addr",
		"Continuously read from an address",
		MENU_TYPE_CMD, {func: read_test_func}, NULL
	},
	&(menu_item){
		"write_test", "(1|2|4) addr data",
		"Continuously write to an address",
		MENU_TYPE_CMD, {func: write_test_func}, NULL
	},
	&(menu_item){
		"mem_test", NULL,
		"Simple mem test",
		MENU_TYPE_CMD, {func: mem_test_func}, NULL
	},
	&(menu_item){
		"init_delay", NULL,
		"Recalibrate udelay and CPU MHz",
		MENU_TYPE_CMD, {func: init_delay_func}, NULL
	},
	&separator,
	&main_menu,
	NULL
};
menu_item test_menu = {
	"test",				/* cmd */
	"Test Menu",			/* title */
	"Tests and validation",		/* help */
	MENU_TYPE_MENU,			/* action */
	{items: test_menu_items},	/* action */
	&main_menu			/* parent */
};


/*
 *  this defines the lcd boot menu
 */
menu_item *lcdboot_menu_items[] = {
	&(menu_item){ 
		"Boot from disk", NULL, NULL,
		MENU_TYPE_CMD, {func: bfd_func}, NULL 
	},
	&(menu_item){ 
		"Boot from ROM", NULL, NULL,
		MENU_TYPE_CMD, {func: bfr_func}, NULL 
	},
	&(menu_item){ 
		"Boot from net", NULL, NULL,
		MENU_TYPE_CMD, {func: bfn_func}, NULL 
	},
	&(menu_item){ 
		"Boot net kernel", NULL, NULL,
		MENU_TYPE_CMD, {func: bfnn_func}, NULL 
	},
	&(menu_item){ 
		"Boot from CMOS", NULL, NULL,
		MENU_TYPE_CMD, {func: bfX_func}, NULL 
	},
	&lcd_cfg_menu,
	&lcd_bfd_menu,
	&(menu_item){ 
		"System Info", NULL, NULL,
		MENU_TYPE_CMD, {func: lcd_sysinfo_func}, NULL 
	},
	NULL
};
menu_item lcdboot_menu = {
	"Main menu",			/* cmd */
	"Select option:",		/* title */
	NULL,				/* help */
	MENU_TYPE_MENU,			/* type */
	{items: lcdboot_menu_items},	/* action */
	&lcdboot_menu
};


menu_item *lcd_cfg_menu_items[] = {
	&(menu_item){
		"From disk", NULL, NULL,
		MENU_TYPE_CMD, {func: set_boot_method_func}, NULL
	},
	&(menu_item){
		"From ROM", NULL, NULL,
		MENU_TYPE_CMD, {func: set_boot_method_func}, NULL
	},
	&(menu_item){
		"From network", NULL, NULL,
		MENU_TYPE_CMD, {func: set_boot_method_func}, NULL
	},
	&(menu_item){
		"From net kernel", NULL, NULL,
		MENU_TYPE_CMD, {func: set_boot_method_func}, NULL
	},
	&(menu_item){
		"From settings", NULL, NULL,
		MENU_TYPE_CMD, {func: set_boot_method_func}, NULL
	},
	&lcdboot_menu,
	NULL
};
menu_item lcd_cfg_menu = {
	"Configure boot",			/* cmd */
	"Select default:",			/* title */
	NULL,					/* help */
	MENU_TYPE_MENU,				/* type */
	{items: lcd_cfg_menu_items},		/* action */
	&lcdboot_menu
};


menu_item *lcd_bfd_menu_items[] = {
	&(menu_item){ "hda1", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hda2", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hda3", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hda4", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdb1", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdb2", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdb3", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdb4", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdc1", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdc2", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdc3", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdc4", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdd1", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdd2", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdd3", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "hdd4", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "sda1", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "sda2", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "sda3", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "sda4", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "sdb1", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "sdb2", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "sdb3", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "sdb4", NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "md0",  NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "md1",  NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "md2",  NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&(menu_item){ "md3",  NULL, NULL, MENU_TYPE_CMD, 
		{func: set_root_dev_func}, NULL },
	&lcdboot_menu,
	NULL
};
menu_item lcd_bfd_menu = {
	"Config boot disk",			/* cmd */
	"Select boot disk:",			/* title */
	NULL,					/* help */
	MENU_TYPE_MENU,				/* type */
	{items: lcd_bfd_menu_items},		/* action */
	&lcdboot_menu
};

void menu_do_console(void)
{
    menu_handle_console(&main_menu);
}

void menu_do_lcd(void)
{
    menu_handle_lcd(&lcdboot_menu);
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
