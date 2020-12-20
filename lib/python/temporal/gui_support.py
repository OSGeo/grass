"""
GUI support functions


(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""
from .core import get_available_temporal_subprojects, init_dbif
from .factory import dataset_factory
import grass.script as gscript

###############################################################################


def tlist_grouped(type, group_type=False, dbif=None):
    """List of temporal elements grouped by subprojects.

    Returns a dictionary where the keys are subproject
    names and the values are lists of space time datasets in that
    subproject. Example:

    .. code-block:: python

        >>> import grass.temporalas tgis
        >>> tgis.tlist_grouped('strds')['PERMANENT']
        ['precipitation', 'temperature']

    :param type: element type (strds, str3ds, stvds)
    :param group_type: TBD

    :return: directory of subprojects/elements
    """
    result = {}
    dbif, connected = init_dbif(dbif)

    subproject = None
    if type == 'stds':
        types = ['strds', 'str3ds', 'stvds']
    else:
        types = [type]
    for type in types:
        try:
            tlist_result = tlist(type=type, dbif=dbif)
        except gscript.ScriptError as e:
            warning(e)
            continue

        for line in tlist_result:
            try:
                name, subproject = line.split('@')
            except ValueError:
                warning(_("Invalid element '%s'") % line)
                continue

            if subproject not in result:
                if group_type:
                    result[subproject] = {}
                else:
                    result[subproject] = []

            if group_type:
                if type in result[subproject]:
                    result[subproject][type].append(name)
                else:
                    result[subproject][type] = [name, ]
            else:
                result[subproject].append(name)

    if connected is True:
        dbif.close()

    return result

###############################################################################


def tlist(type, dbif=None):
    """Return a list of space time datasets of absolute and relative time

    :param type: element type (strds, str3ds, stvds)

    :return: a list of space time dataset ids
    """
    id = None
    sp = dataset_factory(type, id)
    dbif, connected = init_dbif(dbif)

    subprojects = get_available_temporal_subprojects()

    output = []
    temporal_type = ["absolute", 'relative']
    for type in temporal_type:
        # For each available subproject
        for subproject in subprojects.keys():
            # Table name
            if type == "absolute":
                table = sp.get_type() + "_view_abs_time"
            else:
                table = sp.get_type() + "_view_rel_time"

            # Create the sql selection statement
            sql = "SELECT id FROM " + table
            sql += " WHERE subproject = '%s'" % (subproject)
            sql += " ORDER BY id"

            dbif.execute(sql, subproject=subproject)
            rows = dbif.fetchall(subproject=subproject)

            # Append the ids of the space time datasets
            for row in rows:
                for col in row:
                    output.append(str(col))

    if connected is True:
        dbif.close()

    return output
