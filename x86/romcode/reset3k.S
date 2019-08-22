;
; Copyright 1999, Cobalt Networks
; $Id: reset3k.S,v 1.1.1.1 2003/06/10 22:42:27 iceblink Exp $
; 
; File:	reset.S
;
; Description: This file contains code in the reset vector segment. 
;
; Written for nasm
;	
; Author(s): Erik Gilling, Tim Hockin
;

ALI_VID equ		0x10b9
ALI_1541_DID equ	0x1541
	
PCI_ID_1541 equ		0
PCI_ID_1541_L equ		0
ALI_1541_ADselID equ    PCI_ID_1541_L
PCI_ID_7101 equ		((14 - 11) <<  3)
PCI_ID_7101_L equ		((14 - 11) <<  11)
PCI_ID_1543 equ		((18 - 11) <<  3)
PCI_ID_1543_L equ		((18 - 11) <<  11)
PCI_ID_5229 equ		((26 - 11) <<  3)
PCI_ID_5229_L equ		((26 - 11) <<  11)

%macro is3k_sp 0
	mov ah, PCI_ID_1541
	mov al, 0x0
	call_sp rd_cfg_dw
	mov cx, ax
	shl ecx, 16

	mov ah, PCI_ID_1541
	mov al, 0x2
	call_sp rd_cfg_dw

	and eax, 0x0000ffff
	and ecx, 0xffff0000

	or ecx, eax
	cmp ecx, (ALI_1541_DID | (ALI_VID << 16) )
%endmacro
	
%macro is3k 0
	mov bx, (PCI_ID_1541_L | 0x0 )
	mov di, 3
	call pci_read_cfg
	cmp ecx, (ALI_VID | (ALI_1541_DID << 16) )
%endmacro
		
magic3k_1:	
	;
	; magic from the bios guide.
	;

	mov dx, PCI_IO_CONFIG_PORT
	mov al, 0x51
	out dx, al
	mov al, 0x23
	out dx, al
	mov al, 0x2e
	out dx, al
	mov al, 0x02
	inc dl
	out dx, al
	mov al, 0xbb
	dec dl
	out dx,al

	jmp magic3k_1_done


warmboot3k:	
	; this is to try to detect a warm boot.... it failed
	;  we solve this by using the watchdog to get a real reset

	mov ax, (PCI_ID_1541_L+0x5a)
	PORT_80BL 0x13
	call_sp rd_cfg_db
	PORT_80 0x14

	cmp al, 0xff
	jz near warm_boot

	jmp warmboot3k_done


probe_l2_3k:	
	;
	; probe l2 cache
	;

%define _l2_cache_probe_	
%ifdef _l2_cache_probe_	
	; size l2 cache
	mov di, 0x1					; read 2 bytes!
	mov bx, (PCI_ID_1541_L+0x40)		; note this reads 0x40, 0x41
	call pci_read_cfg
	or cx, 0x0412	 			; set 512K SRAM
	test cl, 0x40				; 0x40 = cl, 0x41 = ch
	jz Tag_setup_done				; zf = use external Tag
	or ch, 0x1	 				; dis. ext Tag
Tag_setup_done:
	call pci_write_cfg

	xor dx, dx
	mov ds, dx
	mov si, dx

	mov di, 0x1					; again, read 2 bytes
	mov bx, (PCI_ID_1541_L+0x41)
	call pci_read_cfg

   	or ch, 0x99					; enable L2/non_dirty/force hi

	mov eax, 80000040h
	mov dx, 0xcf8				; FIXME: why not pci_write_cfg
	out dx, eax
	mov dl, 0xfd
	mov ax, cx
	out dx, ax

	;
	; size sram 
	;

	mov dword [ds:si], 55aa55aah		; 0:0 -> 5555aaaah
	mov dword [ds:si+10h], 0aa55aa55h	; dummy write
	mov bx, 0xc00
	cmp dword [ds:si], 55aa55aah
	jnz short no_cache 			; nz = no cache or bad
	mov bx, 0x4000				; L2 = 256k

cs_again:
	mov ds, bx
	cmp dword [ds:si], 55aa55aah	 
	jz short sizing_cache_end
	shl bx, 0x1
	cmp bh, 0x0
	jnz short cs_again
	mov bh, 0xc0				; 1M cache
	 
sizing_cache_end:
	shr bh, 0x4
	sub bh, 0x3
	and bh, 0xc 				; 0c/0/4/8h = none/256k/512k/1M

no_cache:
	cmp bh, 0x8
	jnz short cache_tmp			; zf, L2 = 1MB
   	or bh, 0x20					; set 2-bank and

