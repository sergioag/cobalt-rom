#!/usr/bin/perl

my $offset = $ARGV[0];
my $text;

while ( <STDIN> )
  {
    chop;
    $text .= $_;
  }

$text =~ /\{(.*)\}/;
my $data = $1;
$data =~ s/\"//g;
my @vals = split( /,/, $data );
my ($width, $height, $colors, $cpc) = split(/\s+/, @vals[0]);

if( $width < 23 )
  {
    print "Width is less than 23\n";
    die;
  }

if( $height < 18 )
  {
    print "height is less than 18\n";
    die;
  }

if( $cpc != 1 )
  {
    print "more than one charater per color is not supported\n";
    die;
  }

my $black_char = 0;
foreach $color ( 1 .. $colors )
  {
    $char = substr( @vals[$color], 0, 1);
    ($foo, $foo, $val) = split( /\s+/, substr( @vals[$color], 1) );
    if( $val eq "#000000" )
      {
	$black_char = $char;
      }
  }

if( ! $black_char )
  {
    print "can't find black pixel\n";
    die;
  }

foreach $row ( 0 .. 1 )
  {
    foreach $col (0 .. 3)
      {
	print_font( \@vals, $offset, $row * 4 + $col + 1, $row * 10, $col * 6 );
      }
  }

sub print_font{
  my $vals = shift;
  my $offset = shift;
  my $start = shift;
  my $startr = shift;
  my $startc = shift;

  print "font", $offset+$start;

  foreach $i ( 0+$startr .. 7+$startr )
    {
      $val = 0;
      foreach $j ( 0+$startc .. 4+$startc )
      {
	$val <<= 1;
	if( substr( $vals->[$i+3], $j, 1 ) eq $black_char )
	  {
	    $val += 1;
	  }
      }
      printf( " 0x%02x", $val );
    }
  print "\n";
}
# LICENSE:
# This software is subject to the terms of the GNU GENERAL 
# PUBLIC LICENSE Version 2, June 1991
