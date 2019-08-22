/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: memory.c,v 1.1.1.1 2003/06/10 22:42:20 iceblink Exp $
 */

#include <io.h>
#include <string.h>

#include "common/memory.h"
#include "common/i2c.h"
#include "common/msr.h"
#include "common/rammap.h"
#include "common/rom.h"
 
#include "memory.h"

static void copybytes(unsigned char *str, unsigned long val);

/* read_mem_func()
 *   handles rb, rw, and rl commands
 */
int read_mem_func(int argc, char *argv[])
{
	uint32_t data;
	unsigned long address;

	if (argc != 2)
		return EBADNUMARGS;

	if (stoli(argv[1], &address))
		return EBADNUM;
    
	data = *((uint32_t *) address);

	switch (argv[0][1]) 
	{
	case 'b': 	
		printf("\n0x%0x: 0x%02x\n",
		       (unsigned int) address, data & 0xff);
		break;

	case 'w': 	
		printf("\n0x%0x: 0x%04x\n",
		       (unsigned int) address, data & 0xffff);
		break;

	case 'l': 	
		printf("\n0x%0x: 0x%08x\n",
		       (unsigned int) address, data & 0xffffffff);
		break;

	default:
		return EBADARG;
	}
    
	return 0;
}


/* write_mem_func()
 *   handles wb, ww, wl, and wm commands
     wb, ww, wl takes 2 arguments: address, data
     wm takes 3 arguments: address, data, length
     Note that wm will write ONE byte from specified
     starting address for the length (in bytes) specified

 */
int write_mem_func(int argc, char *argv[])
{
	unsigned long address, data;
	unsigned long ctr = 0L;
	unsigned long dataLen;
	 
	if (argc == 4) {
		
		if (argv[0][1] != 'm') {
			return EBADNUMARGS;
		}
		if (stoli(argv[3], &dataLen)) {
			return EBADNUM;
		}
	}
	if (argc == 3) {
		if (argv[0][1] !='b' && argv[0][1] != 'w' 
		 && argv[0][1] != 'l') {
			return EBADNUMARGS;
		}
	}

	if (stoli(argv[1], &address) || stoli(argv[2], &data)) 
		return EBADNUM;
	
	switch (argv[0][1]) 
	{
	case 'b': 	
		*((unsigned char *) address) = (unsigned char) data;
		break;

	case 'w':
		*((unsigned short *) address) = (unsigned short) data;
		break;

	case 'l':
		*((unsigned long *) address) = (unsigned long) data;
		break;

	case 'm':
		while (ctr <dataLen) {
			*((unsigned long *) (address + (ctr*4) ))  = (unsigned long) data;
			ctr += 1;
		}
		break;
	default:
		return EBADARG;
	}

	return 0;
}


/* disp_mem_func()
 *   handles the dm command
 */
int disp_mem_func(int argc, char *argv[])
{
	unsigned long address, length, val, ctr = 0;
	unsigned char asciistr[20] = "",temp[6];
    
	if (argc != 3)
		return EBADNUMARGS;

	if (stoli(argv[1], &address) || stoli(argv[2], &length))
		return EBADNUM;

	length = (length|0x11) >> 2;
   
	while (ctr < length) {
		if ((ctr % 4) == 0) {
			printf("%s\n0x%0x:  ", asciistr, 
				(unsigned int)(address+(ctr<<2)));
			asciistr[0] = '\0';
		}
		val = *((unsigned long *)(address + (ctr<<2)));
		copybytes(temp, val);
		catpstr(asciistr, temp);
		printf("%08x ", (unsigned int)val);
		ctr++;
	}

	printf("\n");
	
	return 0;
}

