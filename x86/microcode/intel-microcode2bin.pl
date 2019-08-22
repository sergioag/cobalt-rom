#!/usr/bin/perl

while (<>) {
    next unless /(0x[0-9a-f]{8}),\s+(0x[0-9a-f]{8}),\s+(0x[0-9a-f]{8}),\s+(0x[0-9a-f]{8})/;
    print pack("LLLL", hex($1), hex($2), hex($3), hex($4));
}
# LICENSE:
# This software is subject to the terms of the GNU GENERAL 
# PUBLIC LICENSE Version 2, June 1991
