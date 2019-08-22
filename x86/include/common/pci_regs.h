/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: pci_regs.h,v 1.1.1.1 2003/06/10 22:42:03 iceblink Exp $
 *
 * defines for various PCI registers
 */

#ifndef PCI_REGISTERS_H
#define PCI_REGISTERS_H

#define PCI_VENDOR_ID                   0x00	/* 16 bits */
#define PCI_DEVICE_ID                   0x02	/* 16 bits */

#define PCI_COMMAND			0x04	/* 16 bits */
#define  PCI_CMD_IO			0x1	/* I/O space */
#define  PCI_CMD_MEMORY			0x2	/* Memory space */
#define  PCI_CMD_MASTER			0x4	/* bus mastering */
#define  PCI_CMD_SPECIAL		0x8	/* special cycles */
#define  PCI_CMD_INVALIDATE		0x10	/* mem write/invalidate */
#define  PCI_CMD_VGA_PALETTE		0x20	/* palette snooping */
#define  PCI_CMD_PARITY			0x40	/* PERR reporting */
#define  PCI_CMD_STEPPING		0x80	/* address/data stepping */
#define  PCI_CMD_SERR			0x100	/* SERR reporting */
#define  PCI_CMD_FBBE			0x200	/* back-to-back writes */

#define PCI_STATUS			0x06	/* 16 bits */
#define  PCI_STATUS_66			0x20	/* 66 MHz */
#define  PCI_STATUS_UDF			0x40	/* user-defined functions */
#define  PCI_STATUS_FBBE		0x80	/* fast back-to-back */

#define PCI_CLASS_ID			0x08	/* 32:8 bits */
#define  PCI_CLASS_BRIDGE	       	0x06
#define  PCI_CLASS_BRIDGE_HOST		0x0600
#define  PCI_CLASS_BRIDGE_ISA		0x0601
#define  PCI_CLASS_BRIDGE_PCI		0x0604

#define PCI_REVISION_ID			0x08	/*    8 bits */
#define PCI_CACHE_LINE_SIZE		0x0c	/*    8 bits */
#define PCI_LATENCY_TIMER		0x0d	/*    8 bits */

#define PCI_HTYPE                	0x0e	/* header type */
#define  PCI_HTYPE_STANDARD		0x00
#define  PCI_HTYPE_PCI_BRIDGE		0x01
#define  PCI_HTYPE_CARDBUS_BRIDGE	0x02

#define PCI_BIST			0x0f
#define PCI_BUSNUM			0x44
#define PCI_SBUSNUM			0x45

#define PCI_CARDBUS_CIS                 0x28
#define PCI_SUBSYS_VENDOR		0x2c
#define PCI_SUBSYS_DEVICE		0x2e
#define PCI_EXPANSION_ROM               0x30    /* expansion rom bar */

#define PCI_INTERRUPT_LINE              0x3c
#define PCI_INTERRUPT_PIN               0x3d

#define PCI_BAR_ADDR(reg)		((byte)(0x10 + (4 * (reg))))
#define PCI_BAR_IS_IO			(1 << 0)
#define PCI_BAR_IS_LOWMEM		(1 << 1)
#define PCI_BAR_IS_64BIT		(1 << 2)
#define PCI_BAR_IS_PREFETCH		(1 << 3)

/* PCI-PCI bridge configs */
#define PCI2PCI_BUS_PRI			0x18	/* primary bus   */
#define PCI2PCI_BUS_SEC			0x19	/* secondary bus */
#define PCI2PCI_BUS_SUB			0x1a	/* subordinate bus */
#define PCI2PCI_LATENCY			0x1b	/* secondary latency */
#define PCI2PCI_IO_BASE			0x1c	/* IO passthru base */
#define PCI2PCI_IO_LIMIT		0x1d	/* IO passthru limit */
#define PCI2PCI_STATUS			0x1e	/* secondary status */
#define PCI2PCI_MMIO_BASE		0x20	/* MMIO passthru base */
#define PCI2PCI_MMIO_LIMIT		0x22	/* MMIO passthru limit */
#define PCI2PCI_PMMIO_BASE		0x24	/* PMMIO passthru base */
#define PCI2PCI_PMMIO_LIMIT		0x26	/* PMMIO passthru limit */
#define PCI2PCI_PMMIO_BASE_UPPER	0x28	/* PMMIO base (hi 32 bits) */
#define PCI2PCI_PMMIO_LIMIT_UPPER	0x2c	/* PMMIO limit (hi 32 bits) */
#define PCI2PCI_IO_BASE_UPPER		0x30	/* IO base (hi 16 bits) */
#define PCI2PCI_IO_LIMIT_UPPER		0x34	/* IO limit (hi 16 bits) */
#define PCI2PCI_EXPANSION_ROM           0x38    /* expansion rom bar */
#define PCI2PCI_BRIDGE_CTRL		0x3e	/* bridge control */
#define  PCI2PCI_BCTL_PERR		0x1
#define  PCI2PCI_BCTL_SERR		0x2
#define  PCI2PCI_BCTL_ISA		0x4
#define  PCI2PCI_BCTL_VGA		0x8
#define  PCI2PCI_BCTL_MABORT		0x20
#define  PCI2PCI_BCTL_RESET		0x40
#define  PCI2PCI_BCTL_FBBE		0x80
#define  PCI2PCI_BCTL_PRI_DISC_TIMER	0x100
#define  PCI2PCI_BCTL_SEC_DISC_TIMER	0x200
#define  PCI2PCI_BCTL_DT_STATUS		0x400
#define  PCI2PCI_BCTL_DT_SERR		0x800

/* PCI-Cardbus bridge configs */
#define PCI2CB_SOCKET_BAR		0x10	/* BAR for socket/ExCA regs */
#define PCI2CB_CAPABILITY		0x14
#define PCI2CB_STATUS2			0x16	/* like PCI2PCI */
#define PCI2CB_BUS_PCI			0x18	/* PCI bus # of bridge*/
#define PCI2CB_BUS_CB			0x19	/* cardbus bus # of bridge */
#define PCI2CB_BUS_SUB			0x1a	/* subordinate bus number */
#define PCI2CB_LATENCY			0x1b
#define PCI2CB_MMIO_BASE0		0x1c
#define PCI2CB_MMIO_LIMIT0		0x20
#define PCI2CB_MMIO_BASE1		0x24
#define PCI2CB_MMIO_LIMIT1		0x28
#define PCI2CB_IO_BASE0			0x2c
#define PCI2CB_IO_LIMIT0		0x30
#define PCI2CB_IO_BASE1			0x34
#define PCI2CB_IO_LIMIT1		0x38
#define PCI2CB_BRIDGE_CTRL		0x3e
#define  PCI2CB_BCTL_PERR		0x1
#define  PCI2CB_BCTL_SERR		0x2
#define  PCI2CB_BCTL_ISA		0x4
#define  PCI2CB_BCTL_VGA		0x8
#define  PCI2CB_BCTL_MABORT		0x20
#define  PCI2CB_BCTL_RESET		0x40
#define  PCI2CB_BCTL_INTR		0x80
#define  PCI2CB_BCTL_PREFETCH0		0x100
#define  PCI2CB_BCTL_PREFETCH1		0x200
#define  PCI2CB_BCTL_POST		0x400
#define PCI2CB_16BIT_LEGACY_BAR         0x44

#endif

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
