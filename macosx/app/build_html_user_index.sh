#!/bin/sh

#generates user addon HTML man pages docs/html/index.html
# Markus Neteler, 2003, 2004, 2005, 2006

# William Kyngesburye:
#    This one builds the addon index, from both the global
#    /Library/GRASS/$GRASS_MMVER/Modules and the user's
#    $HOME/Library/$GRASS_MMVER/GRASS/Modules
#    Each is in their own section, in the same index file.
# 
#    global help pages are symlinked to the user dir, so user doesn't need perms
#    to write there for redirects.
#    main and section indexes from GRASS.app are redirected from user dir, not
#    symlinked, so relative paths stay valid.

############# nothing to configure below ############

# $1 is current path to GRASS.app/Contents/MacOS, defaults to /Applications
if [ "$1" != "" ] ; then
	GISBASE=$1
else
	GISBASE="/Applications/GRASS-$GRASS_MMVER.app/Contents/MacOS"
fi

GRASS_MMVER=`cut -d . -f 1-2 "$GISBASE/etc/VERSIONNUMBER"`
GRASSVERSION=`cat "$GISBASE/etc/VERSIONNUMBER"`
HTMLDIR="$GISBASE_USER/Modules/docs/html"
HTMLDIRG="$GISBASE_SYSTEM/Modules/docs/html"

