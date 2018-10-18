#!/bin/sh
# -*- coding: utf-8 -*-
#
# Derived for GRASS GIS from
# https://trac.osgeo.org/gdal/browser/trunk/gdal/scripts/fix_typos.sh
#
# Run in main source code directory of GRASS GIS:
# sh tools/fix_typos.sh
#
#
###############################################################################
# $Id$
#
#  Project:  GDAL
#  Purpose:  (Interactive) script to identify and fix typos
#  Author:   Even Rouault <even.rouault at spatialys.com>
#
###############################################################################
#  Copyright (c) 2016, Even Rouault <even.rouault at spatialys.com>
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
###############################################################################

if ! test -d fix_typos; then
    # Get our fork of codespell that adds --words-white-list and full filename support for -S option
    mkdir fix_typos
    cd fix_typos
    git clone https://github.com/rouault/codespell
    cd codespell
    git checkout gdal_improvements
    cd ..
    # Aggregate base dictionary + QGIS one + Debian Lintian one
    curl https://raw.githubusercontent.com/qgis/QGIS/master/scripts/spelling.dat | sed "s/:/->/" | grep -v "colour->" | grep -v "colours->" > qgis.txt
    curl https://anonscm.debian.org/cgit/lintian/lintian.git/plain/data/spelling/corrections| grep "||" | grep -v "#" | sed "s/||/->/" > debian.txt
    cat codespell/data/dictionary.txt qgis.txt debian.txt | awk 'NF' > grassgis_dict.txt
    echo "difered->deferred" >> grassgis_dict.txt
    echo "differed->deferred" >> grassgis_dict.txt
    cd ..
fi

EXCLUDED_FILES="*/.svn*,configure,config.status,config.sub,*/autom4te.cache/*"
EXCLUDED_FILES="$EXCLUDED_FILES,*/lib/cdhc/doc/goodness.ps,*/lib/cdhc/doc/goodness.tex,*/macosx/pkg/resources/ReadMe.rtf"
EXCLUDED_FILES="$EXCLUDED_FILES,*/lib/gis/FIPS.code,*/lib/gis/projection,*/lib/proj/parms.table,*/lib/proj/units.table,*/lib/proj/desc.table"
EXCLUDED_FILES="$EXCLUDED_FILES,*/locale/po/*.po"
EXCLUDED_FILES="$EXCLUDED_FILES,*/fix_typos/*,fix_typos.sh,*.eps,geopackage_aspatial.html"
EXCLUDED_FILES="$EXCLUDED_FILES,PROVENANCE.TXT,libtool,ltmain.sh,libtool.m4"
WORDS_WHITE_LIST="poSession,FIDN,TRAFIC,HTINK,repID,oCurr,INTREST,oPosition"
WORDS_WHITE_LIST="$WORDS_WHITE_LIST,CPL_SUPRESS_CPLUSPLUS,SRP_NAM,ADRG_NAM,'SRP_NAM,AuxilaryTarget"
# libtiff
WORDS_WHITE_LIST="$WORDS_WHITE_LIST,THRESHHOLD_BILEVEL,THRESHHOLD_HALFTONE,THRESHHOLD_ERRORDIFFUSE"
# GRASS GIS
WORDS_WHITE_LIST="$WORDS_WHITE_LIST,thru"

MYPATH=`pwd`

touch $MYPATH/fix_typos/typos_whitelist.txt
python3 $MYPATH/fix_typos/codespell/codespell.py -w -i 3 -q 2 -S $EXCLUDED_FILES \
       -x $MYPATH/fix_typos/typos_whitelist.txt --words-white-list=$WORDS_WHITE_LIST \
       -D $MYPATH/fix_typos/grassgis_dict.txt  .

