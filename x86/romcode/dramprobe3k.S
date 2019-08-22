;; Copyright (c) 1999,2000 Cobalt Networks, Inc. 
;; $Id: dramprobe3k.S,v 1.1.1.1 2003/06/10 22:42:23 iceblink Exp $

setup_dram3k:

	;the ram probe routines need to be in 'big real mode'
	;this lets real mode access the full 4G of memory

	;this is unreal
	lgdt	[LGDT_Scratch2]
	
        ; POST	0xE1

	;enable protected mode
	mov	ebx, cr0
	or	bl,  1
	mov	cr0, ebx
	
	;clear prefetch queue
	jmp	cpq1
cpq1:
	
	;set ES as a flat segment
	;segment selector 8 (see LGDT_Scratch2)
	mov	cx, 8
	cs mov	es, cx

	;back to real mode
	mov	ebx, cr0
	and	bl,  0xfe
	mov	cr0, ebx

	;clear prefetch queue
	jmp	cpq2
cpq2:

	;set the base for EX to 0.  The segment descriptor cache holds the
	;rest of the Big Real Mode info, and is invisible to us at this point
	xor	cx, cx
	mov	es, cx
	;
	;enter BIG-REAL MODE (aka UNREAL MODE, aka FLAT-REAL MODE)
	;
	
	PORT_80 0xa0

set_SDRAM_timings:
	mov cx, 0
tmp0003:	
	mov ax, (PCI_ID_1541 + 0xf5)		; read SDRAM PLL
	call_sp rd_cfg_db
	test al, 0x80				; loop till PLL clock > ref clk
	jnz tmp0005

	inc cl					; increment until PLL clock
	mov ax, (PCI_ID_1541 + 0xf5)		; write cx back to speed up. 
	call_sp wr_cfg_db
	out 0x0ed,al		; delay
	jmp tmp0003
	
	;;  We enter here only from tmp0005 below.
	;; the loop above incremented the SDRAM pll until that clock got
	;; ahead of us, then tmp0005 below incremented once more.
test_f5h_bit7:
	mov ax, (PCI_ID_1541 + 0xf5)		; read PLL again
	call_sp rd_cfg_db			
	test al, 0x80
	jnz tmp0005				; if we still lag, incr more
	mov ch, cl				; otherwise, bump PLL by 8 ?
	add ch, 8
	xchg ch,cl
	mov ax, (PCI_ID_1541 + 0xf5)
	call_sp wr_cfg_db
	xchg ch,cl
	out 0x0ed,al		; delay

	mov ax, (PCI_ID_1541 + 0xf5)
	call_sp rd_cfg_db
	test al, 0x80
	jz fixed_DRAM_DPLL
	
tmp0005:	
	inc cl
	mov ax, (PCI_ID_1541 + 0xf5)		; write value back
	call_sp wr_cfg_db
	out 0x0ed,al		;  delay
	jmp test_f5h_bit7

fixed_DRAM_DPLL:	
	dec cl					; back up one to perfect value.
	mov ax, (PCI_ID_1541 + 0xf5)
	call_sp wr_cfg_db

skip_DPLL:	

	PORT_80 0xa1
	
	;; resets DRAM Page Mode Counter Control
	mov cl, 0x0	
	mov ax, (PCI_ID_1541 + 0x5f)	; close pages after 4 CPU clk
	call_sp wr_cfg_db		; and enable refresh

	;; sets SDRAM CAS Latency to 3 HCLKINs, also slow RAS precharge
	;; and RAS cycle time, etc.
	mov cl, 0x00
	mov ax, (PCI_ID_1541 + 0x48)
	call_sp wr_cfg_db

	;; enable SDRAM internal page detection
	mov al, 0x3
	test cl, 0x10		; but we just set it to Zero.  Erik??
	jnz tmp0005_5
	mov al, 0x2
tmp0005_5:	
	mov cl, al
	
	mov ax, (PCI_ID_1541 + 0x49) ; 0x2 - enable internal page detection
	call_sp wr_cfg_db

	xor ecx,ecx		; keep memory size
	xor ebx,ebx		; keep current bank/DRAM type

	mov al, 0x60		; start at bank 0
	
	PORT_80BL 0xa2

