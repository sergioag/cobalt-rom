/*
 * 
 * Filename: diagdefs.h
 * 
 * Description: Some general definitions used by the diagnostics 
 * 
 * Author(s): Timothy Stonis
 * 
 * Copyright 1997, Cobalt Microserver, Inc.
 * 
 */

#define KSEG0_Base		0x80000000
#define KSEG1_Base		0xA0000000

// Some useful Galileo registers/base addresses (boot time kseg1 mapping) 
#define kGal_InternalBase	( 0x14000000 | KSEG1_Base ) 
#define kGal_DevBank0Base	( 0x1C000000 | KSEG1_Base )
#define kGal_DevBank1Base 	( 0x1C800000 | KSEG1_Base )

#define kGal_RAS10Lo		0x008
#define kGal_RAS10Hi		0x010
#define kGal_RAS32Lo		0x018
#define kGal_RAS32Hi		0x020

#define kGal_PCIIOLo		0x048
#define kGal_PCIIOHi		0x050

#define kGal_RAS10LoCfg		0x000
#define kGal_RAS10HiCfg		0x03
#define kGal_RAS32LoCfg		0x004
#define kGal_RAS32HiCfg		0x07

#define kGal_PCIIOLoCfg		0x000
#define kGal_PCIIOHiCfg		0x0F


#define kGal_DevBank0PReg	0x45C
#define kGal_DevBank1PReg	0x460
#define kGal_DevBank2PReg	0x464
#define kGal_DevBank3PReg	0x468
#define kGal_DevBankBPReg	0x46C

#define kGal_DRAMCReg		0x448
#define kGal_DRAM0PReg		0x44C
#define kGal_DRAM1PReg		0x450
#define kGal_DRAM2PReg		0x454
#define kGal_DRAM3PReg		0x458

#define kGal_TimeRetry		0xC04
#define kGal_ConfigAddr		0xCF8
#define kGal_ConfigData		0xCFC
#define kGal_PCICfgEn		0x1F // Generate config cycle 
#define kGal_DevNum		0x00 // 0 Internally 
#define kGal_DevNum_Ext		0x06 // Technically 0x06, on the PCI bus 
#define kGal_MasMemEn		0x06
#define kGal_Latency		0x700
#define kGal_LimRetry 		0x0008070F  // Used to avoid retry forever when Gal talks to itself
#define kGal_Retry 		0x0000070F  // Used to avoid retry forever when Gal talks to itself


#define kGal_RAS01StartReg	0x10
#define kGal_RAS23StartReg	0x14
#define kGal_RAS01SizeReg	0x0C08
#define kGal_RAS23SizeReg	0x0C0C


#define kGal_RAS01Start		0x000
#define kGal_RAS23Start		0x00800000
#define kGal_RAS01Size		0x007FFFFF
#define kGal_RAS23Size		0x007FFFFF


// Paramter information for devices, DRAM, etc
#define	kGal_DevBank0Cfg	0x1446DB33
#define	kGal_DevBank1Cfg	0x144FE667
#define kGal_DevBank2Cfg	0x1466DB33
#define kGal_DevBank3Cfg	0x146FDFFB
#define	kGal_DevBankBCfg	0x1446DC43
#define	kGal_DRAMConfig		0x00000300
#define	kGal_DRAM0Config	0x00000010
#define	kGal_DRAM1Config	0x00000010
#define	kGal_DRAM2Config	0x00000010
#define	kGal_DRAM3Config	0x00000010

#define	kGal_DRAM0Hi		0x00000003
#define	kGal_DRAM0Lo		0x00000000
#define	kGal_DRAM1Hi		0x00000007
#define	kGal_DRAM1Lo		0x00000004
#define	kGal_DRAM2Hi		0x0000000B
#define	kGal_DRAM2Lo		0x00000008
#define	kGal_DRAM3Hi		0x0000000F
#define	kGal_DRAM3Lo		0x0000000C

#define kGal_RAS0Lo		0x400
#define kGal_RAS0Hi		0x404
#define kGal_RAS1Lo		0x408
#define kGal_RAS1Hi		0x40C
#define kGal_RAS2Lo		0x410
#define kGal_RAS2Hi		0x414
#define kGal_RAS3Lo		0x418
#define kGal_RAS3Hi		0x41C

