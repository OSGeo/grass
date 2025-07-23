#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#            Riya Saxena <29riyasaxena AT gmail>
#
# PURPOSE:   This module contains utility functions for InteractiveMap.
#
# COPYRIGHT: (C) 2021-2024 Caitlin Haedrich, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Utility functions warpping existing processes in a suitable way"""

from collections.abc import Mapping
import tempfile
import json
import os
import multiprocessing

from pathlib import Path
import grass.script as gs


def get_region(env=None):
    """Returns current computational region as dictionary.

    Additionally, it adds long key names.
    """
    region = gs.region(env=env)
    region["east"] = region["e"]
    region["west"] = region["w"]
    region["north"] = region["n"]
    region["south"] = region["s"]
    return region


def get_location_proj_string(env=None):
    """Returns projection of environment in PROJ.4 format"""
    out = gs.read_command("g.proj", flags="fp", format="proj4", env=env)
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
    # reproject all corners, otherwise reproj. region may be underestimated
    # even better solution would be reprojecting vector region like in r.import
    proj_input = (
        f"{region['east']} {region['north']}\n"
        f"{region['west']} {region['north']}\n"
        f"{region['east']} {region['south']}\n"
        f"{region['west']} {region['south']}\n"
    )
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
    proj_output, stderr = proc.communicate(proj_input)
    if proc.returncode:
        raise RuntimeError(
            _("Encountered error while running m.proj: {}").format(stderr)
        )
    output = proj_output.splitlines()
    # get the largest bbox
    latitude_list = []
    longitude_list = []
    for row in output:
        longitude, latitude, unused = row.split(" ")
        longitude_list.append(float(longitude))
        latitude_list.append(float(latitude))
    region["east"] = max(longitude_list)
    region["north"] = max(latitude_list)
    region["west"] = min(longitude_list)
    region["south"] = min(latitude_list)
    return region


def reproject_latlon(coord):
    """Reproject coordinates

    :param coord: coordinates given as tuple (latitude, longitude)
    :return: reprojected coordinates (returned as tuple)
    """
    # Prepare the input coordinate string
    coord_str = f"{coord[1]} {coord[0]}\n"

    # Start the m.proj command
    proc = gs.start_command(
        "m.proj",
        input="-",
        flags="i",
        separator=",",
        stdin=gs.PIPE,
        stdout=gs.PIPE,
        stderr=gs.PIPE,
    )

    proc.stdin.write(gs.encode(coord_str))
    proc.stdin.close()
    proc.stdin = None
    proj_output, _ = proc.communicate()

    output = gs.decode(proj_output).splitlines()
    east, north, elev = map(float, output[0].split(","))

    return east, north, elev


def _style_table(html_content):
    """
    Use to style table displayed in popup.

    :param html_content: HTML content to be displayed

    :return str: formatted HTML content
    """
    css = """
    <style>
        table {
            border-collapse: collapse;
            width: 100%;
            font-size: 12px;
            font-family: Arial, sans-serif;
        }
        th {
            background-color: #666666;
            color: white;
            text-align: center;
            font-size: 12px;
        }
        td {
            padding-left: 3px;
            padding-right: 3px;
            border: 1px solid #ddd;
        }
        tr:nth-child(even) {
            background-color: #f9f9f9;
        }
        tr:nth-child(odd) {
            background-color: #ffffff;
        }
        td:last-child {
            text-align: right;
        }
    </style>
    """
    return f"{css}{html_content}"


def _format_nested_table(attributes):
    """
    Format nested attributes into an HTML table row.

    :param attributes: Dictionary of nested attributes to format.

    :return: str: HTML formatted string containing rows for each non-empty attribute.
    """
    nested_table = ""
    for sub_key, sub_value in attributes.items():
        if sub_value:
            nested_table += f"""
            <tr>
            <td>{sub_key}</td>
            <td>{sub_value}</td>
            </tr>
            """
    return nested_table


def _format_regular_output(items):
    """
    Format attributes into an HTML table.

    :param items: List of key-value pairs (tuples) to process.

    :return: str: HTML formatted string containing rows for specified attributes.
    """
    regular_output = ""
    for key, value in items:
        if key in {"Category", "Layer"}:
            regular_output += f"""
            <tr>
            <td>{key}</td>
            <td>{value}</td>
            </tr>
            """
    return regular_output


