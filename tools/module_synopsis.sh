#!/bin/sh
############################################################################
#
# TOOL:         module_synopsis.sh
# AUTHOR:       M. Hamish Bowman, Dept. Marine Science, Otago Univeristy,
#                 New Zealand
# PURPOSE:      Runs through GRASS modules and creates a synopsis list of
#		  module names and descriptions.
# COPYRIGHT:    (c) 2007 Hamish Bowman, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################
#
# PDF output requires the Palatino font.
# Run this script from the tools/ directory in the souce code.
#   (TeX needs to be able to find grasslogo_vector.pdf)
#

if  [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program." 1>&2
    exit 1
fi

for FILE in txt tex ; do
    if [ -e "$GISBASE/etc/module_synopsis.$FILE" ] ; then
    #  echo "ERROR: module_synopsis.$FILE already exists" 1>&2
    #  exit 1
       #blank it
       g.message -w "Overwriting \"\$GISBASE/etc/module_synopsis.$FILE\""
       \rm "$GISBASE/etc/module_synopsis.$FILE"
    fi
done
for FILE in html pdf ; do
    if [ -e "$GISBASE/docs/$FILE/module_synopsis.$FILE" ] ; then
    #  echo "ERROR: module_synopsis.$FILE already exists" 1>&2
    #  exit 1
       #blank it
       g.message -w "Overwriting \"\$GISBASE/docs/$FILE/module_synopsis.$FILE\""
       \rm "$GISBASE/docs/$FILE/module_synopsis.$FILE"
    fi
done


TMP="`g.tempfile pid=$$`"
if [ $? -ne 0 ] || [ -z "$TMP" ] ; then
    g.message -e "Unable to create temporary files" 
    exit 1
fi


g.message "Generating module synopsis (writing to \$GISBASE/etc/) ..."

SYNOP="$GISBASE/etc/module_synopsis.txt"

OLDDIR="`pwd`"
cd "$GISBASE"


### generate menu hierarchy

#### fixme: no longer exists
MDPY="$GISBASE/etc/wxpython/gui_modules/menudata.py"

# python menudata.py commands
# python menudata.py tree
# python menudata.py strings
python "$MDPY" commands | sed -e 's/ | /|/' -e 's/[ -].*|/|/' \
  | sort -u > "$TMP.menu_hierarchy"

# for running with GRASS 6.4 after generating with GRASS 6.5.
#\cp "$TMP.menu_hierarchy" "$GISBASE/etc/gui/menu_hierarchy.txt"
#\cp "$GISBASE/etc/gui/menu_hierarchy.txt" "$TMP.menu_hierarchy"

### given a module name return where it is in the menu tree
find_menu_hierarchy()
{
  MODL=$1

  # unwrap wrapper scripts
  if [ "$MODL" = "g.gui" ] ; then
    MODL="g.change.gui.py"
  elif  [ "$MODL" = "v.type" ] ; then
    MODL="v.type_wrapper.py"
  fi

  PLACEMENT=`grep "^$MODL|" "$TMP.menu_hierarchy" | cut -f2 -d'|' | head -n 1`

  # combine some modules which are listed twice
  if [ "$MODL" = "g.region" ] ; then
      PLACEMENT=`echo "$PLACEMENT" | sed -e 's/Display/Set or Display/'`
  elif  [ "$MODL" = "r.reclass" ] || [ "$MODL" = "v.reclass" ] ; then
      PLACEMENT=`echo "$PLACEMENT" | sed -e 's/Reclassify.*$/Reclassify/'`
  fi

  echo "$PLACEMENT"
}


### execute the loop for all modules
for DIR in bin scripts ; do
  cd $DIR

  for MODULE in ?\.* db.* r3.* ; do
    unset label
    unset desc
#    echo "[$MODULE]"

    case "$MODULE" in
      g.parser | "r3.*" | g.module_to_skip)
	continue
	;;
    esac

    eval `$MODULE --interface-description | head -n 5 | tail -n 1 | \
        tr '"' "'" | sed -e 's/^[ \t]./desc="/' -e 's/$/"/' -e 's/[^\."]"$/&./'`

    if [ -z "$label" ] && [ -z "$desc" ] ; then
	continue
    fi

    MODULE_MENU_LOC=`find_menu_hierarchy "$MODULE"`

    BUFF=""
    if [ -z "$label" ] ; then
	BUFF="$MODULE: $desc"
    else
	BUFF="$MODULE: $label"
    fi
    if [ -n "$MODULE_MENU_LOC" ] ; then
        BUFF="$BUFF {$MODULE_MENU_LOC}"
    fi
    if [ -n "$BUFF" ] ; then
       #echo "$BUFF"
       echo "$BUFF" >> "$TMP"
    fi
  done

  cd ..
