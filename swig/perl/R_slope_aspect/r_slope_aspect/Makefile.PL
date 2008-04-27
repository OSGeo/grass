use ExtUtils::MakeMaker;
$Verbose = 1;
$ranlib = 'ranlib';
$fopt = '-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -Wall -fpic -ggdb -DPACKAGE=\""grassmods"\"';

@incdirs = ('/usr/include',"$grass_inc_dir");
@c_files = ('r_slope_aspect.c','opennew.c');

$incs = '';
foreach (@incdirs) {
    $incs .= " -I$_";
}
$co_files = $c_files = join(' ',@c_files);
$co_files =~ s/\.c/.o/g;

WriteMakefile(
	      NAME      => 'R_slope_aspect::r_slope_aspect',
	      INC	=> $incs,
	      DEFINE    => $fopt,
	      clean     => {'FILES' => 'libr_slope_aspect$(LIB_EXT)'},
	      );

sub MY::top_targets {
'
pure_all ::

all :: static

static :: libr_slope_aspect$(LIB_EXT)

libr_slope_aspect$(LIB_EXT): ' . $c_files . ' ' . $co_files . '
	$(AR) cr libr_slope_aspect$(LIB_EXT) ' . $co_files . '
	' . $ranlib . ' libr_slope_aspect$(LIB_EXT)
';
}