detect_bank_loop:
;; input  AL: even bank register
;;	Initial entry, al == 0x60, ecx, ebx == 0, cl == 2

	PORT_80 al

	    ;; AL == config register
	    ;;	- set up as Fast Page, 256M and see what happens
	mov	ah,0				; M1541 
	mov	di,0x81ff			; 64 MA mapping/fast page mode
	mov	bh,al				; save current bank#(index#)
	call_sp 	wr_cfg_dw
;;; detect DARM type ( EDO or Fast Page DRAM  )
;; can not destroy bit 31 - 8 of ECX ,BH
	call_sp	detect_fp_edo_dram
	test	bl,0x1

	jz	short detect_SDRAM		; zf ,may be have SDRAM
	
	mov	ax,(ALI_1541_ADselID+0x5f)
	mov	cl,0x00				; enb. refresh
	call_sp wr_cfg_db           


	mov	al,bh				; get current bank#(index#)
	mov	ah,0x00
	call_sp rd_cfg_dw
	and	ax,0xcfff			; cls DRAM type
	or	ah,bl				; set correction type
	and	ah,~(0x01)			; cls test flag
	mov	di,ax
	mov	al,bh				; get current bank#(index#)
	mov	ah,0x0
	call_sp wr_cfg_dw

	call_sp	detect_FP_EDO_MA		; detect MA type

	mov	al,bh				; get current bank#(index#)
	mov	ah,0x0
	call_sp rd_cfg_dw
	and	ax,0x3fff			; cls MA type
	or	ah,bl				; set correction type
	and	ah,~(0x1)			; cls test flag
	mov	di,ax
	mov	al,bh				; get current bank#(index#)
	mov	ah,0x0
	call_sp wr_cfg_dw
	jmp	sizing_bank_size


detect_SDRAM:
;;; detect Sync. DRAM type
	;; First entry, bh == 0x60
	;; Set SDRAM as Tbl 4-10 (11 bit Column), 256 MB.
	mov	al,bh				; get current bank#(index#)
	mov	ah,0x0
	mov	di,0xf1ff			; SDRAM col=11 bits/512MB
	call_sp wr_cfg_dw

	call_sp detect_sdram_type
	test	bl,0x01
	jz	NEAR sizing_bank_size		; zf, none DRAM

;; support 13 by 9 SDRAM
	mov	ax,0x70				; read 256 MBit support reg.
	call_sp rd_cfg_dw
	mov	dx,ax
	mov	al,bh				; get current bank#(index#)
	and	al,0x0f
	shr	al,0x1				; al=0/1/2/3/4/5/6/7
	mov	ah,0x0				; ie. al == bank
	bts	dx,ax				; set 256M MA
	add	al,0x8
	bts	dx,ax				; set 4 banks  
	mov	di,dx				; ??? should this be reg 71? cj
	mov	ax,0x70
	call_sp wr_cfg_dw


   	call_sp	detect_sdram_MA
	test	bl,0x1
	jz	short sizing_bank_size

set_SDRAM_MA:
	mov	ax,0x0200			; set 512MB/cls DRAM type
	or	ah,bl				; set SDRAM type and MA
	and	ah, ~(0x0d)			; cls test flag
	mov	di,ax
	mov	al,bh				; get current bank#(index#)
	mov	ah,0x0
	call_sp wr_cfg_dw
	

;solve 2 bank and 4 bank SDRAM mix in the system problem
	test	bl,0x8
	jnz	short is_4_internal_bank	; nz, 4 internal banks
	or	ebx,0x10000
is_4_internal_bank:

	mov	ax,0x70
	call_sp rd_cfg_dw
	mov	dx,ax

;; BH = index 60/62/64/66/68/6a/6c/6eh
	mov	al,bh				; get current bank#(index#)
	and	al,0x0f
	shr	al,0x1				; al=0/1/2/3/4/5/6/7
	mov	ah,0x0
	test	bl,0x4

	jnz	short chk_SDRAM_intbk		; zf , not 256M MA 
	btr	dx,ax

chk_SDRAM_intbk:
	test	bl,0x8


	jnz	short .tmp			; zf ,2 internal banks
	add	al,0x8
	btr	dx,ax				; set 2 banks

.tmp:
	mov	di,dx
	mov	ax,0x70
	call_sp wr_cfg_dw

