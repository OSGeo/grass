#!/bin/sh

############################################################################
#
# MODULE:       svnlog.sh
# AUTHOR(S):    Huidae Cho <grass4u gmail.com>
# PURPOSE:      Create a combined log message from all or specified SVN
#		revisions for backporting and release news
# COPYRIGHT:    (C) 2017 by Huidae Cho
#               and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

prefix="revision"
get_prefix=0

# collect revision numbers from command line arguments, if any
# e.g., r70666, r70637 => -r70666 -r70637
revs=""
for i; do
	if [ "$i" = "-h" -o "$i" = "--help" ]; then
		cat<<EOT
Usage: svnlog.sh [OPTION]... [REVISION]...

Options:
  -h, --help           print this help
  -p, --prefix PREFIX  print PREFIX followed by a space and the revision number;
                       print no space if PREFIX is empty
EOT
		exit
	fi
	if [ "$i" = "-p" -o "$i" = "--prefix" ]; then
		get_prefix=1
		continue
	fi
	if [ $get_prefix -eq 1 ]; then
		prefix=`echo $i`
		get_prefix=0
		continue
	fi
	# remove any non-numeric characters
	r=`echo $i | sed 's/[^0-9]//g'`
	if [ "$r" = "" ]; then
		continue
	fi
	# prepare svn log options
	revs="$revs -r$r"
done

if [ "$prefix" != "" ]; then
	prefix="$prefix "
fi

# retreive and combine log messages
svn log $revs | awk --assign prefix="$prefix" '
/^------------------------------------------------------------------------$/{
	if(rev != ""){
		gsub(/^[ \t\n]+|[ \t\n]+$/, "", msg)
		nlines = split(msg, lines, /\n/)
		if(nlines == 0)
			printf "No log message (" prefix "%s)\n", rev
		else if(nlines == 1)
			printf "%s (" prefix "%s)\n", msg, rev
		else{
			# add separators for multiple lines
			if(!prev_multi)
				printf "%s\n", $0
			printf "%s\n(" prefix "%s)\n%s\n", msg, rev, $0
		}
		prev_multi = nlines > 1
	}
	started = 1
	rev = msg = ""
	next
}
{
	if(started){
		rev = $1
		started = 0
		getline
		next
	}
	msg = msg $0 "\n"
}'