def query_raster(coord, raster_list):
    """
    Queries raster data at specified coordinates.

    :param coord: Coordinates given as a tuple (latitude, longitude).
    :param list raster_list: List of raster names to query.

    :return str: HTML formatted string containing the results of the raster queries.
    """
    output_list = ["""<table>"""]

    for raster in raster_list:
        raster_output = gs.raster.raster_what(map=raster, coord=coord)

        output = f"""<tr>
        <th colspan='2'>Raster: {raster}</th></tr>"""

        if raster in raster_output[0]:
            if "value" in raster_output[0][raster]:
                value = raster_output[0][raster]["value"]
                output += f"""
                <tr>
                <td>Value</td>
                <td>{value}</td>
                </tr>
                """
            items = raster_output[0][raster].items()
            formatted_output = _format_regular_output(items)
            output += formatted_output

        output_list.append(output)

    if len(output_list) == 1:
        return ""

    output_list.extend(("</table>", "<br>"))
    final_output = "".join(output_list)
    return _style_table(final_output)


def _process_vector_output(vector, coord, distance):
    """
    Process the output of a vector query.

    :param vector: Name of the vector map to query.
    :param coord: Coordinates given as a tuple for querying.
    :param distance: Distance within which to query the vector attributes.

    :return: str: HTML formatted string containing the vector output, including regular
                   attributes and any nested attribute tables.
    """
    vector_output = gs.vector.vector_what(map=vector, coord=coord, distance=distance)
    if len(vector_output[0]) <= 2:
        return ""

    items = list(vector_output[0].items())
    attributes_output = ""
    regular_output = _format_regular_output(items)

    for key, value in items:
        if key == "Attributes" and isinstance(value, dict):
            attributes_output = _format_nested_table(value)

    vector_html = f"""
    <tr>
    <th colspan='2'>
    Vector: {vector}
    </th>
    </tr>
    """
    return vector_html + attributes_output + regular_output


def query_vector(coord, vector_list, distance):
    """
    Queries vector data at specified coordinates.

    :param coord: Coordinates given as a tuple (latitude, longitude).
    :param list vector_list: List of vector names to query.
    :param distance: Distance within which to query the vector attributes.

    :return: str: HTML formatted string containing the results of the vector queries.
    """
    output_list = ["<table>"]

    for vector in vector_list:
        vector_html = _process_vector_output(vector, coord, distance)
        if vector_html:
            output_list.append(vector_html)

    if len(output_list) == 1:
        return ""

    output_list.extend(("</table>", "<br>"))
    final_output = "".join(output_list)
    return _style_table(final_output)


def estimate_resolution(
    raster: str, mapset: str, location: str, dbase: str, env: Mapping
) -> float:
    """Estimates resolution of reprojected raster.

    :param raster: name of raster
    :param mapset: mapset of raster
    :param location: name of source location
    :param dbase: path to source database
    :param dict env: target environment

    :return estimate: estimated resolution of raster in destination environment
    """
    output = gs.read_command(
        "r.proj",
        flags="g",
        input=raster,
        mapset=mapset,
        project=location,
        dbase=dbase,
        env=env,
    ).strip()
    params = gs.parse_key_val(output, vsep=" ")
    output = gs.read_command("g.region", flags="ug", env=env, **params)
    keyval = gs.parse_key_val(output, val_type=float)
    cell_ns = (keyval["n"] - keyval["s"]) / keyval["rows"]
    cell_ew = (keyval["e"] - keyval["w"]) / keyval["cols"]
    return (cell_ew + cell_ns) / 2.0


def setup_location(name, path, epsg, src_env):
    """Setup temporary location with different projection but
    same computational region as source location

    :param str name: name of new location
    :param path path: path to new location's database
    :param str epsg: EPSG code
    :param dict src_env: source environment

    :return str rcfile: name of new locations rcfile
    :return dict new_env: new environment
    """
    # Create new environment
    rcfile, new_env = gs.create_environment(path, name, "PERMANENT")
    # Location and mapset
    gs.create_location(path, name, epsg=epsg, overwrite=True)
    # Reproject region
    set_target_region(src_env, new_env)
    return rcfile, new_env


def set_target_region(src_env, tgt_env):
    """Set target region based on source region.

    Number of rows and columns is preserved.
    """
    region = get_region(env=src_env)
    from_proj = get_location_proj_string(src_env)
    to_proj = get_location_proj_string(env=tgt_env)
    new_region = reproject_region(region, from_proj, to_proj)
    # Set region to match original region extent
    gs.run_command(
        "g.region",
        n=new_region["north"],
        s=new_region["south"],
        e=new_region["east"],
        w=new_region["west"],
        rows=new_region["rows"],
        cols=new_region["cols"],
        env=tgt_env,
    )


def get_map_name_from_d_command(module, **kwargs):
    """Returns map name from display command.

    Assumes only positional parameters.
    When more maps are present (e.g., d.rgb), it returns only 1.
    Returns empty string if fails to find it.
    """
    special = {"d.his": "hue", "d.legend": "raster", "d.rgb": "red", "d.shade": "shade"}
    parameter = special.get(module, "map")
    return kwargs.get(parameter, "")


