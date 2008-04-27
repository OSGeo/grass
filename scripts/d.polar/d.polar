#!/bin/sh
############################################################################
#
# MODULE:       d.polar
# AUTHOR(S):    Markus Neteler. neteler itc.it
#               algorithm + EPS output by Bruno Caprile
#               d.graph plotting code by Hamish Bowman
# PURPOSE:      Draws polar diagram of angle map. The outer circle considers
#               all cells in the map. If one or many of them are NULL (no data),
#               the figure will not reach the outer circle. The vector inside
#               indicates the prevalent direction.
# COPYRIGHT:    (C) 2006 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%Module
#%  description: Draws polar diagram of angle map such as aspect or flow directions
#%  keywords: display, diagram
#%End
#%option
#% key: map
#% type: string
#% key_desc: name
#% gisprompt: old,cell,raster
#% description: Name of raster angle map
#% required : yes
#%End
#%option
#% key: undef
#% type: double
#% description: Pixel value to be interpreted as undefined (different from NULL)
#% required : no
#%End
#%option
#% key: eps
#% type: string
#% gisprompt: new_file,file,output
#% description: Name of optional EPS output file
#% required : no
#%end
#%flag
#% key: x
#% description: Plot using Xgraph
#%end


if [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program." 1>&2
    exit 1
fi

if [ "$1" != "@ARGS_PARSED@" ] ; then
    exec g.parser "$0" "$@"
fi

PROG=`basename $0`

#### check if we have awk
if [ ! -x "`which awk`" ] ; then
    g.message -e "awk required, please install awk or gawk first"
    exit 1
fi


if [ -n "$GIS_OPT_EPS" ] && [ $GIS_FLAG_X -eq 1 ] ; then
    g.message -e "Please select only one output method"
    exit 1
fi


#### check if we have xgraph (if no EPS output requested)
if [ $GIS_FLAG_X -eq 1 ] ; then
  if [ ! -x "`which xgraph`" ] ; then
    g.message -e "xgraph required, please install first (www.xgraph.org)"
    exit 1
  fi
fi

# set environment so that awk works properly in all locales
unset LC_ALL
LC_NUMERIC=C
export LC_NUMERIC


TMP="`g.tempfile pid=$$`"
if [ $? -ne 0 ] || [ -z "${TMP}" ] ; then
    g.message -e "unable to create temporary files"
    exit 1
fi
#TMP=`dirname $TMP`

wordcount()
{
     awk '
     # Print list of word frequencies
     {
         for (i = 1; i <= NF; i++)
             freq[$i]++
	     total++
     }

     END {
         for (word in freq)
             printf "%s %d\n", word, freq[word]
     }' $1
}

cleanup()
{
   \rm -f "${TMP}" "${TMP}_"*
}

#################################
# this file contains everthing:
r.stats -1 "$GIS_OPT_MAP" > "${TMP}_raw"
TOTALNUMBER=`wc -l "${TMP}_raw" | awk '{print $1}'`

g.message "Calculating statistics for polar diagram... (be patient)"

#wipe out NULL data and undef data if defined by user
# - generate degree binned to integer, eliminate NO DATA (NULL):
# change 360 to 0 to close polar diagram:
cat "${TMP}_raw" | grep -v '^\*$' | grep -v "^${GIS_OPT_UNDEF}$" | \
    awk '{printf "%d\n", int($1 + .5)}' | sed 's+^360+0+g'  > "${TMP}_binned"

# make radians
cat "${TMP}_binned" | awk '{printf "%f\n", (3.14159265 * $1 ) / 180.}'  > "${TMP}_binned_radians"

#################################
# generate numbers for max circle
TOTALVALIDNUMBER=`wc -l "${TMP}_binned_radians" | awk '{print $1}'`

if [ "$TOTALVALIDNUMBER" -eq 0 ] ; then
   g.message -e "No data pixel found"
   cleanup
   exit 1
fi

#################################
# unit vector on raw data converted to radians without no data:
cat "${TMP}_raw" | grep -v '^\*' | grep -v "^${GIS_OPT_UNDEF}$" | awk 'BEGIN {sum = 0.0}
NR == 1{}
       {sum += cos(3.14159265 * $1 / 180.)}
END{print sum}' > "${TMP}_cos_sums"

cat "${TMP}_raw" | grep -v '^\*' | grep -v "^${GIS_OPT_UNDEF}$" | awk 'BEGIN {sum = 0.0}
NR == 1{}
       {sum += sin(3.14159265 * $1 / 180.)}
END{print sum}' > "${TMP}_sin_sums"

# cos -> x, sin -> y
echo "`cat "${TMP}_cos_sums"` $TOTALVALIDNUMBER" | awk '{printf "%.8f\n", $1/$2}' > "${TMP}_x_unit_vector"
echo "`cat "${TMP}_sin_sums"` $TOTALVALIDNUMBER" | awk '{printf "%.8f\n", $1/$2}' > "${TMP}_y_unit_vector"
UNITVECTOR=`paste -d' ' "${TMP}_x_unit_vector" "${TMP}_y_unit_vector"`

#################################
# how many are there?:
wordcount "${TMP}_binned_radians" | sort -n -t ' ' -k 1 > "${TMP}_occurrences"

# find the maximum value
MAXRADIUS="`cat "${TMP}_occurrences" | sort -n -t ' ' -k 2 | tail -n 1 | cut -d' ' -f2`"

# now do cos() sin()
cat "${TMP}_occurrences" | awk '{printf "%f %f\n", cos($1) * $2 , sin($1) *$2}' > "${TMP}_sine_cosine"

# to close the curve, we replicate the first value
REPLICATE=`head -n 1 "${TMP}_sine_cosine"`
echo "\"Real data angles"           >  "${TMP}_sine_cosine_replic"
cat "${TMP}_sine_cosine" >> "${TMP}_sine_cosine_replic"
echo $REPLICATE >> "${TMP}_sine_cosine_replic"

PI=3.14159265358979323846
if [ -n "$GIS_OPT_EPS" ] || [ $GIS_FLAG_X -eq 1 ] ; then
  rm -f "${TMP}_outercircle"
    echo "\"All Data incl. NULLs"           > "${TMP}_outercircle"

  awk -v PI=$PI -v TOTAL=$TOTALNUMBER -v TOTALVALID=$TOTALVALIDNUMBER \
      -v MAXRADIUS=$MAXRADIUS 'BEGIN {
	for(i=0; i<=360; i++) {
	    printf("%.8f %.8f\n",
	      cos(i * PI/180) * TOTAL/TOTALVALID * MAXRADIUS,
	      sin(i * PI/180) * TOTAL/TOTALVALID * MAXRADIUS)
	}
   }' >> "${TMP}_outercircle"
