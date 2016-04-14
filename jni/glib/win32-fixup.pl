#!/usr/bin/perl

$major = 1;
$minor = 3;
$micro = 7;
$binary_age = 0;
$interface_age = 0;
$gettext_package = "glib20";
$current_minus_age = 0;

sub process_file
{
        my $outfilename = shift;
	my $infilename = $outfilename . ".in";
	
	open (INPUT, "< $infilename") || exit 1;
	open (OUTPUT, "> $outfilename") || exit 1;
	
	while (<INPUT>) {
	    s/\@GLIB_MAJOR_VERSION\@/$major/g;
	    s/\@GLIB_MINOR_VERSION\@/$minor/g;
	    s/\@GLIB_MICRO_VERSION\@/$micro/g;
	    s/\@GLIB_INTERFACE_AGE\@/$interface_age/g;
	    s/\@GLIB_BINARY_AGE\@/$binary_age/g;
	    s/\@GETTEXT_PACKAGE\@/$gettext_package/g;
	    s/\@LT_CURRENT_MINUS_AGE@/$current_minus_age/g;
	    print OUTPUT;
	}
}

process_file ("config.h.win32");
process_file ("glibconfig.h.win32");
process_file ("glib/makefile.msc");
process_file ("glib/glib.rc");
process_file ("gmodule/makefile.msc");
process_file ("gmodule/gmodule.rc");
process_file ("gobject/makefile.msc");
process_file ("gobject/gobject.rc");
process_file ("gthread/makefile.msc");
process_file ("gthread/gthread.rc");
process_file ("tests/makefile.msc");