int inbwl_func(int argc, char *argv[])
{
	uint32_t data;
	unsigned long port;

	if (argc != 2)
		return EBADNUMARGS;

	if (stoli(argv[1], &port))
		return EBADNUM;
    
	switch (argv[0][2]) 
	{
	case 'b': 	
		data = inb(port);
		printf("\n0x%0x: 0x%02x\n",
		       (unsigned int) port, data & 0xff);
		break;
	    
	case 'w': 	
		data = inw(port);
		printf("\n0x%0x: 0x%04x\n",
		       (unsigned int) port, data & 0xffff);
		break;
	    
	case 'l': 	
		data = inl(port);
		printf("\n0x%0x: 0x%08x\n",
		       (unsigned int) port, data & 0xffffffff);
		break;

	default:
		return EBADARG;
	}
    
	return 0;
}

int outbwl_func(int argc, char *argv[])
{
	unsigned long port, data;
    
	if (argc != 3)
		return EBADNUMARGS;

	if (stoli(argv[1], &port) || stoli(argv[2], &data))
		return EBADNUM;

	switch (argv[0][3]) 
	{
	case 'b':
		outb((unsigned char) data, port);
		break;
	    
	case 'w':
		outw((unsigned short) data, port);
		break;
	    
	case 'l':
		outl((unsigned long) data, port);
		break;

	default:
		return EBADARG;
	}
	return 0;
}

int mem_test_func(int argc, char *argv[] __attribute__ ((unused)))
{
	char *memory = (char *) 0x0 ;
	unsigned int i;

	if (argc != 1)
		return EBADNUMARGS;

	printf("Writing: ");
	for (i = RAM_SERIAL_LOAD; i< (RAM_SERIAL_LOAD_MAX); i += 0x4)
		*((unsigned int*) (memory+i)) = i;

	printf("\nReading: ");
	for (i = RAM_SERIAL_LOAD ; i< (RAM_SERIAL_LOAD_MAX); i += 0x4) {
		if (*((unsigned int*) (memory+i)) != i) {
			printf("bad readback at 0x%0x\n", i);
			break;
		}
	}

	return 0;
}

int copy_mem_func(int argc, char *argv[])
{
	unsigned long dest, src, len;
    
	if (argc != 4)
		return EBADNUMARGS;

	if (stoli(argv[1], &dest) ||
	    stoli(argv[2], &src)  ||
	    stoli(argv[3], &len))
		return EBADNUM;

	memcpy((void *)dest, (void *)src, (unsigned int)len);

	return 0;
}

int move_mem_func(int argc, char *argv[])
{
	unsigned long dest, src, len;
    
	if (argc != 4)
		return EBADNUMARGS;

	if (stoli(argv[1], &dest) ||
	    stoli(argv[2], &src)  ||
	    stoli(argv[3], &len))
		return EBADNUM;

	memmove((void *)dest, (void *)src, (unsigned int)len);

	return 0;
}

#ifdef __OUTDATED__

int uber_mem_test_func( int argc, char *argv[] __attribute__ ((unused)) )
{
    char *memory = (char *) 0x0 ;
    unsigned int i;

    if( argc != 1 )
	return EBADNUMARGS;

    printf( "Writing: " );
    for( i = 0x100000 ; i< mem_size - 0x100000 ; i += 0x4 )
    {
	if( !(i%0x1000) )
	    printf( "0x%0x\n", i );

	*((unsigned int*)(memory+i)) = i;
    }

    printf( "\nReading: " );
    for( i = 0x100000 ; i< (mem_size - 0x100000); i += 0x4 )
    {
	outb( 0x10, 0x80 );
	if( !(i%0x1000) )
	    printf( "0x%0x\n", i );
	outb( 0x11, 0x80 );

	if( *((unsigned int*)(memory+i)) != i )
	{
	outb( 0x12, 0x80 );
	    printf( "bad readback at 0x%0x\n", i );
	    break;
	outb( 0x13, 0x80 );
	}
	outb( 0x14, 0x80 );
    }
	outb( 0x15, 0x80 );

    return 0;
}

