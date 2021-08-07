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
    """Returns projection of environment in PROJ.4 format"""
    out = gs.read_command("g.proj", flags="jf", env=env)
    return out.strip()


def reproject_region(region, from_proj, to_proj):
    """Reproject boundary of region from one projection to another.

    :param dict region: region to reproject as a dictionary with long key names
                    output of get_region
    :param str from_proj: PROJ.4 string of region; output of get_location_proj_string
    :param str in_proj: PROJ.4 string of target location;
                    output of get_location_proj_string

    :return dict region: reprojected region as a dictionary with long key names
    """
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
    """Estimates resolution of reprojected raster.

    :param str raster: name of raster
    :param str dbase: path to source database
    :param str location: name of source location
    :param str env: target environment

    :return float estimate: estimated resolution of raster in destination
                            environment
    """
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


def setup_location(name, dbase_location, epsg, src_env):
    """Setup temporary location with different projection but
    same computational region as source location

    :param str name: name of new location
    :param path dbase_location: path to new location database
    :param str epsg: EPSG code
    :param dict src_env: source environment

    :return str rcfile: name of new locations rcfile
    :return dict new_env: new environment
    """
    # Create new environment
    rcfile, new_env = gs.create_environment(dbase_location, name, "PERMANENT")
    # Location and mapset
    gs.create_location(dbase_location, name, epsg=epsg, overwrite=True)
    # Reproject region
    region = get_region(env=src_env)
    from_proj = get_location_proj_string(src_env)
    to_proj = get_location_proj_string(env=new_env)
    new_region = reproject_region(region, from_proj, to_proj)
    # Set region to match original region extent
    gs.run_command(
        "g.region",
        n=new_region["north"],
        s=new_region["south"],
        e=new_region["east"],
        w=new_region["west"],
        env=new_env,
    )
    return rcfile, new_env
