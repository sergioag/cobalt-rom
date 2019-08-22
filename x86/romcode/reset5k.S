; $Id: reset5k.S,v 1.1.1.1 2003/06/10 22:42:27 iceblink Exp $
; 
; Copyright (c) 1999-2000 Cobalt Networks, Inc.
; 
; contains code in the reset vector segment
; written for nasm
;	
; Author(s):	Erik Gilling
;		Tim Hockin
;		Duncan Laurie
;


;IO Base Address
CMS_BASE			equ	0xC50
PM_BASE				equ	0x500
GPIO_BASE			equ	0x600
SIO_BASE			equ	0x2E
ACPI_BASE			equ	0xCD6
;ServerWorks PCI IDs
PCI_VID_SERVERWORKS		equ	0x1166
PCI_DID_SERVERWORKS_LE		equ	0x0009
PCI_DID_SERVERWORKS_OSB4	equ	0x0200
PCI_DID_SERVERWORKS_CSB5	equ	0x0201
PCI_DID_SERVERWORKS_CSB5_LPC	equ	0x0230

;ServerWorks NorthBridge dev
PCI_ID_CNBLE			equ	(0x0 << 3)
PCI_ID_CNBLE_2			equ	(PCI_ID_CNBLE | 1)
PCI_ID_CNBLE_L			equ	(0x0 << 11)
SW_PCI0				equ	PCI_ID_CNBLE
SW_PCI1				equ	PCI_ID_CNBLE_2
SW_NORTH			equ	PCI_ID_CNBLE

;ServerWorks SouthBridge dev
SW_SOUTH			equ	(0xf << 3)
SW_SOUTH_L			equ	(0xf << 11)
SW_SOUTH_LPC			equ	(SW_SOUTH | 3)
PCI_ID_OSB4			equ	SW_SOUTH
PCI_ID_OSB4_L			equ	SW_SOUTH_L
PCI_ID_CSB5			equ	SW_SOUTH
PCI_ID_CSB5_L			equ	SW_SOUTH_L

;check for 5000 series system
%macro is5k_sp 0
	mov	ah, SW_NORTH
	mov	al, 0x0
	call_sp	rd_cfg_dw
	mov	cx, ax
	shl	ecx, 16

	mov	ah, SW_NORTH
	mov	al, 0x2
	call_sp	rd_cfg_dw
	and	eax, 0x0000ffff
	or	ecx, eax
	cmp	ecx, (PCI_DID_SERVERWORKS_LE | (PCI_VID_SERVERWORKS << 16))
%endm
%macro is5k 0
	mov	bx, (PCI_ID_CNBLE_L | 0x0)
	mov	di, 3
	call	pci_read_cfg
	cmp	ecx, (PCI_VID_SERVERWORKS | (PCI_DID_SERVERWORKS_LE << 16))
%endm

BOARDTYPE_MONTEREY	equ	PCI_DID_SERVERWORKS_OSB4
BOARDTYPE_ALPINE	equ	PCI_DID_SERVERWORKS_CSB5

;check for Version 1 or 2 system
%macro is5k_type_sp 1
	mov	ah, SW_SOUTH
	mov	al, 0
	call_sp	rd_cfg_dw
	mov	cx, ax
	shl	ecx, 16

	mov	ah, SW_SOUTH
	mov	al, 2
	call_sp	rd_cfg_dw
	and	eax, 0x0000ffff
	or	ecx, eax
	cmp	ecx, ((PCI_VID_SERVERWORKS << 16) | %1)
%endm
%macro is5k_type 1
	mov	bx, SW_SOUTH_L
	mov	di, 3
	call	pci_read_cfg
	cmp	ecx, (PCI_VID_SERVERWORKS | (%1 << 16))
%endm

; generic macro for writing to an INDEX-DATA I/O port
; 
;    %1 => BASE
;    %2 => INDEX
;    %3 => DATA
; 
%macro iowrite 3
	mov	dx, %1
	mov	al, %2
	out	dx, al
	inc	dx
	mov	al, %3
	out	dx, al
%endm

; generic macro for reading from an INDEX-DATA I/O port
; 
;    %1 => BASE
;    %2 => INDEX
;    al <= DATA
; 
%macro ioread 2
	mov	dx, %1
	mov	al, %2
	out	dx, al
	inc	dx
	in	al, dx
%endm

; generic macro to set bits in an INDEX-DATA I/O port
; 
;    %1 => BASE
;    %2 => INDEX
;    %3 => BITFIELD
; 
%macro ioset 3
	ioread	%1, %2
	or	al, %3
	out	dx, al
%endm

%macro cm_write 2
	iowrite CMS_BASE, %1, %2
%endm

%macro pm_write 2
	iowrite PM_BASE, %1, %2
%endm

%macro pnp_write 2
	iowrite SIO_BASE, %1, %2
%endm

%macro pnp_read 1
	ioread SIO_BASE, %1
%endm

%macro pnp_set 2
	ioset SIO_BASE, %1, %2
%endm

%macro acpi_read 1
	ioread ACPI_BASE, %1
%endm

%macro acpi_write 2
	iowrite ACPI_BASE, %1, %2
%endm
	
; disable a logical device in SIO
; 
;    %1 => LOGICAL DEV
; 
%macro pnp_disable 1
	iowrite SIO_BASE, 0x07, %1
	iowrite SIO_BASE, 0x30, 0x0
%endm

; enable a logical device in SIO
; 
;    %1 => LOGICAL DEV
;    %2 => IOPORT BASE
;    %3 => INTERRUPT
; 
%macro pnp_enable 1
	iowrite SIO_BASE, 0x07, %1
	iowrite SIO_BASE, 0x30, 0x1
%endm

