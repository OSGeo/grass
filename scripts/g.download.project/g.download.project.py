#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.download.project
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
# PURPOSE:   Download and extract project (location) from web
# COPYRIGHT: (C) 2017-2024 by the GRASS Development Team
#
#    This program is free software under the GNU General
#    Public License (>=v2). Read the file COPYING that
#    comes with GRASS for details.
#
#############################################################################

"""Download GRASS projects"""

# %module
# % label: Download GRASS project from the web
# % description: Get GRASS project from an URL or file path
# % keyword: general
# % keyword: data
# % keyword: download
# % keyword: import
# %end
# %option
# % key: url
# % multiple: no
# % type: string
# % label: URL of the archive with a project to be downloaded
# % description: URL of ZIP, TAR.GZ, or other similar archive
# % required: yes
# %end
# %option G_OPT_M_LOCATION
# % key: name
# % required: no
# % multiple: no
# % key_desc: name
# %end
# %option G_OPT_M_DBASE
# % key: path
# % required: no
# % multiple: no
# %end

import atexit
import os
import shutil
from pathlib import Path

import grass.script as gs
from grass.grassdb.checks import is_location_valid
from grass.script.utils import try_rmdir
from grass.utils.download import DownloadError, download_and_extract, name_from_url


def find_location_in_directory(path, recurse=0):
    """Return path to location in one of the subdirectories or None

    The first location found is returned. The expected usage is looking for one
    location somewhere nested in subdirectories.

    By default only the immediate subdirectories of the provided directory are
    tested, but with ``recurse >= 1`` additional levels of subdirectories
    are tested for being locations.

    Directory names are sorted to provide a stable result.

    :param path: Path to the directory to search
    :param recurse: How many additional levels of subdirectories to explore
    """
    assert recurse >= 0
    full_paths = [os.path.join(path, i) for i in os.listdir(path)]
    candidates = sorted([i for i in full_paths if os.path.isdir(i)])
    for candidate in candidates:
        if is_location_valid(candidate):
            return candidate
    if recurse:
        for candidate in candidates:
            result = find_location_in_directory(candidate, recurse - 1)
            if result:
                return result
    return None


def location_name_from_url(url):
    """Create location name from URL"""
    return gs.legalize_vector_name(name_from_url(url))


def main(options, unused_flags):
    """Download and copy location to destination"""
    url = options["url"]
    name = options["name"]
    database = options["path"]

    if not database:
        # Use the current database path.
        database = gs.gisenv()["GISDBASE"]
    if not name:
        name = location_name_from_url(url)
    destination = Path(database) / name

    if destination.exists():
        gs.fatal(
            _(
                "Project named <{name}> already exists in <{directory}>, download canceled"
            ).format(name=name, directory=database)
        )

    gs.message(_("Downloading and extracting..."))
    try:
        directory = download_and_extract(url)
        if not directory.is_dir():
            gs.fatal(_("Archive contains only one file and no mapset directories"))
        atexit.register(lambda: try_rmdir(directory))
    except DownloadError as error:
        gs.fatal(_("Unable to get the project: {error}").format(error=error))
    if not is_location_valid(directory):
        gs.verbose(_("Searching for valid project..."))
        # This in fact deal with location being on the third level of directories
        # thanks to how the extraction functions work (leaving out one level).
        result = find_location_in_directory(directory, recurse=1)
        if result:
            # We just want to show relative path in the message.
            # The relative path misses the root directory (name), because we
            # loose it on the way. (We should use parent directory to get the
            # full relative path, but the directory name is different now.
            # This is the consequence of how the extract functions work.)
            relative = os.path.relpath(result, start=directory)
            gs.verbose(
                _("Project found in a nested directory '{directory}'").format(
                    directory=relative
                )
            )
            directory = result
        else:
            # The list is similarly misleading as the relative path above
            # as it misses the root directory, but it still should be useful.
            files_and_dirs = os.listdir(directory)
            gs.fatal(
                _(
                    "The downloaded file is not a valid GRASS project."
                    " The extracted file contains these files and directories:"
                    "\n{files_and_dirs}"
                ).format(files_and_dirs=" ".join(files_and_dirs))
            )
    gs.verbose(_("Copying to final destination..."))
    shutil.copytree(src=directory, dst=destination)
    gs.message(_("Path to the project now <{path}>").format(path=destination))


if __name__ == "__main__":
    main(*gs.parser())
