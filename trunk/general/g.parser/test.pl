#!/usr/bin/perl -w
use strict;
# g.parser demo script for perl programming

#%module
#% description: g.parser test script (perl)
#% keyword: keyword1
#% keyword: keyword2
#%end
#%flag
#% key: f
#% description: A flag
#%end
#%option G_OPT_R_MAP
#% key: raster
#% required: yes
#%end
#%option G_OPT_V_MAP
#% key: vector
#%end
#%option
#% key: option1
#% type: string
#% description: An option
#% required: no
#%end

if ( !$ENV{'GISBASE'} ) {
    printf(STDERR  "You must be in GRASS GIS to run this program.\n");
    exit 1;
}

if( $ARGV[0] ne '@ARGS_PARSED@' ){
    my $arg = "";
    for (my $i=0; $i < @ARGV;$i++) {
        $arg .= " $ARGV[$i] ";
    }
    system("$ENV{GISBASE}/bin/g.parser $0 $arg");
    exit;
}

#### add your code here ####

print  "\n";
if ( $ENV{'GIS_FLAG_F'} eq "1" ){
   print "Flag -f set\n"
}
else {
   print "Flag -f not set\n"
}

printf ("Value of GIS_OPT_option1: '%s'\n", $ENV{'GIS_OPT_OPTION1'});
printf ("Value of GIS_OPT_raster: '%s'\n", $ENV{'GIS_OPT_RASTER'});
printf ("Value of GIS_OPT_vect: '%s'\n", $ENV{'GIS_OPT_VECTOR'});

#### end of your code ####