sizing_bank_size:
	call_sp	sizing_memory

	shl	ecx,0x3

	shl	ebp,0x4

	test	bl,0x1
	jz	short sizing_bank_end

;; CL=8 --> have 512MB ,CL=0 --> have not 512MB
	mov	al,bh				; get current bank#(index#)
	inc	al
	mov	ah,0x0
	call_sp rd_cfg_db
;;<<K030299
	shr	al,0x4
	or	bp,ax				; keep type and MA of DRAM
	test	cl,0x8
	jz	short sizing_bank_end		; zf, have not 512MB DRAM
	mov	al,bh				; get current bank#(index#)
	and	eax,0x0f
	shr	al,0x1				; al=0/1/2/3/4/5/6/7
	add	al,24
	bts	ebx,eax				; set 512MB


sizing_bank_end:
;;input bl bit0   = 1 ,DRAM present
;;	   bit2   = 256M MA mapping ,if bit0 =1 ; for SDRAM only
;;	   bit3   = inrenal banks ,if bit0 =1	; for SDRAM only
;;	   bit5.4 = DRAM type ,if bit0 =1
;;	   bit7.6 = MA mapping type ,if bit0 =1

	mov	al,bh				; get current bank#(index#)
	mov	ah,0x0
	mov	di,0x0				; cls current bank information
	call_sp wr_cfg_dw
	mov	ah,bh				; get current bank#(index#)
	add	ah,0x2				; next block
	cmp	ah,0x70				; total 8banks
	jz	short .tmp1			; zf ,detection memory finish
	xchg	ah,al				; al= DRAM bank config. reg
	jmp	detect_bank_loop
.tmp1:

;solve 2 bank and 4 bank SDRAM mix in the system problem
;when this case occur ,BIOS must set all SDRAM to 2 internalbank
	test	ebx,0x10000
	jz	short .tmp2			; nz, have 2 internal bank SDRAM
	mov	ax,(ALI_1541_ADselID+0x71)
	mov	cl,0x0
	call_sp	wr_cfg_db
.tmp2:	

;;; restore DRAM type/MA for each bank

	mov	bx,(ALI_1541_ADselID+0x6f)
.tmp3:
	mov	ax,bp				; temporal register
	mov	cl,al
	shl	cl,0x4
	mov	ax,bx
	call_sp	wr_cfg_db
	shr	ebp,0x4
   	sub	bx,0x2
	cmp	bl,0x5f
	jnz	short .tmp3


;;input ECX
;;ECX =bit 31-29 /28-26 /25-23 /22-20 /19-17 /16-14 /13-11 /10-8--> memory size
;;    bank   0      1      2      3      4	5      6      7
;; 000 --> 0   ,001 --> 4  ,010 --> 8  ,011 --> 16  ,100 --> 32
;; 101 --> 64  ,110 --> 128  ,111 --> 256 MB
;; check 512MB DRAM from Bank0 to 7

	and	ecx,0xffffff00
	shr	ebx,24
	mov	al,bl
	mov	bl,0x0
	mov	ah,0x0				; counter

recheck_512M_DRAM:
	rol	ecx,3				; total 8banks
	mov	dl,cl
	and	dl,0x7
	or	bl,dl
	test	al,0x1
	jz	short .tmp			; nz ,have 512MB
   	mov	bl,0x8
.tmp:
	shr	al,1
	inc	ah
	cmp	ah,0x8
	jz	short end_recheck_512M_DRAM
	shl	ebx,0x4
	jmp	short recheck_512M_DRAM


end_recheck_512M_DRAM:

;;output EBX
;;EBX =bit 31-28 /27-24 /23-20 /19-16 /15-12 /11-8 /7-4 /3-0 --> memory size
;;    bank   0      1      2      3      4      5    6	  7
;; 0000 --> 0MB  ,0001 --> 4MB  ,0010 --> 8MB  ,0011 --> 16MB  ,0100 --> 32MB
;; 0101 --> 64MB  ,0110 --> 128MB  ,0111 --> 256MB  ,1000 --> 512MB

;; calculation memory size for each bank
	mov	sp,0x60
	mov	di,0