int cache_test_func( int argc, char *argv[] __attribute__ ((unused)) )
{
/*    unsigned int * block1 = ( (unsigned int *)0xc05000);*/
    unsigned int * block2 = ( (unsigned int *)0xd00000);
    unsigned char * block1 = ( (unsigned char *)0xc05000);
    unsigned int i,j;

    if( argc != 1 )
	return EBADNUMARGS;

	//fill region
    for( i=0 ; i<(8*1024*4) ; i++ )
    {
	block1[i] = (char ) i % 0xff;
    }
	/*
    for( i=0 ; i<1024 ; i++ )
    {
	if( block1[1023-i] != (1023-i) )
	    printf( "PASS 1: block[%0x]=%0x\n", (1023-i), block1[(1023-i)]);
    }
  
    for( i=0 ; i< 1024 ; i++ )
    {
	if( block1[i] != i )
	    printf( "PASS 2: block[%0x]=%0x\n", i, block1[i]);
    }
	*/
    printf( "This should be 0: %d\n", block1[0] );
    printf( "This should be 126: %d\n", block1[126] );

	//flush cache read 512K
    for( i=0 ; i < ( 512 * 256) ; i++ )
    {
	j = block2[i];
    }

/*    for( i=0 ; i<1024 ; i++ )
    {
	if( block1[1023-i] != (1023-i) )
	    printf( "PASS 3: block[%0x]=%0x\n", (1023-i), block1[(1023-i)]);
    }
  
    for( i=0 ; i< 1024 ; i++ )
    {
	if( block1[i] != i )
	    printf( "PASS 4: block[%0x]=%0x\n", i, block1[i]);
	    }*/



    printf( "This should be 0: %d\n", block1[0] );
    printf( "This should be 126: %d\n", block1[126] );

    return 0;
}

int l1_func( int argc, char *argv[] )
{
    unsigned int cr0;
    unsigned int pci_val;
    
    if( argc != 2 )
	return EBADNUMARGS;

    if( argv[1][0] == '0' )
    {
	getcr0( cr0 );
	cr0 |= 0x60000000;
	setcr0( cr0 );

	pci_val = pci_read_config( 0x0, PCI_DEVFN(0,0), 0x40, 0x1 );
	pci_val &= ~(0x00000001);
	pci_write_config( 0x0, PCI_DEVFN(0,0), 0x40, pci_val, 0x1 );
    }
    else
    {
	pci_val = pci_read_config( 0x0, PCI_DEVFN(0,0), 0x40, 0x1 );
	pci_val |= 0x00000001;
	pci_write_config( 0x0, PCI_DEVFN(0,0), 0x40, pci_val, 0x1 );

	getcr0( cr0 );
	cr0 &= ~(0x60000000);
	setcr0( cr0 );
    }

    return 0;
}

int l2_func( int argc, char *argv[] )
{
    unsigned int pci_val;
    
    if( argc != 2 )
	return EBADNUMARGS;

    if( argv[1][0] == '0' )
    {
	pci_val = pci_read_config( 0x0, PCI_DEVFN(0,0), 0x42, 0x1 );
	pci_val &= ~(0x00000001);
	pci_write_config( 0x0, PCI_DEVFN(0,0), 0x42, pci_val, 0x1 );
    }
    else
    {
	pci_val = pci_read_config( 0x0, PCI_DEVFN(0,0), 0x42, 0x1 );
	pci_val |= 0x00000001;
	pci_write_config( 0x0, PCI_DEVFN(0,0), 0x42, pci_val, 0x1 );
    }

    return 0;
}
#endif /* __OUTDATED__ */

#define getcr0(cr0) asm("mov %%cr0, %0":"=r" (cr0))
#define setcr0(cr0) asm("mov %0, %%cr0": :"r" (cr0))

int read_test_func(int argc, char *argv[])
{
	unsigned int       *addr;
	unsigned long int  address, size;
	unsigned char      data1;
	unsigned short int data2;
	unsigned int       data4;
    
	if (argc != 3)
		return EBADNUMARGS;

	if (stoli(argv[1], &size) || stoli(argv[2], &address))
		return EBADNUM;

	addr = (unsigned int *) address;
    
	for (;;) {
		switch (size)
		{
		case 1:
			data1 = *((unsigned char *) addr);
			break;
		
		case 2:
			data2 = *((unsigned short int *) addr);
			break;

		case 4:
			data4 = *((unsigned int *) addr);
			break;

		default:
			break;
		}
	}

	return 0;
}

