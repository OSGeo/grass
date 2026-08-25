#!/usr/bin/env python3
############################################################################
#
# MODULE:       r.drain
# AUTHOR(S):    Markus Metz
# PURPOSE:      Wrapper for r.path. If no input directions are provided,
#               directions are determined with r.fill.dir
# COPYRIGHT:    (C) 2017 by metz, and the GRASS Development Team
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

# %module
# % description: Traces a flow through an elevation model or cost surface on a raster map.
# % keyword: raster
# % keyword: hydrology
# % keyword: cost surface
# %end
# %option
# % key: input
# % type: string
# % required: yes
# % multiple: no
# % key_desc: name
# % description: Name of input elevation or cost surface raster map
# % gisprompt: old,cell,raster
# %end
# %option
# % key: direction
# % type: string
# % required: no
# % multiple: no
# % key_desc: name
# % label: Name of input movement direction map associated with the cost surface
# % description: Direction in degrees CCW from east
# % gisprompt: old,cell,raster
# % guisection: Cost surface
# %end
# %option
# % key: output
# % type: string
# % required: yes
# % multiple: no
# % key_desc: name
# % description: Name for output raster map
# % gisprompt: new,cell,raster
# %end
# %option
# % key: drain
# % type: string
# % required: no
# % multiple: no
# % key_desc: name
# % label: Name for output drain vector map
# % description: Recommended for cost surface made using knight's move
# % gisprompt: new,vector,vector
# %end
# %option
# % key: start_coordinates
# % type: double
# % required: no
# % multiple: yes
# % key_desc: east,north
# % description: Coordinates of starting point(s) (E,N)
# % gisprompt: old,coords,coords
# % guisection: Start
# %end
# %option
# % key: start_points
# % type: string
# % required: no
# % multiple: yes
# % key_desc: name
# % label: Name of starting vector points map(s)
# % gisprompt: old,vector,vector
# % guisection: Start
# %end
# %flag
# % key: c
# % description: Copy input cell values on output
# % guisection: Path settings
# %end
# %flag
# % key: a
# % description: Accumulate input values along the path
# % guisection: Path settings
# %end
# %flag
# % key: n
# % description: Count cell numbers along the path
# % guisection: Path settings
# %end
# %flag
# % key: d
# % description: The input raster map is a cost surface (direction surface must also be specified)
# % guisection: Cost surface
# %end
# %rules
# % required: start_coordinates, start_points
# %end

import sys
import atexit

import grass.script as gs


def cleanup():
    """Delete temporary direction map."""
    if tmp_maps:
        gs.run_command(
            "g.remove",
            flags="f",
            quiet=True,
            type="raster",
            name=tmp_maps,
            errors="ignore",
        )


def main():
    valmap = options["input"]
    dirmap = options["direction"]
    rpathmap = options["output"]
    vpathmap = options["drain"]
    start_coords = options["start_coordinates"]
    start_pnts = options["start_points"]

    global tmp_maps
    tmp_maps = False
    atexit.register(cleanup)

    if not dirmap:
        valmap_name = valmap.split("@", maxsplit=1)[0]
        # get directions with r.fill.dir, without sink filling
        dirmap = gs.append_node_pid(f"{valmap_name}_dir")
        fill_map = gs.append_node_pid(f"{valmap_name}_fill")
        area_map = gs.append_node_pid(f"{valmap_name}_area")
        tmp_maps = dirmap + "," + fill_map + "," + area_map
        gs.run_command(
            "r.fill.dir",
            input=valmap,
            output=fill_map,
            direction=dirmap,
            areas=area_map,
            flags="f",
            format="grass",
        )

    # create args for r.path
    kwargs = {}
    kwargs["input"] = dirmap
    if flags["c"] or flags["a"]:
        kwargs["values"] = valmap
    kwargs["format"] = "degree"
    if start_coords:
        kwargs["start_coordinates"] = start_coords
    if start_pnts:
        kwargs["start_points"] = start_pnts
    if rpathmap:
        kwargs["raster_path"] = rpathmap
    if vpathmap:
        kwargs["vector_path"] = vpathmap

    pathflags = ""
    if flags["c"]:
        pathflags += "c"
    if flags["a"]:
        pathflags += "a"
    if flags["n"]:
        pathflags += "n"

    gs.run_command("r.path", flags=pathflags, **kwargs)

    return 0


if __name__ == "__main__":
    options, flags = gs.parser()
    sys.exit(main())