.tmp:
	and	di,0xfff			; keep A31 to A20
	rol	ebx,4
	mov	cl,bl
	and	cl,0x0f
	mov	ch,0
	shl	cx,1
	mov	si, memory_config_table
	add	si,cx
	mov	cx, [WORD ds:si]		; get DRAM size

	add	di,cx				; memory size in the bank
	mov	ax,sp
	shl	esp,16
	call_sp	rd_cfg_dw
	or	di,ax
	shr	esp,16
	mov	ax,sp
	shl	esp,16
	call_sp	wr_cfg_dw
	shr	esp,16
	add	sp,2				; next bank#
	cmp	sp,0x70
	jnz	short .tmp

	mov	bx,0x60
sub_1M_each_bank:
	mov	ax,bx
	call_sp	rd_cfg_dw
	test	ax,0x0fff
	jz	short .tmp		; zf ,none DRAM
	dec	ax
	mov	di,ax
	mov	ax,bx
	call_sp	wr_cfg_dw
.tmp:
	add	bx,2
	cmp	bx,0x70
	jnz	short sub_1M_each_bank




%ifdef __bad_ram__
;; << check and set ECC function
;;<< index 58h bit0 of CMOS control DRAM Data Integrity Mode (Parity/ECC)
	mov	al,0xd8
%ifdef _ali_cmos_
	call_sp	cmos_data_in
%else
	and al, ~(0x1)
%endif
	test	al,0x1
	jz	short .tmp2
	mov	ax,(ALI_1541_ADselID+0x50)
	mov	cl,1				; ECC
	call_sp	wr_cfg_db
.tmp2:

%endif

	mov	ax,(ALI_1541_ADselID+0x8d)
	call_sp	rd_cfg_db
	and	al,7
	cmp	al,6
	jnz	short .tmp3			; zf ,is 60 or 66MHz
	mov	ax,(ALI_1541_ADselID+0x73)
	mov	cl,0
	call_sp  wr_cfg_db

	mov	ax,(ALI_1541_ADselID+0xee)

	mov	di,2100h
	call_sp  wr_cfg_dw

	jmp	short init_parity_DRAM

.tmp3:
	call_sp	MDin_pipe_for_83_100MHz		; for 83/100 MHz only

init_parity_DRAM:

%ifdef __bad_ram__	
	mov	ax,(ALI_1541_ADselID+0x6e)
	call_sp  rd_cfg_dw
	inc	ax
	and     eax,0xfff
	mov	bx,ax
	shl	eax,18
	mov	ecx,eax

	mov	ax,(ALI_1541_ADselID+0x58)
	mov	di,0xffff			; shadow enb. for F/E/D/C page
	call_sp	wr_cfg_dw	

;; init. all DRAMs data
	cld
	mov	eax,0xffffffff
	mov	edi,0
	db	0x67
	rep	stosd

	mov	ax,(ALI_1541_ADselID+0x51)
	mov	cl,0xff				; reset ECC/parity error flag
	call_sp wr_cfg_db

        mov     cx,bx
        shr     cx,1				; total memory/2
        xor     esi,esi
.tmp:						; test DRAM parity ok or not
        mov     DWORD [DWORD es:esi],0x1000100	; data pattern
        mov     DWORD [DWORD es:esi+4],0x1000100

;;<< ECC function detection 
	shl	ecx,16
	mov	ax,(ALI_1541_ADselID+0x50)
	call_sp rd_cfg_db

	or	al,0x02				; ECC test mode
	mov	cl,al
	mov	ax,(ALI_1541_ADselID+0x50)
	call_sp wr_cfg_db

        mov     DWORD [DWORD es:esi+0x20],0
        mov     eax,[DWORD es:esi+0x20]
	mov	ax,(ALI_1541_ADselID+0x50)
	and	cl, ~(0x02)
	call_sp wr_cfg_db

	mov	ax,(ALI_1541_ADselID+0x51)
	mov	cl,0xff				; reset ECC/parity error flag
	call_sp wr_cfg_db

	shr	ecx,16
;;>> ECC function detection ,k071698

        mov     eax,[DWORD es:esi]
        mov     eax,[DWORD es:esi+4]
        add     esi, 0x200000
        loop    .tmp1_1
	jmp .tmp1_2
.tmp1_1:
	jmp .tmp
.tmp1_2:	
	
	mov	ax,(ALI_1541_ADselID+0x50)
	call_sp rd_cfg_db
	mov	bl,al

	mov	ax,(ALI_1541_ADselID+0x51)
	call_sp rd_cfg_db
	mov	ah,al
	and     ah,0x11				; keep testing result
	mov     al,0xf2
