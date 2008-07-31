#!/bin/sh

# HANDLE WITH CARE
# loops over entire GRASS source code tree and indents source code according to SUBMITTING rules

#are we in the tools/ dir? We should not.
l$ tools 2> /dev/null
if [ $? -ne 0 ] ; then
 echo "ERROR: this script must be run from the main GRASS source code directory"
 exit 1
fi

echo "Indenting *.c and *.h..."
find . -type f -name "*.[ch]" | xargs ./tools/grass_indent.sh

echo "Indenting *.cpp..."
find . -type f -name "*.cpp" | xargs ./tools/grass_indent.sh

echo "Indenting *.cc..."
find . -type f -name "*.cc" | xargs ./tools/grass_indent.sh

