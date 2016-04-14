#!/usr/bin/env perl
#
# Parse PCI_ROM and ISA_ROM entries from a source file on stdin and
# output the relevant Makefile variable definitions to stdout
#
# Based upon portions of Ken Yap's genrules.pl

use strict;
use warnings;

die "Syntax: $0 driver_source.c" unless @ARGV == 1;
my $source = shift;
open DRV, "<$source" or die "Could not open $source: $!\n";

( my $family, my $driver_name ) = ( $source =~ /^(.*?([^\/]+))\..$/ )
    or die "Could not parse source file name \"$source\"\n";

my $printed_family;

sub rom {
  ( my $type, my $image, my $desc, my $vendor, my $device, my $dup ) = @_;
  my $ids = $vendor ? "$vendor,$device" : "-";
  unless ( $printed_family ) {
    print "\n";
    print "# NIC\t\n";
    print "# NIC\tfamily\t$family\n";
    print "DRIVERS += $driver_name\n";
    $printed_family = 1;
  }
  print "\n";
  return if ( $vendor && ( ( $vendor eq "ffff" ) || ( $device eq "ffff" ) ) );
  print "# NIC\t$image\t$ids\t$desc\n";
  print "DRIVER_$image = $driver_name\n";
  print "ROM_TYPE_$image = $type\n";
  print "ROM_DESCRIPTION_$image = \"$desc\"\n";
  print "PCI_VENDOR_$image = 0x$vendor\n" if $vendor;
  print "PCI_DEVICE_$image = 0x$device\n" if $device;
  print "ROMS += $image\n" unless $dup;
  print "ROMS_$driver_name += $image\n" unless $dup;
}

while ( <DRV> ) {
  next unless /(PCI|ISA)_ROM\s*\(/;

  if ( /^\s*PCI_ROM\s*\(
         \s*0x([0-9A-Fa-f]{4})\s*, # PCI vendor
         \s*0x([0-9A-Fa-f]{4})\s*, # PCI device
         \s*\"([^\"]*)\"\s*,	   # Image
         \s*\"([^\"]*)\"\s*,	   # Description
         \s*.*\s*		   # Driver data
       \)/x ) {
    ( my $vendor, my $device, my $image, my $desc ) = ( lc $1, lc $2, $3, $4 );
    rom ( "pci", lc "${vendor}${device}", $desc, $vendor, $device );
    rom ( "pci", $image, $desc, $vendor, $device, 1 );
  } elsif ( /^\s*ISA_ROM\s*\(
	      \s*\"([^\"]*)\"\s*,  # Image
	      \s*\"([^\"]*)\"\s*   # Description
	    \)/x ) {
    ( my $image, my $desc ) = ( $1, $2 );
    rom ( "isa", $image, $desc );
  } else {
    warn "Malformed PCI_ROM or ISA_ROM macro on line $. of $source\n";
  }
}

close DRV;