%ifdef _ali_cmos_
	call_sp	cmos_data_out
%else
	and ah, ~(0x10)
%endif
	test	ah,0x10
	jz	short .tmp2			; nz ,none Parity DRAM or
						; memory errors
	test	bl,1
	jz	short .tmp2			; zf ,is Parity function
	mov	ax,(ALI_1541_ADselID+0x50)
	mov	cl,0				; set Parity
	call_sp	wr_cfg_db
	jmp	init_parity_DRAM

.tmp2:
	mov	ax,(ALI_1541_ADselID+0x51)
	mov	cl,0xff				; reset ECC/parity error flag
	call_sp wr_cfg_db

	mov	ax,(ALI_1541_ADselID+0x58)
	mov	di,0x0				; shadow dis. for F/E/D/C page
	call_sp	wr_cfg_dw	

;;<< k031898 ,ECC timing at 100/83MHz
	test	bl,1
	jz	short .tmp3			; zf ,is Parity function
	mov	ax,(ALI_1541_ADselID+0x73)
	call_sp rd_cfg_db
	cmp	al,0
	jz	short .tmp3			; zf, is 66/60MHz so skip
	mov	ax,(ALI_1541_ADselID+0x73)

	mov	cl,03h

	call_sp  wr_cfg_db
.tmp3:
;;>> k031898 ,ECC timing at 100/83MHz

%endif
	
;;<< k052598
;solve M5M4V16S30CTP-10 (Mitsubishi) and HYB39S1680AT-8 (Siemens) SDRAM problem
	mov	ax,(ALI_1541_ADselID+0x49)
	call_sp rd_cfg_db
	and	al, ~(0x02)			; disabled internal page detection
	mov	cl,al
	mov	ax,(ALI_1541_ADselID+0x49)
	call_sp wr_cfg_db
;;>> k052598


	jmp dramprobe_done	
	
;	END

;;-----------------------------------------------
;; can not destroy bit 31 - 8 of ecx 
detect_fp_edo_dram:
	shl	esp,16				; store sp

	mov	bl,0x00                		; no memory
	call_sp	detect_dram			; write/read cross check

;;<< k012398
	mov	ax,(ALI_1541_ADselID+0x5f)
	mov	cl,01h				; dis. refresh
	call_sp wr_cfg_db           
;;>> k012398

	test	bl,0x1
	jz	short exit_detect_edo_fp_dram	; zf ,none DRAM
	mov	DWORD [DWORD es:esi+0x10],0xaa5555aa ; dummy write

	mov	cl,0x60				; enb. EDO detection
	mov	ax,(ALI_1541_ADselID+0x46)	; EDO detection timer 512 clk
	call_sp	wr_cfg_db

	cmp	DWORD [DWORD es:esi+0h],0x5555aaaa
	jnz	short .tmp			; zf ,is EDO
	or	bl,0x10
.tmp:	
	mov	cl,0x00			    	; dis. EDO detection
	mov	ax,(ALI_1541_ADselID+0x46)
	call_sp	wr_cfg_db

exit_detect_edo_fp_dram:
	shr	esp,16	; restore sp
        ret_sp

;;;------------------------
;; can not destroy bit 31 - 8 of ECX ,BH 
detect_dram:
	xor	esi,esi
	mov	DWORD [DWORD es:esi+0x0],0x5555aaaa 	; write pattern
	mov	DWORD [DWORD es:esi+0x4],0xaa5555aa 	; write pattern
	cmp	DWORD [DWORD es:esi+0x0],0x5555aaaa	; read pattern
	jnz	short .tmp				; nz, none DRAM
	or	bl,0x1
.tmp:
	ret_sp

