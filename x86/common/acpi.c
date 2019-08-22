/*
 * ACPI support. 
 * Copyright (c) 2000 Cobalt Networks, Inc.
 *
 * author: asun@cobalt.com
 *
 * on IA-PC systems, the Root System Description Pointer (RSDP) is
 * located on a 16-byte boundary in one of the following places:
 *   1) the first 1KB of the extended bios data area.
 *   2) ROM address between 0xE0000 - 0xFFFFF.
 *
 * although only the RSDP really needs to be in low memory, i stick
 * the other tables there as well.
 *
 * this eventually needs to deal with APIC support.  
 *
 * this handles both the Serverworks OSB4 and the National SuperI/O.
 *
 * steps:
 * 1) initialize pnp registers
 * 2) allocate space (2 bytes) for each logical device. 
 * 3) select logical devices using SUPERIO_DEV.
 * 4) allocate 2 bytes of io space for each logical device needed. assign
 *    these using PNP_NS_ADDR_LOW/HIGH. activate the device.
 * 5) once this is done, you can index into the logical device using the
 *    base addresses.
 *    -- NOTE: you need to select the bank for the RTC/APC.
 */

#include <string.h>
#include <io.h>

#include "common/isapnp.h"
#include "common/cmos.h"
#include "common/memory.h"
#include "common/region.h"
#include "common/rom.h"
#include "common/pci.h"
#include "common/systype.h"
#include "common/rtc.h"
#include "common/pirq.h"
#include "common/acpi.h"

#define SETUP_TABLES      1
#define SUPERIO_IRQ_REDIRECT  15

/* each of these represents the first of a pair of registers. e.g., 
 * 0x20 -> 0x20/0x21 registers. the first one has the low order bits
 * while the second has the high order bits. */
#define PM1_EVT_REG       0x20
#define PM1_CNT_REG       0x22
#define PM_TMR_REG        0x24
#define P_CNT_REG         0x26
#define GPE0_BLK_REG      0x28

/* plug-n-play registers */
#define SUPERIO_SIOC1      0x21
#define SUPERIO_SIOC2      0x22
#define SUPERIO_SIOC3      0x25

#define SUPERIO_ACPI_SUPPORT    0x10

#define SUPERIO_PM1_EVT_REG  0x08
#define SUPERIO_PM_TMR_REG   0x0a
#define SUPERIO_PM1_CNT_REG  0x0c
#define SUPERIO_GPE1_BLK_REG 0x0e

/* root system descriptor pointer (RSDP) location. we need to start
 * at a 16-byte aligned boundary. stick everything near the end */
#define ACPI_BASE         0xffe00
#define ACPI_END          0xfffff

#define ACPI_REVISION     2  

/* table signatures */
#define ACPI_RSDP1_SIG    0x20445352 /* 'RSD ' */
#define ACPI_RSDP2_SIG    0x20525450 /* 'PTR ' */

#define ACPI_RSDT_SIG     0x54445352 /* 'RSDT' */
#define ACPI_XSDT_SIG     0x54445358 /* 'XSDT' */
#define ACPI_FADT_SIG     0x50434146 /* 'FACP' */
#define ACPI_DSDT_SIG     0x54445344 /* 'DSDT' */
#define ACPI_FACS_SIG     0x53434146 /* 'FACS' */

#define ACPI_TABLE_NUM    1
#define ACPI_HEADER_SIZE  36

#define ACPI_RSDP_SIZE    ACPI_HEADER_SIZE
#define ACPI_RSDT_SIZE    (ACPI_HEADER_SIZE + 4*ACPI_TABLE_NUM)
#define ACPI_XSDT_SIZE    (ACPI_HEADER_SIZE + 8*ACPI_TABLE_NUM)
#define ACPI_FADT_SIZE    244

