"""
Checking objects in a GRASS GIS Spatial Database

(C) 2020 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
"""


import os
import sys
import datetime
from pathlib import Path
from grass.script import gisenv
import grass.script as gs
import glob


def subproject_exists(database, project, subproject):
    """Returns True whether subproject path exists."""
    project_path = os.path.join(database, project)
    subproject_path = os.path.join(project_path, subproject)
    if os.path.exists(subproject_path):
        return True
    return False


def project_exists(database, project):
    """Returns True whether project path exists."""
    project_path = os.path.join(database, project)
    if os.path.exists(project_path):
        return True
    return False


# TODO: distinguish between valid for getting maps and usable as current
# https://lists.osgeo.org/pipermail/grass-dev/2016-September/082317.html
# interface created according to the current usage
def is_subproject_valid(subproject_path):
    """Return True if GRASS Subproject is valid"""
    # WIND is created from DEFAULT_WIND by `g.region -d` and functions
    # or modules which create a new subproject. Most modules will fail if
    # WIND doesn't exist (assuming that neither GRASS_REGION nor
    # WIND_OVERRIDE environmental variables are set).
    return os.access(os.path.join(subproject_path, "WIND"), os.R_OK)


def is_project_valid(database, project):
    """Return True if GRASS Project is valid

    :param database: Path to GRASS GIS database directory
    :param project: name of a Project
    """
    # DEFAULT_WIND file should not be required until you do something
    # that actually uses them. The check is just a heuristic; a directory
    # containing a PERMANENT/DEFAULT_WIND file is probably a GRASS
    # project, while a directory lacking it probably isn't.
    return os.access(
        os.path.join(database, project, "PERMANENT", "DEFAULT_WIND"), os.F_OK
    )


def is_subproject_current(database, project, subproject):
    genv = gisenv()
    if (database == genv['GISDBASE'] and
            project == genv['LOCATION_NAME'] and
            subproject == genv['MAPSET']):
        return True
    return False


def is_project_current(database, project):
    genv = gisenv()
    if (database == genv['GISDBASE'] and
            project == genv['LOCATION_NAME']):
        return True
    return False


def is_current_user_subproject_owner(subproject_path):
    """Returns True if subproject owner is the current user.
    On Windows it always returns True."""
    # Note that this does account for libgis built with SKIP_MAPSET_OWN_CHK
    # which disables the ownerships check, i.e., even if it was build with the
    # skip, it still needs the env variable.
    if os.environ.get("GRASS_SKIP_MAPSET_OWNER_CHECK", None):
        # Subproject just needs to be accessible for writing.
        return os.access(subproject_path, os.W_OK)
    # Subproject needs to be owned by user.
    if sys.platform == 'win32':
        return True
    stat_info = os.stat(subproject_path)
    subproject_uid = stat_info.st_uid
    return subproject_uid == os.getuid()


def is_different_subproject_owner(subproject_path):
    """Returns True if subproject owner is different from the current user"""
    return not is_current_user_subproject_owner(subproject_path)


def get_subproject_owner(subproject_path):
    """Returns subproject owner name or None if owner name unknown.
    On Windows it always returns None."""
    if sys.platform == 'win32':
        return None
    try:
        path = Path(subproject_path)
        return path.owner()
    except KeyError:
        return None


def is_current_subproject_in_demoproject():
    return gisenv()['LOCATION_NAME'] == "world_latlong_wgs84"


def is_subproject_locked(subproject_path):
    """Check if the subproject is locked"""
    lock_name = ".gislock"
    lockfile = os.path.join(subproject_path, lock_name)
    return os.path.exists(lockfile)


def get_lockfile_if_present(database, project, subproject):
    """Return path to lock if present, None otherwise

    Returns the path as a string or None if nothing was found, so the
    return value can be used to test if the lock is present.
    """
    lock_name = ".gislock"
    lockfile = os.path.join(database, project, subproject, lock_name)
    if os.path.isfile(lockfile):
        return lockfile
    return None


