#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains utility functions for InteractiveMap.
#
# COPYRIGHT: (C) 2021 Caitlin Haedrich, and by the GRASS Development Team
#
#            This program is free software under the GNU Gernal Public
#            License (>=v2). Read teh file COPYING that comes with GRASS
#            for details.

import os
import grass.script as gs


def convert_coordinates_to_latlon(x, y, proj_in):
    """This function reprojects coordinates to WGS84, the required
    projection for vectors in folium.

    Arguments:
        x -- x coordinate (string)
        y -- y coordinate (string)
        proj_in -- proj4 string of location (for example, the output
        of g.region run with the `g` flag."""

    # Reformat input
    coordinates = f"{x}, {y}"
    # Reproject coordinates
    new_coords = gs.read_command(
        "m.proj",
        coordinates=coordinates,
        proj_in=proj_in,
        separator="comma",
        flags="do",
    )
    # Reformat from string to array
    new_coords = new_coords.strip()  # Remove '\n' at end of string
    new_coords = new_coords.split(",")  # Split on comma
    new_coords = [float(value) for value in new_coords]  # Convert to floats
    return new_coords[1], new_coords[0]  # Return Lat and Lon


def get_region(env=None):
    """Returns current computational region as dictionary.
    Adds long key names.
    """
    region = gs.region(env=env)
    region["east"] = region["e"]
    region["west"] = region["w"]
    region["north"] = region["n"]
    region["south"] = region["s"]
    return region


def get_location_proj_string(env=None):
    out = gs.read_command("g.proj", flags="jf", env=env)
    return out.strip()


def reproject_region(region, from_proj, to_proj):
    region = region.copy()
    proj_input = "{east} {north}\n{west} {south}".format(**region)
    proc = gs.start_command(
        "m.proj",
        input="-",
        separator=" , ",
        proj_in=from_proj,
        proj_out=to_proj,
        flags="d",
        stdin=gs.PIPE,
        stdout=gs.PIPE,
        stderr=gs.PIPE,
    )
    proc.stdin.write(gs.encode(proj_input))
    proc.stdin.close()
    proc.stdin = None
    proj_output, stderr = proc.communicate()
    if proc.returncode:
        raise RuntimeError("reprojecting region: m.proj error: " + stderr)
    enws = gs.decode(proj_output).split(os.linesep)
    elon, nlat, unused = enws[0].split(" ")
    wlon, slat, unused = enws[1].split(" ")
    region["east"] = elon
    region["north"] = nlat
    region["west"] = wlon
    region["south"] = slat
    return region


def estimate_resolution(raster, dbase, location, env):
    output = gs.read_command(
        "r.proj", flags="g", input=raster, dbase=dbase, location=location, env=env
    ).strip()
    params = gs.parse_key_val(output, vsep=" ")
    output = gs.read_command("g.region", flags="ug", env=env, **params)
    output = gs.parse_key_val(output, val_type=float)
    cell_ns = (output["n"] - output["s"]) / output["rows"]
    cell_ew = (output["e"] - output["w"]) / output["cols"]
    estimate = (cell_ew + cell_ns) / 2.0
    return estimate