; enable a logical device in SIO
; 
;    %1 => LOGICAL DEV
;    %2 => IOPORT BASE
;    %3 => INTERRUPT
;
%macro sio_setup 1-3 0,0

	;enable the device
	iowrite SIO_BASE, 0x07, %1
	iowrite SIO_BASE, 0x30, 1

	;set the ioport base
	mov	cx, %2
	jcxz	%%skip_base
	iowrite SIO_BASE, 0x60, ch
	iowrite SIO_BASE, 0x61, cl
%%skip_base:

	;set the interrupt (read-modify-write to preserve bit 4)
	mov	cx, %3
	and	cx, 0xf
	jcxz	%%skip_irq
	ioread	SIO_BASE, 0x70
	and	al, 0xf0
	or	al, cl
	out	dx, al
%%skip_irq:
%endm

; enable a general purpose chip select
; 
;    %1 => IOBASE ADDR
;    %2 => LOW ADDR
;    %3 => HIGH ADDR
;    %4 => MASK ADDR
;
%macro gpcs_setup 4
	mov	dx, %1
	mov	al, %2
	out	dx, al
	mov	al, %3
	inc	dx
	out	dx, al
	mov	al, %4
	inc	dx
	out	dx, al
%endm

	
;#############################################################
;
;   Alpine  :  ServerWorks CSB5 + National ServerIO PC87414
;
;#############################################################
iomagic_5k_alpine:

	; set SIO_WAKEUP polarity to active low
	; enable SCI on wakeup, GBL_EN, Legacy Event, TIMER, SLEEPBTN,
	; and RTC
	iowrite 0xcd6, 0x10, 0xfe

	; enable general chip select in CSB5 power mgmt
	iowrite 0xcd6, 0x3c, 0xfa

	; GPCSL[0] decodes IO at 0x900 for X-Bus and FLASH ROM writing
	gpcs_setup 0xf50, 0x00, 0x09, 0x00

	; GPCSL[2] decodes IO at  0x80 for debug lights
	gpcs_setup 0xf56, 0x80, 0x00, 0x00

	;
	; wait for the internal clocks to stabilize before
	; configuring the ServerI/O and enabling its devices
	;
	mov dx, SIO_BASE
	mov al, 0x29		; clock generator
	out dx, al
	inc dx
ckvalid:
	out 0xed, al		; delay
	in al, dx		; 
	test al, 0x10		; CKVALID (bit 4) set when clock is stable
	jz ckvalid

	jmp adm1029_init
	
adm1029_done:

	;; enable GPE0-7
	acpi_write 0x30, 0x55
	acpi_write 0x31, 0x55
	
	pnp_read 0x22		; SIOCF2 (22h)
	or al, 10000000b	;   bit 7=1, pin 55, WATCH_DOG#
	out dx, al		; 

	pnp_read 0x23		; SIOCF3 (23h)
	and al, ~00001000b	;   bit 3=0, pin 36, PWR_BUTTON#
	or  al,  01100001b	;   bit 6=1, pin 38, SIO_WAKEUP
	out dx, al	 	;   bit 5=1, pin 37, SIO_SMI#
				;   bit 2=0, pin 35, GPIOE42
				;   bit 1=0, pin 51, GPIOE45 not PWR_LED
				;   bit 0=1, pin 50, FAULT_LED

	pnp_read 0x24		; SIOCF4 (24h)
				;   bit 6=0, +
				;   bit 5=0, pin 45, SIO_GPIO53
	and al, ~01101000b	;   bit 3=0, disable SMI->IRQ2
	or  al,  10000000b	;   bit 7=1, pin 13, GPIO7
	out dx, al

	sio_setup 1, 0x378, 7	; ENABLE Parallel Port, base 0x378, IRQ7
 	pnp_write 0xf0, 0x72	;                       set EPP 1.9 mode

	sio_setup 2, 0x2F8, 3	; ENABLE Serial UART 2, base 0x2F8, IRQ3
	sio_setup 3, 0x3F8, 4	; ENABLE Serial UART 1, base 0x3F8, IRQ4
	
	sio_setup 16, 0x70, 8	; ENABLE Real-Time Clock, base 0x70, IRQ8
	ioread	0x70, 0x0b	;   select Control Register B
	and al, ~00000101b	;   disable DST (0=0), enable BCD (2=0)
	or  al,  00000010b	;   enable 24-hour Mode (1=1)
	out dx, al

	mov	dx, 0x70	; read CRD until the oscillator is stable
	mov	al, 0x0d 	
	out	dx, al		
	inc	dx		
	
.alpine_rtc:			
	in	al, dx	
	out	0xed, al	; delay
	in	al, dx	
	test	al, 0x80
	jz	.alpine_rtc
	
	sio_setup 4, PM_BASE	; ENABLE System Wake-Up Control, base 0x500
	pnp_write 0x62, 0x05	;  [PM1b_EVT_BLK]
	pnp_write 0x63, 0x20	;    set base 0x520
	pnp_write 0x64, 0x05	;  [PM1b_CNT_BLK]
	pnp_write 0x65, 0x30	;    set base 0x530
	pnp_write 0x66, 0x05	;  [GPE1_BLK]
	pnp_write 0x67, 0x40	;    set base 0x540

	mov dx, PM_BASE+0xf	; select the ACPI bank of SWC
	mov al, 0x2		
	out dx, al
	mov dx, PM_BASE+0x10	; CONFIGURE sleep-type encodings
	mov al, 0
slp_typ:
	out dx, al
	inc dx
	inc al
	cmp al, 5
	jle slp_typ

	mov dx, PM_BASE+0x17	; enable PWRBTN and RTC Alarm ACPI Events
	mov al, 0x05
	out dx, al
	
%if 0
       	mov dx, PM_BASE		; set up RTC Alarm Event to allow wake-up
	mov al, 0x18		; RTC_EVT_STS
	out dx, al
	inc dx
	mov al, 0x3		; allow wake from S3 or S45
	out dx, al