#define ACPI_RSDP_OFF     (ACPI_BASE) 
#define ACPI_RSDT_OFF     (ACPI_RSDP_OFF + ACPI_RSDP_SIZE)
#define ACPI_XSDT_OFF     (ACPI_RSDT_OFF + ACPI_RSDT_SIZE)
#define ACPI_FADT_OFF     (ACPI_XSDT_OFF + ACPI_XSDT_SIZE)
#define ACPI_FACS_OFF     0x0
#define ACPI_DSDT_OFF     0x0

#define ACPI_FADT_REVISION 3

#define OEMID             {'C', 'O', 'B', 'T'}
#define OEMMODELID        {'M', 'O', 'N', 'T', 'E', 'R', 'E', 'Y' }
#define OEMREVISION       1

/* not sure what should go there */
#define CREATOR_ID        (('C'<<0x0)+('O'<<0x8)+('B'<<0x10)+('T'<<0x18))
#define CREATOR_REVISION  0x01000000

#define PM_PROFILE_UNSPECIFIED  0x0
#define PM_PROFILE_DESKTOP      0x1
#define PM_PROFILE_MOBILE       0x2
#define PM_PROFILE_WORKSTATION  0x3
#define PM_PROFILE_ENTERPRISE   0x4
#define PM_PROFILE_SOHO         0x5
#define PM_PROFILE_APPLIANCE    0x6

#define PM_PROFILE_DEFAULT      PM_PROFILE_UNSPECIFIED

#define FFEATURE_FLAG_WBINVD         (1 << 0)
#define FFEATURE_FLAG_WBINVD_FLUSH   (1 << 1)
#define FFEATURE_FLAG_PROC_C1        (1 << 2)
#define FFEATURE_FLAG_P_LVL2_UP      (1 << 3)
#define FFEATURE_FLAG_PWR_BUTTON     (1 << 4)
#define FFEATURE_FLAG_SLP_BUTTON     (1 << 5)
#define FFEATURE_FLAG_FIX_RTC        (1 << 6)
#define FFEATURE_FLAG_RTC_S4         (1 << 7)
#define FFEATURE_FLAG_TMR_VAL_EXT    (1 << 8)
#define FFEATURE_FLAG_DCK_CAP        (1 << 9)
#define FFEATURE_FLAG_RESET_REG_SUP  (1 << 10)
#define FFEATURE_FLAG_SEALED_CASE    (1 << 11)
#define FFEATURE_FLAG_HEADLESS       (1 << 12)
#define FFEATURE_FLAG_CPU_SW_SLP     (1 << 13)

#define BA_FLAG_LEGACY_DEVICES       (1 << 0)
#define BA_FLAG_8042                 (1 << 1)

#define GAS_ADDR_SYSTEM_MEMORY      0
#define GAS_ADDR_SYSTEM_IO          1
#define GAS_ADDR_PCI_CONFIG         2

/* block sizes */
#define	PM1_EVT_LEN             0x4
#define	PM1_CNT_LEN             0x2 
#define	PM1_CNT_LEN_A           0x4 /* really only 2 but needs to be DWORD aligned */
#define	PM2_CNT_LEN             0x0
#define	PM_TMR_LEN              0x4
#define	P_CNT_LEN               0x6 /* really only 6 but needs to be DWORD aligned */
#define	P_CNT_LEN_A             0x8 
#define	GPE0_BLK_LEN            0x8
#define	GPE1_BLK_LEN            0x10
#define	GPE1_BASE               0x0

/* need to look at the pentium pro STPCLK# and klamath SLP# specs on
 * timings. for now, we just turn this stuff off.  */
#define	P_LVL2_LAT              0x65
#define	P_LVL3_LAT              0x3e9

#define	FLUSH_SIZE              0x0
#define	FLUSH_STRIDE            0x0
#define	DUTY_OFFSET             0x0
#define	DUTY_WIDTH              0x0

/* rtc cmos registers. turn this off for now. */
#define	DAY_ALRM                0x0
#define	MON_ALRM                0x0
#define	CENTURY                 0x0

