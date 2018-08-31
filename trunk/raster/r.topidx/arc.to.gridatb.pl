#!/usr/bin/env perl
if($#ARGV != 1 || $ARGV[0] =~ m/^-*help$/){
	print "Usage: arc.to.gridatb arc_file gridatb_file\n";
	exit;
}
open IN, $ARGV[0] or die;
$head = 0;
while(<IN>){
	if(m/^ncols[ \t]+([0-9.]+)[ \t]*$/i){
		$ncols = $1;
		$head |= 0x1;
	}elsif(m/^nrows[ \t]+([0-9.]+)[ \t]*$/i){
		$nrows = $1;
		$head |= 0x2;
	}elsif(m/^xllcorner[ \t]+([0-9.]+)[ \t]*$/i){
		$xllcorner = $1;
		$head |= 0x4;
	}elsif(m/^yllcorner[ \t]+([0-9.]+)[ \t]*$/i){
		$yllcorner = $1;
		$head |= 0x8;
	}elsif(m/^cellsize[ \t]+([0-9.]+)[ \t]*$/i){
		$cellsize = $1;
		$head |= 0x10;
	}elsif(m/^nodata_value[ \t]+([0-9.]+)[ \t]*$/i){
		$nodata_value = $1;
		$head |= 0x20;
	}else{
		die;
	}
	if($head == 0x3f){
		last;
	}
}
open OUT, ">$ARGV[1]" or die;
print OUT <<EOT
arc.to.gridatb $ARGV[0] $ARGV[1]
$ncols $nrows $cellsize
EOT
;
while(<IN>){
	s/(?=^|[ \t]*)$nodata_value(\.0*)?(?=([ \t]*|$))/9999.00/g;
	print OUT;
}
close IN;
close OUT;
