;$Id: reset.S,v 1.1.1.1 2003/06/10 22:42:25 iceblink Exp $
; Copyright 1999-2000, Cobalt Networks, Inc.
; 
; this file contains code in the reset vector segment. 
; written for nasm
;	
; Author(s): Erik Gilling, Tim Hockin, Duncan Laurie
;

%macro POST 1
        mov     al, %1
        out     0x80, al
        jmp     %%1
%%1:    jmp     %%2
%%2:    
%endmacro

%define PROBE_DRAM
%define new_stack_code
	
BITS 16

ROMSIZE			equ	128
BOOT_SEG		equ	0xF000
ROM_RESET		equ	0xFFF0
k_CS			equ	0x10
k_DS			equ	0x18
	
PCI_IO_CONFIG_PORT	equ	0x370
PCI_IO_INDEX_PORT	equ	0x370
PCI_IO_DATA_PORT	equ	0x371
	
PCI_CONFIG_ADDR		equ	0xCF8
PCI_CONFIG_DATA		equ	0xCFC

PCI_CONFIG_ENABLE	equ	0x1F
PCI_CONFIG_BUS		equ	0x10
PCI_CONFIG_DEVICE	equ	0x0B
PCI_CONFIG_FUNCTION	equ	0x08
PCI_CONFIG_REGISTER	equ	0x02

SMB_IO_BASE		equ	0x3A80

UART_BASE	equ	0x3F8	; UART1 Base Address
UART_DLL	equ	0	; Divisor Latch LSB
UART_DLM	equ	1	; Divisor Latch MSB
UART_RX		equ	0	; Receive Buffer (r)
UART_TX		equ	0	; Transmit Holding (w)
UART_IER	equ	1	; Interrupt Enable
UART_IIR	equ	2	; Interrupt Ident (r)
UART_FCR	equ	2	; FIFO Control (w)
UART_LCR	equ	3	; Line Control
UART_MCR	equ	4	; Modem Control
UART_LSR	equ	5	; Line Status
UART_MSR	equ	6	; Modem Status
UART_SCR	equ	7	; Scratch

%macro uart_get 1
	mov	dx, (UART_BASE + %1)
	in	al, dx
%endmacro
%macro uart_set 2
	mov	dx, (UART_BASE + %1)
	mov	al, %2
	out	dx, al
%endmacro

	TIMES ROMCODE_OFFSET-($-$$) DB 0

%include "pci.S"

%include "delay5k.S"
%include "i2c5k.S"
%include "adm1029.S"
	
%include "reset3k.S"
%include "dramprobe3k.S"
	
%include "reset5k.S"
%include "dramprobe5k.S"


rom_start:
	cli
	cld
	clc

	POST	0x05

	;clear out cr0
	xor	eax, eax
	mov	cr0, eax

	;invalidate cache
	wbinvd

	;invalidate TLB
	xor	eax, eax
	mov	cr3, eax

	POST	0x10

	;set stack and data segments for access to the rom	
	mov	ax, BOOT_SEG
	mov	ss, ax
	mov	ds, ax	

	;set these to begining of the address space
	xor	ax, ax
	mov	es, ax	
	mov	fs, ax
	mov	gs, ax

	;check smp capability
	is5k_sp
	jne	no_smp

	;read msr-27
	mov	ecx, 27
	rdmsr

	;check bootstrap processor flag (bit 8)
	test	eax, 0x100
	jnz	is_bsp

	;load register BX with 1 (bx is 0 for BSP only)
	xor	bx, bx
	inc	bx
	hlt

is_bsp:
	POST	0x78

no_smp:


	;if systype is 3k, then do some magic from the bios guide
	is3k_sp
	je	near magic3k_1

	jmp	pci_init

magic3k_1_done:	
	;; 	POST	0x12		

	;this is to try to detect a warm boot.... it failed
	;we solve this by using the watchdog to get a real reset
	jmp	near warmboot3k
warmboot3k_done:
	
	;initialize first set of PCI values
pci_init:
	is3k_sp
	jne	.not3k
	mov	si, pci_init_table_3k
	jmp	.loaded
	