#define FFEATURE_FLAG_DEFAULT (FFEATURE_FLAG_WBINVD | \
  FFEATURE_FLAG_WBINVD_FLUSH | FFEATURE_FLAG_PWR_BUTTON | \
  FFEATURE_FLAG_RESET_REG_SUP | FFEATURE_FLAG_HEADLESS)

#define	IAPC_BOOT_ARCH         0x0
#define	RESET_REG              { GAS_ADDR_SYSTEM_MEMORY, 0x2, 0x0, \
                                 0x0, 0x0cf9 }
#define	RESET_VAL              (byte) 0x06


typedef struct rsdp_table_s {
	dword sig1, sig2;
	byte checksum;
	byte oem_id[6];
	byte revision;
	dword rsdt_addr;
	dword length;
	qword xdst_addr;
	byte extended_csum;
	byte reserved[3];
} __attribute__ ((packed)) rsdp_table_t;

typedef struct sdth_table_s {
	dword signature;
	dword length;
	byte revision;
	byte checksum;
	byte oem_id[6];
	byte oem_tableid[8];
	dword oem_revision;
	dword creator_id;
	dword creator_revision;
} __attribute__ ((packed)) sdth_table_t;

typedef struct rsdt_table_s {
	sdth_table_t table;
	dword entries[ACPI_TABLE_NUM];
} __attribute__ ((packed)) rsdt_table_t;

typedef struct xsdt_table_s {
	sdth_table_t table;
	qword entries[ACPI_TABLE_NUM];
} __attribute__ ((packed)) xsdt_table_t;

typedef struct gas_addr_s {
	byte addr_space_id;
	byte register_bit_width;
	byte register_bit_offset;
	byte reserved;
	qword address;
} __attribute__ ((packed)) gas_addr_t;

typedef struct fadt_table_s {
	sdth_table_t table;
	dword facs_addr;
	dword dsdt_addr;
	byte reserved0;
	byte preferred_pm_profile;
	word sci_int;
	dword smi_cmd;
	byte acpi_enable;
	byte acpi_disable;
	byte s4bios_req;
	byte pstate_cnt;

	/* acpi 1.0 compatible fields: */
	dword pm1a_evt_blk; 
	dword pm1b_evt_blk; 
	dword pm1a_cnt_blk;
	dword pm1b_cnt_blk;
	dword pm2_cnt_blk;
	dword pm_tmr_blk;
	dword gpe0_blk;
	dword gpe1_blk;

	byte pm1_evt_len;
	byte pm1_cnt_len;
	byte pm2_cnt_len;
	byte pm_tmr_len;
	byte gpe0_blk_len;
	byte gpe1_blk_len;
	byte gpe1_base;
	byte cst_cnt;
	word p_lvl2_lat;
	word p_lvl3_lat;
	word flush_size;   /* acpi 1.0 */
	word flush_stride; /* acpi 1.0 */
	byte duty_offset; 
	byte duty_width;
	byte day_alrm;
	byte mon_alrm;
	byte century;
        word iapc_boot_arch; 
	byte reserved1;
	dword flags;
	gas_addr_t reset_reg;
	byte reset_value;
	byte reserved2[3];

	/* acpi 2.0 fields: */
	qword x_firmware_ctrl;
	qword x_dsdt;
	gas_addr_t x_pm1a_evt_blk; 
	gas_addr_t x_pm1b_evt_blk; 
	gas_addr_t x_pm1a_cnt_blk;
	gas_addr_t x_pm1b_cnt_blk;
	gas_addr_t x_pm2_cnt_blk;
	gas_addr_t x_pm_tmr_blk;
	gas_addr_t x_gpe0_blk;
	gas_addr_t x_gpe1_blk;
	/* end */
} __attribute__ ((packed)) fadt_table_t;


