#!/usr/bin/perl -w 
#$Id: makerom.pl,v 1.3 2003/12/11 00:55:04 thockin Exp $
# script to take a spec file and input files and build a ROM image
# Assumptions:
#	ramcode may load another ELF at RAM_MONITOR_ADDR
#	blank space is filled with binary 0xff
use strict;
my( $fpos );
#
# get settings from ROM.conf
#
open(SET, "< ROM.conf") || die "$!";
my %settings;
while (defined($_ = <SET>)) {
	if (/^\s*#/) {
		# comment - skip it
		next;
	}
	my @kv = split(/\s*=\s*/, $_);
	chomp($kv[1]);
	$settings{$kv[0]} = eval($kv[1]);
}
close(SET);

# source the specification file
my $spec = require $ARGV[0];

print "ROM: ", $$spec{NAME}, "\n";

# create the output file. write in chunks.
my $chunksize = 32768;
my $ff = chr(0xff) x $chunksize;

open(ROM, "> $$spec{NAME}") || die "$!";
$fpos = 0x0;
# Start by filling the whole rom
seek( ROM, 0, 0 );
while ($fpos < $spec->{SIZE}) {
	print ROM $ff;
	$fpos += $chunksize;
}

#
# load the boot segment (romcode + ramcode )
#
seek( ROM, $spec->{BOOT_START}, 0 );
$fpos = $spec->{BOOT_START};
open(BOOT, "x86/cobalt-x86.rom") || die "$!";
my ($len, $data);
my $lenttl = 0;
while ($len = read(BOOT, $data, 1024)) {
    $lenttl += $len;
    if ($lenttl > $settings{ROM_PAGE_SIZE}) {
	die "Boot segment is larger than 1 page\n\n";
    }
    $fpos += $len;
    print ROM $data;
}
close(BOOT);


# build the cobalt_romfs filesystem
unlink "/tmp/$$spec{NAME}.fs";
my $fssize = $spec->{FS_SIZE};
qx( cobalt_romfs/co_mkfs /tmp/$$spec{NAME}.fs $fssize );

# write all the requested files to the fs
my ($file, $files);
$files = $$spec{FS_FILES};
foreach $file (@$files) {
    if (-f $$file[1]) {
        printf("     adding file '%s' as '%s' (%d bytes)\n",
               $$file[1], $$file[0], (stat($$file[1]))[7]);
        if( system( "cobalt_romfs/co_cpto /tmp/$$spec{NAME}.fs $$file[1] $$file[0]") != 0 )
	{
	    die "Error copying $$file[1]\n\n";
	}
    } else {
        printf("     ERROR: couldn't find file '%s'\n", $$file[1]);
    }
}

# load the filesystem image
seek( ROM, $spec->{FS_START}, 0 );
$fpos = $spec->{FS_START};
open(FS, "/tmp/$$spec{NAME}.fs") || die "$!";
while ($len = read(FS, $data, 1024)) {
    if ($len >= $fssize) {
	die "CRFS segment are larger than ",
            $spec->{FS_SIZE}/1024, "kb\n\n";
    }
    $fpos += $len;
    print ROM $data;
}
close(FS);

# clean up filesystem
unlink "/tmp/$$spec{NAME}.fs";

close(ROM);

#
# checksum it, and insert checksum value
# removed
#
#print "     checksumming...\n";
#system("./tools/romsetcsum $$spec{NAME} 0 $settings{ROM_CSUM_OFFSET}");
#my $csum = `./tools/romcsum $$spec{NAME}`;
#chomp($csum);
#system("./tools/romsetcsum $$spec{NAME} $csum $settings{ROM_CSUM_OFFSET}");

#
# done
#
print "     $$spec{NAME} created (", (stat($$spec{NAME}))[7], " bytes)\n\n";

# LICENSE:
# This software is subject to the terms of the GNU GENERAL 
# PUBLIC LICENSE Version 2, June 1991