def get_subproject_lock_info(subproject_path):
    """Get information about .gislock file.
    Assumes lock file exists, use is_subproject_locked to find out.
    Returns information as a dictionary with keys
    'owner' (None if unknown), 'lockpath', and 'timestamp'.
    """
    info = {}
    lock_name = ".gislock"
    info['lockpath'] = os.path.join(subproject_path, lock_name)
    try:
        info['owner'] = Path(info['lockpath']).owner()
    except KeyError:
        info['owner'] = None
    info['timestamp'] = (datetime.datetime.fromtimestamp(
        os.path.getmtime(info['lockpath']))).replace(microsecond=0)
    return info


def can_start_in_subproject(subproject_path, ignore_lock=False):
    """Check if a subproject from a gisrc file is usable for new session"""
    if not is_subproject_valid(subproject_path):
        return False
    if not is_current_user_subproject_owner(subproject_path):
        return False
    if not ignore_lock and is_subproject_locked(subproject_path):
        return False
    return True


def dir_contains_project(path):
    """Return True if directory *path* contains a valid project"""
    if not os.path.isdir(path):
        return False
    for name in os.listdir(path):
        if os.path.isdir(os.path.join(path, name)):
            if is_project_valid(path, name):
                return True
    return False


# basically checking project, possibly split into two functions
# (subproject one can call project one)
def get_subproject_invalid_reason(database, project, subproject, none_for_no_reason=False):
    """Returns a message describing what is wrong with the Subproject

    The goal is to provide the most suitable error message
    (rather than to do a quick check).

    :param database: Path to GRASS GIS database directory
    :param project: name of a Project
    :param subproject: name of a Subproject
    :returns: translated message
    """
    # Since we are trying to get the one most likely message, we need all
    # those return statements here.
    # pylint: disable=too-many-return-statements
    project_path = os.path.join(database, project)
    subproject_path = os.path.join(project_path, subproject)
    # first checking the project validity
    # perhaps a special set of checks with different messages mentioning subproject
    # will be needed instead of the same set of messages used for project
    project_msg = get_project_invalid_reason(
        database, project, none_for_no_reason=True
    )
    if project_msg:
        return project_msg
    # if project is valid, check subproject
    if subproject not in os.listdir(project_path):
        # TODO: remove the grass.py specific wording
        return _(
            "Subproject <{subproject}> doesn't exist in GRASS Project <{project}>"
        ).format(subproject=subproject, project=project)
    if not os.path.isdir(subproject_path):
        return _("<%s> is not a GRASS Subproject because it is not a directory") % subproject
    if not os.path.isfile(os.path.join(subproject_path, "WIND")):
        return (
            _(
                "<%s> is not a valid GRASS Subproject"
                " because it does not have a WIND file"
            )
            % subproject
        )
    # based on the is_subproject_valid() function
    if not os.access(os.path.join(subproject_path, "WIND"), os.R_OK):
        return (
            _(
                "<%s> is not a valid GRASS Subproject"
                " because its WIND file is not readable"
            )
            % subproject
        )
    # no reason for invalidity found (might be valid)
    if none_for_no_reason:
        return None
    return _(
        "Subproject <{subproject}> or Project <{project}> is invalid for an unknown reason"
    ).format(subproject=subproject, project=project)


def get_project_invalid_reason(database, project, none_for_no_reason=False):
    """Returns a message describing what is wrong with the Project

    The goal is to provide the most suitable error message
    (rather than to do a quick check).

    By default, when no reason is found, a message about unknown reason is
    returned. This applies also to the case when this function is called on
    a valid project (e.g. as a part of larger investigation).
    ``none_for_no_reason=True`` allows the function to be used as part of other
    diagnostic. When this function fails to find reason for invalidity, other
    the caller can continue the investigation in their context.

    :param database: Path to GRASS GIS database directory
    :param project: name of a Project
    :param none_for_no_reason: When True, return None when reason is unknown
    :returns: translated message or None
    """
    project_path = os.path.join(database, project)
    permanent_path = os.path.join(project_path, "PERMANENT")

    # directory
    if not os.path.exists(project_path):
        return _("Project <%s> doesn't exist") % project_path
    # permament subproject
    if "PERMANENT" not in os.listdir(project_path):
        return (
            _(
                "<%s> is not a valid GRASS Project"
                " because PERMANENT Subproject is missing"
            )
            % project_path
        )
    if not os.path.isdir(permanent_path):
        return (
            _(
                "<%s> is not a valid GRASS Project"
                " because PERMANENT is not a directory"
            )
            % project_path
        )
    # partially based on the is_project_valid() function
    if not os.path.isfile(os.path.join(permanent_path, "DEFAULT_WIND")):
        return (
            _(
                "<%s> is not a valid GRASS Project"
                " because PERMANENT Subproject does not have a DEFAULT_WIND file"
                " (default computational region)"
            )
            % project_path
        )
    # no reason for invalidity found (might be valid)
    if none_for_no_reason:
        return None
    return _("Project <{project_path}> is invalid for an unknown reason").format(
        project_path=project_path
    )