static fadt_table_t serverworks_fadt_table = {
	{ ACPI_FADT_SIG,            /* 'FACP' */
	  ACPI_FADT_SIZE,           /* table length */
	  ACPI_FADT_REVISION,       /* fadt revision */
	  0x0,                      /* CALC: checksum */
	  OEMID,                    /* oem id */
	  OEMMODELID,               /* model id */
	  OEMREVISION,              /* oem revision */
	  CREATOR_ID,               /* creator id */
	  CREATOR_REVISION          /* creator revision */
	},
	ACPI_FACS_OFF,            /* FACS address */
	ACPI_DSDT_OFF,            /* DSDT address */
	0x0,                      /* reserved */
	PM_PROFILE_DEFAULT,       /* pm profile */
	IRQ_ACPI,                 /* SCI interrupt */
	0x0,                      /* SMI command port: reserved */
	0x0,                      /* acpi enable: reserved */
	0x0,                      /* acpi disable: reserved */
	0x0,                      /* s4 bios */
	0x0,                      /* pstate control */

	0x0,                      /* CALC: pm1a_evt_blk */
	0x0,                      /* CALC: pm1b_evt_blk */
	0x0,                      /* CALC: pm1a_cnt_blk */
	0x0,                      /* CALC: pm1b_cnt_blk */
	0x0,                      /* CALC: pm2_cnt_blk */
	0x0,                      /* CALC: pm_tmr_blk */
	0x0,                      /* CALC: gpe0_blk */
	0x0,                      /* CALC: gpe1_blk */
	PM1_EVT_LEN,              /* pm1_evt_len */
	PM1_CNT_LEN,              /* pm1_cnt_len */
	PM2_CNT_LEN,              /* pm2_cnt_len */          
	PM_TMR_LEN,               /* pm_tmr_len */          
	GPE0_BLK_LEN,             /* gpe0 block length */
	GPE1_BLK_LEN,             /* gpe1 block length */
	GPE1_BASE,                /* where gpe1 based events start */
	0x0,                      /* C States Changed notification. */
	P_LVL2_LAT,               /* hw latency to enter C2 state */
	P_LVL3_LAT,               /* hw latency to enter C3 state */
	FLUSH_SIZE,               /* flush size */
	FLUSH_STRIDE,             /* flush stride */
	DUTY_OFFSET,              /* offset of duty cyle setting within
				     P_CNT register */
	DUTY_WIDTH,       
	DAY_ALRM,                 /* day alarm field of rtc */
	MON_ALRM,                 /* month alarm field of rtc */
	CENTURY,                  /* century field of rtc */
	IAPC_BOOT_ARCH,           /* boot arch. flags */
	0x0,                      /* reserved */
	FFEATURE_FLAG_DEFAULT,    /* fixed feature flags */
	RESET_REG,                /* reset register */
	RESET_VAL,                /* reset value */
	{0x0, 0x0, 0x0},          /* reserved */
	ACPI_FACS_OFF,            /* FACS address: 64-bit value */
	ACPI_DSDT_OFF,            /* DSDT address: 64-bit value */
	{ 0x0, 0x0, 0x0, 
	  0x0, 0x0 },             /* CALC: x_pm1a_evt_blk */
	{ 0x0, 0x0, 0x0, 
	  0x0, 0x0 },             /* CALC: x_pm1b_evt_blk */
	{ 0x0, 0x0, 0x0, 
	  0x0, 0x0 },             /* CALC: x_pm1a_cnt_blk */
	{ 0x0, 0x0, 0x0, 
	  0x0, 0x0 },             /* CALC: x_pm1b_cnt_blk */
	{ 0x0, 0x0, 0x0, 
	  0x0, 0x0 },             /* CALC: x_pm2_cnt_blk */
	{ 0x0, 0x0, 0x0, 
	  0x0, 0x0 },             /* CALC: x_pm_tmr_blk */
	{ 0x0, 0x0, 0x0, 
	  0x0, 0x0 },             /* CALC: x_gpe0_blk */
	{ 0x0, 0x0, 0x0, 
	  0x0, 0x0 }              /* CALC: x_gpe1_blk */
};

