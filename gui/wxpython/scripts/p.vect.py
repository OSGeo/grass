#!/usr/bin/env python
############################################################################
#
# MODULE:       p.vect
# AUTHOR(S):    Jachym Cepicky, Hamish Bowman
#               Converted to Python by Huidae Cho
# PURPOSE:      Displays vector map layer in the active map display window.
# COPYRIGHT:    (C) 2009 by The GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
############################################################################

#%module
#% description: Displays vector map layer in the active map display window.
#% keywords: display, vector
#%end
#%flag
#% key: a
#% description: Get colors from map table column (of form RRR:GGG:BBB)
#% guisection: Colors
#%end
#%flag
#% key: c
#% description: Random colors according to category number (or layer number if 'layer=-1' is given)
#% guisection: Colors
#%end
#%flag
#% key: i
#% description: Use values from 'cats' option as feature id
#% guisection: Selection
#%end
#%flag
#% key: z
#% description: Colorize polygons according to z height
#%end
#%option
#% key: map
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Name of input vector map
#% gisprompt: old,vector,vector
#%end
#%option
#% key: display
#% type: string
#% required: no
#% multiple: yes
#% options: shape,cat,topo,dir,attr,zcoor
#% description: Display
#% answer: shape
#%end
#%option
#% key: type
#% type: string
#% required: no
#% multiple: yes
#% options: point,line,boundary,centroid,area,face
#% description: Feature type
#% answer: point,line,boundary,centroid,area,face
#% guisection: Selection
#%end
#%option
#% key: layer
#% type: string
#% required: no
#% multiple: no
#% label: Layer number (if -1, all layers are displayed)
#% description: A single vector map can be connected to multiple database tables. This number determines which table to use.
#% answer: 1
#% gisprompt: old_layer,layer,layer_all
#% guisection: Selection
#%end
#%option
#% key: cats
#% type: string
#% required: no
#% multiple: no
#% key_desc: range
#% label: Category values
#% description: Example: 1,3,7-9,13
#% guisection: Selection
#%end
#%option
#% key: where
#% type: string
#% required: no
#% multiple: no
#% key_desc: sql_query
#% label: WHERE conditions of SQL statement without 'where' keyword
#% description: Example: income < 1000 and inhab >= 10000
#% guisection: Selection
#%end
#%option
#% key: color
#% type: string
#% required: no
#% multiple: no
#% label: Line color
#% description: Either a standard GRASS color, R:G:B triplet, or "none"
#% answer: black
#% gisprompt: old_color,color,color_none
#% guisection: Colors
#%end
#%option
#% key: fcolor
#% type: string
#% required: no
#% multiple: no
#% label: Area fill color
#% description: Either a standard GRASS color, R:G:B triplet, or "none"
#% answer: 200:200:200
#% gisprompt: old_color,color,color_none
#% guisection: Colors
#%end
#%option
#% key: rgb_column
#% type: string
#% required: no
#% multiple: no
#% key_desc: name
#% description: Name of color definition column (for use with -a flag)
#% answer: GRASSRGB
#% gisprompt: old_dbcolumn,dbcolumn,dbcolumn
#% guisection: Colors
#%end
#%option
#% key: zcolor
#% type: string
#% required: no
#% multiple: no
#% key_desc: style
#% description: Type of color table (for use with -z flag)
#% answer: terrain
#% guisection: Colors
#%end
#%option
#% key: width
#% type: integer
#% required: no
#% multiple: no
#% description: Line width
#% answer: 0
#% guisection: Lines
#%end
#%option
#% key: wcolumn
#% type: string
#% required: no
#% multiple: no
#% key_desc: name
#% description: Name of column for line widths (these values will be scaled by wscale)
#% gisprompt: old_dbcolumn,dbcolumn,dbcolumn
#% guisection: Lines
#%end
#%option
#% key: wscale
#% type: double
#% required: no
#% multiple: no
#% description: Scale factor for wcolumn
#% answer: 1
#% guisection: Lines
#%end
#%option
#% key: icon
#% type: string
#% required: no
#% multiple: no
#% options: basic/marker,basic/circle,basic/arrow1,basic/star,basic/point,basic/triangle,basic/box,basic/arrow2,basic/octagon,basic/cross2,basic/pushpin,basic/diamond,basic/cross1,basic/x,demo/smrk,demo/muchomurka,extra/dive_flag,extra/half-box,extra/bridge,extra/fiducial,extra/ping,extra/offbox_ne,extra/adcp,extra/alpha_flag,extra/4pt_star,extra/half-circle,extra/offbox_nw,extra/fancy_compass,extra/airport,extra/compass,extra/offbox_se,extra/fish,extra/target,extra/offbox_sw,extra/n_arrow1,extra/pentagon,extra/n_arrow2,geology/strike_circle,geology/strike_box,geology/strike_line,geology/strike_triangle
#% description: Point and centroid symbol
#% answer: basic/x
#% guisection: Symbols
#%end
#%option
#% key: size
#% type: integer
#% required: no
#% multiple: no
#% description: Symbol size
#% answer: 5
#% guisection: Symbols
#%end
#%option
#% key: size_column
#% type: string
#% required: no
#% multiple: no
#% key_desc: name
#% description: Name of numeric column containing symbol size
#% gisprompt: old_dbcolumn,dbcolumn,dbcolumn
#% guisection: Symbols
#%end
#%option
#% key: rot_column
#% type: string
#% required: no
#% multiple: no
#% key_desc: name
#% label: Name of numeric column containing symbol rotation angle
#% description: Measured in degrees CCW from east
#% gisprompt: old_dbcolumn,dbcolumn,dbcolumn
#% guisection: Symbols
#%end
#%option
#% key: llayer
#% type: string
#% required: no
#% multiple: no
#% label: Layer number or name
#% description: Layer number for labels (default: the given layer number)
#% answer: 1
#% gisprompt: old_layer,layer,layer
#% guisection: Labels
#%end
#%option
#% key: attrcol
#% type: string
#% required: no
#% multiple: no
#% key_desc: name
#% description: Name of column to be displayed
#% gisprompt: old_dbcolumn,dbcolumn,dbcolumn
#% guisection: Labels
#%end
#%option
#% key: lcolor
#% type: string
#% required: no
#% multiple: no
#% label: Label color
#% description: Either a standard color name or R:G:B triplet
#% answer: red
#% gisprompt: old_color,color,color
#% guisection: Labels
#%end
#%option
#% key: bgcolor
#% type: string
#% required: no
#% multiple: no
#% label: Label background color
#% description: Either a standard GRASS color, R:G:B triplet, or "none"
#% answer: none
#% gisprompt: old_color,color,color_none
#% guisection: Labels
#%end
#%option
#% key: bcolor
#% type: string
#% required: no
#% multiple: no
#% label: Label border color
#% description: Either a standard GRASS color, R:G:B triplet, or "none"
#% answer: none
#% gisprompt: old_color,color,color_none
#% guisection: Labels
#%end
#%option
#% key: lsize
#% type: integer
#% required: no
#% multiple: no
#% description: Label size (pixels)
#% answer: 8
#% guisection: Labels
#%end
#%option
#% key: font
#% type: string
#% required: no
#% multiple: no
#% description: Font name
#% guisection: Labels
#%end
#%option
#% key: encoding
#% type: string
#% required: no
#% multiple: no
#% description: Text encoding
#% guisection: Labels
#%end
#%option
#% key: xref
#% type: string
#% required: no
#% multiple: no
#% options: left,center,right
#% description: Label horizontal justification
#% answer: left
#% guisection: Labels
#%end
#%option
#% key: yref
#% type: string
#% required: no
#% multiple: no
#% options: top,center,bottom
#% description: Label vertical justification
#% answer: center
#% guisection: Labels
#%end
#%option
#% key: minreg
#% type: double
#% required: no
#% multiple: no
#% description: Minimum region size (average from height and width) when map is displayed
#%end
#%option
#% key: maxreg
#% type: double
#% required: no
#% multiple: no
#% description: Maximum region size (average from height and width) when map is displayed
#%end
#%option
#% key: opacity
#% type: string
#% required: no
#% multiple: no
#% key_desc: val
#% answer: 100
#% description: Set opacity between 0-100%
#%end

import sys
import os
import grass.script as grass

def construct_command(cmd):
    line = cmd
    for key, val in options.iteritems():
        if val != "":
            line += " %s=%s" % (key, val)
    for key, val in flags.iteritems():
        if val == True:
            line += " -%s" % key
    return line

def main():
    cmd_file = grass.gisenv()["GRASS_PYCMDFILE"]

    if cmd_file == "" or os.path.exists(cmd_file) == False:
        grass.message(_("GRASS_PYCMDFILE - File not found. Run p.mon."), "e")
        return

    cmd = construct_command("d"+os.path.basename(sys.argv[0])[1:-3])

    fp = open(cmd_file, "a")
    fp.write("%s\n" % cmd)
    fp.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