cache_tmp:
	and ch, (~0x59)				; dis. L2/set hit & dirty bit normal
	and cl, (~0x0c)				; clear L2 size << k021198
	or cl, bh					; set L2 size/bank
	mov eax, 0x80000040
	mov dx, 0xcf8				; FIXME: why not pci_write_cfg
	out dx, eax
	mov dl, 0xfd
	mov ax, cx
	out dx, ax
%endif

	jmp probe_l2_done

	
enable_l1_cache_3k:		
%ifdef __ENABLE_CACHE_IN_ROMCODE__
 	mov di, 0x0	
	mov bx, (PCI_ID_1541_L+0x40)
	call pci_read_cfg
	or cl, 0x1
	call pci_write_cfg
%endif

	jmp enable_l1_done

	
enable_l2_cache_3k:	
%define _l2_cache_
%ifdef _l2_cache_

	; enable l2 cache
	mov di, 0x0
	mov bx, (PCI_ID_1541_L+0x41)
	call pci_read_cfg
	and cl, 0xc				; figure out if we have any l2
	cmp cl, 0xc
	jnz l2_present
	jmp exit_enb_L2

l2_present:

%ifdef __ENABLE_CACHE_IN_ROMCODE__
	mov di, 0x1
	mov bx, (PCI_ID_1541_L+0x41)
	call pci_read_cfg
	; ch has register 0x42
	or ch, 0x15			; turn on always dirty bit 
	call pci_write_cfg
	and cl, 0xc			; mask off size bits
	shr cl, 0x2

	mov eax, 0x40000		; 256k << size_bits = ttl_cache
	shl eax, cl

	cld

	mov esi, 0x100000
	mov ecx, eax
	wbinvd
	rep a32 es lodsd		; prime the cache

	mov di, 0x0
	mov bl, 0x42
	call pci_read_cfg

	and cl, 11101011b		; leave it in normal mode

	call pci_write_cfg
%endif

exit_enb_L2:

%endif
	jmp enable_l2_cache_done


pci_init_table_3k:
	;; 1541 config
	pci_config_byte PCI_ID_1541, 0xf6, 0x00 
	pci_config_byte PCI_ID_1541, 0xf7, 0x03 
	pci_config_byte PCI_ID_1541, 0xf5, 0x00 
	pci_config_byte PCI_ID_1541, 0xef, 0x29 
	pci_config_byte PCI_ID_1541, 0xee, 0x0c 
	pci_config_byte PCI_ID_1541, 0x73, 0x02

	;;   This was missing with the internal tag probe
	;; 	pci_config_byte PCI_ID_1541, 0x40, 0x00
	
 	pci_config_byte PCI_ID_1541, 0x42, 0x40 
 	pci_config_byte PCI_ID_1541, 0x45, 0x04 
	pci_config_byte PCI_ID_1541, 0x4b, 0x01 
	pci_config_byte PCI_ID_1541, 0x58, 0xff 
	pci_config_byte PCI_ID_1541, 0x59, 0xff 
	pci_config_byte PCI_ID_1541, 0x5f, 0x02 
	pci_config_byte PCI_ID_1541, 0x90, 0x30 
	pci_config_byte PCI_ID_1541, 0x8d, 0x00
	pci_config_byte PCI_ID_1541, 0xec, 0x00 

	;; 1543 config
	pci_config_byte PCI_ID_1543, 0x41, 0x23
	pci_config_byte PCI_ID_1543, 0x42, 0x08 
	pci_config_byte PCI_ID_1543, 0x43, 0x8f 
	pci_config_byte PCI_ID_1543, 0x47, 0x00 
	pci_config_byte PCI_ID_1543, 0x58, 0x5c 
	pci_config_byte PCI_ID_1543, 0x6d, 0x20 
	pci_config_byte PCI_ID_1543, 0x72, 0x0a 

	;; GPIO pins.  (needs to be set up early) 
	pci_config_byte PCI_ID_7101, 0x7d, 0xf0 ; 
	pci_config_byte PCI_ID_7101, 0x7e, 0xe0 ;
	
	;; end config
	pci_config_byte 0x00, 0x00, 0x00 
	
	

