#!/bin/sh

############################################################################
#
# MODULE:       mkhtml.sh
# AUTHOR(S):    Markus Neteler, Glynn Clements
# PURPOSE:      create HTML manual page snippets
# COPYRIGHT:    (C) 2007 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

PGM=$1

if ! grep -i '<html>' ${PGM}.tmp.html > /dev/null 2>&1 ; then
    echo > ${PGM}.tmp.html
fi

if test -f ${PGM}.html ; then
    cat ${PGM}.html >> ${PGM}.tmp.html
elif test -f description.html ; then
    cat description.html >> ${PGM}.tmp.html
fi

if ! grep -i '<html>' ${PGM}.tmp.html > /dev/null ; then
    cat > ${PGM}.tmp.html.header <<-EOF
	<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
	<html>
	<head>
	<title>GRASS GIS: ${PGM}</title>
	<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
	<link rel="stylesheet" href="grassdocs.css" type="text/css">
	</head>
	<body bgcolor="white">
	<img src="grass_logo.png" alt="GRASS logo"><hr align=center size=6 noshade>
	<h2>NAME</h2>
	<em><b>${PGM}</b></em>
	EOF
    grep -iv '</body>\|</html>' ${PGM}.tmp.html >> ${PGM}.tmp.html.header
    mv -f ${PGM}.tmp.html.header ${PGM}.tmp.html
fi

# if </html> is found, suppose a complete html is provided.
# otherwise, generate module class reference:
if ! grep -i '</html>' ${PGM}.tmp.html > /dev/null ; then
    MODCLASS=`echo ${PGM} | cut -d'.' -f1`
    case $MODCLASS in
	d)  INDEXNAME=display	;;
	db) INDEXNAME=database	;;
	g)  INDEXNAME=general	;;
	i)  INDEXNAME=imagery	;;
	m)  INDEXNAME=misc	;;
	pg) INDEXNAME=postGRASS	;;
	ps) INDEXNAME=postscript ;;
	p)  INDEXNAME=paint	;;
	r)  INDEXNAME=raster	;;
	r3) INDEXNAME=raster3D	;;
	s)  INDEXNAME=sites	;;
	v)  INDEXNAME=vector	;;
	*)  INDEXNAME=$MODCLASS	;;
    esac
    cat >> ${PGM}.tmp.html <<-EOF
	<HR>
	<P><a href="index.html">Main index</a> - <a href="$INDEXNAME.html">$INDEXNAME index</a> - <a href="full_index.html">Full index</a></P>
	<P>&copy; 2003-2008 <a href="http://grass.osgeo.org">GRASS Development Team</a></p>
	</body>
	</html>
	EOF
fi
