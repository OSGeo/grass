#!/usr/bin/env perl
if($#ARGV < 1 || $#ARGV == 2 || $#ARGV > 3 || $ARGV[0] =~ m/^-*help$/){
	print "Usage: gridatb.to.arc gridatb_file arc_file [xllcorner yllcorner]\n";
	exit;
}
$xllcorner = 0;
$yllcorner = 0;
if($#ARGV == 3){
	$xllcorner = $ARGV[2];
	$yllcorner = $ARGV[3];
}
open IN, $ARGV[0] or die;
$title = <IN>;
($ncols, $nrows, $cellsize) = (<IN> =~ m/^[ \t]*([0-9.]+)[ \t]+([0-9.]+)[ \t]+([0-9.]+)[ \t]*$/);
open OUT, ">$ARGV[1]" or die;
print OUT <<EOT
ncols         $ncols
nrows         $nrows
xllcorner     $xllcorner
yllcorner     $yllcorner
cellsize      $cellsize
NODATA_value  9999
EOT
;
while(<IN>){
	print OUT;
}
close IN;
close OUT;