def get_project_invalid_suggestion(database, project):
    """Return suggestion what to do when specified project is not valid

    It gives suggestion when:
     * A subproject was specified instead of a project.
     * A GRASS database was specified instead of a project.
    """
    project_path = os.path.join(database, project)
    # a common error is to use subproject instead of project,
    # if that's the case, include that info into the message
    if is_subproject_valid(project_path):
        return _(
            "<{project}> looks like a subproject, not a project."
            " Did you mean just <{one_dir_up}>?"
        ).format(project=project, one_dir_up=database)
    # confusion about what is database and what is project
    if dir_contains_project(project_path):
        return _(
            "It looks like <{project}> contains projects."
            " Did you mean to specify one of them?"
        ).format(project=project)
    return None


def get_subproject_name_invalid_reason(database, project, subproject_name):
    """Get reasons why subproject name is not valid.

    It gets reasons when:
     * Name is not valid.
     * Name is reserved for OGR layers.
     * Subproject in the same path already exists.

    Returns message as string if there was a reason, otherwise None.
    """
    message = None
    subproject_path = os.path.join(database, project, subproject_name)

    # Check if subproject name is valid
    if not gs.legal_name(subproject_name):
        message = _(
            "Name '{}' is not a valid name for project or subproject. "
            "Please use only ASCII characters excluding characters {} "
            "and space.").format(subproject_name, '/"\'@,=*~')
    # Check reserved subproject name
    elif subproject_name.lower() == 'ogr':
        message = _(
            "Name '{}' is reserved for direct "
            "read access to OGR layers. Please use "
            "another name for your subproject.").format(subproject_name)
    # Check whether subproject exists
    elif subproject_exists(database, project, subproject_name):
        message = _(
            "Subproject  <{subproject}> already exists. Please consider using "
            "another name for your subproject.").format(subproject=subproject_path)

    return message


def get_project_name_invalid_reason(grassdb, project_name):
    """Get reasons why project name is not valid.

    It gets reasons when:
     * Name is not valid.
     * Project in the same path already exists.

    Returns message as string if there was a reason, otherwise None.
    """
    message = None
    project_path = os.path.join(grassdb, project_name)

    # Check if subproject name is valid
    if not gs.legal_name(project_name):
        message = _(
            "Name '{}' is not a valid name for project or subproject. "
            "Please use only ASCII characters excluding characters {} "
            "and space.").format(project_name, '/"\'@,=*~')
    # Check whether project exists
    elif project_exists(grassdb, project_name):
        message = _(
            "Project  <{project}> already exists. Please consider using "
            "another name for your project.").format(project=project_path)

    return message


def is_subproject_name_valid(database, project, subproject_name):
    """Check if subproject name is valid.

    Returns True if subproject name is valid, otherwise False.
    """
    return gs.legal_name(subproject_name) and subproject_name.lower() != "ogr" and not \
        subproject_exists(database, project, subproject_name)


def is_project_name_valid(database, project_name):
    """Check if project name is valid.

    Returns True if project name is valid, otherwise False.
    """
    return gs.legal_name(project_name) and not \
        project_exists(database, project_name)