fi


# fix vector length to become visible (x? of $MAXRADIUS):
AUTOSTRETCH="1"
echo "\"Avg. Direction" > "${TMP}_vector"
echo "0 0"         >> "${TMP}_vector"
echo "$UNITVECTOR $MAXRADIUS $AUTOSTRETCH" | \
    awk '{printf "%f %f\n", $1 * $3, $2 * $3}' >> "${TMP}_vector"


###########################################################


plot_xgraph()
{
# by M.Neteler
echo "" > "${TMP}_newline"
cat "${TMP}_sine_cosine_replic" "${TMP}_newline" "${TMP}_outercircle" \
      "${TMP}_newline" "${TMP}_vector" | xgraph
}



plot_dgraph()
{
# by H.Bowman

# use d.info and d.frame to create a square frame in the center of the window.
FRAME_DIMS="`d.info -f | cut -f2- -d' '`"
FRAME_WIDTH="`echo $FRAME_DIMS | awk '{printf("%d", $2 - $1)}'`"
FRAME_HEIGHT="`echo $FRAME_DIMS | awk '{printf("%d", $4 - $3)}'`"

# take shorter side as length of frame side
if [ $FRAME_WIDTH -lt $FRAME_HEIGHT ] ; then
    MIN_SIDE=$FRAME_WIDTH
else
    MIN_SIDE=$FRAME_HEIGHT
fi

DX="`echo $FRAME_WIDTH $MIN_SIDE | awk '{printf("%d", 0.5+(($1 - $2)/2) )}'`"
DY="`echo $FRAME_HEIGHT $MIN_SIDE | awk '{printf("%d", 0.5+(($1 - $2)/2) )}'`"

FRAME_LEFT="`echo $FRAME_DIMS | cut -f1 -d' '`"
FRAME_TOP="`echo $FRAME_DIMS | cut -f3 -d' '`"

# new square frame dims in pixels
DFR_T=`expr $FRAME_TOP + $DY`
DFR_L=`expr $FRAME_LEFT + $DX`
DFR_B=`expr $DFR_T + $MIN_SIDE`
DFR_R=`expr $DFR_L + $MIN_SIDE`

WIN_DIMS="`d.info -d | cut -f2- -d' '`"
WIN_WIDTH="`echo $WIN_DIMS | cut -f1 -d' '`"
WIN_HEIGHT="`echo $WIN_DIMS | cut -f2 -d' '`"

# new square frame dims in percentage of overal window
# note d.info shows 0,0 as top left, d.frame expects 0%,0% as bottom left
PER_T="`echo $DFR_B $WIN_HEIGHT | awk '{printf("%f", 100 - ($1 * 100. / $2) ) }'`"
PER_B="`echo $DFR_T $WIN_HEIGHT | awk '{printf("%f", 100 - ($1 * 100. / $2) ) }'`"
PER_L="`echo $DFR_L $WIN_WIDTH | awk '{printf("%f", $1 * 100. / $2 )}'`"
PER_R="`echo $DFR_R $WIN_WIDTH | awk '{printf("%f", $1 * 100. / $2 )}'`"


# save current frame name to restore later
ORIG_FRAME="`d.frame -p`"

# create square frame within current frame
d.frame -c frame=d_polar.$$ at=$PER_T,$PER_B,$PER_L,$PER_R


# polyline calculations
RING=0.95
SCALEVAL=`awk -v RING=$RING -v TOTAL=$TOTALNUMBER -v TOTALVALID=$TOTALVALIDNUMBER \
    'BEGIN { print RING * TOTALVALID/TOTAL }'`

cat "${TMP}_sine_cosine_replic" | tail -n +2 | awk -v SCL=$SCALEVAL -v MAX=$MAXRADIUS \
    '{printf "%f %f\n", ((SCL * $1/MAX) +1)*50, ((SCL * $2/MAX) +1)*50}' \
       > "${TMP}_sine_cosine_replic_normalized"

# create circle
awk -v RING=$RING -v PI=$PI 'BEGIN {
   for(i=0; i<=360; i++) {
     printf("%f %f\n", 50*(1+(RING * sin(i * (PI/180)))),
	    50*(1+(RING * cos(i * (PI/180)))) )
   }
 }' > "${TMP}_circle"