/* allocate an i/o port */
static dword acpi_allocate(pci_dev *dev, unsigned int size, 
			   unsigned int port)
{
	dword addr;

	addr = region_allocate(PCI_BAR_IO, size);
 	outb(port, ACPI_INDEX_REG);
	outb(addr & 0xff, ACPI_DATA_REG);
	outb(port + 1, ACPI_INDEX_REG);
	outb((addr >> 8) & 0xff, ACPI_DATA_REG);
	DPRINTF("     OSB4 I/O port: %02x:%02x I/O base 0x%04x [size=%d]\n",
		dev->bus->busnum, dev->devfn, addr, size);
	return addr;
}

static void acpi_gas_assign(gas_addr_t *gas, qword addr,
			    byte type, byte len, byte offset)
{
	gas->addr_space_id = type;
	gas->register_bit_width = len;
	gas->register_bit_offset = offset;
	gas->address = addr;
}

#if SETUP_TABLES
static void table_init(const int irq)
{
	static rsdp_table_t serverworks_rsdp_table = {
		ACPI_RSDP1_SIG,           /* signature 'RSD ' */
		ACPI_RSDP2_SIG,           /* signature 'PTR ' */
		0x0,                      /* CALC: checksum */
		OEMID,                    /* oem id */
		ACPI_REVISION,            /* revision */
		ACPI_RSDT_OFF,            /* rsdt address */
		ACPI_RSDP_SIZE,           /* table length */
		ACPI_XSDT_OFF,            /* xdst address */
		0x0,                      /* CALC: extended checksum */
		{0x0, 0x0, 0x0}           /* reserved */
	};

	static rsdt_table_t serverworks_rsdt_table = {
		{ ACPI_RSDT_SIG,            /* 'RSDT' */
		  ACPI_RSDT_SIZE,           /* rsdt length */
		  ACPI_REVISION,            /* revision */
		  0x0,                      /* CALC: checksum */
		  OEMID,                    /* oem id */
		  OEMMODELID,               /* model id */
		  OEMREVISION,              /* oem revision */
		  CREATOR_ID,               /* creator id */
		  CREATOR_REVISION          /* creator revision */
		},
		{ ACPI_FADT_OFF }           /* description headers */
	};

	static xsdt_table_t serverworks_xsdt_table = {
		{ ACPI_XSDT_SIG,            /* 'XSDT' */
		  ACPI_XSDT_SIZE,           /* table length */
		  ACPI_REVISION,            /* revision */
		  0x0,                      /* CALC: checksum */
		  OEMID,                    /* oem id */
		  OEMMODELID,               /* model id */
		  OEMREVISION,              /* oem revision */
		  CREATOR_ID,               /* creator id */
		  CREATOR_REVISION          /* creator revision */
		},
		{ ACPI_FADT_OFF }           /* description header addresses */
	};
	rsdp_table_t *rsdp = &serverworks_rsdp_table;
	rsdt_table_t *rsdt = &serverworks_rsdt_table;
	xsdt_table_t *xsdt = &serverworks_xsdt_table;
	fadt_table_t *fadt = &serverworks_fadt_table;
	byte sum, i;

	/* assign the interrupt */
	fadt->sci_int = irq;

	/* calculate the checksum. the first 20 bytes must sum to 0 */
	for (sum = i = 0; i < 20; i++) 
		sum -= *((byte *) rsdp + i);
	rsdp->checksum = sum;

	/* extended checksum. the size of the entire table */
	for (; i < ACPI_RSDP_SIZE; i++)
		sum -= *((byte *) rsdp + i);
	rsdp->extended_csum = sum;

	/* do the same for the RSDT */
	for (i = sum = 0; i < ACPI_RSDT_SIZE; i++)
		sum -= *((byte *) rsdt + i);
	rsdt->table.checksum = sum;

	/* and the XSDT */
	for (sum = i = 0; i < ACPI_XSDT_SIZE; i++)
		sum -= *((byte *) xsdt + i);
	xsdt->table.checksum = sum;

	/* figure out FADT checksum */
	for (sum = i = 0; i < ACPI_FADT_SIZE; i++)
		sum -= *((byte *) fadt + i);
	fadt->table.checksum = sum;

	/* we need to map the RSDP to the ACPI base so that the os
	 * can find it. we'll just stick everything right past
	 * it as well. */
	memcpy((void *) ACPI_RSDP_OFF, rsdp, ACPI_RSDP_SIZE);
	DPRINTF("     Relocation: RSDP from 0x%08x to 0x%08x (%3d bytes, checksum=0x%02x)\n",
		(dword) rsdp, ACPI_RSDP_OFF, ACPI_RSDP_SIZE, 
		rsdp->extended_csum);

	/* map the rsdt right afterwards */
	memcpy((void *) ACPI_RSDT_OFF, rsdt, ACPI_RSDT_SIZE);
	DPRINTF("     Relocation: RSDT from 0x%08x to 0x%08x (%3d bytes, checksum=0x%02x)\n",
		(dword) rsdt, ACPI_RSDT_OFF, ACPI_RSDT_SIZE, 
		rsdt->table.checksum);

	/* followed by the XSDT */
	memcpy((void *) ACPI_XSDT_OFF, xsdt, ACPI_XSDT_SIZE);
	DPRINTF("     Relocation: XSDT from 0x%08x to 0x%08x (%3d bytes, checksum=0x%02x)\n",
		(dword) xsdt, ACPI_XSDT_OFF, ACPI_XSDT_SIZE, 
		xsdt->table.checksum);

	/* and the fadt */
	memcpy((void *) ACPI_FADT_OFF, fadt, ACPI_FADT_SIZE);
	DPRINTF("     Relocation: FADT from 0x%08x to 0x%08x (%3d bytes, checksum=0x%02x)\n",
		(dword) fadt, ACPI_FADT_OFF, ACPI_FADT_SIZE, 
		fadt->table.checksum);
}
#endif