write_html_header()
{
# $1: filename
# $2: page title
# $3: is it main index

echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">
<html>
<head>
 <title>$2</title>
 <meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">
 <meta name=\"Author\" content=\"GRASS Development Team\">" > $1
if [ "$3" ] ; then
	echo " <meta name=\"AppleTitle\" content=\"$2\">
 <meta name=\"AppleIcon\" content=\"GRASS-$GRASS_MMVER/grass_icon.png\">
 <meta name=\"robots\" content=\"anchors\">" >> $1
fi
echo " <link rel=\"stylesheet\" href=\"grassdocs.css\" type=\"text/css\">
</head>
<body bgcolor=\"#FFFFFF\">

<img src=\"grass_logo.png\" alt=\"GRASS logo\">
<hr class=\"header\">

<h2>GRASS GIS $GRASSVERSION Reference Manual</h2>

<p>Geographic Resources Analysis Support System, commonly referred to as
<a href=\"http://grass.osgeo.org\">GRASS</a>, 
is a Geographic Information System (GIS) used for geospatial data management
and analysis, image processing, graphics/maps production, spatial modeling,
and visualization. GRASS is currently used in academic and commercial settings
around the world, as well as by many governmental agencies and environmental
consulting companies.</p>

<p>This reference manual details the use of modules distributed with
Geographic Resources Analysis Support System (GRASS), an open source (GNU
GPL'ed), image processing and geographic information system (GIS).</p>

" >> $1
}

write_html_footer()
{
# $1: filename
echo "<hr class=\"header\">" >> $1
echo "<p><a href=\"$GISBASE/docs/html/index.html\">Help Index</a> | <a href=\"$GISBASE/docs/html/full_index.html\">Full Index</a> | <a href=\"$HTMLDIR/addon_index.html\">Addon Index</a><br>" >> $1
echo "&copy; 2003-2008 <a href=\"http://grass.osgeo.org\">GRASS Development Team</a></p>" >> $1
echo "</body>" >> $1   
echo "</html>" >> $1
}

FULLINDEX=addon_index.html

################
echo "Rebuilding Addon HTML manual pages index..."

#copy over CSS:
cp -f "$GISBASE/docs/html/grassdocs.css" "$HTMLDIR/"
#copy over GRASS logo:
cp -f "$GISBASE/docs/html/grass_logo.png" "$HTMLDIR/"
cp -f "$GISBASE/docs/html/grass_icon.png" "$HTMLDIR/"

#process all global HTML pages:
if [ -d "$HTMLDIRG" ] ; then
cd "$HTMLDIRG"

#get list of available GRASS modules:
CMDLISTG=`ls -1 *.*.html 2> /dev/null | grep -v index.html | cut -d'.' -f1 | sort -u`
else
CMDLISTG=""
fi

#process all user HTML pages:
cd "$HTMLDIR"

# don't really need to delete these, as removed global modules won't get indexed,
# though old symlinks will accumulate.  I'm just worried about wildcard deletes.
#rm -f global_*.html

#get list of available GRASS modules:
CMDLIST=`ls -1 *.*.html 2> /dev/null | grep -v index.html | cut -d'.' -f1 | sort -u`

#write main index:
#echo "Generating HTML manual pages index (help system)..."
write_html_header $FULLINDEX "GRASS GIS $GRASSVERSION Addon Reference Manual" 1
echo "<p>Command guide:</p>" >> $FULLINDEX
echo "<table border=0>" >> $FULLINDEX
echo "<tr><td>&nbsp;&nbsp;d.*  </td><td>display commands</td><td>&nbsp;&nbsp;ps.* </td><td>postscript commands</td></tr>" >> $FULLINDEX
echo "<tr><td>&nbsp;&nbsp;db.* </td><td>database commands</td><td>&nbsp;&nbsp;r.*  </td><td>raster commands</td></tr>" >> $FULLINDEX
echo "<tr><td>&nbsp;&nbsp;g.*  </td><td>general commands</td><td>&nbsp;&nbsp;r3.* </td><td>raster3D commands</td></tr>" >> $FULLINDEX
echo "<tr><td>&nbsp;&nbsp;i.*  </td><td>imagery commands</td><td>&nbsp;&nbsp;v.*  </td><td>vector commands</td></tr>" >> $FULLINDEX
echo "<tr><td>&nbsp;&nbsp;m.*  </td><td>miscellaneous commands</td></tr>" >> $FULLINDEX
echo "</table>" >> $FULLINDEX

# global commands:
echo "<h3>Global addon command index:</h3>" >> $FULLINDEX
echo "<table>" >> $FULLINDEX
if [ "$CMDLISTG" = "" ] ; then
  echo "<tr><td valign=\"top\"><td>[There are no global addon help pages.]</td></tr>" >> $FULLINDEX
else
  for i in $CMDLISTG
  do
    cd "$HTMLDIRG"
    CMDLISTI="`ls -1 $i.*.html`"
    cd "$HTMLDIR"
    for i in $CMDLISTI
    do
      BASENAME=`basename $i .html`
      SHORTDESC="`cat "$HTMLDIRG/$i" | awk '/NAME/,/SYNOPSIS/' | grep '<em>' | cut -d'-' -f2- | sed 's+^ ++g' | grep -vi 'SYNOPSIS' | head -n 1`"
#      echo "<tr><td valign=\"top\"><a href=\"$HTMLDIRG/$i\">$BASENAME</a></td> <td>$SHORTDESC</td></tr>" >> $FULLINDEX
      # make them local to user to simplify page links
      echo "<tr><td valign=\"top\"><a href=\"global_$i\">$BASENAME</a></td> <td>$SHORTDESC</td></tr>" >> $FULLINDEX
      ln -sf "$HTMLDIRG/$i" global_$i
    done
  done
fi
echo "</table>" >> $FULLINDEX

# user commands:
echo "<h3>User addon command index:</h3>" >> $FULLINDEX
echo "<table>" >> $FULLINDEX
if [ "$CMDLIST" = "" ] ; then
  echo "<tr><td valign=\"top\"><td>[There are no user addon help pages.]</td></tr>" >> $FULLINDEX
else
  for i in $CMDLIST
  do 
    for i in `ls -1 $i.*.html`
    do
      BASENAME=`basename $i .html`
      SHORTDESC="`cat $i | awk '/NAME/,/SYNOPSIS/' | grep '<em>' | cut -d'-' -f2- | sed 's+^ ++g' | grep -vi 'SYNOPSIS' | head -n 1`"
      echo "<tr><td valign="top"><a href=\"$i\">$BASENAME</a></td> <td>$SHORTDESC</td></tr>" >> $FULLINDEX
    done
  done
fi
echo "</table>" >> $FULLINDEX

write_html_footer $FULLINDEX
# done full index

# user redirects to app dir for main index files

for i in index full_index display database general imagery misc postscript raster raster3D vector
do
echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">
<html>
<head>
 <title></title>
 <meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">
 <meta http-equiv=\"Refresh\" content=\"0; $GISBASE/docs/html/$i.html\">
</head>
<body>
</body>
</html>" > $i.html
done

# add Help Viewer links in user docs folder

mkdir -p $HOME/Library/Documentation/Help/
ln -sfh ../../GRASS/$GRASS_MMVER/Modules/docs/html $HOME/Library/Documentation/Help/GRASS-$GRASS_MMVER-addon
ln -sfh $GISBASE/docs/html $HOME/Library/Documentation/Help/GRASS-$GRASS_MMVER