pci_init_table2_3k:
	;; from a5pbiosg.txt
	pci_config_byte PCI_ID_1541, 0x0d, 0x40	; PCI latency
	pci_config_byte PCI_ID_1541, 0x13, 0xe0
	
 	;pci_config_byte PCI_ID_1541, 0x40, 0x07
 	;pci_config_byte PCI_ID_1541, 0x41, 0x04
 	;pci_config_byte PCI_ID_1541, 0x42, 0x40
	pci_config_byte PCI_ID_1541, 0x43, 0x75	; pipeline stuff
	pci_config_byte PCI_ID_1541, 0x44, 0xaa	; EDO stuff
	pci_config_byte PCI_ID_1541, 0x45, 0x96	; EDO stuff
	pci_config_byte PCI_ID_1541, 0x46, 0x00	; EDO stuff
	pci_config_byte PCI_ID_1541, 0x47, 0x07 ;0x70	; EDO stuff
	pci_config_byte PCI_ID_1541, 0x48, 0x04 ;sdram config
	pci_config_byte PCI_ID_1541, 0x49, 0xe5
	pci_config_byte PCI_ID_1541, 0x4a, 0xa0
	pci_config_byte PCI_ID_1541, 0x4b, 0x5c
	pci_config_byte PCI_ID_1541, 0x4c, 0xff	; 0x33 DRAM latency
	pci_config_byte PCI_ID_1541, 0x4e, 0xff
	pci_config_byte PCI_ID_1541, 0x4f, 0xff

	pci_config_byte PCI_ID_1541, 0x51, 0xf0	; ecc/parity
	pci_config_byte PCI_ID_1541, 0x53, 0xcc
	pci_config_byte PCI_ID_1541, 0x55, 0x05
	pci_config_byte PCI_ID_1541, 0x56, 0x03;0x00
	pci_config_byte PCI_ID_1541, 0x57, 0xf0;0x00
	pci_config_byte PCI_ID_1541, 0x5a, 0xff
	pci_config_byte PCI_ID_1541, 0x5b, 0x00
	pci_config_byte PCI_ID_1541, 0x5d, 0x13
	pci_config_byte PCI_ID_1541, 0x5e, 0x08

	pci_config_byte PCI_ID_1541, 0x72, 0x06
;	pci_config_byte PCI_ID_1541, 0x73, 0x02

	pci_config_byte PCI_ID_1541, 0x84, 0x0f
	pci_config_byte PCI_ID_1541, 0x85, 0xe4 ; 0xe6
	pci_config_byte PCI_ID_1541, 0x86, 0x07
	pci_config_byte PCI_ID_1541, 0x87, 0x1e
	pci_config_byte PCI_ID_1541, 0x88, 0xea
	pci_config_byte PCI_ID_1541, 0x89, 0x20
	pci_config_byte PCI_ID_1541, 0x8a, 0x20
	pci_config_byte PCI_ID_1541, 0x8d, 0x4b
	pci_config_byte PCI_ID_1541, 0x8e, 0x42
	pci_config_byte PCI_ID_1541, 0x8f, 0x32

	pci_config_byte PCI_ID_1541, 0xb8, 0x00
	pci_config_byte PCI_ID_1541, 0xb9, 0x00
	pci_config_byte PCI_ID_1541, 0xba, 0x00
	pci_config_byte PCI_ID_1541, 0xbb, 0x00
	pci_config_byte PCI_ID_1541, 0xbc, 0x08

		
	pci_config_byte PCI_ID_1541, 0xc2, 0xfe
	pci_config_byte PCI_ID_1541, 0xc3, 0xe3
	pci_config_byte PCI_ID_1541, 0xc8, 0xbf
	pci_config_byte PCI_ID_1541, 0xc9, 0x4a
	
	pci_config_byte PCI_ID_1541, 0xcd, 0x80	; reserved
	pci_config_byte PCI_ID_1541, 0xce, 0x08	; reserved
	
	pci_config_byte PCI_ID_1541, 0xd0, 0x00	
	pci_config_byte PCI_ID_1541, 0xd1, 0x00	
	pci_config_byte PCI_ID_1541, 0xd2, 0x00	
	pci_config_byte PCI_ID_1541, 0xd3, 0x00	

	pci_config_byte PCI_ID_1541, 0xe0, 0x00
	pci_config_byte PCI_ID_1541, 0xe1, 0x00
	pci_config_byte PCI_ID_1541, 0xe2, 0x00
	pci_config_byte PCI_ID_1541, 0xe3, 0x00
	pci_config_byte PCI_ID_1541, 0xe4, 0x00
	pci_config_byte PCI_ID_1541, 0xe5, 0x00
	pci_config_byte PCI_ID_1541, 0xe6, 0x00
	pci_config_byte PCI_ID_1541, 0xe7, 0x00
	pci_config_byte PCI_ID_1541, 0xe8, 0x00
	pci_config_byte PCI_ID_1541, 0xe9, 0x00
	pci_config_byte PCI_ID_1541, 0xea, 0x00
	pci_config_byte PCI_ID_1541, 0xeb, 0x00
	pci_config_byte PCI_ID_1541, 0xec, 0x08	
	pci_config_byte PCI_ID_1541, 0xed, 0x10	; 0x11 clock compensate
	pci_config_byte PCI_ID_1541, 0xee, 0x00	
	pci_config_byte PCI_ID_1541, 0xef, 0x29

	pci_config_byte PCI_ID_1541, 0xf3, 0x08
	pci_config_byte PCI_ID_1541, 0xf7, 0x03

	pci_config_byte PCI_ID_1541, 0x90, 0x30 ; lock stuff

	
	pci_config_byte PCI_ID_1543, 0x40, 0x33
	pci_config_byte PCI_ID_1543, 0x41, 0x13
	pci_config_byte PCI_ID_1543, 0x42, 0x0b
	pci_config_byte PCI_ID_1543, 0x43, 0xea
	pci_config_byte PCI_ID_1543, 0x44, 0x5d ; IDE0 = edge, IRQ 14
	pci_config_byte PCI_ID_1543, 0x45, 0x01
	pci_config_byte PCI_ID_1543, 0x47, 0x41
	pci_config_byte PCI_ID_1543, 0x48, 0x90
	pci_config_byte PCI_ID_1543, 0x49, 0x30

	pci_config_byte PCI_ID_1543, 0x53, 0x00 ; leave USB enabled
	pci_config_byte PCI_ID_1543, 0x55, 0x02
	pci_config_byte PCI_ID_1543, 0x5b, 0x01
	
	pci_config_byte PCI_ID_1543, 0x60, 0x60
	pci_config_byte PCI_ID_1543, 0x6d, 0x60

	pci_config_byte PCI_ID_1543, 0x70, 0x12
	pci_config_byte PCI_ID_1543, 0x72, 0x0a
	pci_config_byte PCI_ID_1543, 0x74, 0x57 ; USB = edge, IRQ 6
	pci_config_byte PCI_ID_1543, 0x75, 0x1f ; IDE1 = edge, IRQ 15
	
	
	pci_config_byte PCI_ID_5229, 0x04, 0x05
	pci_config_byte PCI_ID_5229, 0x09, 0xfa
	pci_config_byte PCI_ID_5229, 0x50, 0x01
	pci_config_byte PCI_ID_5229, 0x58, 0x02
	pci_config_byte PCI_ID_5229, 0x59, 0x33
	pci_config_byte PCI_ID_5229, 0x5a, 0x33