void acpi_osb4_init(pci_dev *dev)
{
	fadt_table_t *fadt = &serverworks_fadt_table;
	word addr;

	/* acpi control: 0x0e. enable acpi decode */
	outb(0x0e, ACPI_INDEX_REG);
	outb(0x8, ACPI_DATA_REG);

	/* acpi control 1: 0x10. don't do anything here right now */
	/* allocate i/o ports */

	/* pm 1 enable registers: turn off everything */
	addr = acpi_allocate(dev, PM1_EVT_LEN, PM1_EVT_REG);
	fadt->pm1a_evt_blk = addr;
	acpi_gas_assign(&fadt->x_pm1a_evt_blk, addr, GAS_ADDR_SYSTEM_IO,
			PM1_EVT_LEN, 0x0);

	/* pm 1 control register: turn on SCI */
	addr = acpi_allocate(dev, PM1_CNT_LEN_A, PM1_CNT_REG);
	fadt->pm1a_cnt_blk = addr;
	acpi_gas_assign(&fadt->x_pm1a_cnt_blk, addr, GAS_ADDR_SYSTEM_IO,
			PM1_CNT_LEN, 0x0);
	outw(0x1, addr);

	/* pm timer register */
	addr = acpi_allocate(dev, PM_TMR_LEN, PM_TMR_REG);
	fadt->pm_tmr_blk = addr;
	acpi_gas_assign(&fadt->x_pm_tmr_blk, addr, GAS_ADDR_SYSTEM_IO,
			PM_TMR_LEN, 0x0);
	
	/* XXX: processor control block needs to deal with DUTY_WIDTH
	 *      and DUTY_OFFSET */
	addr = acpi_allocate(dev, P_CNT_LEN_A, P_CNT_REG);

	/* gpe 0 block: enable general events. 
	 * hot swap switch is gevent 6 */ 
	addr = acpi_allocate(dev, GPE0_BLK_LEN, GPE0_BLK_REG);
	fadt->gpe0_blk = addr;
	acpi_gas_assign(&fadt->x_gpe0_blk, addr, GAS_ADDR_SYSTEM_IO,
			GPE0_BLK_LEN, 0x0);
	outb(0x40, addr + 4);       /* enable gevent 6 */
	outb(0xff, addr);           /* clear status bits */
	outb(0xff, addr + 1);       /* clear status bits */
	outb(0xff, addr + 2);       /* clear status bits */
	outb(0xff, addr + 3);       /* clear status bits */
}