%endif
        
	mov dx, PM_BASE+0x31    ; Set Sleep state 0
	mov al, 0x20
	out dx, al
	
	mov dx, PM_BASE+0xa	;  [LEDCTL]
	mov al, 0x20		;    set 2 leds, low polarity
	out dx, al
	inc dx
	mov al, 0x77
	out dx, al		; turn both LOGO and SYSFAULT on

	mov dx, PM_BASE+0xf	; select the WDT bank of SWC
	mov al, 0x3		
	out dx, al
	mov dx, PM_BASE+0x11	; set the WDT timeout to 0
	mov al, 0
	out dx, al
	mov dx, PM_BASE+0x12	; enable the WDT software-refresh trigger
	mov al, 0x80
	out dx, al

	sio_setup 7, GPIO_BASE	; ENABLE General Purpose In/Out, base 0x600
	pnp_write 0xf0, 0x22	;  [GPIO22] FAN1_RPM
	pnp_set   0xf1, 0x1	;           output enable
	pnp_write 0xf0, 0x23	;  [GPIO23] FAN2_RPM
	pnp_set   0xf1, 0x1	;           output enable
	pnp_write 0xf0, 0x24	;  [GPIO24] FAN3_RPM
	pnp_set   0xf1, 0x1	;           output enable
	pnp_write 0xf0, 0x25	;  [GPIO25] PS_FAN_RPM
	pnp_set   0xf1, 0x1	;           output enable
	pnp_write 0xf0, 0x26	;  [GPIO26] CPU_FAN
	pnp_set   0xf1, 0x1	;           output enable
	mov dx, GPIO_BASE+6	; GPDO base for port 2
	in al, dx		; 
	and al, ~01000000b	; drive GPIO26 signal low
	out dx, al		; to turn CPU FAN on

	pnp_write 0xf0, 0x30	; configure GPIO 30 (Vbat select) as output
	pnp_write 0xf3, 0x10	; set VDDLOAD bit
	mov dx, GPIO_BASE+0x8	; and set it to off
	in al, dx
	and al, 0xfe	
	out dx, al
	pnp_write 0xf1, 0x03	; set for push-pull, output enabled 

	pnp_write 0xf0, 0x42	; set wake-on-power GPIOE42 to active high
	pnp_write 0xf1, 0x20	; this makes sure we only get the event
	pnp_write 0xf2, 0	; after power-loss

	mov dx, PM_BASE		; set up wake-on-LAN to allow wake-up
	mov al, 0xe		; GPIOE46		
	out dx, al
	inc dx
	mov al, 0x3		; allow wake from S3 or S45
	out dx, al
	
	pnp_write 0xf0, 0x55	; configure GPIO 55 (WEBLED) as output
	pnp_write 0xf1, 0x01	;
	mov dx, GPIO_BASE+0xe	; and set it to off
	in al, dx
	or al, 0x20
	out dx, al

	pnp_write 0xf0, 0x60	; set up GPO60 (IDE2_RST) as output
	pnp_write 0xf3, 0x10	; set VDDLOAD bit
	mov dx, GPIO_BASE+0x10	; and set it to off
	in al, dx
	and al, 0xfe
	out dx, al	
	pnp_write 0xf1, 0x03	; set for push-pull, output enabled

	jmp iomagic_done	


;#############################################################
;
;   RaQ-XTR  :  ServerWorks OSB4 + National SuperIO PC87314
;
;#############################################################
iomagic_5k_raqxtr:

	cm_write 0x80, 0x0
	cm_write 0x81, SIO_BASE
	cm_write 0x82, 0x0
	cm_write 0x84, SIO_BASE+1
	cm_write 0x85, 0x0

	sio_setup 8, PM_BASE	; ENABLE Power Management
	pm_write 0x08, 0x10	;  set PM1 EVT register
	pm_write 0x09, 0x05	;  I/O port: 0x510
	pm_write 0x0c, 0x20	;  set PM1 CNT register
	pm_write 0x0d, 0x05	;  I/O port: 0x520
	pm_write 0x0e, 0x30	;  set GPE1 register
	pm_write 0x0f, 0x05	;  I/O port: 0x530

	ioread	PM_BASE, 0x1	; set FER2
	or	al, 10111000b	; enable GPIO, GPE, PM1 CNT, and PM1 EVT
	out	dx, al

	mov	dx, 0x530	; clear GPE status
	in	al, dx
	out	dx, al

	;; wait for alpine for this  turn in off for now
	mov	dx, 0x534	; enable PME1 for wake on lan on ptlobos
	in	al, dx
	and	al, ~00000001b	; PME1_E is bit 0	
	out	dx, al

	
;force SuperI/O configuration 1 register (SIOC1, reg 21h)
;to use UART2 rather than GPIO30-36
;this is done because the hardware strap on the motherboard
;is incorrect and the National Semiconductor documentation
;doesn't list the strap configuration
;
	mov	dx, 02Eh
	mov	al, 21h
	out	dx, al
	inc	dx
	in	al, dx
	or	al, 8		; select UART2
	out	dx, al

	mov	al, 0x03	; select Power Management Control 2 Register (PMC2)
	mov	dx, 0x500
	out	dx, al
	mov	al, 0x07	; bits 1,0 - (11) set clock source to on-chip clock multiplier
	mov	dx, 0x501	; bit 2    -  (1) enable on-chip clock multiplier
	out	dx, al

