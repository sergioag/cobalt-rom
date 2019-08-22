;; Copyright (c) 1999,2000 Cobalt Networks, Inc.
;; $Id: pci.S,v 1.1.1.1 2003/06/10 22:42:24 iceblink Exp $

%ifndef INCLUDE_PCI_S
%define INCLUDE_PCI_S

;; implements calls by faking the stack
%macro call_sp 1
	mov sp, %%ret_addr
	jmp %1
%%ret_addr:	
%endmacro

%macro ret_sp 0
	mov dx, sp
	jmp dx
%endmacro
		
	;whee...  these are from the acer stuff (a5pbg170.txt)

	;; Write a PCI config word
	
;---------------------------------------;
;;Input : ECX   = write data
;;   AH/L = device# / reg#
wr_cfg_dl:
	mov edx,0x80000000
	or dx,ax
	mov eax,edx
	shl edx,16
	mov dx,0x0cf8
	and al,0x0fc
	out dx,eax
	shr edx,16
	mov al,dl
	and al,3
	mov dx,0x0cfc
	add dl,al
	mov eax, ecx
	out dx,eax
	jmp .tmp
.tmp:
	ret_sp

	;; read a PCI config word
;---------------------------------------;
;;Input : AH/L = device# / reg#
;;Output: ECX   = read data
rd_cfg_dl:
	mov edx, 0x80000000
	or dx, ax      
	mov eax, edx
	shl edx, 16
	mov dx, 0x0cf8
	and al, 0x0fc
	out dx, eax
	shr edx, 16
	mov al, dl
	and al, 3
	mov dx, 0x0cfc
	add dl, al      
	in eax, dx
	mov ecx, eax
	jmp .tmp
.tmp:	
	ret_sp
	
		;; Write a PCI config word
	
;---------------------------------------;
;;Input : DI   = write data
;;   AH/L = device# / reg#
wr_cfg_dw:
	mov edx,0x80000000
	or dx,ax
	mov eax,edx
	shl edx,16
	mov dx,0x0cf8
	and al,0x0fc
	out dx,eax
	shr edx,16
	mov al,dl
	and al,3
	mov dx,0x0cfc
	add dl,al
	mov ax,di
	out dx,ax
	jmp tmp0001
tmp0001:
	ret_sp

	;; read a PCI config word
;---------------------------------------;
;;Input : AH/L = device# / reg#
;;Output: AX   = read data
rd_cfg_dw:
	mov edx, 0x80000000
	or dx, ax      
	mov eax, edx
	shl edx, 16
	mov dx, 0x0cf8
	and al, 0x0fc
	out dx, eax
	shr edx, 16
	mov al, dl
	and al, 3
	mov dx, 0x0cfc
	add dl, al      
	in ax, dx
	jmp tmp0002
tmp0002:	
	ret_sp

	;; write a PCI config byte
;---------------------------------------
;;Input	: CL   = write data
;;   AH/L = device# / reg#
wr_cfg_db:
	mov edx, 0x80000000
	or dx, ax
	mov eax, edx
	shl edx, 16
	mov dx, 0x0cf8
	and al, 0x0fc
	out dx, eax
	shr edx, 16
	mov al, dl
	and al, 3
	mov dx, 0x0cfc
	add dl, al
	mov al, cl
	out dx, al
	ret_sp

%macro write_cfg_byte 3
	mov ah, %1
	mov al, %2
	mov cl, %3
	call_sp wr_cfg_db
%endmacro
	
%macro pci_config_byte 3
	db %2
	db %1
	db %3
%endmacro
	
	;; read a PCI config byte
;---------------------------------------;
;;Input : AH/L = device# / reg#
;;Output: AL   = read data
rd_cfg_db:
	mov edx, 0x80000000
	or dx,ax      
	mov eax,edx
	shl edx, 16
	mov dx, 0x0cf8
	and al, 0x0fc
	out dx,eax
	shr edx, 16
	mov al,dl
	and al, 3
	mov dx, 0x0cfc
	add dl,al      
	in al,dx
	jmp tmp0002_5
tmp0002_5:
	ret_sp

%macro PORT_80 1
        mov     al, %1
        out     0x80, al
        jmp     %%1
%%1:    jmp     %%2
%%2:    
%endmacro

%macro PORT_80R 1
        out     0x80, %1
        jmp     %%1
%%1:    jmp     %%2
%%2:    
%endmacro

%macro PORT_80BL 1
	mov	bl, al
	mov	al, %1
        out     0x80, al
	mov	al, bl
        jmp     %%1
%%1:    jmp     %%2
%%2:    
%endmacro

%macro call_cfg_byte 3
	mov di, 0x0
	mov bx, (%1 + %2)
	mov cl, %3
	call pci_write_cfg
%endmacro

;--------------------------------------------------------------------------
; Input:  BL = Register #                              
;         BH = Bits 7-3: PCI device number              
;              Bits 2-0: Function number within the device  
;         DI = One of the following values: Byte/Word/Dword
; Output: CL, CX, ECX = Data read (width depends on value of DI on entry)
;--------------------------------------------------------------------------
pci_read_cfg:
smi_read_cfg:

	xor eax, eax 
	or  eax, 80000000h	;enable PCI access
	mov ax, bx      ;Add in reg./dev/func # part
	and al, 0xFC
	mov dx, 0xcf8   ;CFG_SPACE_INDEX_REG
	out dx, eax

	mov dl, bl    ;Get original config register address
	and dx, 0x3
	add dx, 0xcfc   ;Add offset (0/1/2/3) to data addr
	cmp di, 0x0   
	jne short r_cfg_try_word  ;zf ,read BYTE
	in al, dx
	jmp short r_cfg_done

r_cfg_try_word:
	cmp di, 0x1    ;zf, read WORD
	jne short r_cfg_try_dword
	in ax, dx
	jmp short r_cfg_done

r_cfg_try_dword:    ;read DWORD
	in eax, dx

r_cfg_done:
	mov ecx, eax

	ret

;--------------------------------------------------------------------------
; Input:  BL = Register #                               
;         BH = Bits 7-3: PCI device number                              
;              Bits 2-0: Function number within the device              
;         DI = Low byte = One of the following values: Byte/Word/Dword
;--------------------------------------------------------------------------
pci_write_cfg:
smi_write_cfg:

	xor eax, eax		
	or  eax, 80000000h	;enable PCI access

	mov ax, bx    ;Add in reg./dev/fun # part
	and al, 0xFC
	mov dx, 0xcf8   ;CFG_SPACE_INDEX_REG
	out dx, eax
	mov dl, bl    ;Get original config register address
	and dx, 0x03
	add dx, 0xcfc   ;Add offset (0/1/2/3) to data addr
	mov eax, ecx
	cmp di, 0x0   
	jne short w_cfg_try_word    ;zf ,write BYTE
	out dx, al
	jmp short w_cfg_done

w_cfg_try_word:
	cmp di, 0x1   
	jne short w_cfg_try_dword   ;zf, write WORD
	out dx, ax
	jmp short w_cfg_done

w_cfg_try_dword:    ;zf, write DWORD
	out dx, eax

w_cfg_done:
	ret

%endif
