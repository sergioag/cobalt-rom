
;; $Id: dramprobe5k.S,v 1.1.1.1 2003/06/10 22:42:24 iceblink Exp $
;; 
;; NOTE: places where changes would need to be made for 100MHz optimal
;;       performance are given below. due to concerns about the Serverworks
;; 	 NB, however, we go with the more conservative assumption of 
;;       assumption of basing timings against a 133MHz FSB.
;;       the only thing that we know we can change without a problem is the
;; 	 refresh rate. we do that in the ramcode.
;; 
;; ADDITIONAL NOTE: 4GB support is currently implemented by shaving
;; off the last 16MB as that area gets reserved for the IOAPIC
;; table. it also simplifies things as we don't need to consider 
;; DRAM row 7 special unless we have exactly 4GB.
	

setup_dram5k:	
; disable parity NMIs - they are enabled later in ramcode
	call_sp disable_nmi_parity

	;; zero out the memory row decode registers (80h-8Fh)
	;; also zero out Memory Row Presence Registers (90h)

	mov bl, 0
.zero_mrd:
	mov cl, 0x0
	mov al, 0x80
	add al, bl
	mov ah, PCI_ID_CNBLE
	call_sp wr_cfg_db	

	inc bl

	cmp bl, 0x11		;17 registers to zero out
	jne .zero_mrd

	;; zero out the row attribute registers
	mov bl, 0
.zero_rar:
	mov cl, 0x0
	mov al, 0x7c
	add al, bl
	mov ah, PCI_ID_CNBLE
	call_sp wr_cfg_db	

	inc bl

	cmp bl, 0x4
	jne .zero_rar
	
;Zero out ECC Flow control register (EFCR, reg 0E0h)
	mov	cl, 0
	mov	al, 0xE0
	mov	ah, PCI_ID_CNBLE
	call_sp	wr_cfg_db
	
; here are the current uses for FS:
; 0x00FF - refresh rate
; 0x0F00 - number of rows for buffer strength setting (we include
;          'bad' dimms for this calculation as well)
; 0x1000 - 100MHz bit
	xor	bx, bx

	mov	dx, GPIO_BASE+3
	xor	ax, ax
	in	al, dx
	bt	ax, 1		; if GPIO11=1 then FSB is 133MHz
	jc	.fsb133
	or	bx, 0x1000	; otherwise FSB is 100MHz
.fsb133:
	mov	fs, bx

dramprobe5:	
;
;We want to detect the memory type.
;This is done by reading Byte 2
;If memory type == 2, it's EDO
;If memory type == 4, it's SDRAM
;We don't support EDO 
;
probeloop:

;print 'DIMM Slot '
	mov	ah, 0Dh
	call_sp UART1PrintChar

	mov	ah, 0Ah
	call_sp UART1PrintChar

	mov	si, DIMMProbeMsg 
	call_sp UART1PrintString

	mov ah, bl
	add ah, 30h			;convert to ASCII
	call_sp UART1PrintChar

	mov	ah, bl
	or 	ah, 0x50
	mov 	al, 0x02
	call_sp i2c_readb	; read Memory Type

	cmp 	al, 0x04
	je	.doRowCalc	;DIMM type is OK

;assuming that the slot is unpopulated
	mov	di, 1		;error = 1, no DIMM found
	cmp	al, 0xFF
	jne	.notSDRAM
	jmp	nextSPDDIMM

.notSDRAM:
	mov	di, 2		;error = 2, not SDRAM
	jmp	nextSPDDIMM

.doRowCalc:
;read the number of rows on this DIMM. this is done here because even
;disqualified memory will affect memory load.
        mov	ah, bl          ;save DIMM# in AH
        or	ah, 0x50
        mov	al, 0x05
        call_sp i2c_readb       ; read row
	mov	cx, fs		; we save it in fs
	add	ch, al
	mov	fs, cx

;the memory controller supports CAS Latency of 2/3 only
;reject the DIMM if it's not so
	mov	ah, bl
	or	ah, 0x50
	mov	al, 0x12	;SPD byte 18
	call_sp	i2c_readb

	test	al, 4		;CAS latency == 3?
	jnz	.CASLatencyOK

; 100MHz -- this should test against 100MHz FSB. according to
;        ServerWorks, CAS latency of 2 is not supported on 133MHZ 
;	 FSB. 
 	test	al, 2		;CAS latency == 2?
	jz	.CLBad
	
	call_sp	FSBGetTiming
	mov	ch, BYTE[cs:esi+1]
	test	ch, 0x1		;CL2 okay?
	jnz	.CASLatencyOK

.CLBad:	
	mov	di, 3		;error = 3, CAS Latency not 2/3
	jmp	nextSPDDIMM

;
;We want to make sure that the SPD XSUM matches
;This is done by reading and summing SPD bytes 0-62 (inclusive)
;and then compare the byte-wise sum with the value in SPD byte 63
;If SPD Xsum doesn't match then we just skip the bad DIMM
;
.CASLatencyOK:
	xor	bh, bh		;Sum of SPD bytes 0-62
	xor	cl, cl		;SPD counter
.doSPDXsum:
	mov	ah, bl
	or	ah, 0x50
	mov	al, cl		;read this SPD byte
	call_sp	i2c_readb
	add	bh, al		;do a byte-wise sum
	inc	cl		;read next SPD byte
	cmp	cl, 63		;last SPD byte yet?
	jne	.doSPDXsum	;not yet, so read some more
	mov	ah, bl
	or	ah, 0x50
	mov	al, cl
	call_sp	i2c_readb
	cmp	al, bh		;value in byte 63 == byte-wise sum?
	je	.doRegisteredDIMM

	mov	di, 4		;error = 4, SPD Xsum doesn't match
	stc
	jmp	nextSPDDIMM

.doRegisteredDIMM:

;We get here because SPD Xsum is good	
;We now know that the DIMM is type SDRAM.
;However, we support registered SDRAM only so we need to make additional
;checks
;
;We need to read SPD byte 21 to see if bits 4,1 are all set.
;If they are set, then it's registered SDRAM DIMM

	mov	ah, bl
	or	ah, 0x50
	mov	al, 21		;SPD byte 21
	call_sp	i2c_readb

	and	al, 00010010b	;bits 1 and 4 must be set
	cmp	al, 00010010b	;are they set?
	je	.doECC
	mov	di, 5		;error = 5, not registered DIMM
	stc
	jmp	nextSPDDIMM
