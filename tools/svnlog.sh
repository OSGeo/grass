#!/bin/sh

############################################################################
#
# MODULE:       Create a combined log message for all or specified SVN revisions
# AUTHOR(S):    Huidae Cho <grass4u gmail.com>
# PURPOSE:      Create a log message for backporting and release news
# COPYRIGHT:    (C) 2017 by Huidae Cho
#                and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

# collect revision numbers from command line arguments, if any
# e.g., r70666, r70637 => -r70666 -r70637
revs=""
for i; do
	# remove any non-numeric characters
	r=`echo $i | sed 's/[^0-9]//g'`
	if [ "$r" = "" ]; then
		continue
	fi
	# prepare svn log options
	revs="$revs -r$r"
done

svn log $revs | awk '
/^------------------------------------------------------------------------$/{
	if(msg != ""){
		gsub(/\n+$/, "", msg)
		split(msg, lines, /\n/)
		printf "%s", msg
		if(length(lines) == 1)
			printf " (backport %s)\n", rev
		else
			# add a separator for multiple lines
			printf "\n(backport %s)\n%s\n", rev, $0
	}
	started = 1
	msg = ""
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
