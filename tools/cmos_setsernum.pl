#!/usr/bin/perl
# Jonathan Mayer
# (c) Cobalt Networks, 1999
# A quick hack to stuff the serial number into CMOS

$_ = shift(@ARGV);

if (!m/^[A-Za-z0-9\-_]{13,13}$/) {
	die ("usage: setsernum.pl 13char_sernum\n  ");
}

@t = split(//,$_);
my $addr = 0x40;
foreach $t (@t) {
	$cmd = "/usr/sbin/cmos $addr ".ord($t);
	#print STDERR "$cmd\n";
	system($cmd);
	$addr++;
}
my $csum = cmos_gen_csum($_);
$cmd = "/usr/sbin/cmos 79 $csum";
#print STDERR "$cmd\n";
system($cmd);
my $err = system("grep -q Alpine /proc/cobalt/systype >& /dev/null");
if ($err) {
	$cmd = "/usr/sbin/cmos 254 $csum"; # backward compat hack.
	system($cmd);
}

# magic algorithm from ROM
sub cmos_gen_csum ($)
{
	my $str = shift;
	my $sum = 0;
	my @key = qw/ c N o E b T a W l O t R ! /;
	my @chars = split(//,$str);
	while (@chars) { $sum += ord(shift(@chars)) ^ ord(shift(@key)); }
	$sum = (($sum & 0x7F) ^ (0xD6)) & 0xFF; 

	return $sum;
}
# LICENSE:
# This software is subject to the terms of the GNU GENERAL 
# PUBLIC LICENSE Version 2, June 1991
