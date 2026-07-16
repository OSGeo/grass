"""Space time dataset import functions.

Usage:

.. code-block:: python

    import grass.temporal as tgis

    input = "/tmp/temp_1950_2012.tar.gz"
    output = "temp_1950_2012"
    directory = "/tmp"
    title = "My new dataset"
    descr = "May new shiny dataset"
    location = None
    link = True
    exp = True
    overr = False
    create = False
    tgis.import_stds(
        input,
        output,
        directory,
        title,
        descr,
        location,
        link,
        exp,
        overr,
        create,
        "strds",
    )


(C) 2012-2026 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

import os
import tarfile
from pathlib import Path

import grass.script as gs
from grass.exceptions import CalledModuleError

from .core import get_current_mapset, get_tgis_message_interface, init
from .factory import dataset_factory
from .register import register_maps_in_space_time_dataset

proj_file_name = Path("proj.txt")
init_file_name = Path("init.txt")
list_file_name = Path("list.txt")

# This global variable is for unique vector map export,
# since single vector maps may have several layer
# and therefore several attribute tables
imported_maps = {}

############################################################################


def _import_raster_maps_from_gdal(
    maplist: list[dict[str, str]],
    *,
    overr: bool,
    exp: bool,
    location: str | None,
    link: bool,
    format_: str,
    set_current_region: bool = False,
    memory: int = 300,
) -> None:
    impflags = ""
    if overr:
        impflags += "o"
    if exp or location:
        impflags += "e"
    for row in maplist:
        name = row["name"]
        if format_ == "GTiff":
            filename = row["filename"] + ".tif"
        elif format_ == "AAIGrid":
            filename = row["filename"] + ".asc"
            if not overr:
                impflags += "o"

        try:
            if link:
                gs.run_command(
                    "r.external",
                    input=filename,
                    output=name,
                    flags=impflags,
                    overwrite=gs.overwrite(),
                )
            else:
                gs.run_command(
                    "r.in.gdal",
                    input=filename,
                    output=name,
                    memory=memory,
                    flags=impflags,
                    overwrite=gs.overwrite(),
                )

        except CalledModuleError:
            gs.fatal(
                _("Unable to import/link raster map <%s> from file %s.")
                % (name, filename),
            )

        # Set the color rules if present
        filename = row["filename"] + ".color"
        if Path(filename).is_file():
            try:
                gs.run_command(
                    "r.colors",
                    map=name,
                    rules=filename,
                    overwrite=gs.overwrite(),
                )
            except CalledModuleError:
                gs.fatal(_("Unable to set the color rules for raster map <%s>.") % name)

    # Set the computational region from the last map imported
    if set_current_region is True:
        gs.run_command("g.region", raster=name)


############################################################################


def _import_raster_maps(
    maplist: list[dict[str, str]],
    *,
    set_current_region: bool = False,
) -> None:
    # We need to disable the projection check because of its
    # simple implementation
    impflags = "o"
    for row in maplist:
        name = row["name"]
        filename = row["filename"] + ".pack"
        try:
            gs.run_command(
                "r.unpack",
                input=filename,
                output=name,
                flags=impflags,
                overwrite=gs.overwrite(),
                verbose=True,
            )

        except CalledModuleError:
            gs.fatal(
                _("Unable to unpack raster map <%s> from file %s.") % (name, filename),
            )

    # Set the computational region from the last map imported
    if set_current_region is True:
        gs.run_command("g.region", raster=name)


############################################################################


def _import_vector_maps_from_gml(
    maplist: list[dict[str, str]],
    *,
    overr: bool,
    exp: bool,
    location: str | None,
    link: bool,
) -> None:
    impflags = ""
    if overr:
        impflags += "o"
    if not link and (exp or location):
        impflags += "e"
    for row in maplist:
        name = row["name"]
        filename = f"{row['filename']}.xml"

        try:
            if link:
                gs.run_command(
                    "v.external",
                    input=filename,
                    output=name,
                    flags=impflags or None,
                    overwrite=gs.overwrite(),
                )
            else:
                gs.run_command(
                    "v.in.ogr",
                    input=filename,
                    output=name,
                    flags=impflags or None,
                    overwrite=gs.overwrite(),
                )

        except CalledModuleError:
            gs.fatal(
                _("Unable to import vector map <%s> from file %s.") % (name, filename),
            )


############################################################################


def _import_vector_maps(maplist: list[dict[str, str]]) -> None:
    # We need to disable the projection check because of its
    # simple implementation
    impflags = "o"
    for row in maplist:
        # Separate the name from the layer
        name = row["name"].split(":")[0]
        # Import only unique maps
        if name in imported_maps:
            continue
        filename = f"{row['filename']}.pack"
        try:
            gs.run_command(
                "v.unpack",
                input=filename,
                output=name,
                flags=impflags,
                overwrite=gs.overwrite(),
                verbose=True,
            )

        except CalledModuleError:
            gs.fatal(
                _("Unable to unpack vector map <%s> from file %s.") % (name, filename),
            )

        imported_maps[name] = name


############################################################################


def import_stds(
    input: str | Path,
    output: str,
    directory: str | Path | None = None,
    title: str | None = None,
    descr: str | None = None,
    location: str | None = None,
    link: bool = False,
    exp: bool = False,
    overr: bool = False,
    create: bool = False,
    stds_type: str = "strds",
    base: str | None = None,
    set_current_region: bool = False,
    memory: int = 300,
) -> None:
    """Import space time datasets of type raster and vector.

    :param input: Name of the input archive file
    :param output: The name of the output space time dataset
    :param directory: The extraction directory
    :param title: The title of the new created space time dataset
    :param descr: The description of the new created space time dataset
    :param location: The name of the location that should be created,
                    maps are imported into this location
    :param link: Switch to link raster maps instead importing them
    :param exp: Extend location extents based on new dataset
    :param overr: Override projection (use location's projection)
    :param create: Create the location specified by the "location"
                  parameter and exit.
                  Do not import the space time datasets.
    :param stds_type: The type of the space time dataset that should be imported
    :param base: The base name of the new imported maps, it will be
                 extended using a numerical index.
    :param memory: Cache size for raster rows, used in r.in.gdal
    """
    old_state = gs.get_raise_on_error()
    gs.set_raise_on_error(True)

    input_path = Path(input)
    # Check if input file and extraction directory exits
    if not input_path.exists():
        gs.fatal(_("Space time raster dataset archive <%s> not found") % str(input))
    if not create:
        directory = directory or gs.tempdir()
        if not Path(directory).exists():
            gs.fatal(_("Extraction directory <%s> not found") % directory)

    with tarfile.open(name=input_path, mode="r") as tar:
        # Check for important files
        msgr = get_tgis_message_interface()
        msgr.message(
            _("Checking validity of input file (size: %0.1f MB). Make take a while...")
            % (input_path.stat().st_size / (1024 * 1024.0)),
        )
        # Make sure that the basenames of the files are used for comparison
        member_basenames = [Path(name).name for name in tar.getnames()]

        if str(init_file_name) not in member_basenames:
            gs.fatal(_("Unable to find init file <%s>") % init_file_name)
        if str(list_file_name) not in member_basenames:
            gs.fatal(_("Unable to find list file <%s>") % list_file_name)
        if str(proj_file_name) not in member_basenames:
            gs.fatal(_("Unable to find projection file <%s>") % proj_file_name)

        msgr.message(_("Extracting data..."))
        # The 'data' extraction filter was added in Python 3.12 and backported to
        # 3.11.4 (PEP 706). Refuse to extract without it rather than
        # extracting unsafely.    if hasattr(tarfile, "data_filter"):
        if not hasattr(tarfile, "data_filter"):
            gs.fatal(_("Extracting may be unsafe; upgrade Python to 3.11.4 or newer"))
        tar.extractall(path=directory, filter="data")

    # We use a new list file name for map registration
    new_list_file_name = list_file_name.with_name(f"{list_file_name.stem}_new")
    # Save current working directory path
    old_cwd = Path.cwd()

    # Switch into the data directory
    os.chdir(directory)

    # Check projection information
    if not location:
        temp_name = gs.tempfile()
        proj_name = proj_file_name.absolute()

        # We need to convert projection strings generated
        # from other programs than g.proj into
        # new line format so that the grass file comparison function
        # can be used to compare the projections
        proj_content = proj_name.read_text(encoding="utf-8")
        proj_content = proj_content.replace(" +", "\n+")
        proj_content = proj_content.replace("\t+", "\n+")

        proj_name_tmp = f"{temp_name}_in_projection"
        Path(proj_name_tmp).write_text(proj_content)

        with Path(temp_name).open("w") as temp_file:
            p = gs.start_command("g.proj", flags="p", format="proj4", stdout=temp_file)
            p.communicate()

        if not gs.compare_key_value_text_files(temp_name, proj_name_tmp, sep="="):
            if overr:
                gs.warning(_("Projection information does not match. Proceeding..."))
            else:
                diff = "".join(gs.diff_files(temp_name, proj_name))
                gs.warning(
                    _(
                        "Difference between PROJ_INFO file of "
                        "imported map and of current location:"
                        "\n{diff}",
                    ).format(diff=diff),
                )
                gs.fatal(_("Projection information does not match. Aborting."))

    # Create a new location based on the projection information and switch
    # into it
    old_env = gs.gisenv()
    target_gisrc = None
    old_gisrc = os.environ.get("GISRC")
    if location:
        try:
            proj4_string = proj_file_name.read_text(encoding="utf-8").strip()
            print(proj4_string)
            gs.create_location(
                dbase=old_env["GISDBASE"],
                location=location,
                proj4=proj4_string,
            )
            # Just create a new location and return
            if create:
                os.chdir(old_cwd)
                return
        except Exception as e:
            gs.fatal(
                _("Unable to create location %(l)s. Reason: %(e)s")
                % {"l": location, "e": str(e)},
            )
        # Create a temporary environment
        try:
            target_gisrc, _target_env = gs.create_environment(
                old_env["GISDBASE"],
                location,
                "PERMANENT",
            )
            os.environ["GISRC"] = target_gisrc

        except OSError as e:
            gs.fatal(
                _("Unable to create environment for location %s. Reason: %s")
                % (location, e),
            )
        # create default database connection
        try:
            gs.run_command("t.connect", flags="d")
        except CalledModuleError:
            gs.fatal(
                _("Unable to create default temporal database in new location %s")
                % location,
            )

    try:
        # Make sure the temporal database exists
        init()

        fs = "|"
        maplist = []
        mapset = get_current_mapset()
        semantic_label_column = 4
        line_count = 0
        with (
            list_file_name.open("r") as list_file,
            new_list_file_name.open("w") as new_list_file,
        ):
            # get number of lines to correctly form the suffix
            max_count = len(list_file.readlines())
            list_file.seek(0)

            # Read the map list from file
            for line_count, line in enumerate(list_file, 1):
                map_line = line.rstrip()
                if not map_line:
                    continue

                line_list = map_line.split(fs)

                # The filename is actually the base name of the map
                # that must be extended by the file suffix
                filename = line_list[0].strip().split(":")[0]
                if base:
                    mapname = f"{base}_{gs.get_num_suffix(line_count + 1, max_count)}"
                    mapid = f"{mapname}@{mapset}"
                else:
                    mapname = filename
                    mapid = f"{mapname}@{mapset}"

                row = {
                    "filename": filename,
                    "name": mapname,
                    "id": mapid,
                    "start": line_list[1].strip(),
                    "end": line_list[2].strip(),
                    "semantic_label": line_list[3].strip()
                    if len(line_list) == semantic_label_column
                    else "",
                }

                new_list_file.write(
                    f"{mapname}{fs}{row['start']}{fs}{row['end']}"
                    f"{fs}{row['semantic_label']}\n",
                )

                maplist.append(row)

        # Read the init file
        fs = "="
        init_data = {}
        with init_file_name.open("r") as init_file:
            for line in init_file:
                init_line = line.rstrip()
                if not init_line:
                    continue

                kv = init_line.split(fs)
                init_data[kv[0]] = kv[1].strip()

        if (
            "temporal_type" not in init_data
            or "semantic_type" not in init_data
            or "number_of_maps" not in init_data
        ):
            gs.fatal(
                _("Key words %(t)s, %(s)s or %(n)s not found in init file.")
                % {"t": "temporal_type", "s": "semantic_type", "n": "number_of_maps"},
            )

        if line_count != int(init_data["number_of_maps"]):
            gs.fatal(_("Number of maps mismatch in init and list file."))

        format_ = "GTiff"
        type_ = "strds"

        if "stds_type" in init_data:
            type_ = init_data["stds_type"]
        if "format" in init_data:
            format_ = init_data["format"]

        if stds_type != type_:
            gs.fatal(_("The archive file is of wrong space time dataset type"))

        # Check the existence of the files
        if format_ == "GTiff":
            for row in maplist:
                filename = Path(row["filename"]).with_suffix(".tif")
                if not filename.exists():
                    gs.fatal(
                        _("Unable to find GeoTIFF raster file <%s> in archive.")
                        % filename,
                    )
        elif format_ == "AAIGrid":
            for row in maplist:
                filename = Path(row["filename"]).with_suffix(".asc")
                if not filename.exists():
                    gs.fatal(
                        _("Unable to find AAIGrid raster file <%s> in archive.")
                        % filename,
                    )
        elif format_ == "GML":
            for row in maplist:
                filename = Path(row["filename"]).with_suffix(".xml")
                if not filename.exists():
                    gs.fatal(
                        _("Unable to find GML vector file <%s> in archive.") % filename,
                    )
        elif format_ == "pack":
            for row in maplist:
                if type_ == "stvds":
                    filename = Path(row["filename"].split(":")[0]).with_suffix(".pack")
                else:
                    filename = Path(row["filename"]).with_suffix(".pack")
                if not filename.exists():
                    gs.fatal(
                        _("Unable to find GRASS package file <%s> in archive.")
                        % filename,
                    )
        else:
            gs.fatal(_("Unsupported input format"))

        # Check the space time dataset
        ident = f"{output}@{mapset}"
        sp = dataset_factory(type_, ident)
        if sp.is_in_db() and gs.overwrite() is False:
            gs.fatal(
                _(
                    "Space time %(t)s dataset <%(sp)s> is already in"
                    " the database. Use the overwrite flag.",
                )
                % {"t": type_, "sp": sp.get_id()},
            )

        # Import the maps
        if type_ == "strds":
            if format_ in {"GTiff", "AAIGrid"}:
                _import_raster_maps_from_gdal(
                    maplist,
                    overr=overr,
                    exp=exp,
                    location=location,
                    link=link,
                    format_=format_,
                    set_current_region=set_current_region,
                    memory=memory,
                )
            if format_ == "pack":
                _import_raster_maps(maplist, set_current_region=set_current_region)
        elif type_ == "stvds":
            if format_ == "GML":
                _import_vector_maps_from_gml(
                    maplist, overr=overr, exp=exp, location=location, link=link
                )
            if format_ == "pack":
                _import_vector_maps(maplist)

        # Create the space time dataset
        if sp.is_in_db() and gs.overwrite() is True:
            gs.info(
                _(
                    "Overwriting space time %(sp)s dataset "
                    "<%(id)s> and unregister all maps.",
                )
                % {"sp": sp.get_new_map_instance(None).get_type(), "id": sp.get_id()},
            )
            sp.delete()
            sp = sp.get_new_instance(id)

        temporal_type = init_data["temporal_type"]
        semantic_type = init_data["semantic_type"]
        relative_time_unit = None
        if temporal_type == "relative":
            if "relative_time_unit" not in init_data:
                gs.fatal(
                    _("Key word %s not found in init file.") % ("relative_time_unit"),
                )
            relative_time_unit = init_data["relative_time_unit"]
            sp.set_relative_time_unit(relative_time_unit)

        gs.verbose(
            _("Create space time %s dataset.")
            % sp.get_new_map_instance(None).get_type(),
        )

        sp.set_initial_values(
            temporal_type=temporal_type,
            semantic_type=semantic_type,
            title=title,
            description=descr,
        )
        sp.insert()

        # register the maps
        fs = "|"
        register_maps_in_space_time_dataset(
            type=sp.get_new_map_instance(None).get_type(),
            name=output,
            file=new_list_file_name,
            start="file",
            end="file",
            unit=relative_time_unit,
            dbif=None,
            fs=fs,
            update_cmd_list=False,
        )

    # Make sure the location is switched back correctly
    finally:
        os.chdir(old_cwd)
        if location:
            # Restore the original session
            if old_gisrc is not None:
                os.environ["GISRC"] = old_gisrc
            else:
                os.environ.pop("GISRC", None)

            init()

            # Delete the temporary session file
            if target_gisrc:
                gs.try_remove(target_gisrc)

        gs.set_raise_on_error(old_state)
