/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */


#ifdef THIS_NEVER_WORKED_RIGHT
/* only support controller 0 */
int disk_defects_func(int argc, char *argv[])
{
    unsigned long drive;
    unsigned int count = 0;
    unsigned int max = 0;
    unsigned short temp = 0;
    unsigned int i;

    if (argc != 2) {
	return EBADNUMARGS;
    }
    if (stoli(argv[1], &drive) != 0) {
        return EBADNUM;
    }
    if (drive == 0) {
	drive = 0xa0;
    } else if (drive == 1) {
	drive = 0xb0;
    } else {
	return EBADARG;
    }

    outb_p(drive, IDE0_BASE0 + 6);

    /* magic for READ DEFECT LIST LENGTH */
    outb_p(0x00, IDE0_BASE0 + 2);
    outb_p(0xff, IDE0_BASE0 + 3);
    outb_p(0xff, IDE0_BASE0 + 4);
    outb_p(0x3f, IDE0_BASE0 + 5);
    /* do the read */
    outb_p(0xf0, IDE0_BASE0 + 7);

    /* 
     * this is length in sectors - we convert per the following formula:
     * length_in_sectors = (((max_num_defects) * 8 + 4) + 511)/512
     * or:
     * max_num_defects = (((length_in_sectors * 512) - 511) - 4)/8
     */
    max = inb_p(IDE0_BASE0 + 2) | (inb_p(IDE0_BASE0 + 3)<<8);

    /* now do the read - read the manual for why this is this way... */
    outb_p(0xf0, IDE0_BASE0 + 7);

    temp = inw_p(IDE0_BASE0 + 0);
    /* magic */
    if (temp != 0x1d00) {
	printf("Got bad header word - 0x%x\n", temp);
	return 0;
    }
    printf("max errors: %d (%d sectors)\n", 
	(((max * 512) - 511) - 4)/8, max);

    /* this should be num_errors * 8 but byte swapped */
    count = inw(IDE0_BASE0 + 0);
    count = ((count & 0xff) << 8) | (count >> 8);
    printf("%d errors (C/H/S): \n", count/8);

    for (i = 0; i < count/8; i++) {
	int cyl, head, sect;

    	temp = inw(IDE0_BASE0 + 0);
    	temp = ((temp & 0xff) << 8) | (temp >> 8);
    	cyl = temp<<8;
    	temp = inw(IDE0_BASE0 + 0);
    	temp = ((temp & 0xff) << 8) | (temp >> 8);
    	cyl |= (temp >> 8);

    	head = temp & 0xff;

    	temp = inw(IDE0_BASE0 + 0);
    	temp = ((temp & 0xff) << 8) | (temp >> 8);
    	sect = temp << 16;
    	temp = inw(IDE0_BASE0 + 0);
    	temp = ((temp & 0xff) << 8) | (temp >> 8);
    	sect |= temp;

        printf("  error %d @ %d/%d/%d\n", i, cyl, head, sect);
    }

    return 0;
}
#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