;;;------------------------
;;output	DL=BL ,MA type
;; can not destroy bit 31 - 8 of ECX ,BH
detect_FP_EDO_MA:
	mov	esi,0x3400800			; A25/A24/23/22/11= 11011
	mov     WORD [es:esi],0x55aa
	mov	esi,0x2400800			; A25/A24/23/22/11= 10011
	mov	WORD [es:esi],0xaa55
	mov	esi,0x2c00800			; A25/A24/23/22/11= 10111
	mov     WORD [es:esi],0x5a5a
     	mov	esi,0x0c00800			; A25/A24/23/22/11= 00111
	mov     WORD [es:esi],0xa5a5
	mov	esi,0x0800800			; A25/A24/23/22/11= 00101
	mov     WORD [es:esi],0x3535
	mov	esi,0x3400800			; A25/A24/23/22/11= 11011
	mov     WORD ax,[es:esi]
   	mov	dl,0x80
	cmp	ax,0x55aa
	jz	short .tmp			; zf ,is 64M MA mapping
	cmp     ax,0xa5a5
	jz	short .tmp			; zf ,is 64M MA mapping
   	mov	dl,0xc0
	cmp     ax,0x5a5a
	jz	short .tmp			; zf ,1Mx16 or 2Mx8 is MA mapping
   	mov	dl,0x40				; is STD MA mapping
.tmp:
	or	bl,dl
	ret_sp

;;;------------------------
;; can not destroy bit 31 - 8 of ECX ,BH
detect_sdram_type:
	shl	esp,16				; store sp


	mov	ax,(ALI_1541_ADselID+0x48)	; read mode reg
	call_sp rd_cfg_dw
	or	al,0x20				; set NOP mode
	mov	di,ax
	mov	ax,(ALI_1541_ADselID+0x48)	; write mode reg
	call_sp wr_cfg_dw

	mov	ax,3000				; delay 200us (clk=200MHz)
.tmp:	dec	ax
	jnz	short .tmp

;;; Auto init.
	and	di,~(0x20)
	or	di,0xE0				; set Auto command.
	mov	ax,(ALI_1541_ADselID+0x48)	; write mode reg
	call_sp wr_cfg_dw           

	xor	ax,ax
	sahf
	mov	ax,5
	mov	cl,2
	div	cl
	lahf

	test	di,0x10
	jnz	short is_cas_2			; nz ,CAS latency 2
	cmp	ah,0x02
	jz	short is_cyrix_cas_3		; zf ,is Cyrix
	mov	eax,[DWORD es:esi+0x1d0]   ; special addr. for CAS latency3
	jmp	short set_normal_cmd
is_cyrix_cas_3:
	mov	eax,[DWORD es:esi+0x190]   ; CAS latency3 ,M1 CPU+linear
	jmp	short set_normal_cmd
is_cas_2:
	cmp	ah,0x2
	jz	short is_cyrix_cas_2		; zf ,is Cyrix
	mov	eax,[DWORD es:esi+0x150]   ; special addr. for CAS latency2
	jmp	short set_normal_cmd
is_cyrix_cas_2:
	mov	eax,[DWORD es:esi+0x110]	; CAS latency2 ,M1 CPU+linear

set_normal_cmd:
	and	di,~(0xE0)			; NORMAL COMMAND
	mov	ax,(ALI_1541_ADselID+0x48)
	call_sp wr_cfg_dw           

	mov	ax,(ALI_1541_ADselID+0x5f)
	mov	cl,0x00				; enb. refresh
	call_sp wr_cfg_db

  	call_sp	detect_dram

	mov	cl,0x30
	test	bl,1
	jnz	short detect_sdram_type_end	; nz, is SDRAM

	mov	cl,0

detect_sdram_type_end:
	or	bl,cl
	shr	esp,16	; restore sp
	ret_sp

;;;------------------------
;; can not destroy bit 31 - 8 of ECX ,BH
detect_sdram_MA:
	shl	esp,16				; store sp
	xor	esi,esi
	mov	WORD [es:esi+0x3800],0x5aa5	; A13/12/11 = 111
	mov	WORD [es:esi+0x1800],0xa55a	; A13/12/11 = 011
	mov	WORD [es:esi+0x0800],0x33cc	; A13/12/11 = 001
	mov	WORD [es:esi+0x0000],0xcc33	; A13/12/11 = 000
	mov	ax,WORD [es:esi+0x3800]	; A13/12/11 = 111


	mov	dl,0xc0
	cmp	ax,0x5aa5
	jz	short .tmp			; zf ,is col=11 MA
	mov	dl,0x80
	cmp	ax,0xa55a
	jz	short .tmp			; zf ,is col=10 MA
	mov	dl,0x40
	cmp	ax,0x33cc
	jz	short .tmp			; zf ,is col=9 MA
	mov	dl,0x00
	cmp	ax,0xcc33
	jz	short .tmp			; zf ,is col=8 MA
	and	bl,~(0x1)			; DRAM error
	jmp	short detect_sdram_MA_end