static void superio_init(void)
{
	fadt_table_t *fadt = &serverworks_fadt_table;
	byte val;
	word addr;

	/* assign SCI -- on the OSB4, it's hardwired to IRQ15. */
	outb(SUPERIO_SIOC3, PNP_NS_INDEX_PORT);
	val = (inb(PNP_NS_DATA_PORT) & 0x0f) | (SUPERIO_IRQ_REDIRECT << 4);
	outb(val, PNP_NS_DATA_PORT);
	DPRINTF("     SIO_Wakeup: SuperI/O SCI -> IRQ%d\n", SUPERIO_IRQ_REDIRECT);

	/* make SIO_Wakeup trigger on rising edge */
	outb( 0x10, ACPI_INDEX_REG );
	val = inb( ACPI_DATA_REG ) | 0x80;
	outb( val, ACPI_DATA_REG );
	
	/* pm1 evt blk */
	outb(SUPERIO_PM1_EVT_REG, PM_ADDR_PORT);
	addr = inb(PM_DATA_PORT);
	outb(SUPERIO_PM1_EVT_REG + 1, PM_ADDR_PORT);
	addr |= inb(PM_DATA_PORT) << 8;
	if (addr) {
		fadt->pm1b_evt_blk = addr;
		acpi_gas_assign(&fadt->x_pm1b_evt_blk, addr, 
				GAS_ADDR_SYSTEM_IO, PM1_EVT_LEN, 0x0);
		/* enable power button */
		outb(0x01, addr + 3);

		/* reset events */
		outb(0xff, addr);
		outb(0xff, addr + 1);
	}

	/* pm1 cnt blk */
	outb(SUPERIO_PM1_CNT_REG, PM_ADDR_PORT);
	addr = inb(PM_DATA_PORT);
	outb(SUPERIO_PM1_CNT_REG + 1, PM_ADDR_PORT);
	addr |= inb(PM_DATA_PORT) << 8;
	if (addr) {
		fadt->pm1b_cnt_blk = addr;
		acpi_gas_assign(&fadt->x_pm1b_cnt_blk, addr, 
				GAS_ADDR_SYSTEM_IO, PM1_CNT_LEN, 0x0);
		/* route pm events to SCI */
		outb(0x01, addr);
	}

	/* gpe1 blk */
	outb(SUPERIO_GPE1_BLK_REG, PM_ADDR_PORT);
	addr = inb(PM_DATA_PORT);
	outb(SUPERIO_GPE1_BLK_REG + 1, PM_ADDR_PORT);
	addr |= inb(PM_DATA_PORT) << 8;
	if (addr) {
		fadt->gpe1_blk = addr;
		acpi_gas_assign(&fadt->x_gpe1_blk, addr, 
				GAS_ADDR_SYSTEM_IO, GPE1_BLK_LEN, 0x0);
		/* don't need to do anything here */
	}
}


void acpi_init(void)
{
	pci_dev *dev;

	if (!systype_is_5k())
		return;

	dev = pci_lookup(PCI_VENDOR_SERVERWORKS, 
			 PCI_DEVICE_SERVERWORKS_OSB4);
	if (!dev)
		return;

	printf("Initializing ACPI:\n");

	superio_init();

#if SETUP_TABLES
	/* set up acpi tables */
	table_init(dev->irq);
#endif
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
