#!/bin/sh

# Indent source code according to GRASS GIS submitting rules

# Should be in sync with:
#     https://trac.osgeo.org/grass/wiki/Submitting/C
#     https://grasswiki.osgeo.org/wiki/Development#Explanation_of_C_indentation_rules

# Dependencies:
#     indent

# Changes and their reasons:
#    -ts8 -ut -> --no-tabs
#        Do not use 8 space wide tabs when indent level is 4

# TODO: replace short flags by long ones to improve readability


if [ $# -lt 1 ] ; then
 echo "No files specified (give file name(s) as parameter)"
 exit 1
else
 indent -npro -bad -bap -bbb -br -bli0 -bls -cli0 -ncs -fc1 -hnl -i4 \
      -nbbo -nbc -nbfda -nbfde -ncdb -ncdw -nce -nfca -npcs -nprs \
      -npsl -nsc -nsob -saf -sai -saw -sbi0 -ss --no-tabs "$@"

 # fix broken gettext macros:
 grep -l '\<_$' "$@" | \
  while read file ; do sed -i -e '/[( \t]_$/{;N;s/\n[ \t]*//;}' "$file" ; done

 # restore original file with timestamp if indent did not change anything
 for file in "$@" ; do
  cmp "$file"~ "$file" > /dev/null && mv -f "$file"~ "$file"
 done
fi