done

# ps.map doesn't jive with the above loop.
for MODULE in ps.map ; do
    unset label
    unset desc

    eval `$MODULE --interface-description | head -n 5 | tail -n 1 | \
        tr '"' "'" | sed -e 's/^[ \t]./desc="/' -e 's/$/"/' -e 's/[^\."]"$/&./'`

    if [ -z "$label" ] && [ -z "$desc" ] ; then
	continue
    fi

    MODULE_MENU_LOC=`find_menu_hierarchy "$MODULE"`

    BUFF=""
    if [ -z "$label" ] ; then
	BUFF="$MODULE: $desc"
    else
	BUFF="$MODULE: $label"
    fi
    if [ -n "$MODULE_MENU_LOC" ] ; then
        BUFF="$BUFF {$MODULE_MENU_LOC}"
    fi
    if [ -n "$BUFF" ] ; then
       #echo "$BUFF"
       echo "$BUFF" >> "$TMP"
    fi
done


# these don't use the parser at all.
cat << EOF >> "$TMP"
g.parser: Full parser support for GRASS scripts.
photo.2image: Marks fiducial or reseau points on an image to be ortho-rectified and then computes the image-to-photo coordinate transformation parameters.
photo.2target: Create control points on an image to be ortho-rectified.
photo.camera: Creates or modifies entries in a camera reference file.
photo.elev: Selects target elevation model for ortho-rectification.
photo.init: Creates or modifies entries in a camera initial exposure station file for imagery group referenced by a sub-block.
photo.rectify: Rectifies an image by using the image to photo coordinate transformation matrix created by photo.2image and the rectification parameters created by photo.2target.
photo.target: Selects target location and mapset for ortho-rectification.
EOF

## with --dictionary-order db.* ends up in the middle of the d.* cmds
#sort --dictionary-order "$TMP" > "$SYNOP"
sort "$TMP" > "$SYNOP"
\rm -f "$TMP"

cp "$SYNOP" "${TMP}.txt"

####### create HTML source #######
# poor cousin to full_index.html from tools/build_html_index.sh
# todo $MODULE.html links
g.message "Generating HTML (writing to \$GISBASE/docs/html/) ..."

#### write header
cat << EOF > "${TMP}.html"
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>`g.version | cut -f1 -d'('` Command list</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<link rel="stylesheet" href="grassdocs.css" type="text/css">
</head>
<body bgcolor="white">

<img src="grass_logo.png" alt="_\|/_ GRASS logo">
<hr align=center size=6 noshade>

<!-- prettier:
<BR><BR><BR><BR>
<center>
<img src="../../images/grasslogo.gif" alt="_\|/_ GRASS logo">
<BR><BR>
<hr width="450" align=center size=6 noshade>
-->

<center>
<H1>`g.version | cut -f1 -d'('` Command list</H1>
<h3>`date "+%e %B %Y"`</h3>
</center>
<BR><BR><BR>

<!--
<i><font size="-1" color="#778877">
   Menu position follows description if applicable.</font></i>
<BR><BR>
-->

<!--
# so it works from $WEB/gdp/grassmanuals/
#   untested:
sed -i -e 's+\(a href="\)\([^#\.h]\)+\1../../grass64/manuals/html64_user/\2+'
-->

<h4>Command types:</h4>
<ul>
  <li> d.* - <a href="#d">display commands</a>
  <li> db.* - <a href="#db">database</a> commands
  <li> g.* - <a href="#g">general</a> commands
  <li> i.* - <a href="#i">imagery</a> commands
  <li> m.* - <a href="#m">miscellanous</a> commands
  <li> ps.* - <a href="#ps">PostScript</a> commands
  <li> r.* - <a href="#r">raster</a> commands
  <li> r3.* - <a href="#r3">raster3D</a> commands
  <li> v.* - <a href="#v">vector</a> commands
  <li> <a href="wxGUI.html">wxGUI</a> - GUI frontend (wxPython)
  <li> <a href="nviz.html">NVIZ</a> - <i>n</i>-dimensional visualization suite
  <li> <a href="xganim.html">xganim</a> - raster map slideshow viewer