def get_reasons_subprojects_not_removable(subprojects, check_permanent):
    """Get reasons why subprojects cannot be removed.

    Parameter *subprojects* is a list of tuples (database, project, subproject).
    Parameter *check_permanent* is True of False. It depends on whether
    we want to check for permanent subproject or not.

    Returns messages as list if there were any failed checks, otherwise empty list.
    """
    messages = []
    for grassdb, project, subproject in subprojects:
        message = get_reason_subproject_not_removable(grassdb, project,
                                                  subproject, check_permanent)
        if message:
            messages.append(message)
    return messages


def get_reason_subproject_not_removable(grassdb, project, subproject, check_permanent):
    """Get reason why one subproject cannot be removed.

    Parameter *check_permanent* is True of False. It depends on whether
    we want to check for permanent subproject or not.

    Returns message as string if there was failed check, otherwise None.
    """
    message = None
    subproject_path = os.path.join(grassdb, project, subproject)

    # Check if subproject is permanent
    if check_permanent and subproject == "PERMANENT":
        message = _("Subproject <{subproject}> is required for a valid project.").format(
            subproject=subproject_path)
    # Check if subproject is current
    elif is_subproject_current(grassdb, project, subproject):
        message = _("Subproject <{subproject}> is the current subproject.").format(
            subproject=subproject_path)
    # Check whether subproject is in use
    elif is_subproject_locked(subproject_path):
        message = _("Subproject <{subproject}> is in use.").format(
            subproject=subproject_path)
    # Check whether subproject is owned by different user
    elif is_different_subproject_owner(subproject_path):
        message = _("Subproject <{subproject}> is owned by a different user.").format(
            subproject=subproject_path)

    return message


def get_reasons_projects_not_removable(projects):
    """Get reasons why projects cannot be removed.

    Parameter *projects* is a list of tuples (database, project).

    Returns messages as list if there were any failed checks, otherwise empty list.
    """
    messages = []
    for grassdb, project in projects:
        messages += get_reasons_project_not_removable(grassdb, project)
    return messages


def get_reasons_project_not_removable(grassdb, project):
    """Get reasons why one project cannot be removed.

    Returns messages as list if there were any failed checks, otherwise empty list.
    """
    messages = []
    project_path = os.path.join(grassdb, project)

    # Check if project is current
    if is_project_current(grassdb, project):
        messages.append(_("Project <{project}> is the current project.").format(
            project=project_path))
        return messages

    # Find subprojects in particular project
    tmp_gisrc_file, env = gs.create_environment(grassdb, project, 'PERMANENT')
    env['GRASS_SKIP_MAPSET_OWNER_CHECK'] = '1'

    g_subprojects = gs.read_command(
        'g.subprojects',
        flags='l',
        separator='comma',
        quiet=True,
        env=env).strip().split(',')

    # Append to the list of tuples
    subprojects = []
    for g_subproject in g_subprojects:
        subprojects.append((grassdb, project, g_subproject))

    # Concentenate both checks
    messages += get_reasons_subprojects_not_removable(subprojects, check_permanent=False)

    gs.try_remove(tmp_gisrc_file)
    return messages


def get_reasons_grassdb_not_removable(grassdb):
    """Get reasons why one grassdb cannot be removed.

    Returns messages as list if there were any failed checks, otherwise empty list.
    """
    messages = []
    genv = gisenv()

    # Check if grassdb is current
    if grassdb == genv['GISDBASE']:
        messages.append(_("GRASS database <{grassdb}> is the current database.").format(
            grassdb=grassdb))
        return messages

    g_projects = get_list_of_projects(grassdb)

    # Append to the list of tuples
    projects = []
    for g_project in g_projects:
        projects.append((grassdb, g_project))
    messages = get_reasons_projects_not_removable(projects)

    return messages


def get_list_of_projects(dbase):
    """Get list of GRASS projects in given dbase

    :param dbase: GRASS database path

    :return: list of projects (sorted)
    """
    projects = list()
    for project in glob.glob(os.path.join(dbase, "*")):
        if os.path.join(
                project, "PERMANENT") in glob.glob(
                os.path.join(project, "*")):
            projects.append(os.path.basename(project))

    projects.sort(key=lambda x: x.lower())

    return projects