int write_test_func(int argc, char *argv[])
{
	unsigned long int  address, size, data;
	unsigned char      data1;
	unsigned short int data2;
	unsigned int       data4;
	unsigned int       *addr;
    
	if (argc != 3)
		return EBADNUMARGS;

	if (stoli(argv[1], &size) || stoli(argv[2], &address) || stoli(argv[3], &data))
		return EBADNUM;

	addr  = (unsigned int *) address;
	data4 = (unsigned int) data;
	data2 = (unsigned short int) data;
	data1 = (unsigned char) data;

	while(1) {
		switch (size)
		{
		case 1:
			*((unsigned char *) addr) = data1;
			break;
		
		case 2:
			*((unsigned short int *) addr) = data2;
			break;

		case 4:
			*((unsigned int *) addr) = data4;
			break;

		default:
			break;
		}
	}

	return 0;
}

int disp_mem_eeprom_func(int argc, char *argv[])
{
	unsigned long int row;
	unsigned char c;
	int i;

	if (argc != 2)
		return EBADNUMARGS;

	if (stoli(argv[1], &row))
		return EBADNUM;
    
	if (row > 3)
		return EBADNUM;

	for (i=0; i<128; i++) {

		c = i2c_read_byte(0xa0 + (row<<1), i);

		if (! (i%16))
			printf("\n%02x: ", i);
		else if(! (i%8))
			printf("  ");

		printf("%02x ", c);
	}

	printf("\n\n");

	return 0;
}

int read_msr_func(int argc, char *argv[])
{ 
	unsigned long int index;
	unsigned long long value;

	if(argc != 2)
		return EBADNUMARGS;
    
	if (stoli(argv[1], &index))
		return EBADNUM;

	value = msr_get(index);
    
	printf("MSR %x: 0x%08x%08x\n",
	       (unsigned int) index,
	       (unsigned int) (value>>32),
	       (unsigned int) value);

	return 0;
}

int write_msr_func(int argc, char *argv[])
{ 
	unsigned long int index, hi_word, lo_word;
	unsigned long long value;

	if (argc != 4)
		return EBADNUMARGS;
    
	if (stoli(argv[1], &index) || stoli(argv[2], &hi_word) || stoli(argv[3], &lo_word))
		return EBADNUM;

	value = hi_word;
	value <<= 32;
	value |= lo_word;

	msr_set(index, value);
	value = msr_get(index);
    
	printf("MSR %x: 0x%08x%08x\n",
	       (unsigned int) index,
	       (unsigned int) (value>>32), 
	       (unsigned int) value);

	return 0;
}

#define NMI_STATUS_REG	0x61

int nmi_enable_func(int argc, char *argv[] __attribute__ ((unused)))
{
    uint8_t reg;

    if (argc != 1) return EBADNUMARGS;

    reg = inb(NMI_STATUS_REG);
    reg &= ~(0xc);
    outb(reg, NMI_STATUS_REG);

    printf("Parity Error NMI:  ENABLED\n");
    return 0;
}

int nmi_disable_func(int argc, char *argv[] __attribute__ ((unused)))
{
    uint8_t reg;

    if (argc != 1) return EBADNUMARGS;

    reg = inb(NMI_STATUS_REG);
    reg |= 0xc;
    outb(reg, NMI_STATUS_REG);

    printf("Parity Error NMI:  DISABLED\n");
    return 0;
}

/* Copy the bytes from the long into the string */
static void 
copybytes(unsigned char *str, unsigned long val)
{
	str[4] = '\0';
	str[0] = (unsigned char) (val & 0x0FF);
	str[1] = (unsigned char) (val>>8 & 0x0FF);
	str[2] = (unsigned char) (val>>16 & 0x0FF);
	str[3] = (unsigned char) (val>>24 & 0x0FF);
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
