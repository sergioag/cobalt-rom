/*
 *
 * Filename: bootdevs.c
 *
 * Description: Functions/data for bootdev<->name translations
 *
 * Author(s): Tim Hockin
 *
 * Copyright 1999 Cobalt Networks, Inc.
 *
 */

#include "common/cmos.h"

#include <string.h>
#include <stdlib.h>

/* list of all bootable devices - yes we need ALL these */
struct boot_dev_t boot_devs[] = {
    {"hda1",  3, 1},  {"hda2",  3, 2},  {"hda3",  3, 3},  {"hda4",  3, 4},
    {"hda5",  3, 5},  {"hda6",  3, 6},  {"hda7",  3, 7},  {"hda8",  3, 8},

    {"hdb1",  3, 65}, {"hdb2",  3, 66}, {"hdb3",  3, 67}, {"hdb4",  3, 68},
    {"hdb5",  3, 69}, {"hdb6",  3, 70}, {"hdb7",  3, 71}, {"hdb8",  3, 72},

    {"hdc1",  22, 1},  {"hdc2",  22, 2},  {"hdc3",  22, 3},  {"hdc4",  22, 4},
    {"hdc5",  22, 5},  {"hdc6",  22, 6},  {"hdc7",  22, 7},  {"hdc8",  22, 8},

    {"hdd1",  22, 65}, {"hdd2",  22, 66}, {"hdd3",  22, 67}, {"hdd4",  22, 68},
    {"hdd5",  22, 69}, {"hdd6",  22, 70}, {"hdd7",  22, 71}, {"hdd8",  22, 72},

    {"hde1",  33, 1}, {"hde2",  33, 2}, {"hde3",  33, 3}, {"hde4",  33, 4},
    {"hde5",  33, 5}, {"hde6",  33, 6}, {"hde7",  33, 7}, {"hde8",  33, 8},

    {"hdf1",  33, 65}, {"hdf2",  33, 66}, {"hdf3",  33, 67}, {"hdf4",  33, 68},
    {"hdf5",  33, 69}, {"hdf6",  33, 70}, {"hdf7",  33, 71}, {"hdf8",  33, 72},

    {"hdg1",  34, 1}, {"hdg2",  34, 2}, {"hdg3",  34, 3}, {"hdg4",  34, 4},
    {"hdg5",  34, 5}, {"hdg6",  34, 6}, {"hdg7",  34, 7}, {"hdg8",  34, 8},

    {"hdh1",  34, 65}, {"hdh2",  34, 66}, {"hdh3",  34, 67}, {"hdh4",  34, 68},
    {"hdh5",  34, 69}, {"hdh6",  34, 70}, {"hdh7",  34, 71}, {"hdh8",  34, 72},

    {"hdi1",  56, 1}, {"hdi2",  56, 2}, {"hdi3",  56, 3}, {"hdi4",  56, 4},
    {"hdi5",  56, 5}, {"hdi6",  56, 6}, {"hdi7",  56, 7}, {"hdi8",  56, 8},

    {"hdj1",  56, 65}, {"hdj2",  56, 66}, {"hdj3",  56, 67}, {"hdj4",  56, 68},
    {"hdj5",  56, 69}, {"hdj6",  56, 70}, {"hdj7",  56, 71}, {"hdj8",  56, 72},

    {"hdk1",  57, 1}, {"hdk2",  57, 2}, {"hdk3",  57, 3}, {"hdk4",  57, 4},
    {"hdk5",  57, 5}, {"hdk6",  57, 6}, {"hdk7",  57, 7}, {"hdk8",  57, 8},

    {"hdl1",  57, 65}, {"hdl2",  57, 66}, {"hdl3",  57, 67}, {"hdl4",  57, 68},
    {"hdl5",  57, 69}, {"hdl6",  57, 70}, {"hdl7",  57, 71}, {"hdl8",  57, 72},

    {"sda1",  8, 1},   {"sda2",  8, 2},  {"sda3",  8, 3},  {"sda4",  8, 4},
    {"sda5",  8, 5},   {"sda6",  8, 6},  {"sda7",  8, 7},  {"sda8",  8, 8},
    {"sda9",  8, 9},   {"sda10", 8, 10}, {"sda11", 8, 11}, {"sda12", 8, 12},
    {"sda13", 8, 13},  {"sda14", 8, 14}, {"sda15", 8, 15}, {"sda16", 8, 16},