clock_stable:			; now poll for clock stable
	out	0xed, al	; delay
	in	al, dx
	test	al, 0x80	; if bit 7 set then on-chip clock is stable and toggling
	jz	clock_stable
	
	; DISABLE
	pnp_disable 1		; Mouse
	pnp_disable 3		; FDC

	; ENABLE
	pnp_enable 6		; UART1
	pnp_enable 5		; UART2

	pnp_enable 4		; PP
	pnp_write 0x60, 0x03	; I/O Port: 0x378
	pnp_write 0x61, 0x78
	pnp_write 0xf0, 0x71	; EPP 1.9 mode
	
	pnp_enable 2

	pnp_write 0x60, 0x00    ; I/O Port: 0x70
	pnp_write 0x61, 0x70    ; 
	pnp_write 0x70, 0x08    ; interrupt: 0x08

	mov	dx, 0x70	; make sure the rtc is in 24hour/BCD mode
	mov	al, 0x0b
	out	dx, al
	inc	dx
	in	al, dx
	and	al, 0xfa	; turn off DST (bit 0), enable BCD (bit 2)
	or	al, 0x02	; turn on 24h mode (bit 1)
	out	dx, al

	mov	dx, 0x70	; read CRD so that we can bank select
	mov	al, 0x0d 	; this is so silly (we ignore the read)
	out	dx, al		; even sillier. we have to do this multiple
	inc	dx		; times until we're running off of Vbat

.xtr_rtc:			; Wait for the oscillator to decide to stay
	in	al, dx		; on during power fail
	out	0xed, al	; pause
	in	al, dx	
	test	al, 0x80
	jz	.xtr_rtc
	
	mov	dx, 0x70	; select bank 2 using CRA
	mov	al, 0x0a
	out	dx, al		
	inc	dx
	in	al, dx
	and	al, 0x8f        ; mask off bits 4-6
	or	al, 0x40	; bank 2
	out	dx, al

	mov	dx, 0x70	; stop failsafe timer and clear
	mov	al, 0x40	; power failure bit (bits 6 and 7 of APCR1)
	out	dx, al		; make sure the MOAP bit (4) is clear
	inc	dx	
	in	al, dx
	or	al, 0xc0
	and	al, 0xef
	out	dx, al

	mov	dx, 0x70	; select RING with APCR2 (bits 1, 3 WOL)
	mov	al, 0x41	; select wake on RI2 (bit 5, WOS)
	out	dx, al
 	inc	dx
	in	al, dx
	or	al, 0x2a
	out	dx, al	

	mov	dx, 0x70	; make sure GPIO24 doesn't use IRRX1
	mov	al, 0x49	; and that LED is connected (APCR3)
	out	dx, al
	inc	dx
	in	al, dx
	and	al, 0xf7	; (GPIO24 is bit 3)
	or	al, 0x04	; (LED is bit 2)
	out	dx, al

	mov	dx, 0x70	; set PME1 (wake on lan) to trigger on falling edge
	mov	al, 0x4a	; set PME2 (power application) to trigger on raising edge
	out	dx, al
	inc	dx
	in	al, dx
	and	al, 0xc0
	or	al, 0x35
	out	dx, al

	mov	dx, 0x70	; select RI2 with APCR5 (bit 6 clear)
	mov	al, 0x4b
	out	dx, al
	inc	dx
	in	al, dx
	and	al, 0xbf
	out	dx, al

	;;  we now use these bits instead of MOAP
%if 0
	mov	dx, 0x70	; clear bits 6 and 7 of APCR6 so that
	mov	al, 0x4c	; MOAP bit (4) of APCR1 determines
	out	dx, al		; everything
	inc	dx	
	in	al, dx
	and	al, 0x7f
	out	dx, al
%endif
	
	mov	dx, 0x70	; select bank 0 using CRA
	mov	al, 0x0a
	out	dx, al		
	inc	dx
	in	al, dx
	and	al, 0x8f        ; mask off bits 4-6
	or	al, 0x20	; bank 0
	out	dx, al						

	pnp_enable 7		; GPIO
	pnp_write 0x60, 0x06    ; I/O port: 0x600
	pnp_write 0x61, 0x00    ; 
	mov	dx, 0x2e	; SIOC2 (GPIO assignments)
	mov	al, 0x22
	out	dx, al
	inc	dx
	in	al, dx
	or	al, 0x44 	; select POR and WDO
	and	al, 0x7f	; select bank 0 GPIO registers	
	out	dx, al
	mov	dx, 0x601	; set watchdog (WDO, GPIO17) as output.
	in	al, dx	
	or	al, 0x80
	out	dx, al

	mov	al, 0x54
	mov	dx, 0x43
	out	dx, al
	out	0xed, al	; delay
	mov	al, 0x18
	mov	dx, 0x41
	out	dx, al
  
	mov	al, 0xff	; set NCP Error Register
	mov	dx, 0xf0	; bit 7 is warm/cold boot indicator
	out	dx, al		; it is set when any value is written
	
	jmp	iomagic_done
		