;
;We need to read SPD byte 11 to see the module configuration type
;if it's == 2, then we have an ECC DIMM
.doECC
	mov	ah, bl
	or	ah, 0x50
	mov	al, 11		;SPD byte 11
	call_sp	i2c_readb
	
	cmp	al, 0x02	;ECC type?
	je	.doDIMMWidth
	
	mov	di, 6		;error = 6
	stc
	jmp	nextSPDDIMM

;We need to check to make sure that the DIMM width is 72 bits
;SPD byte 7 returns the MSB of the DIMM width
;SPD byte 6 returns the LSB of the DIMM width
;
.doDIMMWidth:
	mov	ah, bl
	or	ah, 0x50
	mov	al, 7		;SPD byte 7
	call_sp	i2c_readb
	
	cmp	al, 0		;MSB == 0
	mov	di, 7		;assuming an error
	je	.checkWidth72
	jmp	nextSPDDIMM

.checkWidth72:	
	mov	ah, bl
	or	ah, 0x50
	mov	al, 6		;SPD byte 6
	call_sp	i2c_readb

	cmp	al, 72		;72-bit width
	mov	di, 7		;assuming error 7
	je	noSPDError
	jmp	nextSPDDIMM
noSPDError:
	xor	di, di		;no SPD error

;We get here because the DIMM is registered DIMM and supports ECC
;Chipset support the following mapping table for various DIMM configurations:
;Size / SDRAM Depth				Row Size(MB) / Index value
;-------------------------------------------------------------------------
;64M, 4-bank, 12x8				32/0
;16M, 2-bank, 11x9				16/1
;16M, 2-bank, 11x10				32/2
;128M, 4-bank, 12x11				256/3
;256M, 4-bank, 13x11				512/4
;64M, 2-bank, 13x10				128/5
;64M, 4-bank, 12x10				128/6
;64M, 4-bank, 12x9				64/7
;256M, 4-bank, 13x10				256/8
;256M, 4-bank, 13x9				128/9
;256M, 4-bank, 13x8				64/10
;256M, 4-bank, 12x9				64/11  (redundant with 7)
;256M, 4-bank, 12x10				128/12 (redundant with 6)
;NOTE: there are two redundant entries according to rowxcolxbanksxsize.
;      ServerWorks says that it doesn't matter which value is used.
;Once we detected the row/column, we set:
;BH to RAR type
;CH to DIMM size (in 16MB granularity)
;We then program the Row Attribute Registers (RAR) 7Ch, 7Dh, 7Eh, 7Fh
;depending on which row we found the DIMM
;
;We need to get row, column, banks, density so we can print them out
;Row is in SPD byte 3
;Col is in SPD byte 4
;Banks is in SPD byte 17
;Density is in SPD byte 31

	;;
	;; bh gets set to RAR type
	mov ah, bl
	or ah, 0x50
	mov al, 0x03
	call_sp i2c_readb	; read row
	mov cl, al		; save row

	mov ah, bl
	or ah, 0x50
	mov al, 0x04
	call_sp i2c_readb	; read cols

	mov ch, al		; column in CH

	mov edi, ecx
	shl edi, 16		; row x col is in upper EDI
	shl ecx, 16		; row x col is in upper ECX

	mov ah, bl
	or  ah, 0x50
        mov al, 17		;SPD byte 17
        call_sp i2c_readb	
	mov cl, al		; CL now has # of banks

	mov ah, bl
	or ah, 0x50
	mov al, 31		;SPD byte 31
	call_sp i2c_readb
	mov ch, al		; CH now has density
	mov di, cx		; DI now has banks & density

	mov si, SPDDIMMRowColMsg
	call_sp UART1PrintString

	shr ecx, 16		; CX now has row x col
	mov ah, cl		; row
	cmp ah, 9
	jbe is0To9
	add ah, 37h		; A to F
	jmp short printChar

is0To9:
	add ah, 30h		; to ASCII
printChar:
	call_sp UART1PrintChar

	mov ah, 'x'
	call_sp UART1PrintChar

	mov ah, ch		; col
	cmp ah, 9
	jbe colIs0To9
	add ah, 37h		; to ASCII A - F
	jmp short printCol

colIs0To9:
	add ah, 30h
printCol:
	call_sp UART1PrintChar

	mov	ah, 'x'
	call_sp UART1PrintChar

	mov	ecx, edi
	mov	ah, cl		;banks
	add	ah, 30h		;assuming banks will be 0-9
	call_sp	UART1PrintChar

	mov	ah, ','
	call_sp	UART1PrintChar
	mov	ah, ' '
	call_sp UART1PrintChar

;print out whether DIMM is stacked or not	
	mov	ah, bl
	or	ah, 0x50
	mov	al, 5		;SPD byte 5
	call_sp	i2c_readb
	mov	ah, al
	add	ah, 30h
	call_sp UART1PrintChar
	mov	ah, 'x'
	call_sp UART1PrintChar

	mov	ah, ch		;row size 
	mov	si, DIMM16MBMsg
	cmp	ah, 4
	je	printRowSize

	mov	si, DIMM32MBMsg
	cmp	ah, 8
	je	printRowSize

	mov	si, DIMM64MBMsg
	cmp	ah, 10h
	je	printRowSize

	mov	si, DIMM128MBMsg
	cmp	ah, 20h
	je	printRowSize

	mov	si, DIMM256MBMsg
	cmp	ah, 40h
	je	printRowSize

	mov	si, DIMM512MBMsg
	cmp	ah, 80h
	je	printRowSize

	mov	si, DIMMUnknownSizeMsg

printRowSize:
	call_sp	UART1PrintString
	call_sp VerifyDIMMGeometry
	jnc	goodGeometry

	mov	di, 8			;print out 'Bad DIMM geometry'
	jmp	nextSPDDIMM	
goodGeometry:
	mov	bh, BYTE [cs:esi+5]	;index

	mov	ah, '/'
	call_sp UART1PrintChar

	mov	cl, bh			;save index
	cmp	bh, 9
	jbe	indexIs0To9
	add	bh, 37h			;A - F
	jmp	printIndex

indexIs0To9:
	add	bh, 30h
printIndex:
	mov	ah, bh
	call_sp UART1PrintChar

	mov	bh, cl			;restore index	
	xor	edi, edi
	