.not3k:
	is5k_sp
	jne	.not5k
	mov	si, pci_init_table_5k
	jmp	.loaded
	
.not5k:
	POST	0xF0
	hlt

.loaded:
	xor	di, di		;table counter
	
set_pci_init:
	mov	al, [cs:si]

	cmp	al, 0
	jz	pci_init_done

	inc	si
	mov	ah, [cs:si]
	inc	si
	mov	cl, [cs:si]
	inc	si

	call_sp	wr_cfg_db

	jmp	set_pci_init

pci_init_done:
	is5k_sp
	je	.type_test
	jmp	initUART1Done

.type_test:
	is5k_type_sp BOARDTYPE_MONTEREY
	je	.type_raqxtr
	is5k_type_sp BOARDTYPE_ALPINE
	je	.type_alpine

	jmp	iomagic_done

.type_raqxtr:
	mov	si, pci_init_table_5k_raqxtr
	jmp	.type_done

.type_alpine:
	mov	si, pci_init_table_5k_alpine

.type_done:
	inc	di
	cmp	di, 2
	je	iomagic_do
	jmp	set_pci_init

iomagic_do:
	is5k_type_sp BOARDTYPE_MONTEREY
	jne	.not_raqxtr
	jmp	iomagic_5k_raqxtr

.not_raqxtr:
	is5k_type_sp BOARDTYPE_ALPINE
	jne	iomagic_done
	jmp	iomagic_5k_alpine

iomagic_done:
	call_sp	SetupUART1

initUART1Done:
	mov	ax, BOOT_SEG
	mov	ds, ax

dram_setup:	
	is3k_sp
	jne	.not3k
	jmp	setup_dram3k

.not3k:
	is5k_sp
	jne	.not5k
	jmp	setup_dram5k

.not5k:
	POST	0xF1
	hlt
	

dram_setup_done:	
warm_boot:		
	POST	0x18

	;set up a stack for the rom
	mov	ax, 0x7000
	mov	ss, ax
	mov	sp, 0xfff0
	mov	bp, 0xfff0

	POST	0x40

	is3k
	je	near probe_l2_3k

probe_l2_done:	
	POST	0x42

	;L1 cache setup
	;bits 30,29 of cr0 control cache
	;(cache disbabled if set)
	wbinvd
	mov	eax, cr0

%ifdef __ENABLE_CACHE_IN_ROMCODE__
	and	eax, 0x9fffffff
%else   
	or	eax, 0x60000000
%endif

	mov	cr0, eax

	;clear prefetch queue
	jmp	cpq3
cpq3:

	;invalidate cache
	invd

	;invalidate TLB
	xor	eax, eax
	mov	cr3, eax

	is3k
	je	near enable_l1_cache_3k
enable_l1_done:

	mov	ax, BOOT_SEG
	mov	ss, ax
	mov	ds, ax

	xor	cl, cl
	mov	si, 0x500
	mov	di, GDT

	mov	ax, 0x0
	mov	es, ax
copy_GDT_loop:	
	mov	al, [cs:di]
	mov	[es:si], al

	inc	si
	inc	di
	inc	cl

	cmp	cl, ((LGDT_Scratch - GDT) + 8)
	jnz	copy_GDT_loop

	lgdt	[es:(0x500 + (LGDT_Scratch - GDT))]
	
	POST	0xE0

	;enable protected mode
	mov	ebx, cr0
	or	bl,  1
	mov	cr0, ebx

	;clear prefetch queue
	jmp	clear_prefetch
clear_prefetch:

	;set segment
	mov	cx, (k_DS)
	mov	ds, cx
	mov	es, cx
	mov	fs, cx
	mov	gs, cx

	POST	0xD0


;clear from memory location 1MB to 32 MB
;The rest of the memory will be scrubbed in the kernel
;because it'd take a very long time to scrub the entire
;mem (up to 2GB) in the PROM
	mov 	edi, (1024 * 1024)		;start scrubbing at 1MB
	mov	ecx, (31 * 1024 * 1024 ) / 4	;number of DWORDs
	xor	eax, eax
	rep a32 stosd

%ifdef ROM_TEST_MEMORY
	;
	; test memory
