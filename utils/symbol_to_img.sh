#!/bin/bash

############################################################################
#
# MODULE:       symbol_to_img.sh
# AUTHOR(S):    Anna Petrasova, Hamish Bowman, Vaclav Petras
# PURPOSE:      Renders the GRASS symbols from dir to a dir of PNGs
# SPDX-FileCopyrightText: 2012-2016 Anna Petrasova
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#
#############################################################################

# generates images for gui/images/symbols
# requires ps.map, Inkscape, and ImageMagic

DIR="$(basename $PWD)"
PSMAP_FILE=tmp.psmap
PS_FILE=tmp.ps
PNG_OUT=png_out
POINT_VECTOR=tmp_one_point

v.in.ascii input=- format=standard -n output=$POINT_VECTOR <<EOF
P 1 1
 100 100
 1 1
EOF

rm -r "$PNG_OUT"
mkdir "$PNG_OUT"

for SYMBOL in *
do
    if [ -f "$SYMBOL" ]
    then
        echo -e "border none\npoint 50% 50%\n  symbol $DIR/$SYMBOL\n  end\nend" > "$PSMAP_FILE"
        ps.map input="$PSMAP_FILE" output="$PS_FILE"
        inkscape -f "$PS_FILE" --export-png="$PNG_OUT/$SYMBOL.png" -D -h=30

        rm "$PSMAP_FILE" "$PS_FILE"

        # ImageMagic optimizes PNGs, no optipng needed
        mogrify -gravity center -background transparent -resize 30x30 -extent 30x30 "$PNG_OUT/$SYMBOL.png"
    else
        echo "$SYMBOL is not regular file"
    fi
done

g.remove type=vector name=$POINT_VECTOR -f