rowsdone:	

;program the Row Attribute Register (7C/7D/7E/7F) to indicate
;which row is present
;Note that each register (7C/7D/7E/7F) is divided into two nibbles:
;Bits [3:0] contains the index for ROW 0
;Bits [7:4] contains the index for ROW 1
;For stacked DIMM (where SPD byte 5 returns 2), both of these
;nibbles must be set to the same index.
;	
	mov cl, bh			;index (0-12)
	mov al, bl			;DIMM #
	add al, 0x7c			;Row Attribute Register (RAR)
					;7C/7D/7E/7Fh

	mov ah, PCI_ID_CNBLE		;and write index number
	call_sp wr_cfg_db
	
;program the Memory Row Presence Register (MRPR, reg 90h)
;to indicate  which row is active.
;This register contains 8 bits that indicate which row is active
;for a total of 8 rows
;	
	mov al, 0x90
	call_sp rd_cfg_db

	;; mark memory present
	mov dl, 1
	mov cl, bl
	shl cl, 1
	shl dl, cl
	mov cl, dl
	or cl, al

	mov al, 0x90
	mov ah, PCI_ID_CNBLE
	call_sp wr_cfg_db

;read SPD byte 5 to see if this is stacked DIMM or not
;if it's stacked DIMM then it will return 2
;	
	;; bh will now be set to number of module rows
	mov ah, bl		;save DIMM# in AH
	or ah, 0x50
	mov al, 5
	call_sp i2c_readb	; read row

	mov cl, bh		; DIMM index 	
	mov bh, al		; is it stacked DIMM?

	cmp bh, 2		; stacked DIMM?
	jne .calcrefresh	;no, not stacked DIMM

;we get here because it's stacked (2-sided DIMM)
	mov al, cl		;DIMM index (0-12)
	shl al, 4		;take care of both nibbles
	or cl, al

	mov al, bl		;DIMM#
	add al, 0x7c		;which register (7C/7D/7E/7F) ?

	mov ah, PCI_ID_CNBLE
	call_sp wr_cfg_db	;update both rows for reg 7C/7D/7E/7F
	
	mov al, 0x90		;and update reg 90h as well
	call_sp rd_cfg_db

	;; mark 2nd row present
	mov dl, 2
	mov cl, bl		;DIMM#
	shl cl, 1
	shl dl, cl
	mov cl, dl
	or cl, al
	
	mov al, 0x90
	mov ah, PCI_ID_CNBLE
	call_sp wr_cfg_db
	
