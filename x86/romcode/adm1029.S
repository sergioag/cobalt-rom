; $Id: adm1029.S,v 1.1.1.1 2003/06/10 22:42:23 iceblink Exp $
; Copyright 2001 Sun Microsystems, Inc.
; All Rights Reserved.

;;	okay. for now, you'll need to turn monitoring on (0x1) as the
;; 	strapping options don't have that. we're thinking about changing it so
;; 	that thermal monitoring starts by default. also, the temperatures are
;; 	way off for some reason (-5C vs 34C). also, the analog inputs for Vtt
;; 	and Vcp are voltage*100. you'll have to configure those two inputs as
;; 	they're GPIO's by default.
	
%ifndef INCLUDE_ADM1029_S
%define INCLUDE_ADM1029_S

%include "i2c5k.S"

%define ADM1029_SMB_DEV			(0x5e)     ; I2C Slave Device Address
%define ADM1029_STATUS			(0x00)     ; Main Status Register
%define ADM1029_CONFIG			(0x01)     ; Config Register
%define ADM1029_RESET			(0x0b)     ; write causes Software Reset
%define ADM1029_MFG_ID			(0x0d)     ; Manufacturer ID [41h]
%define ADM1029_REV_ID			(0x0e)     ; Major/Minor Revision

%define ADM1029_GPIO_PRESENT		(0x05)     ; GPIO Present / AIN
%define ADM1029_GPIO_CONTROL(x)		(0x28 + x) ; GPIO Control
%define ADM1029_GPIO_FAULT(x)		(0x38 + x) ; set alarm speed if GPIO asserted

%define ADM1029_FAN_SET_ALRM		(0x07)     ; set Fan X Alarm Speed
%define ADM1029_FAN_SET_PLUG		(0x08)     ; set Fan X HotPlug Speed
%define ADM1029_FAN_SET_FULL		(0x09)     ; set Fan X Full Speed
%define ADM1029_FAN_SPINUP		(0x0c)     ; Fan Spin-up Time
%define ADM1029_FAN_STATUS(x)		(0x10 + x) ; Fan Status
%define ADM1029_FAN_CONTROL(x)		(0x18 + x) ; Fan Control
%define ADM1029_FAN_FAULT(x)		(0x20 + x) ; set alarm speed if Fan Fault
%define ADM1029_FAN_SPEED_FULL(x)	(0x60 + x) ; Normal Fan speed values
%define ADM1029_FAN_SPEED_PLUG(x)	(0x68 + x) ; HotPlug Fan speed values
%define ADM1029_FAN_TACH_VALUE(x)	(0x70 + x) ; Measured Fan Tachometer
%define ADM1029_FAN_TACH_HIGH(x)	(0x78 + x) ; High Limit for Fan Tachometer

%define ADM1029_TEMP_PRESENT		(0x06)     ; Temp Devices Installed
%define ADM1029_TEMP_OFFSET(x)		(0x30 + x) ; Temp Systemic Offset Effect
%define ADM1029_TEMP_CONTROL(x)		(0x40 + x) ; Temp Fault Control
%define ADM1029_TEMP_FAULT(x)		(0x48 + x) ; set alarm speed if Temp Fault
%define ADM1029_TEMP_MIN(x)		(0x80 + x) ; Min Temp for fan speed ctl
%define ADM1029_TEMP_HYST(x)		(0x88 + x) ; Temp range/step for fan speed ctl
%define ADM1029_TEMP_HIGH(x)		(0x90 + x) ; High Limit Temp measurement
%define ADM1029_TEMP_LOW(x)		(0x98 + x) ; Low Limit Temp measurement
%define ADM1029_TEMP_VALUE(x)		(0xa0 + x) ; Measured Temp Value

%define ADM1029_AIN_CONTROL(x)		(0x50 + x) ; Analog Channel Control
%define ADM1029_AIN_FAULT(x)		(0x58 + x) ; set alarm speed of Analog Fault
%define ADM1029_AIN_HIGH(x)		(0xa8 + x) ; High Limit Analog measurement
%define ADM1029_AIN_LOW(x)		(0xb0 + x) ; Low Limit Analog measurement
%define ADM1029_AIN_VALUE(x)		(0xb8 + x) ; Measure Analog Channel Value

%define ADM1029_CTL_CFAULT_OVER		0x01
%define ADM1029_CTL_ALARM_OVER		0x02
%define ADM1029_CTL_INT_OVER		0x04
%define ADM1029_CTL_ALARM_LOW		0x08
%define ADM1029_CTL_CFAULT_UNDER	0x10
%define ADM1029_CTL_ALARM_UNDER		0x20
%define ADM1029_CTL_INT_UNDER		0x40
%define ADM1029_CTL_LATCH		0x80
	
; read a byte from adm1029 i2c device and offset
; 
;    %1 => OFFSET
;    AL <= DATA
; 
%macro adm1029_read 1
	mov ah, ADM1029_SMB_DEV
	mov al, %1
	call_sp i2c_readb
%endm

; write a byte to adm1029 i2c device and offset
; 
;    %1 => OFFSET
;    %2 => DATA
; 
%macro adm1029_write 2
	mov ah, ADM1029_SMB_DEV
	mov al, %1
	mov bl, %2
	call_sp i2c_writeb
%endm

; 
; detect & initialize adm1029
; 
adm1029_init:
	adm1029_read ADM1029_MFG_ID
	cmp al, 0x41		; manufacturers id should be 41h
	jne .mfgid_ok
	jmp adm1029_done
.mfgid_ok:
	
	adm1029_write ADM1029_CONFIG, 0x00		; stop monitoring

	adm1029_write ADM1029_GPIO_PRESENT, 0x64	; enable AIN0,AIN1,TDM1
	adm1029_write ADM1029_GPIO_CONTROL(2), 0x3	; GPIO2 is input, high

	adm1029_write ADM1029_FAN_CONTROL(0), 0x0       ; disable fan monitoring
	adm1029_write ADM1029_FAN_CONTROL(1), 0x0
	adm1029_write ADM1029_FAN_STATUS(0), 0x0
	adm1029_write ADM1029_FAN_STATUS(1), 0x0

	;; enable cpu and case temp and diable sensor 2
	adm1029_write ADM1029_TEMP_CONTROL(0), ADM1029_CTL_CFAULT_OVER
	adm1029_write ADM1029_TEMP_CONTROL(1), ADM1029_CTL_CFAULT_OVER
	adm1029_write ADM1029_TEMP_CONTROL(2), 0

	;; enable voltage monitoring
	adm1029_write ADM1029_AIN_CONTROL(0), (ADM1029_CTL_INT_OVER | ADM1029_CTL_INT_UNDER)
	adm1029_write ADM1029_AIN_CONTROL(1), (ADM1029_CTL_INT_OVER | ADM1029_CTL_INT_UNDER)
	
	;; zero high/low limits, let OS handle things
	adm1029_write ADM1029_TEMP_LOW(0),  0x0
	adm1029_write ADM1029_TEMP_HIGH(0), 0x0
	adm1029_write ADM1029_TEMP_LOW(1),  0x0
	adm1029_write ADM1029_TEMP_HIGH(1), 0x0
	adm1029_write ADM1029_AIN_LOW(0),   0x0
	adm1029_write ADM1029_AIN_HIGH(0),  0x0
	adm1029_write ADM1029_AIN_LOW(1),   0x0
	adm1029_write ADM1029_AIN_HIGH(1),  0x0

	jmp adm1029_done

%endif
