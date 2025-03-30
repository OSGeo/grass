#!/usr/bin/env python3

############################################################################
#
# MODULE:       g.extension.all
#
# AUTHOR(S):    Martin Landa <landa.martin gmail.com>
#
# PURPOSE:      Rebuilds or removes locally installed GRASS Addons extensions
#
# COPYRIGHT:    (C) 2011-2013 by Martin Landa, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

# %module
# % label: Rebuilds or removes all locally installed GRASS Addons extensions.
# % description: By default only extensions built against different GIS Library are rebuilt.
# % keyword: general
# % keyword: installation
# % keyword: extensions
# %end
# %option
# % key: operation
# % type: string
# % description: Operation to be performed
# % required: no
# % options: rebuild,remove
# % answer: rebuild
# %end
# %flag
# % key: f
# % label: Force operation (required for removal)
# % end

import http
import os
import re
import sys

import xml.etree.ElementTree as ET

from urllib import request as urlrequest
from urllib.error import HTTPError, URLError

import grass.script as gs
from grass.exceptions import CalledModuleError

HEADERS = {
    "User-Agent": "Mozilla/5.0",
}
HTTP_STATUS_CODES = list(http.HTTPStatus)


def get_extensions():
    addon_base = os.getenv("GRASS_ADDON_BASE")
    if not addon_base:
        gs.fatal(_("%s not defined") % "GRASS_ADDON_BASE")
    fXML = os.path.join(addon_base, "modules.xml")
    if not os.path.exists(fXML):
        return []

    # read XML file
    fo = open(fXML)
    try:
        tree = ET.fromstring(fo.read())
    except Exception as e:
        gs.error(_("Unable to parse metadata file: %s") % e)
        fo.close()
        return []

    fo.close()

    libgis_rev = gs.version()["libgis_revision"]
    ret = []
    for tnode in tree.findall("task"):
        gnode = tnode.find("libgis")
        if gnode is not None and gnode.get("revision", "") != libgis_rev:
            ret.append(tnode.get("name"))

    return ret


def urlopen(url, *args, **kwargs):
    """Wrapper around urlopen. Same function as 'urlopen', but with the
    ability to define headers.
    """
    request = urlrequest.Request(url, headers=HEADERS)
    return urlrequest.urlopen(request, *args, **kwargs)


def download_modules_xml_file(url, response_format, *args, **kwargs):
    """Generates JSON file containing the download URLs of the official
    Addons

    :param str url: url address
    :param str response_format: content type

    :return response: urllib.request.urlopen response object or None
    """
    try:
        response = urlopen(url, *args, **kwargs)

        if response.code != 200:
            index = HTTP_STATUS_CODES.index(response.code)
            desc = HTTP_STATUS_CODES[index].description
            gs.fatal(
                _(
                    "Download file from <{url}>, return status code {code}, {desc}"
                ).format(
                    url=url,
                    code=response.code,
                    desc=desc,
                ),
            )
        if response_format not in response.getheader("Content-Type"):
            gs.fatal(
                _(
                    "Wrong file format downloaded. "
                    "Check url <{url}>. Allowed file format is "
                    "{response_format}."
                ).format(
                    url=url,
                    response_format=response_format,
                ),
            )
        return response

    except HTTPError as err:
        if err.code == 404:
            gs.fatal(
                _(
                    "The download of the modules.xml file "
                    "from the server was not successful. "
                    "File on the server <{url}> doesn't "
                    "exists."
                ).format(url=url),
            )
        else:
            return download_modules_xml_file(
                url=url,
                response_format=response_format,
            )
    except URLError:
        gs.fatal(
            _("Download file from <{url}>, failed. Check internet connection.").format(
                url=url,
            ),
        )


def find_addon_name(addons):
    """Find correct addon name if addon is a multi-addon
    e.g. wx.metadata contains multiple modules g.gui.cswbrowser etc.

    Examples:
    - for the g.gui.cswbrowser module the wx.metadata addon name is
    returned
    - for the i.sentinel.download module the i.sentinel addon name is
    returned
    etc.

    :param list addons: list of individual addon modules to be reinstalled

    :return set result: set of unique addon names to be reinstalled
    """
    grass_version = os.getenv("GRASS_VERSION", "unknown")
    if grass_version != "unknown":
        major, minor, patch = grass_version.split(".")
    else:
        gs.fatal(_("Unable to get GRASS GIS version."))
    url = "https://grass.osgeo.org/addons/grass{major}/modules.xml".format(
        major=major,
    )
    response = download_modules_xml_file(
        url=url,
        response_format="application/xml",
    )
    tree = ET.fromstring(response.read())
    result = []
    for addon in addons:
        found = False
        addon_pattern = re.compile(r".*{}$".format(addon))
        for i in tree:
            for f in i.findall(".//binary/file"):
                if re.match(addon_pattern, f.text):
                    result.append(i.attrib["name"])
                    found = True
                    break
        if not found:
            gs.warning(
                _(
                    "The <{}> addon cannot be reinstalled. "
                    "Addon wasn't found among the official addons."
                ).format(addon),
            )
    return set(result)


def main():
    remove = options["operation"] == "remove"
    if remove or flags["f"]:
        extensions = gs.read_command("g.extension", quiet=True, flags="a").splitlines()
    else:
        extensions = get_extensions()

    if not extensions:
        if remove:
            gs.info(_("No extension found. Nothing to remove."))
        else:
            gs.info(
                _("Nothing to rebuild. Rebuilding process can be forced with -f flag.")
            )
        return 0

    if remove and not flags["f"]:
        gs.message(_("List of extensions to be removed:"))
        print(os.linesep.join(extensions))
        gs.message(
            _("You must use the force flag (-f) to actually remove them. Exiting.")
        )
        return 0

    for ext in find_addon_name(addons=extensions):
        gs.message("-" * 60)
        if remove:
            gs.message(_("Removing extension <%s>...") % ext)
        else:
            gs.message(_("Reinstalling extension <%s>...") % ext)
        gs.message("-" * 60)
        if remove:
            operation = "remove"
            operation_flags = "f"
        else:
            operation = "add"
            operation_flags = ""
        try:
            gs.run_command(
                "g.extension", flags=operation_flags, extension=ext, operation=operation
            )
        except CalledModuleError:
            gs.error(_("Unable to process extension:%s") % ext)

    return 0


if __name__ == "__main__":
    options, flags = gs.parser()
    sys.exit(main())