; now, save the refresh rate. we need to account for the
; fact that the spd information is not linear. i.e.,
; refresh	value
;   3.9us         0x1
;   7.8		  0x2		
;  15.625         0x0  (we don't care about the larger values) 
;  31.3           0x3
;  62.5		  0x4
; 125.0           0x5
; NOTE: the initial value of fs is 0x0.
.calcrefresh:		
	mov	ah, bl
	or	ah, 0x50
	mov	al, 0x0C	;SPD byte 12, refresh rate	
	call_sp	i2c_readb
	and	al, 0x7f	;mask out bit 7 (self refresh flag)
	mov	cx, fs		;FS is used later on, DO NOT MODIFY!
				;FS stores the current refresh value
	
	cmp	al, cl		;nothing to do 
	je	nextSPDDIMM		
	
	cmp	al, 0x0		; compare against 15us
	je	nextSPDDIMM
;	je	.15us		; (not used)

	cmp	al, 0x2		; > 15us
	jg	nextSPDDIMM	; keep it the default
;	jg	.gt15us		; (not used)
		
.lt15us:			; else it's < 15us
	cmp	cl, 0x0		; FS is 15us
	je	.saveRefreshRate
	cmp	al, cl		; refresh < FS
	jb	.saveRefreshRate		
	jmp	nextSPDDIMM	
		
;; this is here just for completeness' sake
;.15us:		
;	cmp	cl, 0x2		; FS > 15us
;	jg	.saveRefreshRate	
;	jmp	nextSPDDIMM
;
;.gt15us:	
;	cmp	cl, 0x3		; FS is <= 15us
;	jb	nextSPDDIMM
;	cmp	al, cl		; refresh < fs
;	jb	.saveRefreshRate
;	jmp	nextSPDDIMM			
	
.saveRefreshRate	
	mov	cl, al		; store new refresh value
	mov	fs, cx
	
nextSPDDIMM:
	mov	si, SPDDIMMFoundMsg	;assuming no SPD error	
	cmp	di, 0
	je	.printSPDErrorMsg	;well, not really an error


;we have an SPD error message to be printed
	cmp	di, 1
	mov	si, SPDNoDIMMMsg
	je	.printSPDErrorMsg

	mov	si, SPDNotSDRAMMsg
	cmp	di, 2
	je	.printSPDErrorMsg
	
	mov	si, SPDCASLatencyErrorMsg
	cmp	di, 3
	je	.printSPDErrorMsg

	mov	si, SPDXSumErrorMsg
	cmp	di, 4
	je	.printSPDErrorMsg

	mov	si, SPDNotRegisteredMsg
	cmp	di, 5
	je	.printSPDErrorMsg

	mov	si, SPDNotECCMsg
	cmp	di, 6
	je	.printSPDErrorMsg

	mov	si, SPDDIMMWidth72Msg
	cmp	di, 7
	je	.printSPDErrorMsg

	mov	si, DIMMBadGeometryMsg
	cmp	di, 8
	je	.printSPDErrorMsg

	mov	si, SPDUnknownMsg
	 
.printSPDErrorMsg:
	call_sp	UART1PrintString

.noSPDErrorMsg:
	inc bl
	cmp bl, 0x4
	je  setupMRDR
	jmp probeloop

setupMRDR:	
;; set up memory so that it's interleaved.
;; we store everything in ebx (ebx >> 16 = top of memory,
;; 	                       bh = index, bl = memory size) 
;; we grab the memory size by reading the DIMM type
;; NOTE: if we have 4GB, we automatically shave 16MB off the top so
;;       that there's room for the IOAPIC tables. it also simplifies
;;       things as we don't have to stick the last 256MB in row 7.
	mov	ebx, 0x0000FF00	; FF = get first row
setupDIMMRow:	
	mov	ah, bh
	call_sp DIMMGetRow
	jc	near .saveDIMMSettings
	mov	bh, BYTE [cs:esi+1]	; row index
	
	mov	al, 0x90	; get MRPR
	mov	ah, PCI_ID_CNBLE
	call_sp rd_cfg_db

	mov	ah, 1
	mov	cl, bh
	shl	ah, cl
	
	test	al, ah		;  is this row present?
	jz	setupDIMMRow

	;; get size by reading RAR and looking in the DIMM table
	mov	al, bh
	shr	al, 1		; divide by 2 to get RAR
	add	al, 0x7C	
	mov	ah, PCI_ID_CNBLE 
	call_sp	rd_cfg_db

	test	bh, 0x01	; odd?
	jz	.evenrow	
	shr	al, 4		; get the appropriate row

.evenrow	
	and	al, 0x0F		
	mov	ah, al
	call_sp	DIMMGetRowSize
	jc	setupDIMMRow
	mov	ch, BYTE [cs:esi+4]
				
	;; grab current memory upper limit
	mov	eax, ebx
	shr	eax, 16
	mov	cl, al
	
	;; compute register
	mov	al, bh
	shl	al, 1		; 2 regs per row
	add	al, 0x80
	mov	ah, PCI_ID_CNBLE
	call_sp wr_cfg_db	; write memory region start
		
	;; compute register
	mov	al, bh
	shl	al, 1		; 2 regs per row
	add	al, 0x81
	mov	ah, PCI_ID_CNBLE

	add	cl, ch		; add size
	cmp	cl, 0x0		; 4GB?
	jne	.not_four_gig
	mov	cl, 0xFF	; subtract off 16MB
	
.not_four_gig
	call_sp wr_cfg_db	; write memory region end

	;; store current max ram
	xor	eax, eax
	mov	al, cl
	shl	eax, 16
	and	ebx, 0x0000FFFF
	or	ebx, eax
	jmp setupDIMMRow
	
.saveDIMMSettings		; update memory limit
	shr	ebx, 16
	mov	cl, bl
	cmp	cl, 0x0		; 4GB?
	jne	.not_four_gig2
	mov	cl, 0xFF	; subtract off 16MB
	
.not_four_gig2	
	mov	al, 0x70
	mov	ah, PCI_ID_CNBLE
	call_sp wr_cfg_db
	
;; this is unnecessary because we always have at least a 16MB hole
;; at the top of memory
;	cmp	cl, 0x0		; set bit 4 of BCR2 if we have 4GB
;  	jne	doRefreshRate
; 	mov	al, 0x92	 
;  	mov	ah, PCI_ID_CNBLE
; 	call_sp	rd_cfg_db
; 	or	al, 0x10
; 	mov	cl, al
	
; 	mov	al, 0x92
; 	mov	ah, PCI_ID_CNBLE
; 	call_sp	wr_cfg_db
	
doRefreshRate:	
;set refresh rate depending on the value in FS
	call_sp FSBGetTiming	;load FSB timings into esi
	mov	cx, fs		;DIMM refresh rate

;The Refresh Counter Register (RCR, 79h) of the North Bridge
;if FS = 0x0h, reg 79h is set to 1Eh to give refresh rate = 15.4 us 
;if FS = 0x1h, reg 79h is set to 1Eh / 4 (= 7) or refresh rate =  3.85 us
;if FS = 0x2h, reg 79h is set to 1Eh / 2 (= 15) or refresh rate =  7.7 us
;if FS = 0x3h, reg 79h is set to 1Eh * 2 (= 60) or refresh rate ~ 31.3us
;if FS = 0x4h, reg 79h is set to 1Eh * 4 (= 120) or refresh rate ~ 62.5us
;if FS = 0x5h, reg 79h is set to 1Eh * 8 (= 240) or refresh rate ~ 125us
; 
; or given default refresh = x
; if fs == 0x0, value = x
; if fs < 0x3, value = x >> (3 - fs)
; if fs > 0x2, value = x << (fs - 2)
; we base everything upon the default value for the FSB speed.
; 100MHz -- just need to modify the initial value.
	mov	bl, BYTE [cs:esi+2]	; get default refresh rate
	cmp	cl, 0x0		; = 15.4us
	je	setRefreshRate
	cmp	cl, 0x3		; < 15.4us
	jb	setFastRefresh
	
	shl	bl, 2		; > 15.4us
	shr	bl, cl
	jmp	setRefreshRate
	
setFastRefresh:	
	shl	bl, cl		; < 15.4us
	shr	bl, 3
setRefreshRate:
	mov	ax, fs		; we're going to save the "real" refresh rate
	mov	al, bl
	mov	fs, ax
	mov	cl, bl
	
	mov	al, 79h		;RCR
	mov	ah, PCI_ID_CNBLE	
	call_sp wr_cfg_db

;check SDRAM CAS Latency to see if we can set it to 2 instead of
;defaulting to 3
;If all 4 DIMMs support CAS Latency of 2 then register 92h, bit 1
;will be set to 1.
; 
	xor	bx, bx		;BH = SDRAM cycle time, BL = counter

testNextDIMMLatency:

;read MRPR to see if we have an okay DIMM
	mov al, 0x90
	mov ah, PCI_ID_CNBLE
	call_sp rd_cfg_db		

	mov	ah, 1		; just need to check against row 0
	mov	cl, bl
	shl	cl, 1
	shl	ah, cl
	test	al, ah
	jz	nextDIMMLatency

;OK, we found a DIMM.  See if it supports latency = 2
;
	mov	ah, bl
	or	ah, 50h
	mov	al, 18		;SPD byte 18
	call_sp	i2c_readb

	test	al, 2		;CAS latency 2
	jz	doBufferStrength	;latency = 2 not supported

;See which SPD byte we have to read to get cycle time
;We need to determine which SPD byte we've to read:
;Here's the table:
;1.  CAS Latency 3,2,1:  SPD byte 23
;2.  CAS Latency 3,2:    SPD byte 23
;3.  CAS Latency 3,1:	 none
;4.  CAS Latency 3:	 none
;5.  CAS Latency 2,1:    SPD byte 9
;5.  CAS Latency 2:      SPD byte 9
;6.  CAS Latency 1:      none
;NOTE: this depends upon CL2 always being either the highest or the
;      2nd highest CAS latency. 
;
	mov	cl, al
	mov	al, 23		;assume we're going to read SPD byte 23
	and	cl, 00000111b	;CAS latency 3,2,1

	cmp	cl, 6		;CAS latency 3, 2
	jae	readSPDCycleTime

	mov	al, 9		;assume we're going to read SPD byte 9
	cmp	cl, 3
	je	readSPDCycleTime

	cmp	cl, 2
	je	readSPDCycleTime

;everything else is no good so we don't change reg 92h, bit 1
	jmp	doBufferStrength
	
readSPDCycleTime:
	mov 	ah, bl
	or	ah, 50h
	call_sp	i2c_readb

;100MHz -- test against 10ns
	cmp	al, BYTE[cs:esi+3]	;must be >= cycle time
	jb	doBufferStrength

nextDIMMLatency:
	inc	bl
	cmp	bl, 4		;all 4 DIMMs tested for latency == 2?
	jb	testNextDIMMLatency

;Set reg 92h, bit 1 = 1 to indicate we support CAS latency = 2
;100MHz -- CAS latency of 2 is only supported with 100MHz FSB
;; for now, just make sure that CAS latency is set to 3
	mov	al, 92h		;bridge config reg 2 (BCR2)
	mov	ah, PCI_ID_CNBLE
	call_sp	rd_cfg_db	;what's the current setting?

	mov	ah, BYTE[cs:esi+1]	;  CL2 okay?
	test	ah, 0x1
	jz	CL3only
	or	al, 0x02	;CAS latency = 2
	jmp	setCL

CL3only:	
	and	al, 0xfd	;CAS latency = 3
setCL:		
	mov	cl, al
	mov	al, 92h
	mov	ah, PCI_ID_CNBLE
	call_sp wr_cfg_db	;and update reg 92h

doBufferStrength:
;set buffer strength to low if have <= 2 rows
;This is done by:
;1  reading the stored number of rows from FS
;2  If <=2, set buffer strength (reg 90h, bit 0) to 0
;3  If >2 , set buffer buffer strength to 1 (this is the default value)
	mov	dx, fs		; number of rows is stored in the lower
	and	dh, 0x0f	; half of dh
	cmp	dh, 2
	ja	doneSetBufferStrength

	mov	al, 0FDh	;auxillary options register
	mov	ah, PCI_ID_CNBLE
	call_sp	rd_cfg_db
	and	al, 0FEh	;mask off bit 0
	mov	cl, al
	mov	al, 0FDh
	mov	ah, PCI_ID_CNBLE
	call_sp wr_cfg_db

doneSetBufferStrength:

;set DRAM dynamic timings by reading SPD info and then program
;reg 91h (Memory Timing Control Register, MTCR)
;
;100MHz -- this needs to be re-clocked for 100MHz FSB
;This is done by:
;1.  Read SPD byte 30 to get tRAS which corresponds to bit 7 of reg 91h
;2.  Read SPD byte 29 to get tRCD which corresponds to bit 6 of reg 91h
;3.  Read SPD byte 27 to get tRP.  Bits 4:3 are set from tRAS + tRP
;4.  Read SPD byte 18.  Choose a CAS latency of 2 for PC133 DIMMs if
;    they can handle 7.5 ns cycle
;5   Read SPD byte 22, bit 1 to see if DIMM supports auto-precharge
;    Set bit 0 of reg 91h iff tRAS <= tRCD + burst length
;    where burst length = 15ns on a 133 MHz FSB system.
;Note that Bit 2 of reg 91h (Write to Deact) must be set to 0 (2 clocks)
	xor	bl, bl			;DIMM#
	mov	di, 11111000b		;default to fastest timing
getDIMMTiming:
;read MRPR to see if we have an okay DIMM.
	mov al, 0x90
	mov ah, PCI_ID_CNBLE
	call_sp rd_cfg_db		
	
	mov	ah, 1		; just need to check against row 0
	mov	cl, bl
	shl	cl, 1
	shl	ah, cl
	test	al, ah
	jz	near nextDIMM 

readSPD30:
	mov	ah, bl
	or	ah, 0x50
	mov	al, 30			;SPD byte 30 (decimal)
	call_sp	i2c_readb
	mov	ch, al			;use later for bits 4:3
	cmp	al, BYTE[cs:esi+5]	;must be <= 5clks for low latency
	jb	MTCRBit6		;leave fastest timing alone
	and	di, 7Fh			;set tRAS to 6 clks
MTCRBit6:
	mov	ah, bl
	or	ah, 0x50
	mov	al, 29			;SPD byte 29
	call_sp	i2c_readb		;get tRCD
	cmp	al, BYTE[cs:esi+4]	; <= 2 clks? 
	jb	MTCRBit5
	and	di, 10111111b		;bit 6 of reg 91h is tRCD
MTCRBit5:
	mov	ah, bl
	or	ah, 0x50
	mov	al, 27			;SPD byte 27
	call_sp i2c_readb
	cmp	al, BYTE[cs:esi+4]	; <= 2 clks?
	jb	MTCRBits4_3		;yes, leave default value alone
	and	di, 11011111b
MTCRBits4_3:
	add	al, ch
	cmp	al, BYTE[cs:esi+6]	; <= 7 clks
	jb 	nextDIMM		;leave default timing alone
	cmp	al, BYTE[cs:esi+7]	; > 8 clks?
	ja	try9Clks
;check to see if we've already set it to a lower setting
;remember that if we shift right 3 bits then:
;DX = 0 -> 10 clocks
;DX = 1 ->  9 clocks
;DX = 2 ->  8 clocks
;DX = 3 ->  7 clocks
	mov	dx, di			;get current setting
	shr	dx, 3
	cmp	dl, 2			;current setting slower than 8 clks?
	jbe	nextDIMM		;;we're already slow
	or	di, 00010000b
	and	di, 11110111b
	jmp	nextDIMM
try9Clks:
	cmp	al, BYTE[cs:esi+8]	; > 9 clks?
	ja	try10Clks
;check to see if we've already set it to a slower setting
;remember that if we shift right 3 bits then:
;DX = 0 -> 10 clocks
;DX = 1 ->  9 clocks
;DX = 2 ->  8 clocks
;DX = 3 ->  7 clocks
	mov	dx, di			;get current setting
	shr	dx, 3			;get bits 4:3 (tRC)
	cmp	dl, 1			;currrent setting slower than 9?
	jbe	nextDIMM
	and	di, 11101111b
	or	di, 00001000b
	jmp	nextDIMM	
try10Clks:
	and	di, 11100111b		; else, 10 clks
	
nextDIMM:
	inc	bl
	cmp	bl, 4
	je	programMemOverlap
	jmp	getDIMMTiming

programMemOverlap:
; we can enable memory overlap only when tRC == tRP == CAS latency == 3
	mov	cx, di
	test	cl, 0x60		; tRCD/tRP == 3
	jnz	programMTCR

	mov	al, 0x92
	mov	ah, PCI_ID_CNBLE
	call_sp	rd_cfg_db
	test	al, 0x2			; CAS latency == 3
	jnz	programMTCR

	or	cl, 0x2			; enable memory overlap
programMTCR:
	mov	al, 0x91
	mov	ah, PCI_ID_CNBLE
	call_sp wr_cfg_db

;We need to program register 7Bh (SDRAM Power On Sequence Register, SPSR)
;The power on sequence is as follows:
;1.  Program all the memory registers
;2.  Write 1 to Bit 2
;3.  Wait at leat 100 usec
;4.  Write 1 to Bit 3
;5.  Wait at least 200 usec
;6.  Write 1 to Bit 0
;7.  Wait at least 16 refresh cycles
;8.  Write 1 to Bit 1
;9.  Enable memory controller (Bit 0 of reg 92h)
;Register 7Bh contains the following bits:
;Bit 0 = precharge SDRAM
;Bit 1 = write SDRAM Mode Register Command
;Bit 2 = start a NOP command to SDRAM
;Bit 3 = stop the NOP command to SDRAM
;Bit 4 = stop refresh if set to 1.  If set to 0, the NB loads
;        the refresh counter from RCR (reg 79h) and starts counting
;
	;; set Refresh Counter Register to 0x20

	mov cl, 0x20
	mov al, 0x79
	mov ah, PCI_ID_CNBLE
	call_sp wr_cfg_db	; set refresh counter

;At this point, experiment shows that roughly 880,000 I/Os
;can be done in 60 seconds
;This translates to (roughly) 68 usec per I/O
;
	;; 
	;; start the power on sequence for the SDRAM
	;;
	mov cl, 4
	mov al, 0x7b
	mov ah, PCI_ID_CNBLE
	call_sp wr_cfg_db	; start NOP command
;
;wait at least 200 usecs
	mov	ecx, 4
wait200us:
	out	0EDh, al	;68 usec per I/O
	loop	wait200us
	
	mov cl, 8		;send stop NOP command
	mov al, 0x7b
	mov ah, PCI_ID_CNBLE
	call_sp wr_cfg_db	

;wait at least 400 microseconds
	mov	ecx, 6
wait400us:
	out	0EDh, al	;68 usec per OUT instruction
	loop	wait400us
	
	mov cl, 1
	mov al, 0x7b
	mov ah, PCI_ID_CNBLE
	call_sp wr_cfg_db	; precharge SDRAM

;wait at least 16 refresh cycles. refresh is currently set to 0x20 
;assuming 100MHz FSB for the slowest refresh, that gives us
;a refresh period of 30.8us. 30.8*16 = 492.8us. so, wait
;at least 500us.
	mov	ecx, 8
wait500us:		
	out	0EDh, al
	loop	wait500us

	mov	cl, 8
	mov	al, 0x7b
	mov	ah, PCI_ID_CNBLE	
	call_sp wr_cfg_db		;stop refresh

;; wait at least 30 clocks
	out	0EDh, al
	out	0EDh, al
	
	or	cl, 2			; keep refresh stopped
	mov	al, 0x7B
	mov	ah, PCI_ID_CNBLE	
	call_sp	wr_cfg_db		;stop precharge	

;; wait at least 30 clocks
	out	0EDh, al
	out	0EDh, al
	
	mov	cl, 0
	mov	al, 0x7b
	mov	ah, PCI_ID_CNBLE
	call_sp	wr_cfg_db		;start refresh
	
; enable the memory controller
	mov	al, 92h
	mov	ah, PCI_ID_CNBLE
	call_sp rd_cfg_db
	
	mov	cl, al
	or	cl, 0x1		
	mov	al, 92h
	mov	ah, PCI_ID_CNBLE
	call_sp wr_cfg_db
	
; reset refresh counter -- the real value is stored in fs.
	mov	ax, fs
	mov	cl, al
	mov al, 0x79
	mov ah, PCI_ID_CNBLE
	call_sp wr_cfg_db	; set refresh counter

;Clear any error on the SM Bus
;
	mov	dx, SMB_IO_BASE	
	in	al, dx
	out	dx, al

;Clear both correctable / incorrectable errors by reading and
;then writing to Error Status Registers (ERRSTS, 47h)
	mov	ah, PCI_ID_CNBLE
	mov	al, 47h
	call_sp rd_cfg_db

	mov	cl, al
	mov	al, 47h
	mov	ah, PCI_ID_CNBLE
	call_sp wr_cfg_db
	
;enable shadowing, clear the shadow region (640k - 1M), disable shadowing
;0x55 enables writes to the memory regions
        mov     al, 70h                 ;we really want to read
        mov     ah, PCI_ID_CNBLE        ;regs 71h,72h,73h
                                        ;function 0, device 0
        call_sp rd_cfg_dl               ;ECX = value read
        mov     esi, ecx                ;save value for restoration later
        or      ecx, 55555500h          ;set it to R/W
        mov     al, 70h
        mov     ah, PCI_ID_CNBLE
        call_sp wr_cfg_dl               ;and program the chipset


	;; clear the bottom 1M of memory
	cld	
	xor	bx, bx
	xor	ax, ax
	
low_mem_clear_loop:
	mov	es, bx

	;; we need to slpit up the stosb because cx can not hold all
	;; 0x10000 bytes in a segment
	mov	cx, 0x8000
	xor	di, di
	rep	stosb

	mov	cx, 0x8000
	mov	di, cx
	rep	stosb

	cmp	bx, 0xf000
	je	.loop_done
	add	bx, 0x1000
	jmp	low_mem_clear_loop
.loop_done

;and then restore attribute in regs 70h to 73h, inclusive
        mov     al, 70h
        mov     ah, PCI_ID_CNBLE
        mov     ecx, esi                ;restore original value
        call_sp wr_cfg_dl

%ifndef new_stack_code	
;clear top 16 MB of memory (for the stack)
;get the ToM from the chipset
	mov	ah, PCI_ID_CNBLE
	mov	al, 70h
	call_sp	rd_cfg_db

;now we've got units of 16MB, convert to bytes
	movzx	edi, al
	shl	edi, 24
	sub	edi, (16 * 1024 * 1024)		;start at (ToM - 16MB)
	mov	ecx, (16 * 1024 * 1024) / 4	;number of dwords
	xor 	eax, eax
	rep a32 stosd
%endif
	
;clear both correctable and uncorrectable errors
;this is done by reading and then writing to the reg 47h

	mov	al, 47h
	mov	ah, PCI_ID_CNBLE
	call_sp	rd_cfg_db

	mov	cl, al
	mov	al, 47h
	mov	ah, PCI_ID_CNBLE
	call_sp wr_cfg_db

; and enable parity generation / ECC error
; we DO NOT enable NMIs here. we can't do that until we have an
; interrupt stack
	mov	al, 0E0h		;ECC flow control reg
	mov	ah, PCI_ID_CNBLE
	call_sp rd_cfg_db

	mov	cl, al
	or	cl, 4			;ECC is bit 2
	mov	al, 0E0h
	mov	ah, PCI_ID_CNBLE
	call_sp wr_cfg_db

	mov	al, 46h			;Error command reg
	mov	ah, PCI_ID_CNBLE
	call_sp rd_cfg_db

	mov	cl, al
	or	cl, 6			;SERR# and SALERT
	mov	al, 46h
	mov	ah, PCI_ID_CNBLE
	call_sp wr_cfg_db

	;; all should be well now
	jmp dram_setup_done

;This is a stackless routine
;Input:  AH = character to be printed
;
UART1PrintChar: 

;see if CMOS_DEBUG_FLAG is set before printing debugging info
	mov	dx, 70h
	mov	al, 11h
	out	dx, al
	inc	dx
	in	al, dx
	test	al, 01h			;CMOS_CONSOLE_FLAG set?
	jz	printNoChar		;don't print anything
	test	al, 02h			;CMOS_DEBUG_FLAG set?
	jz	printNoChar		;don't print anything

	mov	dx, UART_BASE + UART_LSR
THRNotReady:
	in	al, dx
	test	al, 20h
	jz	THRNotReady

	mov	dx, UART_BASE + UART_MCR
	in	al, dx
	and	al, 0FDh
	out	dx, al

	mov	dx, UART_BASE
	mov	al, ah			;character to be printed
	out	dx, al
printNoChar:
	ret_sp
;
;Stackless routine
;Input: SI points to string to be printed
;Note that string must be null-terminated
;
UART1PrintString:
;see if CMOS_DEBUG_FLAG is set before printing any debugging info
	mov	dx, 70h
	mov	al, 11h
	out	dx, al
	inc	dx
	in	al, dx
	test	al, 01h			;CMOS_CONSOLE_FLAG set?
	jz	stringEnd
	test	al, 02h			;CMOS_DEBUG_FLAG set?
	jz	stringEnd		;don't print anything
	
	mov	dx, UART_BASE + UART_LSR
notReady:
	in	al, dx
	test	al, 20h
	jz	notReady

	mov	dx, UART_BASE + UART_MCR
	in	al, dx
	and	al, 0FDh
	out	dx, al

	mov	dx, UART_BASE
	mov	al, [cs:si]
	cmp	al, 0			;null char?
	je	stringEnd
	out	dx, al
	inc	si
	jmp	UART1PrintString	
stringEnd:
	ret_sp

;Stackless routine
disable_nmi_parity:
	mov	dx, 0x61
	in	al, dx
	out	0xed, al		;I/O delay

	or	al, 0xc			;disable parity
	out	dx, al			
	ret_sp

;VerifyDIMMGeometry ()
;Input: EDI = row x col x density x banks
;       Bits [31:24] = row
;       Bits [23:16] = col
;       Bits [15:8] = density
;       Bits [7:0] = banks
;Exit: ESI points to appropriate index in table
;      Note that ESI can be used to index into the table to get size/index
;
VerifyDIMMGeometry:
	mov	esi, DIMMGeomTable
nextDIMMGeom:
	mov	eax, DWORD [cs:esi]
	cmp	eax, 0FFFFFFFFh
	je	geomNotSupported
	cmp	eax, edi
	je	foundSupportedGeom
	add	esi, 6			;next geometry value
					;skipped size/index 
	jmp	nextDIMMGeom
geomNotSupported:
	stc
	ret_sp
foundSupportedGeom:
	clc
	ret_sp
	
;DIMMGetRowSize ()
;Input: ah = type
;Exit: ESI points to appropriate index in table
DIMMGetRowSize:
	mov	esi, DIMMGeomTable
DIMMNextGeom:
	mov	al, BYTE [cs:esi+5]
	cmp	al, 0xFF
	je	DIMMTypeNotFound
	cmp	al, ah
	je	DIMMTypeFound
	add	esi, 6			;next geometry value
					;skipped size/index 
	jmp	DIMMNextGeom
DIMMTypeNotFound:	
	stc
	ret_sp
DIMMTypeFound:	
	clc
	ret_sp
	
;DIMMGetRow ()
;Input: ah = row index
;Output: esi = points to appropriate index in table
DIMMGetRow:	
	mov	esi, DIMMOrderTable
nextDIMMRow:
	mov	al, BYTE [cs:esi]
	cmp	al, 0x0
	je	DIMMNoRow
	cmp	al, ah
	je	DIMMfoundRow
	add	esi, 2			;next geometry value
	jmp	nextDIMMRow
DIMMNoRow:	
	stc
	ret_sp
DIMMfoundRow:	
	clc
	ret_sp
	
;FSBGetTiming()
;Input:	nothing 
;Output: esi = points to appropriate index in table. if it can't
;        find anything, it will default to 133MHz.
;NOTE: this reads from fs and clobbers ax
FSBGetTiming:	
	mov	ax, fs
	shr	ax, 12		; get FSB value
	and	ah, 0x01
	mov	esi, FSBTimingTable
nextFSBTiming:	
	mov	al, BYTE [cs:esi]
	cmp	al, 0xFF
	je	FSBDefault
	cmp	al, ah
	je	FSBFoundTiming
	add	esi, 9		; next FSB
	jmp	nextFSBTiming
FSBDefault:			; default to the first entry
	mov	esi, FSBTimingTable
FSBFoundTiming:
	ret_sp
	
;Geometry table is composed as follows:
;Bits [7:0] = # banks (SPD byte 17)
;Bits [15:8] = density (SPD byte 31)
;SPD byte 31 is configured as follows:
;Bit 0 = 4 Mbyte
;Bit 1 = 8 Mbyte
;Bit 2 = 16 Mbyte
;Bit 3 = 32 Mbyte
;Bit 4 = 64 Mbyte
;Bit 5 = 128 Mbyte
;Bit 6= 256 Mbyte
;Bit 7 = 512 Mbyte
;Bits [23:16] = col (SPD byte 4)
;Bits [31:24] = row (SPD byte 3)
DIMMGeomTable:
	dd	080C0804h		;32/0, 64M 4-bank, 12x8
	db	32 / 16			;32MB (in terms of 16MB unit)
	db	0x0			;index = 0

	dd	090B0402h		;16/1, 16M 2-bank, 11x9
	db	16 / 16			;16MB (in 16MB unit)
	db	0x1			;index = 1

	dd	0A0B0802h		;32/2, 16M 2-bank, 11x10
	db	32 / 16			;32 MB (in 16MB unit)
	db	0x2			;index = 2

	dd	0B0C4004h		;256/3, 128M 4-bank, 12x11
	db	256 / 16		;256 MB (in 16MB unit)
	db	0x3			;index = 3
		
	dd	0B0D8004h		;512/4, 256M 4-bank, 13x11
	db	512 / 16		;512 MB (in 16MB unit)
	db	0x4			;index = 4
	
	dd	0A0D2002h		;128/5, 64M 2-bank, 13x10
	db	128 / 16		;128 MB
	db	0x5			;index

	dd	0A0C2004h		;128/6, 64M 4-bank, 12x10
	db	128 / 16		;128 MB
	db	0x6			;index = 6

	dd	090C1004h		;64/7, 64M 4-bank, 12x9
	db	64 / 16			;64 MB
	db	0x7

	dd	0A0D4004h		;256/8, 256M 4-bank, 13x10
	db	256 / 16		;256 MB
	db	0x8			;index

	dd	090D2004h		;128/9, 256M 4-bank, 13x9
	db	128 / 16		;128 MB
	db	0x9

	dd	080D1004h		;64/10, 256M 4-bank, 13x8
	db	64 / 16			;64 MB
	db	0xa			;index

	dd	090C1004h		;64/11, 256M 4-bank, 12x9
	db	64 / 16			;64 MB
	db	0xb			;index

	dd	0A0C2004h		;128/12, 256M 4-bank, 12x10
	db	128 / 16		;128 MB
	db	0xc			;index

	dd	0FFFFFFFFh		;end of table
	db	0xFF	
	db	0xFF	
	
;the DIMM order table specifies the memory layout. it does 3 things:	
;   1) preferentially avoids the first socket
;   2) preferentially begins in the last socket
;   3) spreads the memory map as evenly as possible
;we use the fact that dimms have at least one row.
;NOTE: if we use all 4GB (bit 4 of BRCR2 set), then the last 256MB 
;      must be in row 7. as the IOAPIC tables sit at the top 16MB, we
;      don't have to worry about that.
;DIMM search order: 6, 2, 4, 7, 3, 5, 1, 0
DIMMOrderTable:	
	db	0xFF			; DIMM index
	db	0x6			; row 
	
	db	0x6
	db	0x2
	
	db	0x2
	db	0x4
	
	db	0x4
	db	0x7
	
	db	0x7
	db	0x3
	
	db	0x3
	db	0x5
	
	db	0x5
	db	0x1
	
	db	0x1
	db	0x0
	
	db	0x0			; end of table
	
