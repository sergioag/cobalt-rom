#!/usr/bin/perl -w
# quickie to parse in a logo file, and write it to a ROM image

use strict;
use POSIX;

my $romfile = shift || usage();
my $logofile = shift || usage();
my $offset = shift || usage();
my @text;
my @font;
my $line = 0;
my $maxfont=0;
open(LOGO, "< $logofile") || die "can't open $logofile";
while (defined($_ = <LOGO>)) {
	chomp($_);
	$line++;

	# skip comments/blank lines
	if ($_ =~ /^\s*(#.*)?$/) {
		next;
	}
	
	# test valid directives 
	if ($_ =~ /^\s*line(\d)\s+"(.*)"\s*$/i) {
		$text[$1-1] = $2;
	} elsif ($_ =~ /^\s*font(\d{1,2})\s+(.*)\s*$/i) {
		$font[$1-1] = $2;
		if( $1 > $maxfont ) {
		  $maxfont = $1;
		}
	} else {
		die "syntax error at $logofile:$line\n";
	}
}
close(LOGO);

# dump the data into the ROM image at the right place
sysopen(ROM, $romfile, O_RDWR) || die "can't sysopen $romfile";
sysseek(ROM, oct($offset), SEEK_SET) || die "can't sysseek";

# fist strings (with trailing NUL)
my $i;
for ($i = 0; $i < 2; $i++) {
	syswrite(ROM, "$text[$i]            ", 12);
	syswrite(ROM, chr(0), 1);
}

# then font data
for ($i = 0; $i < $maxfont; $i++) {
	my (@bytes) = split(/\s+/, $font[$i]);
	my $j;
	for ($j = 0; $j < 8; $j++) {
		syswrite(ROM, chr(oct($bytes[$j])), 1);
	}
}

exit(0);
#end


sub usage
{
	print STDERR "usage: romsetlogo <romfile> <logofile> <logo offset>\n";
	exit 1;
}
# LICENSE:
# This software is subject to the terms of the GNU GENERAL 
# PUBLIC LICENSE Version 2, June 1991