def get_rendering_size(region, width, height, default_width=600, default_height=400):
    """Returns the rendering width and height based
    on the region aspect ratio.

    :param dict region: region dictionary
    :param integer width: rendering width (can be None)
    :param integer height: rendering height (can be None)
    :param integer default_width: default rendering width (can be None)
    :param integer default_height: default rendering height (can be None)

    :return tuple (width, height): adjusted width and height

    When both width and height are provided, values are returned without
    adjustment. When one value is provided, the other is computed
    based on the region aspect ratio. When no dimension is given,
    the default width or height is used and the other dimension computed.
    """
    if width and height:
        return (width, height)
    region_width = region["e"] - region["w"]
    region_height = region["n"] - region["s"]
    if width:
        return (width, round(width * region_height / region_width))
    if height:
        return (round(height * region_width / region_height), height)
    if region_height > region_width:
        return (round(default_height * region_width / region_height), default_height)
    return (default_width, round(default_width * region_height / region_width))


def save_vector(name, geo_json):
    """Saves the user drawn vector.

    :param geo_json: name of the geojson file to be saved
    :param name: name with which vector should be saved
    """
    with tempfile.NamedTemporaryFile(
        suffix=".geojson", delete=False, mode="w"
    ) as temp_file:
        temp_filename = temp_file.name
        for each in geo_json["features"]:
            each["properties"].clear()
        json.dump(geo_json, temp_file)
    gs.run_command("v.import", input=temp_filename, output=name)
    os.remove(temp_filename)


def get_number_of_cores(requested, env=None):
    """Get the number of cores to use for multiprocessing.

    :param int requested: Desired number of cores.
    :param dict env: Optional process environment.

    :return int: Number of cores to use, constrained by system availability.
    """
    nprocs = gs.gisenv(env).get("NPROCS")
    if nprocs is not None:
        return int(nprocs)

    try:
        num_cores = len(os.sched_getaffinity(0))
    except AttributeError:
        num_cores = multiprocessing.cpu_count()
    return min(requested, max(1, num_cores - 1))


def get_region_bounds_latlon():
    """Gets the current computational region bounds in latlon.

    :return list of tuples: represent the southwest and northeast
                            corners of the region in (latitude, longitude) format.
    """
    region = gs.parse_command("g.region", flags="gbp")
    return [
        (float(region["ll_s"]), float(region["ll_w"])),
        (float(region["ll_n"]), float(region["ll_e"])),
    ]


def update_region(region):
    """Updates the computational region bounds.

    :param dict region: region dictionary
    :return: the new region
    """
    current = gs.region()
    return gs.parse_command(
        "g.region",
        flags="ga",
        n=region["north"],
        s=region["south"],
        e=region["east"],
        w=region["west"],
        nsres=current["nsres"],
        ewres=current["ewres"],
    )


def save_gif(
    input_files,
    output_filename,
    duration=500,
    label=True,
    labels=None,
    font=None,
    text_size=12,
    text_color="gray",
):
    """Creates a GIF animation

    param list input_files: list of paths to source
    param str output_filename: destination gif filename
    param int duration: time to display each frame; milliseconds
    param bool label: include label stamp on each frame
    param list labels: list of labels for each source image
    param str font: font file
    param int text_size: size of label text
    param str text_color: color to use for the text
    """
    # Create a GIF from the PNG images
    import PIL.Image  # pylint: disable=import-outside-toplevel
    import PIL.ImageDraw  # pylint: disable=import-outside-toplevel
    import PIL.ImageFont  # pylint: disable=import-outside-toplevel

    # filepath to output GIF
    filename = Path(output_filename)
    if filename.suffix.lower() != ".gif":
        raise ValueError(_("filename must end in '.gif'"))

    images = []
    for i, file in enumerate(input_files):
        img = PIL.Image.open(file)
        img = img.convert("RGBA", dither=None)
        draw = PIL.ImageDraw.Draw(img)
        if label:
            if font:
                font_obj = PIL.ImageFont.truetype(font, text_size)
            else:
                try:
                    font_obj = PIL.ImageFont.load_default(size=text_size)
                except TypeError:
                    font_obj = PIL.ImageFont.load_default()
            draw.text(
                (0, 0),
                labels[i],
                fill=text_color,
                font=font_obj,
            )
        images.append(img)

    images[0].save(
        fp=filename,
        format="GIF",
        append_images=images[1:],
        save_all=True,
        duration=duration,
        loop=0,
    )

    # Display the GIF
    return filename