test_memory:
	
	;leave botom 1M alone
	mov	esi, 0x0010fff0
	mov	ecx, 0x0010fff0

full_mem_test_loop:	
	mov	DWORD [es:esi], ecx
	mov	ebx,ecx
	not	ebx
	mov	DWORD [es:esi+4], ebx

	mov	ebx, ecx
	and	ebx, 0x000f0000
	jnz	not_one_meg
	mov	ebx, ecx
	shr	ebx, 20

	POST	bl

not_one_meg
	;check to see if pattern is valid
	cmp	DWORD [es:esi], ecx
	jnz	full_mem_test_done

	;check to see if address space is exhasted
	cmp	ecx, 0xfffffff0
	jz	full_mem_test_done

	add	ecx, 0x00010000
	add	esi, 0x00010000
	jmp	full_mem_test_loop

full_mem_test_done:	
	shr	ecx, 20

%else

	;; since we did not walk the memory we need to peek into
	;; the northbridge to find our memory size
find_memory_size:	

	is3k_sp
	jne	.not3k

	xor ecx, ecx
	
	;; read Row 7's top address boundry bits 0-7
	mov     al, 0x6e
	mov	ah, PCI_ID_1541
	call_sp rd_cfg_db
	mov	cl, al		

	;; read Row 7's top address boundry bits 8-11
	mov     al, 0x6f
	mov	ah, PCI_ID_1541
	call_sp rd_cfg_db
	and	al, 0xf
	mov	ch, al
	add	ecx, 1

	jmp .done
	
.not3k:
	is5k_sp
	jne	.not5k

	;; read the Memory Upper Limit Register (MULR)
	mov     al, 0x70
	mov	ah, PCI_ID_CNBLE
	call_sp rd_cfg_db

	and	eax, 0x000000ff ; zero out meaningless garbage

	cmp    ax, 0x0000	; do we have 4 gigs
	jne	.5k_not_four_gig
	mov     ax, 0x0100
.5k_not_four_gig:

	shl	eax, 4		; 16M chunks to 4M chunks
	mov	ecx, eax
	
	jmp .done

.not5k:
	POST	0xF1
	hlt
.done:		
%endif


%ifdef new_stack_code
	;put stack at 32M
	mov	ebx, (32 * 1024 * 1024)

	and	ebx, 0xfffffff0 ; align the stack
%else     
	   ;put stack at top of address space
        mov     ebx, ecx
	shl     ebx, 20
        and     ebx, 0xfffffff0
%endif
	
	mov	ax, (k_DS)
	mov	ss, ax
	mov	ebp, ebx
	mov	esp, ebx
	
	POST	0x50

%ifdef new_stack_code
	shl        ecx, 20
%endif
	push	DWORD ecx		

	;second set of pci values
pci_init2:	
	is3k
	jne	.not3k
	mov	esi, (0xffff0000 + pci_init_table2_3k)
	jmp	.done

.not3k:
	is5k
	jne	.not5k

	is5k_type BOARDTYPE_MONTEREY
	je	.type_raqxtr
	is5k_type BOARDTYPE_ALPINE
	je	.type_alpine
	jmp	.not5k

.type_raqxtr:
	mov	esi, (0xffff0000 + pci_init_table_5k_raqxtr)
	jmp	.done
.type_alpine:
	mov	esi, (0xffff0000 + pci_init_table_5k_alpine)
	jmp	.done

.not5k:
	POST	0xFF
	hlt
.done:	
	xor	di, di

set_pci_postinit:
	mov	bl, [esi]	
	cmp	bl, 0
	jz	pci_postinit_done

	inc	esi
	mov	bh, [esi]
	inc	esi
	mov	cl, [esi]
	inc	esi

	call	pci_write_cfg

	jmp	set_pci_postinit
pci_postinit_done:

	;enable a20 gate 
	mov	al, 0x26
	out	0x92, al

	is3k
	je	near enable_l2_cache_3k

enable_l2_cache_done

	POST	0x30

	;copy ramcode to ram