;the FSB timing table stores all the relevant timing bits for
;133/100FSB timing dependent calculations
FSBTimingTable:	
	db	0		; 133MHz FSB		[0]
	db	0x0		; CL2 okay?		[1]
	db	0x1e		; default refresh rate  [2]
	db	0x75		; 7*10 + 5*.10 ns 	[3]
	db	16		; > 2 clks		[4]
	db	39		; > 5 clks		[5]
	db	53		; > 7 clks		[6]
	db	60		; = 8 clks		[7]
	db	67		; < 9 clks		[8]
	
	db	1		; 100MHz
	db	0x1		; CL2 okay?
	db	0x16		; default refresh rate
	db	0xa0		; 10*10 + 0*.10 ns min. cycle time 
	db	21		; > 2 clks
	db	51		; > 5 clks 
	db	71		; > 7 clks
	db	80		; = 8 clks
	db	90		; = 9 clks
	
	db	0xFF
	
;Error Messages
;DI = error index
;DI = 1 = SPDNoDIMMMsg
;DI = 2 = SPDNotSDRAMMsg
;DI = 3 = SPDCASLatencyErrorMsg
;DI = 4 = SPDXSumErrorMsg
;DI = 5 = SPDNotRegisteredMsg 
;DI = 6 = SPDNotECCMsg
;DI = 7 = SPDDIMMWidth72Msg
;DI = 8 = Unsupported DIMM geometry
;Other values = SPDUnknownMsg

