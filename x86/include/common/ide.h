/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: ide.h,v 1.1.1.1 2003/06/10 22:42:02 iceblink Exp $
 */

#ifndef COMMON_IDE_H
#define COMMON_IDE_H

#define IDE0_BASE0	0x1f0
#define IDE0_BASE1	0x3f6
#define IDE1_BASE0	0x170
#define IDE1_BASE1	0x376

#define IDE_ADDRESS(a)			(((a) & 0xFFFF) >> 3)

#define IDE_DATA_OFFSET			(0)
#define IDE_ERROR_OFFSET		(1)
#define IDE_SECTOR_COUNT_OFFSET		(2)
#define IDE_SECTOR_NUMBER_OFFSET	(3)
#define IDE_CYLINDER_LSB_OFFSET		(4)
#define IDE_CYLINDER_MSB_OFFSET		(5)
#define IDE_DRIVE_HEAD_OFFSET		(6)
#define IDE_STATUS_REG_OFFSET		(7)
#define IDE_CONTROL_REG_OFFSET		(8)
#define IDE_IRQ_REG_OFFSET		(9)
#define IDE_COMMAND_REG_OFFSET		IDE_STATUS_REG_OFFSET

/* Values for HD_COMMAND */
#define WIN_RESTORE		0x10
#define WIN_READ		0x20
#define WIN_WRITE		0x30
#define WIN_WRITE_VERIFY	0x3C
#define WIN_VERIFY		0x40
#define WIN_FORMAT		0x50
#define WIN_INIT		0x60
#define WIN_SEEK		0x70
#define WIN_DIAGNOSE		0x90
#define WIN_SPECIFY		0x91	/* set drive geometry translation */
#define WIN_IDLEIMMEDIATE	0xE1	/* force drive to become "ready" */
#define WIN_SETIDLE1		0xE3
#define WIN_SETIDLE2		0x97

#define WIN_STANDBYNOW1		0xE0
#define WIN_STANDBYNOW2		0x94
#define WIN_SLEEPNOW1		0xE6
#define WIN_SLEEPNOW2		0x99
#define WIN_CHECKPOWERMODE1	0xE5
#define WIN_CHECKPOWERMODE2	0x98
#define WIN_FLUSH_CACHE		0xE7

#define WIN_DOORLOCK		0xDE	/* lock door on removable drives */
#define WIN_DOORUNLOCK		0xDF	/* unlock door on removable drives */

#define WIN_MULTREAD		0xC4	/* read sectors using multiple mode */
#define WIN_MULTWRITE		0xC5	/* write sectors using multiple mode */
#define WIN_SETMULT		0xC6	/* enable/disable multiple mode */
#define WIN_IDENTIFY		0xEC	/* ask drive to identify itself	*/
#define WIN_IDENTIFY_DMA	0xEE	/* same as WIN_IDENTIFY, but DMA */
#define WIN_SETFEATURES		0xEF	/* set special drive features */
#define WIN_READDMA		0xC8	/* read sectors using DMA transfers */
#define WIN_WRITEDMA		0xCA	/* write sectors using DMA transfers */

#define WIN_QUEUED_SERVICE	0xA2	/* */
#define WIN_READDMA_QUEUED	0xC7	/* read sectors using Queued DMA transfers */
#define WIN_WRITEDMA_QUEUED	0xCC	/* write sectors using Queued DMA transfers */

#define WIN_READ_BUFFER		0xE4	/* force read only 1 sector */
#define WIN_WRITE_BUFFER	0xE8	/* force write only 1 sector */

#define WIN_SMART		0xB0	/* self-monitoring and reporting */

/* Additional drive command codes used by ATAPI devices. */
#define WIN_PIDENTIFY		0xA1	/* identify ATAPI device	*/
#define WIN_SRST		0x08	/* ATAPI soft reset command */
#define WIN_PACKETCMD		0xA0	/* Send a packet command. */

#define IDE_CMD_MASTER			(0xa0)
#define IDE_CMD_IDENTIFY		(WIN_IDENTIFY)
#define IDE_CMD_RECALIBRATE		(WIN_RESTORE)

int init_ide(void);
void list_ide(void);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