pci_init_table_5k:

	;###################################
	;  PCI0
	;  ServerWorks NorthBridge (fn 0)
	;  HOST-BRIDGE & MEMORY CONTROLLER
	;###################################

	;----- PCI Command
	; <6>  PERR# signal reporting
	; <2>  Processor->PCI bus cycle forwarding
	; <1>  Enable Memory access
	; <0>  Enable I/O Access
	pci_config_byte SW_PCI0, 0x04, 0x46

	;----- PCI Command
	; <0>  SERR# signal reporting
	pci_config_byte SW_PCI0, 0x05, 0x01

	;----- PCI Master Latency Timer
	pci_config_byte SW_PCI0, 0x0d, 0x40

	;----- Error Command
	; <7>  SERR# on Received Target Abort
	; <6>  SERR# on Transmitted Data Parity Error
	; <5>  SERR# on Received Data Parity Error
	; <4>  SERR# on Address Parity Error
	; <3>  PERR# on Receiving Data Parity Error
	; <2>  SERR# on ECC Uncorrectable Error
	; <1>  SALERT for Correctable Error
	; <0>  SERR# on Received Master Abort
	pci_config_byte SW_PCI0, 0x46, 0x00

	;----- Error Status
	; <6>  PCI Transmitted Data Parity Error
	; <5>  PCI Received Data Parity Error
	; <4>  PCI Address Parity Error
	; <2>  ECC Uncorrectable Error
	; <1>  ECC Correctable Error
	; <0>  Shutdown Cycle Detected
	pci_config_byte SW_PCI0, 0x47, 0xff

	;----- Parity / APIC command
	; <7>  Enable APIC
	; 6:5  APIC base = 0xfec00000
	; <0>  Scrubbing Enable
	pci_config_byte SW_PCI0, 0x48, 0x81

	;----- Bridge Config
	; <4>  memory write posting enable
	; <2>  assert INIT# on shutdown
	; 1:0  arbitration
	;        00 - no arbitration, single bridge mode
	;        01 - internal arbitration, dual bridge mode
	;        10 - external arbitration, multiple bridge mode
	;        11 - reserved
	pci_config_byte SW_PCI0, 0x49, 0x14

	;----- Bridge Type
	; <7>  VGA ISA Alias enable(1)
	; <6>  VGA enable(1)
	; <5>  Flush Req. Sync. enable
	; <4>  PCI Back-to-Back enable
	; <3>  Write combining enable(1)
	; <2>  Reserved
	; <1>  Write Posting enable(1)
	; <0>  Defer proc-bus cycles(1)
	pci_config_byte SW_PCI0, 0x4b, 0x2b

	; Put memory hole at 3G-4G  This can be shrunk later
	pci_config_byte SW_PCI0, 0x74, 0x00
	pci_config_byte SW_PCI0, 0x75, 0x0c
	pci_config_byte SW_PCI0, 0x76, 0xff
	pci_config_byte SW_PCI0, 0x77, 0x1f

	pci_config_byte SW_PCI1, 0x74, 0x00
	pci_config_byte SW_PCI1, 0x75, 0x0c
	pci_config_byte SW_PCI1, 0x76, 0xff
	pci_config_byte SW_PCI1, 0x77, 0x1f

	;----- Debug Options
	; <7>  Disable proc->pci prefetching
	; <0>  CPU to PCI second buffer(1)
	pci_config_byte SW_PCI0, 0x7a, 0x81

	;----- Memory Timing Ctl
	; <7>  tRAS:	0=6 1=5 Clks
	; <6>  tRCD:	0=3 1=2 Clks
	; <5>  tRP:	0=3 1=2 Clks
	; 4:3  tRC:	00=10 01=9 10=8 11=7 Clks
	; <2>  tRWL:	0=2 1=1 Clks
	; <1>  Enable Memory Overlapping (tRP=3 tRCD=3 CAS=3)
	pci_config_byte SW_PCI0, 0x91, 0x00

	;----- Bridge Config 2
	; <4>  Bit 8 of MRDRU7 (set if 4GB)
	; <3>  Enhanced page hit timing(1)
	;      0: 10-1-1-1
	;      1:  9-1-1-1
	; <2>  Single Clock ECC Enable
	; <1>  SDRAM CAS Latency (0=3, 1=2)
	; <0>  Memory Controller Enable
	pci_config_byte SW_PCI0, 0x92, 0x08

	;----- Bridge Config 3
	; <4>  Fast addr decode for memory
	;      0: two clock addr decode
	;      1: one clock addr decode
	pci_config_byte SW_PCI0, 0x93, 0x10

	;----- Performance Option 2
	; <6>  proc->PCI read streaming(1)
	; <5>  Memory POWERDOWN mode(1)
	; <4>  Disable PCI to PCI cycles
	; <3>  proc->PCI write streaming(1)
	pci_config_byte SW_PCI0, 0xcf, 0x48

	;----- Memory Read IDLE
	pci_config_byte SW_PCI0, 0xde, 0x10

	;----- Performace Option 1
	; <7>  PCI->memory trash protect(1)
	; <4>  Burst Write combining(1)
	; <1>  Page Closing Policy enable(1)
	; <0>  Read Idle Counter enable(1)
	pci_config_byte SW_PCI0, 0xdf, 0x93
	
	;----- Drive Strength
	; <5>  Enable hot plug PCI for secondary PCI bus
	; <4>  Chip select Signal buffer
	; <3>  Memory Address Buffer Select, copy A
	; <2>  Enable hot plug PCI for primary PCI bus
	; <1>  Memory Address Buffer Select, copy B
	; <0>  Memory Data Buffer Select
	;Hard coded for:
	;Chip Select signal buffer always high
	;Memory address buffer, copy A & B: always low
	;Memroy data buffer select: always high
	pci_config_byte SW_PCI0, 0xfd, 0x11
	
	;----- CPU Presence Register
	; count from 0
	pci_config_byte SW_PCI0, 0xce, 0x00
	

	;#########################
	;  PCI1
	;  NorthBridge (fn 1)
	;  SECONDARY HOST BRIDGE
	;#########################

	pci_config_byte SW_PCI1, 0x04, 0x47
	pci_config_byte SW_PCI1, 0x05, 0x01
	pci_config_byte SW_PCI1, 0x4b, 0x0b
	pci_config_byte SW_PCI1, 0x0d, 0x40
	pci_config_byte SW_PCI1, 0x46, 0x00
	pci_config_byte SW_PCI1, 0x47, 0xff

	;----- Bus Number Registers for Configuration Cycles
	;
	; 1.  BUSNUM(f0) -> DO NOT SET
	; 2. SBUSNUM(f0) -> highest bus ID behind LE(f0) [0=start]
	; 3.  BUSNUM(f1) -> SBUSNUM(f0) + 1
	; 4. SBUSNUM(f1) -> highest bus ID behind [BUSNUM(f1)=start]
	;
	pci_config_byte SW_PCI0, 0x45, 0x00; SBUSNUM(f0)=0
	pci_config_byte SW_PCI1, 0x44, 0x01;  BUSNUM(f1)=1
	pci_config_byte SW_PCI1, 0x45, 0xff; SBUSNUM(f1)=1

	;  PCI MEMORY REGION PROGRAMMING
	;#################################
	; moved here to support 4Gig

	; invalidate / disable write-combining
	pci_config_byte SW_PCI0, 0xcc, 0x10
	pci_config_byte SW_PCI0, 0xcd, 0x00
	; primary bus region 0
	pci_config_byte SW_PCI0, 0xc0, 0x00
	pci_config_byte SW_PCI0, 0xc1, 0x00
	pci_config_byte SW_PCI0, 0xc2, 0x00
	pci_config_byte SW_PCI0, 0xc3, 0x00
	; primary bus region 1
	pci_config_byte SW_PCI0, 0xc4, 0x00
	pci_config_byte SW_PCI0, 0xc5, 0x00
	pci_config_byte SW_PCI0, 0xc6, 0x00
	pci_config_byte SW_PCI0, 0xc7, 0x00
	; primary bus region 2   set this for IOAPIC and up
	pci_config_byte SW_PCI0, 0xc8, 0xc0
	pci_config_byte SW_PCI0, 0xc9, 0xfe
	pci_config_byte SW_PCI0, 0xca, 0xff
	pci_config_byte SW_PCI0, 0xcb, 0xff

	; invalidate / disable write-combining
	pci_config_byte SW_PCI1, 0xcc, 0x00
	pci_config_byte SW_PCI1, 0xcd, 0x00
	; secondary bus region 0
	pci_config_byte SW_PCI1, 0xc0, 0x00
	pci_config_byte SW_PCI1, 0xc1, 0x00
	pci_config_byte SW_PCI1, 0xc2, 0x00
	pci_config_byte SW_PCI1, 0xc3, 0x00
	; secondary bus region 1
	pci_config_byte SW_PCI1, 0xc4, 0x00
	pci_config_byte SW_PCI1, 0xc5, 0x00
	pci_config_byte SW_PCI1, 0xc6, 0x00
	pci_config_byte SW_PCI1, 0xc7, 0x00
	; secondary bus region 2
	pci_config_byte SW_PCI1, 0xc8, 0x00
	pci_config_byte SW_PCI1, 0xc9, 0x00
	pci_config_byte SW_PCI1, 0xca, 0x00
	pci_config_byte SW_PCI1, 0xcb, 0x00

	
	;##############################
	;  PCI I/O REGION PROGRAMMING
	;##############################

	; invalidate regions
	pci_config_byte SW_PCI0, 0xdc, 0x00
	; primay bus IO region 0
	pci_config_byte SW_PCI0, 0xd0, 0x00
	pci_config_byte SW_PCI0, 0xd1, 0x00
	pci_config_byte SW_PCI0, 0xd2, 0x00
	pci_config_byte SW_PCI0, 0xd3, 0x00
	; primary bus IO region 1
	pci_config_byte SW_PCI0, 0xd4, 0x00
	pci_config_byte SW_PCI0, 0xd5, 0x00
	pci_config_byte SW_PCI0, 0xd6, 0x00
	pci_config_byte SW_PCI0, 0xd7, 0x00
	; primary bus IO region 2
	pci_config_byte SW_PCI0, 0xd8, 0x00
	pci_config_byte SW_PCI0, 0xd9, 0x00
	pci_config_byte SW_PCI0, 0xda, 0x00
	pci_config_byte SW_PCI0, 0xdb, 0x00

	; invalidate regions
	pci_config_byte SW_PCI1, 0xdc, 0x00
	; secondary bus IO region 0
	pci_config_byte SW_PCI1, 0xd0, 0x00
	pci_config_byte SW_PCI1, 0xd1, 0x00
	pci_config_byte SW_PCI1, 0xd2, 0x00
	pci_config_byte SW_PCI1, 0xd3, 0x00
	; secondary bus IO region 1
	pci_config_byte SW_PCI1, 0xd4, 0x00
	pci_config_byte SW_PCI1, 0xd5, 0x00
	pci_config_byte SW_PCI1, 0xd6, 0x00
	pci_config_byte SW_PCI1, 0xd7, 0x00
	; secondary bus IO region 2
	pci_config_byte SW_PCI1, 0xd8, 0x00
	pci_config_byte SW_PCI1, 0xd9, 0x00
	pci_config_byte SW_PCI1, 0xda, 0x00
	pci_config_byte SW_PCI1, 0xdb, 0x00
	
	;----- SMBus IO Base Address
	;      [MONTEREY==ALPINE]
	pci_config_byte SW_SOUTH, 0x90, (0xff & (SMB_BASE))
	pci_config_byte SW_SOUTH, 0x91, (0xff & (SMB_BASE >> 8))
	pci_config_byte SW_SOUTH, 0x92, (0xff & (SMB_BASE >> 16))
	pci_config_byte SW_SOUTH, 0x93, (0xff & (SMB_BASE >> 24))

	;----- SMBus host config: enable controller (bit 0)
	;      [MONTEREY==ALPINE]
	pci_config_byte SW_SOUTH, 0xd2, 0x01

	; end config
	pci_config_byte 0,0,0

	
