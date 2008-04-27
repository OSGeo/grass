#!/usr/bin/env perl
my @fonts = ("cyrilc","gothgbt","gothgrt","gothitt","greekc","greekcs","greekp",
	"greeks","italicc","italiccs","italict","romanc","romancs","romand",
	"romans","romant","scriptc","scripts","cyrilc","gothgbtlu");
my @colors = ("red","orange","yellow","green","blue","indigo","violet", "black",
	"gray","brown","magenta","aqua","grey","cyan","purple");

sub rc{
	printf ".X %s\n.Y %s\n", shift, shift;
}
sub xy{
	printf ".X %s%\n.Y %s%\n", shift, shift;
}
sub font{
	printf ".F %s\n", shift;
}
sub size{
	printf ".S %s\n", shift;
}
sub color{
	printf ".C %s\n", shift;
}
sub rotate{
	printf ".R %s\n", shift;
}
sub align{
	printf ".A %s\n", shift;
}
sub text{
	$_ = shift;
	s/^\./../mg;
	s/\n$//;
	print "$_\n";
}

size 4;
for(my $i = 0; $i < 36; $i++){
	font $fonts[$i%$#fonts];
	size ((($i>=9&&$i<18)||$i>27?36-$i:$i)%9);
	rotate $i*10;
	color $colors[$i%$#colors];
	xy 80+10*cos($i*10/180*3.141593), 50+10*640/480*sin($i*10/180*3.141593);
	text ". $fonts[$i%$#fonts]";
}
size 2;
rotate 0;
font "romans";
color "gray";
rc 1, 1;

undef $/;
open FH, "test.pl";
my $src = <FH>;
close FH;

$src =~ s/\n/\n.L 1\n.L 0\n/g;
$src =~ s/(".*?")/\n.C red\n$1\n.C gray\n/g;
$src = ".L 0\n$src";
print $src;
