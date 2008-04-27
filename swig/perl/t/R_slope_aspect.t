# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

######################### We start with some black magic to print on failure.

# Change 1..1 below to 1..last_test_to_print .
# (It may become useful if the test is moved to ./t subdirectory.)

BEGIN { $| = 1; }
END {print "not ok 1\n" unless $loaded;}
use Grass;
use POSIX;
$loaded = 1;

sub ok {
    my($test,$msg,$r) = @_;
    $tests_failed = 0 unless defined $tests_failed;
    $test_nr = 1 unless defined $test_nr;

    if (1) {
	print $test ? "ok" : "not ok";
	print " $test_nr - $msg             ";
	print $sub_tests ? "\r" : "\n";
    }

    $sub_tests = 0;
    unless ($test) {
	$tests_failed++;
	push @test_failed,"$test_nr - $msg\n";
    }
    $test_nr++;
    $sub_tests = 1 if $r;
    return $test;
}

sub tests_done {
    $test_nr--;
    print "\n$tests_failed/$test_nr tests failed\n";
    print "failed tests were:\n @test_failed\n" if @test_failed;
}

ok(1,"loaded");

######################### End of black magic.

# Insert your test code below (better if it prints "ok 13"
# (correspondingly "not ok 13") depending on the success of chunk 13
# of the test code):

### Basic tests

Grass::r_slope_aspect(1,['abc']);