DIMMProbeMsg	db	"DIMM ", 0
SPDCASLatencyErrorMsg	db	", CAS Latency must be 2 or 3", 0
SPDDIMMFoundMsg db	", OK", 0
SPDNotSDRAMMsg	db	", DIMM is not SDRAM, REJECTED ", 0
SPDNoDIMMMsg		db	", none found", 0
SPDNotECCMsg		db	", not ECC, REJECTED", 0
SPDNotRegisteredMsg	db	", not a registered DIMM, REJECTED", 0
SPDUnknownMsg		db	", unknown SPD error", 0
DIMMBadGeometryMsg	db	", REJECTED", 0
SPDDIMMWidth72Msg	db	", DIMM width is not 72, REJECTED", 0
SPDXSumErrorMsg		db	", checksum does not match, REJECTED", 0
SPDDIMMRowColMsg	db	", RxCxB, Size/Index = ", 0	
DIMM16MBMsg		db	"16MB", 0
DIMM32MBMsg		db	"32MB", 0
DIMM64MBMsg		db	"64MB", 0
DIMM128MBMsg		db	"128MB", 0
DIMM256MBMsg		db	"256MB", 0
DIMM512MBMsg		db	"512MB", 0
DIMMUnknownSizeMsg	db	"???MB", 0