EOF


#### fill in module entries

for SECTION in d db g i m ps r r3 v ; do
    SEC_TYPE="commands"
    case $SECTION in
      d)
	SEC_NAME="Display" ;;
      db)
	SEC_NAME="Database management" ;;
      g)
	SEC_NAME="General GIS management" ;;
      i)
	SEC_NAME="Imagery" ;;
      m)
	SEC_NAME="Miscellaneous"
	SEC_TYPE="tools" ;;
      ps)
	SEC_NAME="PostScript" ;;
      r)
	SEC_NAME="Raster" ;;
      r3)
	SEC_NAME="Raster 3D" ;;
      v)
	SEC_NAME="Vector" ;;
    esac


    cat << EOF >> "${TMP}.html"
</ul>
<BR>

<a name="$SECTION"></a>
<H3>$SEC_NAME $SEC_TYPE:</H3>

<ul>
EOF

    grep "^${SECTION}\." "${TMP}.txt" | \
      sed -e 's/: /| /' -e 's/^.*|/<li> <a href="&.html">&<\/a>:/' \
	  -e 's/|.html">/.html">/' -e 's+|</a>:+</a>:+' \
	  -e 's/&/\&amp;/g' \
	  -e 's+ {+\n     <BR><font size="-2" color="#778877"><i>+' \
	  -e 's+}+</i></font>+' \
	  -e 's+ > + \&rarr; +g'  >> "${TMP}.html"

    if [ "$SECTION" = "i" ] ; then
	# include imagery photo subsection
	cat << EOF >> "${TMP}.html"
</ul>

<h4>Imagery photo.* commands:</h4>

<ul>
EOF

	grep "^photo\." "${TMP}.txt" | \
	  sed -e 's/: /| /' -e 's/^.*|/<li> <a href="&.html">&<\/a>:/' \
	      -e 's/|.html">/.html">/' -e 's+|</a>:+</a>:+' \
	      -e 's/&/\&amp;/g' \
	      -e 's+ {+\n     <BR><font size="-2" color="#778877"><i>+' \
	      -e 's+}+</i></font>+' \
	      -e 's+ > + \&rarr; +g'  >> "${TMP}.html"
    fi

done


#### save footer
cat << EOF >> "${TMP}.html"
</ul>

<hr>
<p>
<a href="index.html">Help Index</a><br>
&copy; 2007-2010 <a href="http://grass.osgeo.org">GRASS Development Team</a>
</p>

</BODY>
</HTML>
EOF

\mv "${TMP}.html" "$GISBASE/docs/html/module_synopsis.html"



####### create LaTeX source #######
g.message "Generating LaTeX source (writing to \$GISBASE/etc/) ..."

#### write header
cat << EOF > "${TMP}.tex"
%% Adapted from LyX 1.3 LaTeX export. (c) 2009 The GRASS Development Team
\documentclass[a4paper]{article}
\usepackage{palatino}
\usepackage[T1]{fontenc}
\usepackage[latin1]{inputenc}
\usepackage{a4wide}
\usepackage{graphicx}
\usepackage{color}
\definecolor{DarkSeaGreen3}{rgb}{0.412,0.545,0.412}