.tmp:
	or	bl,dl				; keep MA
;; detect 4 or 2 internal banks
	mov	WORD [es:esi+0x4004000],0xa5a5	; A26/14 = 11
	mov	WORD [es:esi+0x4000],0x5a5a		; A26/14 = 01
	mov	WORD ax,[es:esi+0x4004000]		; A26/14 = 11
	mov	dl,8
	mov	edi,0x14000000			; A28/26 = 11
	mov	esi,0x04000000			; A28/26 = 01
	cmp	ax,0xa5a5
	jz	short detect_256M_MA		; zf ,is 4 banks
	mov	edi,0x04000000			; A28/26 = 01
	mov	esi,0x0000			; A28/26 = 00
	mov	dl,0x0
	cmp	ax,0x5a5a
	jz	short detect_256M_MA		; zf ,is 2 banks
	and	bl,~(0x1)			; DRAM error
	jmp	short detect_sdram_MA_end

detect_256M_MA:
	or	bl,dl				; k011998
	mov	WORD [es:edi],0x3535		; A28/26
	mov	WORD [es:esi],0x5353	    	; A28/26
	mov	WORD ax,[es:edi]		; A28/26
	mov	dl,4
	cmp	ax,0x3535
	jz	short .tmp			; zf ,is 256M MA
	mov	dl,0
	cmp	ax,0x5353
	jz	short .tmp			; zf ,is not 256M MA
	and	bl,~(0x1)			; DRAM error
	jmp	short detect_sdram_MA_end
.tmp:
	or	bl,dl				; keep MA

detect_sdram_MA_end:
	shr	esp,16	; restore sp
	PORT_80 0x42
	ret_sp

;;;------------------------
;; from min. to max.
;; output : bit 31 - 8 of ecx reg.
;; can not destroy bit 31 - 8 of ECX ,BH
sizing_memory:
	mov	eax,0x5555aaaa
	xor	esi,esi
	mov	cl,0

	mov	[DWORD es:esi],eax
	mov	DWORD [DWORD es:esi+0x100],~(0x5555aaaa) 	; dummy write
	cmp	[DWORD es:esi],eax
	jnz	short end_dram_sizing		; nz ,dram error
	mov	cl,0x20				; min. size = 4MB
	mov	esi,0x200000			; A21
.tmp:
	shl	esi,1				; from A22 to A28
	cmp	[DWORD es:esi],eax
	jz	short end_dram_sizing		; zf ,4/8/16/32/64/128/256MB
	add	cl,0x20
	cmp	esi,0x10000000
	jnz	short .tmp
	mov	cl,1				; 512MB
						
end_dram_sizing:
	ret_sp

;;;------------------------
MDin_pipe_for_83_100MHz:
	shl	esp,16				; store sp

	mov	ax,(ALI_1541_ADselID+0x61)
	mov	di,ax
	mov	bl,0
.tmp:
	call_sp  rd_cfg_db
	test	al,0xe0
	jz	short none_memory
	inc	bl

none_memory:
	add	di,2
	mov	ax,di
	cmp	di,0x71
	jnz	short MDin_pipe_for_83_100MHz.tmp

	mov	di,0xc11		; for M1541 ver E

	cmp	bl,4
	jb	short .tmp

	mov	di,0x11 		; for M1541 ver E
.tmp:

	mov	ax,(ALI_1541_ADselID+0x8d)
	call_sp	rd_cfg_db

	and	al,7
	cmp	al,3
	jnz	short .tmp2		; zf ,is 100MHz
	or	di,08h
.tmp2:
	mov	ax,ALI_1541_ADselID+0xed
	call_sp  wr_cfg_dw

	shr	esp,16				; store sp
	ret_sp

memory_config_table:	
	dw	0		; MByte
	dw	4		; MByte
	dw	8		; MByte
	dw	16		; MByte
	dw	32		; MByte
	dw	64		; MByte
	dw	128		; MByte
	dw	256		; MByte
	dw	512		; MByte
			
dramprobe_done:	
	jmp dram_setup_done

