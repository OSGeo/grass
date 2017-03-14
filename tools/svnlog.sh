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

user=""
compact=0
prefix="revision"
get_user=0
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
  -u, --user USER      search for revisions by USERs separated by a comma
  -c, --compact        print compact log messages; used with --prefix
  -p, --prefix PREFIX  print PREFIX followed by a space and the revision number;
                       print no space if PREFIX is empty; used with --compact
EOT
		exit
	fi
	if [ "$i" = "-u" -o "$i" = "--user" ]; then
		get_user=1
		continue
	fi
	if [ $get_user -eq 1 ]; then
		user=`echo $i | sed 's/^/^(/; s/,/|/g; s/$/)$/'`
		get_user=0
		continue
	fi
	if [ "$i" = "-c" -o "$i" = "--compact" ]; then
		compact=1
		continue
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
svn log $revs |
awk \
	--assign user="$user" \
	--assign compact=$compact \
	--assign prefix="$prefix" \
'BEGIN{
	any = 0
}
/^------------------------------------------------------------------------$/{
	if(compact && rev != ""){
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
	sep = $0
	rev = msg = ""
	next
}
{
	if(started){
		started = 0
		if(user == "" || $3 ~ user){
			any = 1
			skip = 0
			if(compact)
				rev = $1
			else{
				print sep
				print
				print ""
			}
		}else
			skip = 1
		getline
		next
	}else if(!skip){
		if(compact)
			msg = msg $0 "\n"
		else
			print
	}
}
END{
	if(!compact && any)
		print sep
}'
