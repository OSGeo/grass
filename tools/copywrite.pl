#!/usr/bin/perl

## Script to easily add missing copyright statements in
## GRASS source code files
## 2006, written by Schuyler Erle <schuyler , nocat net>
##
## COPYRIGHT:    (C) 2006 by the GRASS Development Team
##
##               This program is free software under the GNU General Public
##               License (>=v2). Read the file COPYING that comes with GRASS
##               for details.
##
## The script searches and opens sequentially all files
## which lack the word "copyright". Additionally the local
## ChangeLog is fetched and contributor names are extracted.
## Then a new header is inserted containing the GPL statement,
## purpose template and authors.
##
## Please run from top source code directory.
## For usage details, see below.

#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#
#
# edit this, and edit the bottom of the file after __END__

# my $editor	= "vi -o";
#my $editor	= "nedit";
my $editor	= "xemacs";

# don't forget to get a copy of 
#   http://www.red-bean.com/cvs2cl/cvs2cl.pl
# and put it in the same directory as this script
#
# run this script as:
#        perl copywrite.pl contributors.csv
#   committer identities can be provided in a file on the commandline
#   the format of the file should be "username,identity", one per line
#   The contributors.csv is already in the main directory of the GRASS
#   source code, so it should be found.
#
####
# to work offline: 
#   $ export PATH=$PATH:$(pwd)
#   $ find . -name main.c | while read i; do \
#	echo $i; (cd `dirname $i`; cvs2cl.pl -f ChangeLog.tmp main.c); done
#
# to clean up after:
#   $ find -name ChangeLog.tmp | xargs rm
#
# otherwise, this script will assume you are online and fetch
# the ChangeLogs on the fly (and remove them automatically).
#
#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#-=-#

use File::Find;
use File::Basename;
use Cwd;
use strict;
use warnings;
use File::Basename;

my $ChangeLog	= "ChangeLog.tmp";
my $cvs2cl	= getcwd . "/cvs2cl.pl -f $ChangeLog";
my $template	= do {local $/; <DATA>};
my %identity;

sub evaluate_file {
    my $name = do { no warnings; $File::Find::name };

    # only process main.c
    return unless $_ eq 'main.c';

    warn "+ Reading $name\n";

    # slurp main.c
    my $main_c = do {local $/; open F, $_ or die "reading $name: $!"; <F>};

    # continue if main.c is 25 lines or longer
    return unless ($main_c =~ y/\n/\n/) >= 25;

    # continue if main.c doesn't mention "copyright"
    return if $main_c =~ /\(?c\)?opyright/gios;

    # run cvs2cl
    warn "+ Checking CVS log $name\n";
    my $fetched_ChangeLog = 0;
    unless (-f $ChangeLog) {
	system "$cvs2cl $_" and die "$cvs2cl $_: $!";
	$fetched_ChangeLog++;
    }

    # get the list of contributors
    my (%username, %year, $user);
    my $original_author = "";
    {
	# Read whole paras at once
	local $/ = "";
	open LOG, $ChangeLog or die "$ChangeLog: $!";
	while (my $line = <LOG>) {

	    # Hack out the pretty printing
	    $line =~ s/\s+/ /gos;

	    # And drop trailing spaces
	    $line =~ s/\s+$//gos;

	    # Match individual check ins
	    if ($line =~ /^(\d{4})-\d\d-\d\d\s+\d\d:\d\d\s+(.+)$/gos) {
		$year{$1}++;
		$user = $2;
		$username{$user}++;
	    }
	    
	    # Note when a user did the initial check in
	    # if ($line =~ /^\s+\*\s+[^:]+:\s+(?:initial|new$)/gios) {
	    #	$original_author = $user;
	    # }
	    $original_author = $user;

	    # Note when a contributor submitted code
	    if ($line =~ /:\s+([^:<]+\s*<[^>]+>)\s*:/gos) {
		$username{$1}++;
	    }
	}
    }
    
    # don't duplicate the original author
    delete $username{$original_author} if $original_author;
    
    #figure out module name
    my $fullname = $name;
    my $mymodule = basename(dirname($fullname));

    # append the list of contributors
    my @years   = sort map {$_ + 0} keys %year;
    my @authors = sort keys %username;
    
    # map committers to identities
    @authors = map { exists( $identity{$_} ) ? $identity{$_} : $_ } @authors;
    $original_author = $identity{$original_author}
			if exists $identity{$original_author};

    # execute the template
    { 
	local $" = ", ";
	$main_c = eval( 
	    "<<END_OF_TEMPLATE\n" . $template . "\nEND_OF_TEMPLATE" );
    }

    # write the new version of main.c
    warn "+ Rewriting $name\n";
    open F, ">$_" or die "writing $name: $!";
    print F $main_c;
    close F;
    
    # load it up in an editor for vetting
    system "$editor $_ $ChangeLog description.html" and die "$editor $_ $ChangeLog: $!";

    # delete the ChangeLog after *only* if this script created it
    unlink $ChangeLog if $fetched_ChangeLog;
}

# committer identities can be provided in a file on the commandline
# the format of the file should be "username,identity", one per line
#
if ($ARGV[0]) {
    while (<>) {
	chomp;
	next unless /^[a-z]/ios;
	my ($user, $id) = split(",", $_, 2);
	$identity{$user} = $id;
    }
}

find( \&evaluate_file, "." );

### edit the template after __END__
__END__
/****************************************************************************
 *
 * MODULE:       $mymodule
 * AUTHOR(S):    $original_author (original contributor)
 *               @authors
 * PURPOSE:      
 * COPYRIGHT:    (C) $years[0]-$years[-1] by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
$main_c

