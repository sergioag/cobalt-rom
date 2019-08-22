;; $Id: i2c5k.S,v 1.1.1.1 2003/06/10 22:42:24 iceblink Exp $
;; Copyright (c) 1999,2000 Cobalt Networks, Inc.

%ifndef INCLUDE_I2C5K_S
%define INCLUDE_I2C5K_S

SMB_BASE		equ		0x3A80

SMB_HOST_STATUS		equ		0x00
 SMB_HOST_STATUS_BUSY	equ		(1 << 0)
SMB_SLAVE_STATUS	equ		0x01
SMB_HOST_CONTROL	equ		0x02
SMB_HOST_COMMAND	equ		0x03
 SMB_CMD_START		equ		(1 << 6)
 SMB_CMD_RW_QUICK	equ		(0 << 2)
 SMB_CMD_RW_BYTE	equ		(1 << 2)
 SMB_CMD_RW_DATA_BYTE	equ		(2 << 2)
 SMB_CMD_RW_DATA_WORD	equ		(3 << 2)
 SMB_CMD_RW_DATA_BLK	equ		(5 << 2)
SMB_HOST_ADDR		equ		0x04
SMB_DATA_0		equ		0x05
SMB_DATA_1		equ		0x06
SMB_BLOCK_DATA		equ		0x07
SMB_SLAVE_CONTROL	equ		0x08
SMB_SHADOW_COMMAND	equ		0x09
SMB_SLAVE_EVENT		equ		0x0a
SMB_SLAVE_DATA		equ		0x0c

SMB_DEV_ADM1029		equ		0x5e ; alpine system monitor
SMB_DEV_AT24C02		equ		0xae ; alpine serial eeprom

	;; ---------------
	;;  wait for idle
	;; ---------------
%macro smb_wait_for_idle 0
	out 0xed, al		; delay

	mov dx, SMB_BASE + SMB_HOST_STATUS
%%smb_wait:
	in  al, dx
	out 0xed, al		; delay

	and al, SMB_HOST_STATUS_BUSY
	cmp al, 0		; wait for busy bit to clear
	je  %%smb_is_idle

	mov al, 0xff
	out dx, al		; clear status reg
	out 0xed, al		; delay

	jmp %%smb_wait
%%smb_is_idle:
%endm
	
	;; ----------------
	;;  write i2c byte
	;; ----------------
	;;  AH => slave device address
	;;  AL => offset
	;;  BL => data to be written
i2c_writeb:			
;	push ax
;	mov dx, SMB_BASE + SMB_HOST_STATUS
;	mov al, 0xff
;	out dx, al 		; clear status register
;	pop ax

	mov dx, SMB_BASE + SMB_HOST_COMMAND
	out dx, al		; set write offset
	
	mov dx, SMB_BASE + SMB_HOST_ADDR
	mov al, ah
	and al, 11111110b	; clear bit 0 for write
	out dx, al		; slave device address

	mov dx, SMB_BASE + SMB_DATA_0
	mov al, bl
	out dx, al		; data byte to write

	mov dx, SMB_BASE + SMB_HOST_CONTROL
	mov al, SMB_CMD_START | SMB_CMD_RW_DATA_BYTE
	out dx, al

	smb_wait_for_idle
	jmp .i2c_writeb_done
.i2c_writeb_done
	ret_sp


	;; ---------------
	;;  read i2c byte
	;; ---------------
	;;  AH => device
	;;  AL => offset
	;; ---------------
	;;  AL <= data
i2c_readb:
	mov dx, SMB_BASE + SMB_HOST_COMMAND
	out dx, al
	shl ah, 1
	or  ah, 0x01

	mov dx, SMB_BASE + SMB_HOST_ADDR
	mov al, ah
	out dx, al
	
	mov al, SMB_CMD_START | SMB_CMD_RW_DATA_BYTE
	mov dx, SMB_BASE + SMB_HOST_CONTROL
	out dx, al
		
	smb_wait_for_idle

	mov dx, SMB_BASE + SMB_DATA_0
	in  al, dx
	jmp .i2c_readb_done
.i2c_readb_done:
	ret_sp

%endif