// Feedback LED indicators during setup code (reset.S, main.c) 
// #define kLED_AllOn	0x0F
#define kLED_StartPat   0x07
#define kLED_FlashTest	0x01
#define kLED_MemTest	0x02
#define kLED_SCCTest	0x03
#define kLED_GalPCI	0x04
#define kLED_SizeMem	0x05
//#define kLED_EnetTest	0x05
//#define kLED_SCSITest	0x06
//#define kLED_IOCTest	0x07
#define kLED_CacheInit	0x09
#define kLED_Quickdone	0x0A
#define kLED_Exception	0x0B
#define kLED_ProcInit	0x0E
#define kLED_AllOff	0x00

#define kLEDBase	kGal_DevBank0Base

#define kRESET		0x0F

#define kBLINK_tlb	0x01
#define kBLINK_xtlb	0x02
#define kBLINK_cache	0x03
#define kBLINK_1data	0x04
#define kBLINK_0data	0x05
#define kBLINK_1addr	0x06
#define kBLINK_0addr	0x07 
#define kBLINK_general	0x0B

// Some memory related constants 
#define kRAM_Start	(0x00000000 | KSEG0_Base)

#define	kTestPat1	0xA5A5A5A5
#define kTestPat2	0x5A5A5A5A
#define kTestPat3       0xC3C3C3C3
#define kTestPat4       0x3C3C3C3C


#define k1Meg_kseg1 	(0x00100000 | KSEG1_Base)
#define k2Meg_kseg1  	(0x00200000 | KSEG1_Base)
#define k3Meg_kseg1  	(0x00300000 | KSEG1_Base)
#define k4Meg_kseg1  	(0x00400000 | KSEG1_Base)
#define k8Meg_kseg1  	(0x00800000 | KSEG1_Base)
#define k16Meg_kseg1  	(0x01000000 | KSEG1_Base)

#define kVectorBase	0x200	
#define kDebugVectors	0x400


/* 
 * offsets to romcode/ramcode/mkernel-shared variables are defined 
 * in reset.S.  make sure these definitions are consistent.
 */

#define kBootloaderFilenameAddr 	((char **) (kRAMCodeAddr + 0x8))
#define kDiskerrorAddr                  ((int *) (kRAMCodeAddr + 36))


#if 0
/*
 * not sure if these are used.  in any case making these constants
 * sucks because these by their nature vary depending on code size
 */

#define kFlashCompAddr	0xBFC1C000
#define kBootLoaderAddr	0xBFC16800

#endif /* 0 */


// Flash definitions AMD 29F040
#define kFlashBase	0xBFC00000

#define WRITE_FLASH(x,y)  (*((volatile unsigned char *) (kFlashBase | (x)) ) = y)
#define READ_FLASH(x)     *((volatile unsigned char *) (kFlashBase | (x)) )

#define kFlash_Addr1	0x5555
#define kFlash_Addr2	0x2AAA
#define kFlash_Data1	0xAA
#define kFlash_Data2	0x55
#define kFlash_Prog     0xA0
#define kFlash_Erase3   0x80
#define kFlash_Erase6   0x10
#define kFlash_Read     0xF0

#define kFlash_ID	0x90
#define kFlash_VenAddr	0x00
#define kFlash_DevAddr	0x01
#define kFlash_VenID	0x01
#define kFlash_DevID	0xA4	// 29F040
//#define kFlash_DevID	0xAD	// 29F016



// Ethernet definitions
#define	kEnet_VIOBase	( 0x10100000 | KSEG1_Base )
#define	kEnet_PIOBase	0x10100000
#define	kEnet_CSCfg	0x45
#define kEnet_DevNum	0x07
#define kEnet_CSR3	0x18
#define kEnet_CSR15	0x78
#define kEnet_CFDD	0x40
#define kEnet_Sleep	0x00

#define kEnet_GEPOut	0x080f0000
#define kEnet_GEPOn	0x000f0000