pci_init_table_5k_raqxtr:

	;#################################
	;        BOARDTYPE_MONTEREY
	;  SERVERWORKS OSB4 SOUTH BRIDGE
	;#################################

	;----- PCI COMMAND
	; 2=1  enable bus master
	; 1=1  enable Memory access
	; 0=1  enable I/O access
	pci_config_byte PCI_ID_OSB4, 0x04, 0x07
	pci_config_byte PCI_ID_OSB4, 0x05, 0x00

	;----- PCI CONTROL
	; 4=1  SREQ Idle (no active DMA)
	; 5=1  EDMA State Machine Idle
	; 6=1  ISA-to-PCI I/O Enable
	; 7=1  EREQ_ State Machine Idle
	pci_config_byte PCI_ID_OSB4, 0x40, 0xf0

	;----- MISC FUNCTION CONTROL
	; 5=1  Client Management and Security Enable
	; 4=1  Additional ROM Enable:	000E0000h to 000EFFFFh
	; 1=1  Additional ROM Enable:	FFF80000h to FFFDFFFFh
	; 0=1  PCI Bus frequency @33MHz (div by 4)
	pci_config_byte PCI_ID_OSB4, 0x41, 0x33

	;----- ISA ADDRESS DECODE CONTROL
	; 3=0  896K -> 960K = addional system ROM at E0000h
	; 2=1  640K -> 768K = video RAM
	; 1=1  512K -> 640K = base memory 1
	; 0=1    0K -> 640K = base memory 0
	pci_config_byte PCI_ID_OSB4, 0x48, 0x07

	;----- OSB4 FEATURE ENABLE (32 bit)
	; 23=0  IDEIRQOUT_EN	(IDE Bus Master IRQ's on IRQ10/11)
	; 15=1  IOAPIC Function Enable
	; 14=1  DMA33 Enable
	; 13=1  interrupt 600ns
	; 11=0  sec_ide_mask	(1=access sent to ISA)
	; 10=0  ioapic_irq_oen	(USBTO0=irq0,USBTO1=irq13)
	;  9=0  usb_legacy_en	
	;  8=1  usb_en
	;  7=0  external arbiter enabled (ro)
	;  6=0   |
	;  5=0   |- gpio_en[2:0]
	;  4=0   |
	;  3=1  ioapic_enable	(0xfec00010-fec00013)
	;  2=1  smi_enable	(Power Management, io=cd6-cd7)
	;  1=1  tmr_enable	(8254 Timer, io=040-043)
	;  0=1  pic_enable	(8259 PIC)
	pci_config_byte PCI_ID_OSB4, 0x64, 0x1f
	pci_config_byte PCI_ID_OSB4, 0x65, 0xc1
	pci_config_byte PCI_ID_OSB4, 0x66, 0x00
	pci_config_byte PCI_ID_OSB4, 0x67, 0x00

	;----- RUN TIME TEST MODE (32 bit)
	; 19=1  GPM[012]X and GPOC3 function as GEVENT pins
	; 18=1  GPCS[012]L function as GEVENT pins
	;  7=1  FFC00000h to FFF7FFFFh Additional System ROM
	;  0=0  Port92 ~ FAST_INIT, FAST_A20 disabled
	pci_config_byte PCI_ID_OSB4, 0x70, 0x80
	pci_config_byte PCI_ID_OSB4, 0x71, 0x00
	pci_config_byte PCI_ID_OSB4, 0x72, 0x0c
	pci_config_byte PCI_ID_OSB4, 0x73, 0x00

	;----- IOAPIC BASE ADDRESS (map to 0xfec0X000 and 0xfec0X010)
	; 4=1  A12 Mask (decode 2 IOAPIC ranges)
	; 3:0  X-base address
	pci_config_byte PCI_ID_OSB4, 0x74, 0x10

	pci_config_byte 0,0,0


pci_init_table_5k_alpine:

	;#################################
	;        BOARDTYPE_ALPINE
	;  SERVERWORKS CSB5 SOUTH BRIDGE
	;#################################

	;----- PCI COMMAND
	; 2=1  enable bus master
	; 1=1  enable Memory access
	; 0=1  enable I/O access
	pci_config_byte PCI_ID_CSB5, 0x04, 0x07
	pci_config_byte PCI_ID_CSB5, 0x05, 0x00

	;----- PCI CONTROL
	; 6=0  MEM_OFF output enable
	; 5=0  enable SMI on GPIO/GEVENT
	; 4=0  reset SMBus/I2C (must set then clear)
	pci_config_byte PCI_ID_CSB5, 0x40, 0x00

	;----- FUNCTION CONTROL
	; 6=0  PCI->LPC delayed read cycle enable
	; 5=1  Client mgmt/security enable (IO at C50h-C52h and C06h-C08h)
	; 4=1  FLUSHBLK enable (90ns masking of flush request)
	; 3=1  ROMCS# enable :  64K : FFFE0000h-FFFEFFFFh (and E0000h-EFFFFh)
	; 2=1  ROMCS# enable :   4M : FFC00000h-FFF7FFFFh
	; 1=1  ROMCS# enable : 384K : FFF80000h-FFFDFFFFh
	; 0=0  IDE PCI req/gnt output in Ext. Arbiter Mode
	pci_config_byte PCI_ID_CSB5, 0x41, 0x3e

	;----- USB & INTERRUPT CONFIGURE
	; 5=0  USB over-current 128ms delay enable
	; 4=0  USB port select (#ports: 0=2, 1=4)
	; 3=0  USB 1.1 enable (0=USB1.0, 1=USB1.1)
	; 2=0  Timer interrupt delivery on IOAPIC0 (0=INT0, 1=INT2)
	; 1=1  Keyboard A20 enable
	; 0=0  USB A20 enable (1=combine with A20 pin)
	pci_config_byte PCI_ID_CSB5, 0x42, 0x02

	;----- ADDITIONAL FEATURE CONTROL
	; 3=1  Flush Block enable
	; 2=1  IDE OSB4 Mode enable
	; 1=0  RAID enable
	pci_config_byte PCI_ID_CSB5, 0x43, 0x0c

	;----- LPC ADDRESS DECODE CONTROL
	; 3=0  896K -> 960K = addional system ROM at E0000h
	; 2=1  640K -> 768K = video RAM
	; 1=1  512K -> 640K = base memory 1
	; 0=1    0K -> 640K = base memory 0
	pci_config_byte PCI_ID_CSB5, 0x48, 0x07

	;----- CSB5 FEATURE ENABLE (32 bit)
	;      SPEC: 00000000 01010010 00000000 10001111
	;      EVAL: 00000001 11010010 11000001 11000111
	;  7=1  I2C slave interface enable
	;  6=1   |
	;  5=0   | GPCS[2:0] enable (for PORT 80)
	;  4=1   |
	;  3=0  XIOAPIC enable (0=IOAPIC, 1=XIOAPIC)
	;  2=1  Power Mgmt enable (IO at CD6h-CD7h)
	;  1=1  8254 Timer enable
	;  0=1  8259 PIC enable
	pci_config_byte PCI_ID_CSB5, 0x64, 0xd7
	; 15=1  (X)IOAPIC enable
	; 14=1  IDE enable (PCI fn 1)
	; 13=0   | XIOAPIC base address (PCI addr bits 15-14)
	; 12=0   | 
	; 11=0  IOAPIC2 enable (+16 PCI IRQs, IOAPIC mode only)
	; 10=0  XIOAPIC IRQ map enable (1=IRQ0/IRQ13 are GPIO)
	;  9=0  Legacy Kbd/Mouse in USB enable (must set bit 8 first)
	;  8=1  USB enable (PCI fn 2)
	pci_config_byte PCI_ID_CSB5, 0x65, 0xc1
	; 23=0  XIOAPIC3 enable (PCI fn 7)
	; 22=1  LPC Subtractive Decode enable
	; 21=0  LPC Prog-I/O snoop enable [reserved in A2.0+]
	; 20=1  LPC Bus enable (PCI fn 3)
	; 19=0  IDE controller reset enable
	; 18=0  IDE clock select enable
	; 17=1  PMBUS serial (SCK_REQ) enable
	; 16=1  Port 92h enable (0=disable & INIT+A20 bits masked)
	pci_config_byte PCI_ID_CSB5, 0x66, 0x53
	; 29=0  Ext. Arbiter for IDE req/gnt enable 
	; 28=0  IRQ15 routing (0=IDE, 1=bypass)
	; 27=0  IRQ14 routing (0=IDE, 1=bypass)
	; 26=0  XIOAPICn hide enable (PCI fn 4,5,6,7)
	; 25=0  IRQ8 input polarity (0=low, 1=high)
	; 24=1  DMA Verify mode enable (1=map to write mode)
	pci_config_byte PCI_ID_CSB5, 0x67, 0x01

	;----- FEATURE ENABLE 1 (32 bit)
	; 4=0  Test Mode enable
	; 3=1  X-Bus address select (0=2, 1=3 ext. components)
	; 2=1  X-ROM enable
	pci_config_byte PCI_ID_CSB5, 0x70, 0x0c
	pci_config_byte PCI_ID_CSB5, 0x71, 0x00
	pci_config_byte PCI_ID_CSB5, 0x72, 0x00
	pci_config_byte PCI_ID_CSB5, 0x73, 0x00

	;----- IRQ MODE CONTROL
	; 7=1  Scan enable (0=all interrupts blocked)
	pci_config_byte PCI_ID_CSB5, 0x78, 0x80

	;----- IMB CONTROL 0
	; 7=1  FLUSHREQ/MEMACK enable
	; 6=0  Special Cycle Encoding enable
	; 3=0  Thrash enable
	; 2=0  Burst Write enable
	; 1=0  Burst Read enable
	pci_config_byte PCI_ID_CSB5, 0xa0, 0x80

	;----- IMB CONTROL 1
	; 3=1  Memory Access enable
	; 2=1  Prefetch Read Cycles enable
	; 1=1  Tag IOCACHE Clear enable
	; 0=0  IMB Lock Order enable
	pci_config_byte PCI_ID_CSB5, 0xa1, 0x0e

	;----- IMB BTR2 CONFIG REGISTER
	; 0=1  Must be set to enable IDE DMA
	pci_config_byte PCI_ID_CSB5, 0xac, 0x01


	;################## LPC ####################

	;----- LPC PCI COMMAND
	pci_config_byte SW_SOUTH_LPC, 0x04, 0x05
	pci_config_byte SW_SOUTH_LPC, 0x05, 0x00

	;----- LPC PCI CLASS CODE
	;pci_config_byte SW_SOUTH_LPC, 0x0a, 0x01

	;----- LPC PCI LATENCY TIMER
	pci_config_byte SW_SOUTH_LPC, 0x0d, 0x40
	
	;----- LPC PCI CONTROL
	; 2=1  LPC-to-PCI DMA enable
	pci_config_byte SW_SOUTH_LPC, 0x40, 0x04

	;----- LPC CONTROL 0
	; 7=1  Serial (2F8-2FF)
	; 6=1  Serial (3F8-3FF)
	; 0=1  Parallel (378-37F)
	pci_config_byte SW_SOUTH_LPC, 0x44, 0xc1

	;----- LPC CONTROL 3
	; 6=1  enable ACPI MICRO (62h and 66h)
	pci_config_byte SW_SOUTH_LPC, 0x47, 0x40

	;----- LPC CONTROL 4
	; 0=1  enable SIO config at (2Eh and 2Fh)
	pci_config_byte SW_SOUTH_LPC, 0x48, 0x01

	;----- LPC MEMORY HOLE START
	pci_config_byte SW_SOUTH_LPC, 0x60, 0x00
	pci_config_byte SW_SOUTH_LPC, 0x61, 0x00

	;----- LPC MEMORY HOLE END
	pci_config_byte SW_SOUTH_LPC, 0x62, 0x00
	pci_config_byte SW_SOUTH_LPC, 0x63, 0x00

	;----- WIDE GENERIC PORT BASE
	pci_config_byte SW_SOUTH_LPC, 0x64, 0x00
	pci_config_byte SW_SOUTH_LPC, 0x65, 0x00

	pci_config_byte 0,0,0
