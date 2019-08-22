/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
.global _start
_start:
	mov $0xc7, %al
	out %al, $0x80
	call ram_main
