# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl R_slope_aspect.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

BEGIN { $| = 1; }
END {print "not ok 1\n" unless $loaded;}
use Grass;
use R_slope_aspect;
use POSIX;
$loaded = 1;

print "ok\n";

r_slope_aspect::r_slope_aspect();