# trend vector
VECT=`cat "${TMP}_vector" | tail -n 1 | awk -v SCL=$SCALEVAL -v MAX=$MAXRADIUS \
    '{printf "%f %f\n", ((SCL * $1/MAX)+1)*50, ((SCL * $2/MAX)+1)*50}'`


# Possible TODOs:
# To have d.polar survive d.redraw, write d.graph commands to a
#  ${TMP}.dpolar.dgraph file and then use d.graph input=${TMP}.dpolar.dgraph.
#  The file will be cleaned up when the grass session exits.
#
# To fill data area with color, use BOTH d.graph's polyline and polygon commands.
#  Using polygon alone gives a jagged boundary due to sampling technique (not a bug).


# plot it!
d.erase
d.graph << EOF

  # draw circle
  #   mandatory when drawing proportional to non-square frame
  color 180:255:180
  polyline
    `cat "${TMP}_circle"`

  # draw axes
  color 180:180:180
  width 0
  move 0 50
  draw 100 50
  move 50 0
  draw 50 100

  # draw the goods
  color red
  width 0
  polyline
   `cat "${TMP}_sine_cosine_replic_normalized"`

  # draw vector
  color blue
  width 3
  move 50 50
  draw $VECT

  # draw compass text
  color black
  width 2
  size 3 3
  move 51 97
   text N
  move 51 1
   text S
  move 1 51
   text W
  move 97 51
   text E

  # draw legend text
  width 0
  size 2
  color 0:180:0
   move 1.5 96.5
   text All data (incl. NULLs)
  color red
   move 1.5 93.5
   text Real data angles
  color blue
   move 1.5 90.5
   text Avg. direction