;; 7101 - Power Management Unit
;;
;; GPIO pins are here too

	pci_config_byte PCI_ID_7101, 0x04, 0x01
	pci_config_byte PCI_ID_7101, 0x14, (SMB_IO_BASE & 0xff)
	pci_config_byte PCI_ID_7101, 0x15, (SMB_IO_BASE >> 8)


	pci_config_byte PCI_ID_7101, 0x46, 0x20
	pci_config_byte PCI_ID_7101, 0x60, 0xf1
	pci_config_byte PCI_ID_7101, 0x70, 0x0f
	pci_config_byte PCI_ID_7101, 0x71, 0x07
	pci_config_byte PCI_ID_7101, 0x78, 0x69
	pci_config_byte PCI_ID_7101, 0x79, 0x02
	

	pci_config_byte PCI_ID_7101, 0x7c, 0x10 ; 

	pci_config_byte PCI_ID_7101, 0x81, 0xb0;
	pci_config_byte PCI_ID_7101, 0x8c, 0x03 ; 
	pci_config_byte PCI_ID_7101, 0x8d, 0x0b ; 

	pci_config_byte PCI_ID_7101, 0xb5, 0x05 ; 
	pci_config_byte PCI_ID_7101, 0xb8, 0x00 ; 
	pci_config_byte PCI_ID_7101, 0xb8, 0x00 ; 
	pci_config_byte PCI_ID_7101, 0xb9, 0x00 ; 

	pci_config_byte PCI_ID_7101, 0xC2, 0x10 ; set GPO[20] high
	pci_config_byte PCI_ID_7101, 0xC3, 0x06 
	pci_config_byte PCI_ID_7101, 0xC4, 0x04 
	pci_config_byte PCI_ID_7101, 0xd8, 0x08 
	pci_config_byte PCI_ID_7101, 0xe0, 0x01 
	pci_config_byte PCI_ID_7101, 0xe2, 0x88 ; set SMBus (I2C) speed
	pci_config_byte PCI_ID_7101, 0xe3, 0x5c 

	pci_config_byte 0x0, 0x0, 0x0