    {"sdb1",  8, 17},  {"sdb2",  8, 18}, {"sdb3",  8, 19}, {"sdb4",  8, 20},
    {"sdb5",  8, 21},  {"sdb6",  8, 22}, {"sdb7",  8, 23}, {"sdb8",  8, 24},
    {"sdb9",  8, 25},  {"sdb10", 8, 26}, {"sdb11", 8, 27}, {"sdb12", 8, 28},
    {"sdb13", 8, 29},  {"sdb14", 8, 30}, {"sdb15", 8, 31}, {"sdb16", 8, 32},

    {"sdc1",  8, 33},  {"sdc2",  8, 34}, {"sdc3",  8, 35}, {"sdc4",  8, 36},
    {"sdc5",  8, 37},  {"sdc6",  8, 38}, {"sdc7",  8, 39}, {"sdc8",  8, 40},
    {"sdc9",  8, 41},  {"sdc10", 8, 42}, {"sdc11", 8, 43}, {"sdc12", 8, 44},
    {"sdc13", 8, 45},  {"sdc14", 8, 46}, {"sdc15", 8, 47}, {"sdc16", 8, 48},

    {"sdd1",  8, 49},  {"sdd2",  8, 50}, {"sdd3",  8, 51}, {"sdd4",  8, 52},
    {"sdd5",  8, 53},  {"sdd6",  8, 54}, {"sdd7",  8, 55}, {"sdd8",  8, 56},
    {"sdd9",  8, 57},  {"sdd10", 8, 58}, {"sdd11", 8, 59}, {"sdd12", 8, 60},
    {"sdd13", 8, 61},  {"sdd14", 8, 62}, {"sdd15", 8, 63}, {"sdd16", 8, 64},

    {"sde1",  8, 65},  {"sde2",  8, 66}, {"sde3",  8, 67}, {"sde4",  8, 68},
    {"sde5",  8, 69},  {"sde6",  8, 70}, {"sde7",  8, 71}, {"sde8",  8, 72},
    {"sde9",  8, 73},  {"sde10", 8, 74}, {"sde11", 8, 75}, {"sde12", 8, 76},
    {"sde13", 8, 77},  {"sde14", 8, 78}, {"sde15", 8, 79}, {"sde16", 8, 80},

    {"sdf1",  8, 81},  {"sdf2",  8, 82}, {"sdf3",  8, 83}, {"sdf4",  8, 84},
    {"sdf5",  8, 85},  {"sdf6",  8, 86}, {"sdf7",  8, 87}, {"sdf8",  8, 88},
    {"sdf9",  8, 89},  {"sdf10", 8, 90}, {"sdf11", 8, 91}, {"sdf12", 8, 92},
    {"sdf13", 8, 93},  {"sdf14", 8, 94}, {"sdf15", 8, 95}, {"sdf16", 8, 96},

    {"sdg1",  8, 97},  {"sdg2",  8, 98}, {"sdg3",  8, 99}, {"sdg4",  8, 100},
    {"sdg5",  8, 101}, {"sdg6",  8, 102},{"sdg7",  8, 103},{"sdg8",  8, 104},
    {"sdg9",  8, 105}, {"sdg10", 8, 106},{"sdg11", 8, 107},{"sdg12", 8, 108},
    {"sdg13", 8, 109}, {"sdg14", 8, 110},{"sdg15", 8, 111},{"sdg16", 8, 112},

    {"scd0",  11, 0},  
    {"scd1",  11, 1},

    {"md0",  9, 0}, {"md1",  9, 1},
    {"md2",  9, 2}, {"md3",  9, 3},
    {"md4",  9, 4}, {"md5",  9, 5},

    {NULL, 0, 0}
};

struct boot_dev_t *
find_dev_by_name(char *name)
{
	int i = 0;

	while (boot_devs[i].name) {
		if (!strcmp(name, boot_devs[i].name)) {
			return &boot_devs[i];
		}
		i++;
	}

	return NULL;
}

struct boot_dev_t *
find_dev_by_num(unsigned short dev)
{
	int i = 0;
	unsigned char maj;
	unsigned char min;

	maj = (dev >> 8) & 0xff;
	min = dev & 0xff;

	while (boot_devs[i].name) {
		if (boot_devs[i].maj == maj && boot_devs[i].min == min) {
			return &boot_devs[i];
		}
		i++;
	}

	return NULL;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