EOF

# back to original frame
d.frame -s frame="$ORIG_FRAME"

}



plot_eps()
{
# EPS output (by M.Neteler and Bruno Caprile, ITC-irst)
g.message "Generating $PSOUT ..."

OUTERRADIUS=$MAXRADIUS
EPSSCALE=0.1
FRAMEALLOWANCE=1.1
HALFFRAME=3000
CENTER=$HALFFRAME,$HALFFRAME
#SCALE=$HALFFRAME / ($OUTERRADIUS * $FRAMEALLOWANCE)
SCALE=`echo $HALFFRAME $OUTERRADIUS $FRAMEALLOWANCE | awk '{printf "%f", $1/ ($2 * $3)}'`

# DIAGRAMLINEWIDTH=CEILING ($HALFFRAME / 200)
DIAGRAMLINEWIDTH=`echo $HALFFRAME | awk '{printf "%.1f", $1 / 400}'`  # ceil? 200
# AXESLINEWIDTH=CEILING ($HALFFRAME / 500)
AXESLINEWIDTH=`echo $HALFFRAME | awk '{printf "%.1f", $1 / 500}'` # ceil?
# AXESFONTSIZE=CEILING ($HALFFRAME / 15)
AXESFONTSIZE=`echo $HALFFRAME | awk '{printf "%.1f", $1 / 16}'` # ceil?
# DIAGRAMFONTSIZE=CEILING ($HALFFRAME / 20)
DIAGRAMFONTSIZE=`echo $HALFFRAME | awk '{printf "%.1f", $1 / 20}'` # ceil?
HALFFRAME_2=`echo $HALFFRAME | awk '{printf "%.1f", $1 * 2}'`

AVERAGEDIRECTIONCOLOR=1 #(blue)
DIAGRAMCOLOR=4 #(red)
CIRCLECOLOR=2 #(green)
AXESCOLOR=0 #(black)

NORTHJUSTIFICATION=2
EASTJUSTIFICATION=6
SOUTHJUSTIFICATION=8
WESTJUSTIFICATION=8

NORTHXSHIFT=`echo 1.02 $HALFFRAME | awk '{printf "%.1f", $1 * $2}'`
NORTHYSHIFT=`echo 1.98 $HALFFRAME | awk '{printf "%.1f", $1 * $2}'`
EASTXSHIFT=`echo 1.98 $HALFFRAME  | awk '{printf "%.1f", $1 * $2}'`
EASTYSHIFT=`echo 1.02 $HALFFRAME  | awk '{printf "%.1f", $1 * $2}'`
SOUTHXSHIFT=`echo 1.02 $HALFFRAME | awk '{printf "%.1f", $1 * $2}'`
SOUTHYSHIFT=`echo 0.02 $HALFFRAME | awk '{printf "%.1f", $1 * $2}'`
WESTXSHIFT=`echo 0.01 $HALFFRAME  | awk '{printf "%.1f", $1 * $2}'`
WESTYSHIFT=`echo 1.02 $HALFFRAME  | awk '{printf "%.1f", $1 * $2}'`

ALLDATASTRING="All Data (NULL included)"
REALDATASTRING="Real Data Angles"
AVERAGEDIRECTIONSTRING="Avg. Direction"

LEGENDSX=`echo "1.95 $HALFFRAME" | awk '{printf "%.1f", $1 * $2}'`
ALLDATALEGENDY=`echo "1.95 $HALFFRAME" | awk '{printf "%.1f", $1 * $2}'`
REALDATALEGENDY=`echo "1.90 $HALFFRAME" | awk '{printf "%.1f", $1 * $2}'`
AVERAGEDIRECTIONLEGENDY=`echo "1.85 $HALFFRAME" | awk '{printf "%.1f", $1 * $2}'`

##########
cat "${GISBASE}/etc/d.polar/ps_defs.eps" > "$PSOUT"

echo "
$EPSSCALE $EPSSCALE scale                           %% EPS-SCALE EPS-SCALE scale
%%
%% drawing axes
%%

col0                                    %% colAXES-COLOR
$AXESLINEWIDTH setlinewidth                          %% AXES-LINEWIDTH setlinewidth
[] 0 setdash
newpath
 $HALFFRAME     0.0 moveto                  %% HALF-FRAME 0.0 moveto
 $HALFFRAME  $HALFFRAME_2 lineto                  %% HALF-FRAME (2 * HALF-FRAME) lineto
    0.0  $HALFFRAME moveto                  %% 0.0 HALF-FRAME moveto
 $HALFFRAME_2  $HALFFRAME lineto                  %% (2 * HALF-FRAME) HALF-FRAME lineto
stroke

%%
%% drawing outer circle
%%

col2                                    %% colCIRCLE-COLOR
$DIAGRAMFONTSIZE /Times-Roman choose-font            %% DIAGRAM-FONTSIZE /Times-Roman choose-font
$DIAGRAMLINEWIDTH setlinewidth                          %% DIAGRAM-LINEWIDTH setlinewidth
[] 0 setdash
newpath
                                        %% coordinates of rescaled, translated outer circle follow
                                        %% first point moveto, then lineto
" >> "$PSOUT"

SUBLENGTH=`wc -l "${TMP}_outercircle" | awk '{printf "%d", $1 - 2}'`
LINE1=`head -n 2 "${TMP}_outercircle" | tail -n 1`
echo $LINE1 $SCALE $HALFFRAME | awk '{printf "%.2f %.2f moveto\n", $1*$3+$4, $2*$3+$4}' >> "$PSOUT"

tail -n $SUBLENGTH "${TMP}_outercircle" | sed "s+\$+ $SCALE $HALFFRAME+g" > "${TMP}_outercircle_lineto"

cat "${TMP}_outercircle_lineto" | awk '{printf "%.2f %.2f lineto\n",$1*$3+$4, $2*$3+$4 }' >> "$PSOUT"
rm -f "${TMP}_outercircle_lineto"

echo "stroke

%%
%% axis titles
%%

col0                                    %% colAXES-COLOR
$AXESFONTSIZE /Times-Roman choose-font            %% AXES-FONTSIZE /Times-Roman choose-font
(N) $NORTHXSHIFT $NORTHYSHIFT $NORTHJUSTIFICATION just-string         %% NORTH-X-SHIFT NORTH-Y-SHIFT NORTH-JUSTIFICATION just-string
(E) $EASTXSHIFT $EASTYSHIFT $EASTJUSTIFICATION just-string         %% EAST-X-SHIFT EAST-Y-SHIFT EAST-JUSTIFICATION just-string
(S) $SOUTHXSHIFT $SOUTHYSHIFT $SOUTHJUSTIFICATION just-string           %% SOUTH-X-SHIFT SOUTH-Y-SHIFT SOUTH-JUSTIFICATION just-string
(W) $WESTXSHIFT $WESTYSHIFT $WESTJUSTIFICATION just-string           %% WEST-X-SHIFT WEST-Y-SHIFT WEST-JUSTIFICATION just-string
$DIAGRAMFONTSIZE /Times-Roman choose-font            %% DIAGRAM-FONTSIZE /Times-Roman choose-font


%%
%% drawing real data diagram
%%

col4                                    %% colDIAGRAM-COLOR
$DIAGRAMLINEWIDTH setlinewidth                          %% DIAGRAM-LINEWIDTH setlinewidth
[] 0 setdash
newpath
                                        %% coordinates of rescaled, translated diagram follow
                                        %% first point moveto, then lineto
" >> "$PSOUT"

SUBLENGTH=`wc -l "${TMP}_sine_cosine_replic" | awk '{printf "%d", $1 - 2}'`
LINE1=`head -n 2 "${TMP}_sine_cosine_replic" | tail -n 1`
echo $LINE1 $SCALE $HALFFRAME | awk '{printf "%.2f %.2f moveto\n", $1*$3+$4, $2*$3+$4}' >> "$PSOUT"

tail -n $SUBLENGTH "${TMP}_sine_cosine_replic" | \
   sed "s+\$+ $SCALE $HALFFRAME+g" > "${TMP}_sine_cosine_replic_lineto"

cat "${TMP}_sine_cosine_replic_lineto" | \
   awk '{printf "%.2f %.2f lineto\n",$1*$3+$4, $2*$3+$4 }' >> "$PSOUT"

rm -f "${TMP}_sine_cosine_replic_lineto"

echo "stroke
%%
%% drawing average direction
%%

col1                                    %% colAVERAGE-DIRECTION-COLOR
$DIAGRAMLINEWIDTH setlinewidth                          %% DIAGRAM-LINEWIDTH setlinewidth
[] 0 setdash
newpath
                                        %% coordinates of rescaled, translated average direction follow
                                        %% first point moveto, second lineto
" >> $PSOUT

SUBLENGTH=`wc -l "${TMP}_vector" | awk '{printf "%d", $1 - 2}'`
LINE1=`head -n 2 "${TMP}_vector" | tail -n 1`
echo $LINE1 $SCALE $HALFFRAME | awk '{printf "%.2f %.2f moveto\n", $1*$3+$4, $2*$3+$4}' >> "$PSOUT"

tail -n $SUBLENGTH "${TMP}_vector" | sed "s+\$+ $SCALE $HALFFRAME+g" > "${TMP}_vector_lineto"

cat "${TMP}_vector_lineto" | awk '{printf "%.2f %.2f lineto\n",$1*$3+$4, $2*$3+$4 }' >> "$PSOUT"
rm -f "${TMP}_vector_lineto"

echo "stroke

%%
%% drawing legends
%%

col2                                    %% colCIRCLE-COLOR
%% Line below: (ALL-DATA-STRING) LEGENDS-X ALL-DATA-LEGEND-Y 4 just-string
($ALLDATASTRING) $LEGENDSX $ALLDATALEGENDY 4 just-string

col4                                    %% colDIAGRAM-COLOR
%% Line below: (REAL-DATA-STRING) LEGENDS-X REAL-DATA-LEGEND-Y 4 just-string
($REALDATASTRING) $LEGENDSX $REALDATALEGENDY 4 just-string

col1                                    %% colAVERAGE-DIRECTION-COLOR
%% Line below: (AVERAGE-DIRECTION-STRING) LEGENDS-X AVERAGE-DIRECTION-LEGEND-Y 4 just-string
($AVERAGEDIRECTIONSTRING) $LEGENDSX $AVERAGEDIRECTIONLEGENDY 4 just-string
" >> "$PSOUT"

g.message "Done."
}


# Now output:

if [ -n "$GIS_OPT_EPS" ] ; then
   PSOUT="`basename $GIS_OPT_EPS .eps`.eps"
   plot_eps
else
   if [ $GIS_FLAG_X -eq 1 ] ; then
     plot_xgraph
   else
     plot_dgraph
   fi
fi


g.message "Average vector:"
echo "direction: `echo "$UNITVECTOR" | \
  awk -v PI=$PI '{ print atan2($2, $1) * 180/PI }'` degrees CCW from East"
echo "magnitude: `echo "$UNITVECTOR " | \
  awk '{ print sqrt($1*$1 + $2*$2)*100 }'` percent of fullscale"


cleanup
exit 0
