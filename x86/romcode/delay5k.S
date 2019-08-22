;; Copyright (c) 1999,2000 Cobalt Networks, Inc.
;; $Id: delay5k.S,v 1.1.1.1 2003/06/10 22:42:23 iceblink Exp $

	;; for timing we assume that a processor will not be running faster the
	;; 2GHz and use generous timings there.  This means that one tick of the
	;; time stamp counter is 0.0005 milliseconds
	;;

NANOSECOND equ		2
MILLISECOND equ		(1000 * NANOSECOND)
MICROSECOND equ		(1000 * MILLISECOND)
	
	;; does a delay loop based on tsc
	;;
	;; destroys EDX, EBX, EAX

%macro tscdelay 1
	rdtsc
	mov ebx, eax
%%wait:
	rdtsc
	sub eax, ebx
	cmp eax, %1
	jl %%wait
%endmacro
