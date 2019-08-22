# $Id: trampoline.S,v 1.2 2003/12/11 17:57:50 iceblink Exp $
# Copyright (c) 2000 Cobalt Networks, Inc.
#
# Initial vector of application processors (APs), loaded at 0x1000
#
## 0x9000(16) - Stack
## 0x9002(16) - GDT_limit
## 0x9004(32) - GDT_ptr
#

.globl mp_boot
.globl mp_boot_end
.globl mp_boot_entry

.code16
mp_boot:
	cli

	xor	%ax, %ax
	mov	%ax, %ss
	mov	%ax, %ds

	### start of scratch RAM
	movl	$0x9000, %ecx

	### save stack pointer
addr32	mov	(%ecx), %eax
	add	$0x1000, %eax
	mov	%ax, %sp

	### GDT limit/base is at 0x9002
	add	$2, %ecx
addr32	lgdt	(%ecx)
	lidt	IDT48 - mp_boot

	### set bit at 0x9800 to signal BSP we are alive
	xor	%eax, %eax
	inc	%al
	movl	$0x9800, %edx
addr32	mov	%eax, (%edx)

	xor	%ebx, %ebx
	inc	%bl
	
	hlt

IDT48:
	.word	0	# idt limit = 0
	.word	0, 0	# idt base = 0L
mp_boot_end:
