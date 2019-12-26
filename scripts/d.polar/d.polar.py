#!/usr/bin/env python3
#
############################################################################
#
# MODULE:       d.polar
# AUTHOR(S):    Markus Neteler. neteler itc.it
#               algorithm + EPS output by Bruno Caprile
#               d.graph plotting code by Hamish Bowman
#               Converted to Python by Glynn Clements
# PURPOSE:      Draws polar diagram of angle map. The outer circle considers
#               all cells in the map. If one or many of them are NULL (no data),
#               the figure will not reach the outer circle. The vector inside
#               indicates the prevalent direction.
# COPYRIGHT:    (C) 2006,2008 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%Module
#% description: Draws polar diagram of angle map such as aspect or flow directions
#% keyword: display
#% keyword: diagram
#%End
#%option G_OPT_R_MAP
#% description: Name of raster angle map
#%End
#%option
#% key: undef
#% type: double
#% description: Pixel value to be interpreted as undefined (different from NULL)
#% required : no
#%End
#%option G_OPT_F_OUTPUT
#% description: Name for optional EPS output file
#% required : no
#%end
#%flag
#% key: x
#% description: Plot using Xgraph
#%end

import os
import string
import types
import math
import atexit
import glob
import shutil
from grass.script.utils import try_remove, basename
from grass.script import core as gcore


def raster_map_required(name):
    if not gcore.find_file(name, 'cell')['file']:
        gcore.fatal(_("Raster map <%s> not found") % name)


def cleanup():
    try_remove(tmp)
    for f in glob.glob(tmp + '_*'):
        try_remove(f)


def plot_xgraph():
    newline = ['\n']
    p = gcore.Popen(['xgraph'], stdin=gcore.PIPE)
    for point in sine_cosine_replic + newline + outercircle + newline + vector:
        if isinstance(point, tuple):
            p.stdin.write("%f %f\n" % point)
        else:
            p.stdin.write(point + '\n')
    p.stdin.close()
    p.wait()


def plot_dgraph():
    # use d.info and d.frame to create a square frame in the center of the
    # window.
    s = gcore.read_command('d.info', flags='d')
    f = s.split()
    frame_width = float(f[2])
    frame_height = float(f[3])

    # take shorter side as length of frame side
    min_side = min(frame_width, frame_height)
    dx = (frame_width - min_side) / 2
    dy = (frame_height - min_side) / 2

    fl = dx
    fr = frame_width - dx
    ft = dy
    fb = frame_height - dy

    tenv = os.environ.copy()
    tenv['GRASS_RENDER_FRAME'] = '%f,%f,%f,%f' % (ft, fb, fl, fr)

    # polyline calculations
    ring = 0.95
    scaleval = ring * totalvalidnumber / totalnumber

    sine_cosine_replic_normalized = [
        ((scaleval * p[0] / maxradius + 1) * 50,
         (scaleval * p[1] / maxradius + 1) * 50)
        for p in sine_cosine_replic if isinstance(p, tuple)]

    # create circle
    circle = [(50 * (1 + ring * math.sin(math.radians(i))),
               50 * (1 + ring * math.cos(math.radians(i))))
              for i in range(0, 361)]

    # trend vector
    vect = [((scaleval * p[0] / maxradius + 1) * 50,
             (scaleval * p[1] / maxradius + 1) * 50)
            for p in vector[1:]]

    # Possible TODOs:
    # To fill data area with color, use BOTH d.graph's polyline and polygon commands.
    # Using polygon alone gives a jagged boundary due to sampling technique
    # (not a bug).

    # plot it!
    lines = [
        # draw circle
        #   mandatory when drawing proportional to non-square frame
        "color 180:255:180",
        "polyline"] + circle + [
        # draw axes
        "color 180:180:180",
        "width 0",
        "move 0 50",
        "draw 100 50",
        "move 50 0",
        "draw 50 100",

        # draw the goods
        "color red",
        "width 0",
        "polyline"] + sine_cosine_replic_normalized + [
        # draw vector
        "color blue",
        "width 3",
        "polyline"] + vect + [

        # draw compass text
        "color black",
        "width 2",
        "size 10 10",
        "move 51 97",
        "text N",
        "move 51 1",
        "text S",
        "move 1 51",
        "text W",
        "move 97 51",
        "text E",

        # draw legend text
        "width 0",
        "size 10",
        "color 0:180:0",
        "move 0.5 96.5",
        "text All data (incl. NULLs)",
        "color red",
        "move 0.5 93.5",
        "text Real data angles",
        "color blue",
        "move 0.5 90.5",
        "text Avg. direction"
    ]

    p = gcore.feed_command('d.graph', env=tenv)
    for point in lines:
        if isinstance(point, tuple):
            p.stdin.write("%f %f\n" % point)
        else:
            p.stdin.write(point + '\n')
    p.stdin.close()
    p.wait()


def plot_eps(psout):
    # EPS output (by M.Neteler and Bruno Caprile, ITC-irst)
    gcore.message(_("Generating %s ...") % psout)

    outerradius = maxradius
    epsscale = 0.1
    frameallowance = 1.1
    halfframe = 3000
    center = (halfframe, halfframe)
    scale = halfframe / (outerradius * frameallowance)

    diagramlinewidth = halfframe / 400
    axeslinewidth = halfframe / 500
    axesfontsize = halfframe / 16
    diagramfontsize = halfframe / 20
    halfframe_2 = halfframe * 2

    averagedirectioncolor = 1  # (blue)
    diagramcolor = 4  # (red)
    circlecolor = 2  # (green)
    axescolor = 0  # (black)

    northjustification = 2
    eastjustification = 6
    southjustification = 8
    westjustification = 8

    northxshift = 1.02 * halfframe
    northyshift = 1.98 * halfframe
    eastxshift = 1.98 * halfframe
    eastyshift = 1.02 * halfframe
    southxshift = 1.02 * halfframe
    southyshift = 0.02 * halfframe
    westxshift = 0.01 * halfframe
    westyshift = 1.02 * halfframe

    alldatastring = "all data (null included)"
    realdatastring = "real data angles"
    averagedirectionstring = "avg. direction"

    legendsx = 1.95 * halfframe
    alldatalegendy = 1.95 * halfframe
    realdatalegendy = 1.90 * halfframe
    averagedirectionlegendy = 1.85 * halfframe

    ##########
    outf = file(psout, 'w')

    prolog = os.path.join(
        os.environ['GISBASE'],
        'etc',
        'd.polar',
        'ps_defs.eps')
    inf = file(prolog)
    shutil.copyfileobj(inf, outf)
    inf.close()

    t = string.Template("""
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
""")
    s = t.substitute(
        AXESLINEWIDTH=axeslinewidth,
        DIAGRAMFONTSIZE=diagramfontsize,
        DIAGRAMLINEWIDTH=diagramlinewidth,
        EPSSCALE=epsscale,
        HALFFRAME=halfframe,
        HALFFRAME_2=halfframe_2
    )
    outf.write(s)

    sublength = len(outercircle) - 2
    (x, y) = outercircle[1]
    outf.write(
        "%.2f %.2f moveto\n" %
        (x * scale + halfframe, y * scale + halfframe))
    for x, y in outercircle[2:]:
        outf.write(
            "%.2f %.2f lineto\n" %
            (x * scale + halfframe, y * scale + halfframe))

    t = string.Template("""
stroke

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
""")
    s = t.substitute(
        AXESFONTSIZE=axesfontsize,
        DIAGRAMFONTSIZE=diagramfontsize,
        DIAGRAMLINEWIDTH=diagramlinewidth,
        EASTJUSTIFICATION=eastjustification,
        EASTXSHIFT=eastxshift,
        EASTYSHIFT=eastyshift,
        NORTHJUSTIFICATION=northjustification,
        NORTHXSHIFT=northxshift,
        NORTHYSHIFT=northyshift,
        SOUTHJUSTIFICATION=southjustification,
        SOUTHXSHIFT=southxshift,
        SOUTHYSHIFT=southyshift,
        WESTJUSTIFICATION=westjustification,
        WESTXSHIFT=westxshift,
        WESTYSHIFT=westyshift
    )
    outf.write(s)

    sublength = len(sine_cosine_replic) - 2
    (x, y) = sine_cosine_replic[1]
    outf.write(
        "%.2f %.2f moveto\n" %
        (x * scale + halfframe, y * scale + halfframe))
    for x, y in sine_cosine_replic[2:]:
        outf.write(
            "%.2f %.2f lineto\n" %
            (x * scale + halfframe, y * scale + halfframe))

    t = string.Template("""
stroke
%%
%% drawing average direction
%%

col1                                    %% colAVERAGE-DIRECTION-COLOR
$DIAGRAMLINEWIDTH setlinewidth                          %% DIAGRAM-LINEWIDTH setlinewidth
[] 0 setdash
newpath
                                        %% coordinates of rescaled, translated average direction follow
                                        %% first point moveto, second lineto
""")
    s = t.substitute(DIAGRAMLINEWIDTH=diagramlinewidth)
    outf.write(s)

    sublength = len(vector) - 2
    (x, y) = vector[1]
    outf.write(
        "%.2f %.2f moveto\n" %
        (x * scale + halfframe, y * scale + halfframe))
    for x, y in vector[2:]:
        outf.write(
            "%.2f %.2f lineto\n" %
            (x * scale + halfframe, y * scale + halfframe))

    t = string.Template("""
stroke

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
""")
    s = t.substitute(
        ALLDATALEGENDY=alldatalegendy,
        ALLDATASTRING=alldatastring,
        AVERAGEDIRECTIONLEGENDY=averagedirectionlegendy,
        AVERAGEDIRECTIONSTRING=averagedirectionstring,
        LEGENDSX=legendsx,
        REALDATALEGENDY=realdatalegendy,
        REALDATASTRING=realdatastring
    )
    outf.write(s)

    outf.close()

    gcore.message(_("Done."))


