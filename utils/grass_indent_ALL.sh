#!/bin/sh

# HANDLE WITH CARE
# loops over entire GRASS source code tree and indents source code according to SUBMITTING rules

#are we in the utils/ dir? We should not.
if [ ! -d utils ] ; then
 echo "ERROR: this script must be run from the main GRASS source code directory" >&2
 exit 1
fi

echo "Indenting *.c and *.h..." >&2
find . -type f -name "*.[ch]" -print0 | xargs -0 ./utils/grass_indent.sh