%define OPTIMIZED_COPY
%ifdef  OPTIMIZED_COPY
	mov	esi, 0xffff0000 + RAMCODE_OFFSET
	mov	edi, RAMCODE_ADDR
	mov	ecx, ((RAM_SIZE + 4) & 0xfffffffc)
	
	cld
	rep	a32 movsd	
%else
	mov	esi, 0xffff0000
	mov	edi, RAMCODE_ADDR
	
copy_rom_loop:	
	mov	eax, [esi]
	mov	[edi], eax

	cmp	si, 0xfffc
	jz	copy_rom_loop_end

	add	esi, 4
	add	edi, 4
	jmp	copy_rom_loop

copy_rom_loop_end:
%endif
	
	POST	0xC6
	
	; enable DMA refresh so EGPO pins work
	; PIT timer 1, LSB only, square wave, 128 counts
	mov	ax, 0x56
	out	0x43, ax
	mov	ax, 0x80
	out	0x41, ax

	POST 0xC7

go_ramcode:	
	jmp	DWORD (k_CS):RAMCODE_ADDR

; set UART1 to 115,200 and N81
SetupUART1:
	uart_set UART_LCR, 0x83 ;set DLAB (no break, no parity, 1 stop)
	uart_set UART_DLL, 0x01 ;set BAUD rate divisor, low byte (/1=115200)
	uart_set UART_DLM, 0x00 ;set BAUD rate divisor, high byte
	uart_set UART_LCR, 0x03 ;clear DLAB in Line Control
	uart_set UART_TX,  0x00 ;clear transmit buffer
	uart_set UART_FCR, 0x4f ;enable/clear 4-byte polled FIFOs
	uart_set UART_IER, 0x00 ;disable all Interrupts
	uart_get UART_LSR       ;query/clear Line Status
	uart_get UART_MSR       ;query/clear Modem Status
	ret_sp

;MACRO: SegDesc
;INPUT:	6
;
; %1 =(8)  BASE
; %2 =(16) base address
; %3 =(16) segment limit
; %4 =(8)  P, DPL, S, TYPE
; %5 =(8)  G, D/B, 0, AVL, LIMIT
; %6 =(8)  BASE
;
; 15   14   13   12   11   10   09   08   07   06   05   04   03   02   01   00
; [        Base address 31:24         ]   [G]  [D]  [0] [AVL] [SG length 19:16]
; [P] [DPL] [DT] [        Type        ]   [        Base address 23:16         ]
; [                            Base address 15:00                             ]
; [                           Segment length 15:00                            ]

%macro SegDesc 6
	dw	(%3&0xffff)	;00-15 = Segment Limit
	dw	(%2&0xffff)	;16-31 = Base Address

	db	(%1&0xff)	;00-07 = base  (BASE)
	db	(%4&0xff)	;08-11 = Segment Type  (TYPE)
				;   12 = Descriptor Type [0=system; 1=code/data]  (S)
				;13-14 = Descriptor Privilege Level  (DPL)
				;   15 = Segment Present  (P)
	db	((%5|%6)&0xff)	;16-19 = Segment Limit  (LIMIT)
				;   20 = Available for use by system software  (AVL)
				;   21 = 0
				;   22 = Default Operation Size [0=16bit; 1=32bit segment]  (D/B)
				;   23 = Granularity  (G)
	db	(%1<<8)		;24-31 = base  (BASE)
%endmacro

ALIGN 8
GDT:	
	dd	0
	dd	0
        SegDesc	0, 0, 0xffff, 10010011b, 0x8f, 0 ;DS  = 0x08
	SegDesc	0, 0, 0xffff, 10011010b, 0xcf, 0 ;kCS = 0x10  CPL0
        SegDesc 0, 0, 0xffff, 10010010b, 0xcf, 0 ;kDS = 0x18

LGDT_Scratch:
	dw	((LGDT_Scratch - GDT) - 1)
	dd	0x500
	dw	0
	
LGDT_Scratch2:
	dw	((LGDT_Scratch - GDT) - 1)
	dd	(0xf0000 + GDT)
	dw	0

	;reset jump vector (0xffff0)
TIMES ROM_RESET-($-$$) DB 0

	;go to the beginning 
	jmp	rom_start
.END