\makeatletter
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Textclass specific LaTeX commands.
 \newenvironment{lyxlist}[1]
   {\begin{list}{}
     {\settowidth{\labelwidth}{#1}
      \setlength{\leftmargin}{\labelwidth}
      \addtolength{\leftmargin}{\labelsep}
      \renewcommand{\makelabel}[1]{##1\hfil}}}
   {\end{list}}

%% last language is the default
\usepackage[german, english]{babel}
\makeatother
\begin{document}
\begin{center}\includegraphics[%
  width=0.3\textwidth]{grasslogo_vector.pdf}\end{center}

\begin{center}{\huge `g.version | cut -f1 -d'('` Command list}\end{center}{\huge \par}

\begin{center}{\large `date "+%e %B %Y"`}\end{center}{\large \par}
\bigskip{}


\section*{Command types:}

\begin{lyxlist}{00.00.0000}
\item [d.{*}]display commands
\item [db.{*}]database commands
\item [g.{*}]general commands
\item [i.{*}]imagery commands
\item [m.{*}]miscellanous commands
\item [ps.{*}]postscript commands
\item [r.{*}]raster commands
\item [r3.{*}]raster3D commands
\item [v.{*}]vector commands
\item [wxGUI]GUI frontend (wxPython)
\item [NVIZ]$n$-dimensional visualization suite
\item [xganim]raster map slideshow viewer
EOF


#### fill in module entries

for SECTION in d db g i m ps r r3 v ; do
    SEC_TYPE="commands"
    case $SECTION in
      d)
	SEC_NAME="Display" ;;
      db)
	SEC_NAME="Database management" ;;
      g)
	SEC_NAME="General GIS management" ;;
      i)
	SEC_NAME="Imagery" ;;
      m)
	SEC_NAME="Miscellaneous"
	SEC_TYPE="tools" ;;
      ps)
	SEC_NAME="PostScript" ;;
      r)
	SEC_NAME="Raster" ;;
      r3)
	SEC_NAME="Raster 3D" ;;
      v)
	SEC_NAME="Vector" ;;
    esac


    cat << EOF >> "${TMP}.tex"
\end{lyxlist}

\smallskip{}
\section*{$SEC_NAME $SEC_TYPE:}

\begin{lyxlist}{00.00.0000}
EOF

    grep "^${SECTION}\." "${TMP}.txt" | \
      sed -e 's/^/\\item [/' -e 's/: /]/' \
          -e 's+ {+ \\\\\n$+' -e 's/}$/$/' \
	  -e 's+ > +\\,\\triangleright\\,|+g' \
          -e 's/\*/{*}/g' -e 's/_/\\_/g' -e 's/&/\\\&/g' \
	| awk '/^\$/ { STR=$0; \
		       gsub(" ", "\\: ", STR); \
		       gsub(/\|/, " ", STR); \
		       sub(/^/, "  \\textcolor{DarkSeaGreen3}{\\footnotesize ", STR); \
		       sub(/$/, "}", STR); \
		       print STR \
		     } ;
	       /^\\/ {print}' >> "${TMP}.tex"

    if [ "$SECTION" = "i" ] ; then
	# include imagery photo subsection
	cat << EOF >> "${TMP}.tex"
\end{lyxlist}

\subsubsection*{Imagery photo.{*} commands:}

\begin{lyxlist}{00.00.0000}
EOF

	grep "^photo\." "${TMP}.txt" | \
	  sed -e 's/^/\\item [/' -e 's/: /]/' >> "${TMP}.tex"
    fi

done


#### save footer
cat << EOF >> "${TMP}.tex"
\end{lyxlist}

\end{document}
EOF


\mv "${TMP}.tex" "$GISBASE/etc/module_synopsis.tex"
\rm -f "${TMP}.txt"


##### FIXME
# post generation tidy-up
# - sort order isn't ideal. try 'sort -n'??
#     fix: *.univar.sh, r.surf.idw2, v.to.rast3, r.out.ppm3, others..
#####


g.message "Converting LaTeX to PDF (writing to \$GISBASE/docs/pdf/) ..."

for PGM in pdflatex ; do
   if [ ! -x `which $PGM` ] ; then
	g.message -e "pdflatex needed for this PDF conversion."
	g.message "Done."
	exit 1
   fi
done

TMPDIR="`dirname "$TMP"`"
cp "$OLDDIR/../man/grasslogo_vector.pdf" "$TMPDIR"
cd "$TMPDIR"

#once working nicely make it quieter
#pdflatex --interaction batchmode "$GISBASE/etc/module_synopsis.tex"
pdflatex "$GISBASE/etc/module_synopsis.tex"

\rm -f module_synopsis.dvi module_synopsis.ps \
       module_synopsis_2.ps grasslogo_vector.pdf

if [ ! -d "$GISBASE/docs/pdf" ] ; then
    mkdir "$GISBASE/docs/pdf"
fi
\mv module_synopsis.pdf "$GISBASE/docs/pdf/"

# Convert to pretty two-up version:
# Open PDF in Acrobat, print-to-file as postscript, then run:
# a2ps module_list.ps -o module_list_2up.ps
# ps2pdf13 module_list_2up.ps

g.message "Done."