#define kEnet_DevVenID_TULIP		0x00021011
#define kEnet_DevVenID_TULIP_FAST	0x00091011
#define kEnet_DevVenID_TULIP_PLUS	0x00141011
#define kEnet_DevVenID_TULIP_21143	0x00191011

// SCSI definitions
#define	kSCSI_VIOBase	( 0x12200000 | KSEG1_Base )
#define	kSCSI_PIOBase	0x12200000 
#define	kSCSI_CSCfg	0x46 
#define kSCSI_DevNum	0x08
#define kSCSI_GPCNTL	0x47
#define kSCSI_GPREG	0x07
#define kSCSI_SCRTCHA	0x34

#define kSCSI_GPIOOut	0x0C
#define kSCSI_LEDsOn	0x00

#define kSCSI_DevVenID	0x00011000

// I/O Controller definitions
#define kIOC_VIOBase	( 0x10000000 | KSEG1_Base )
#define kIOC_RIOBase	0x10000000 
#define kIOC_DevNum	0x09
#define kIOC_ISAFunc	0x00
#define kIOC_IDEFunc	0x01
#define kIOC_USBFunc	0x02
#define kIOC_MiscC0	0x44
#define kIOC_IDEEnable	0x40

#define kIOC_PCIIDEOn 	0x02800085
#define kIOC_PriIDEOn	0x0A

#define kIOC_DevVenID	0x05861106

// Galileo PCI definitions
#define kGal_DevVenID	0x414611AB

// Some PCI Definitions
#define kPCI_DevVen	0x00
#define	kPCI_StatCmd	0x04
#define kPCI_ClassRev	0x08
#define	kPCI_LatCache	0x0C
#define	kPCI_CBIO	0x10
#define	kPCI_CBMEM	0x14

// Random constants
#define kSCCDelay	0x00000001

#ifndef NULL
#define NULL		0x00
#endif

// Constants for DRAM Masks
#define kSimml_Maskp    0x0000001F  //31
#define kSimml_Maskn    0x0000000B  //11
#define kSimmh_Maskp    0x0000001B  //27
#define kSimmh_Maskn    0x00000007  //7
#define kBank_Maskp     0x0000001B  //27
#define kBank_Maskn     0x00000008  //8
#define kOne            0x00000001
#define ones            0xFFFFFFFF

//  Constants for DRAM mapping

#define k2M		0x00200000
#define k4M             0x00400000
#define k8M             0x00800000
#define k16M            0x01000000
#define k32M            0x02000000
#define k64M            0x04000000
#define k128M           0x08000000
#define k192M           0x0C000000
#define k32k            0x00008000
#define k64k            0x00010000


// CMOS storage stuff
// Using the CMOS registors around the RTC on the VIA
#define kCMOS_Addr      0x70
#define kCMOS_Data      0x71

#define WRITE_CMOS(x,y)  (*((volatile unsigned char *) ( 0xB0000000 | (x)) ) = y)
#define READ_CMOS(x)    *((volatile unsigned char *) (0xB0000000 | (x)) )


/*
 * Macros for addresses/values of variables saved in CMOS.
 * the xxx_Flag macros are the offsets into the CMOS
 * for these variables. other kCMOS_xxx_yyy are values
 * relevant to each variable
 */
 
// If Flag is 1, then boot UI, if 0, boot kernel
#define kCMOS_Boot_Flag 0x10
#define kCMOS_Boot_UI   0xA5
#define kCMOS_Boot_BFD  0x01

#define kCMOS_LCD_Flag   0x11
#define kCMOS_LCD_notest 0x01
#define kCMOS_LCD_test   0xA5

#define kCMOS_DEVTAB_INDEX_Flag  0x12

#define kCMOS_Console_Flag 0x13
#define kCMOS_Console_off  0x01
#define kCMOS_Console_on   0xA5

// Macros

#define WRITE_GAL(x,y)  (*((volatile unsigned long *) (0xB4000000 | (x)) ) = y)
#define READ_GAL(x)	*((volatile unsigned long *) (0xB4000000 | (x)) )

#define FORCE_WRITE	asm( "nop; nop; ");


#define ROMSIZE		512

#define TOT_MEM		( KSEG0_Base | ( bank0_size + bank1_size +	\
					 bank2_size + bank3_size) )

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