def main():
    global tmp
    global sine_cosine_replic, outercircle, vector
    global totalvalidnumber, totalnumber, maxradius

    map = options['map']
    undef = options['undef']
    eps = options['output']
    xgraph = flags['x']

    tmp = gcore.tempfile()

    if eps and xgraph:
        gcore.fatal(_("Please select only one output method"))

    # check if we have xgraph (if no EPS output requested)
    if xgraph and not gcore.find_program('xgraph'):
        gcore.fatal(
            _("xgraph required, please install first (www.xgraph.org)"))

    raster_map_required(map)

    #################################
    # this file contains everything:
    rawfile = tmp + "_raw"
    rawf = file(rawfile, 'w')
    gcore.run_command('r.stats', flags='1', input=map, stdout=rawf)
    rawf.close()

    rawf = file(rawfile)
    totalnumber = 0
    for line in rawf:
        totalnumber += 1
    rawf.close()

    gcore.message(
        _("Calculating statistics for polar diagram... (be patient)"))

    # wipe out NULL data and undef data if defined by user
    # - generate degree binned to integer, eliminate NO DATA (NULL):
    # change 360 to 0 to close polar diagram:
    rawf = file(rawfile)
    nvals = 0
    sumcos = 0
    sumsin = 0
    freq = {}
    for line in rawf:
        line = line.rstrip('\r\n')
        if line in ['*', undef]:
            continue
        nvals += 1
        x = float(line)
        rx = math.radians(x)
        sumcos += math.cos(rx)
        sumsin += math.sin(rx)
        ix = round(x)
        if ix == 360:
            ix = 0
        if ix in freq:
            freq[ix] += 1
        else:
            freq[ix] = 1
    rawf.close()

    totalvalidnumber = nvals
    if totalvalidnumber == 0:
        gcore.fatal(_("No data pixel found"))

    #################################
    # unit vector on raw data converted to radians without no data:

    unitvector = (sumcos / nvals, sumsin / nvals)

    #################################
    # how many are there?:
    occurrences = sorted([(math.radians(x), freq[x]) for x in freq])

    # find the maximum value
    maxradius = max([f for a, f in occurrences])

    # now do cos() sin()
    sine_cosine = [(math.cos(a) * f, math.sin(a) * f) for a, f in occurrences]

    sine_cosine_replic = ['"Real data angles'] + sine_cosine + sine_cosine[0:1]

    if eps or xgraph:
        outercircle = []
        outercircle.append('"All Data incl. NULLs')
        scale = 1.0 * totalnumber / totalvalidnumber * maxradius
        for i in range(0, 361):
            a = math.radians(i)
            x = math.cos(a) * scale
            y = math.sin(a) * scale
            outercircle.append((x, y))

    # fix vector length to become visible (x? of $MAXRADIUS):
    vector = []
    vector.append('"Avg. Direction\n')
    vector.append((0, 0))
    vector.append((unitvector[0] * maxradius, unitvector[1] * maxradius))

    ###########################################################

    # Now output:

    if eps:
        psout = basename(eps, 'eps') + '.eps'
        plot_eps(psout)
    elif xgraph:
        plot_xgraph()
    else:
        plot_dgraph()

    gcore.message(_("Average vector:"))
    gcore.message(
        _("direction: %.1f degrees CCW from East") %
        math.degrees(
            math.atan2(
                unitvector[1],
                unitvector[0])))
    gcore.message(
        _("magnitude: %.1f percent of fullscale") %
        (100 *
         math.hypot(
             unitvector[0],
             unitvector[1])))

if __name__ == "__main__":
    options, flags = gcore.parser()
    atexit.register(cleanup)
    main()
