#!/usr/bin/env python3

############################################################################
#
# MODULE:       v.clip
# AUTHOR:       Zofie Cimburova, CTU Prague, Czech Republic
# PURPOSE:      Clips vector features
# COPYRIGHT:    (C) 2016 Zofie Cimburova
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Extracts features of input map which overlay features of clip map.
#% keyword: vector
#% keyword: clip
#% keyword: area
#%end

#%option G_OPT_V_INPUT
#% label: Name of vector map to be clipped
#% key: input
#%end

#%option G_OPT_V_INPUT
#% key: clip
#% label: Name of clip vector map
#%end

#%option G_OPT_V_OUTPUT
#% key: output
#%end

#%flag
#% key: d
#% description: Do not dissolve clip map
#%end

#%flag
#% key: r
#% description: Clip by region
#% suppress_required: yes
#% guisection: Region
#%end

# flags -d and -r are mutualy exclusive
# with flag -r, suppress_required: yes, but input and output must be defined
#%rules
#% exclusive: -d, -r
#% requires_all: -r, input, output
#%end

import os
import sys
import atexit

from grass.script import run_command, message, parser
import grass.script as grass
from grass.exceptions import CalledModuleError

TMP = []


def cleanup():
    for name in TMP:
        try:
            grass.run_command('g.remove', flags='f',
                              type='vector', name=name, quiet=True)

        except CalledModuleError as e:
            grass.fatal(_("Deleting of temporary layer failed. "
                          "Check above error messages and "
                          "see following details:\n%s") % e)


def section_message(msg):
    grass.message('{delim}\n{msg}\n{delim}'.format(msg=msg, delim='-' * 80))


def main():
    input_map = opt['input']
    clip_map = opt['clip']
    output_map = opt['output']

    flag_dissolve = flg['d']
    flag_region = flg['r']

    # ======================================== #
    # ========== INPUT MAP TOPOLOGY ========== #
    # ======================================== #
    vinfo = grass.vector_info_topo(input_map)

    # ==== only points ==== #
    if (vinfo['points'] > 0 and vinfo['lines'] == 0 and vinfo['areas'] == 0):

        # ==================================== #
        # ========== CLIP BY REGION ========== #
        # ==================================== #
        if (flag_region):
            clip_by_region(input_map, output_map, clip_select)

        # ================================== #
        # ========== DEFAULT CLIP ========== #
        # ================================== #
        else:
            section_message("Clipping.")
            # perform clipping
            clip_select(input_map, clip_map, output_map)

    # ==== lines, areas, lines + areas ==== #
    # ==== points + areas, points + lines, points + areas + lines ==== #
    else:
        if (vinfo['points'] > 0):
            grass.warning("Input map contains multiple geometry, "
                          "only lines and areas will be clipped.")

        # ==================================== #
        # ========== CLIP BY REGION ========== #
        # ==================================== #
        if (flag_region):
            clip_by_region(input_map, output_map, clip_overlay)

        # ===================================================== #
        # ========== CLIP WITHOUT DISSOLVED CLIP MAP ========== #
        # ===================================================== #
        elif (flag_dissolve):
            section_message("Clipping without dissolved clip map.")
            clip_overlay(input_map, clip_map, output_map)

        # ========================================================== #
        # ========== DEFAULT CLIP WITH DISSOLVED CLIP MAP ========== #
        # ========================================================== #
        else:
            section_message("Default clipping with dissolved clip map.")

            # setup temporary map
            temp_clip_map = '%s_%s' % ("temp", str(os.getpid()))
            TMP.append(temp_clip_map)

            # dissolve clip_map
            grass.run_command('v.dissolve', input=clip_map,
                              output=temp_clip_map)

            # perform clipping
            clip_overlay(input_map, temp_clip_map, output_map)

    # ======================================== #
    # ========== OUTPUT MAP TOPOLOGY========== #
    # ======================================== #
    vinfo = grass.vector_info_topo(output_map)
    if vinfo['primitives'] == 0:
        grass.warning("Output map is empty.")

    return 0


# clip input map by computational region
# clip_select for points, clip_overlay for areas and lines
def clip_by_region(input_map, output_map, clip_fn):
    section_message("Clipping by region.")

    # setup temporary map
    temp_region_map = '%s_%s' % ("temp", str(os.getpid()))
    TMP.append(temp_region_map)

    # create a map covering current computational region
    grass.run_command('v.in.region', output=temp_region_map)

    # perform clipping
    clip_fn(input_map, temp_region_map, output_map)


def clip_overlay(input_data, clip_data, out_data):
    try:
        grass.run_command('v.overlay', ainput=input_data, binput=clip_data,
                          operator='and', output=out_data, olayer='0,1,0')
    except CalledModuleError as e:
        grass.fatal(_("Clipping steps failed."
                      " Check above error messages and"
                      " see following details:\n%s") % e)


def clip_select(input_data, clip_data, out_data):
    try:
        grass.run_command('v.select', ainput=input_data, binput=clip_data,
                          output=out_data, operator='overlap')

    except CalledModuleError as e:
        grass.fatal(_("Clipping steps failed."
                      " Check above error messages and"
                      " see following details:\n%s") % e)


if __name__ == "__main__":
    atexit.register(cleanup)
    opt, flg = parser()
    sys.exit(main())
