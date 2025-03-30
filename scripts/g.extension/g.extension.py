#!/usr/bin/env python3

############################################################################
#
# MODULE:       g.extension
# AUTHOR(S):    Markus Neteler (original shell script)
#               Martin Landa <landa.martin gmail com> (Pythonized & upgraded for GRASS 7)
#               Vaclav Petras <wenzeslaus gmail com> (support for general sources)
# PURPOSE:      Tool to download and install extensions into local installation
#
# COPYRIGHT:    (C) 2009-2021 by Markus Neteler, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
# TODO:         - update temporary workaround of using grass7 subdir of addon-repo, see
#                 https://github.com/OSGeo/grass-addons/issues/528
#               - add sudo support where needed (i.e. check first permission to write
#                 into $GISBASE directory)
#               - fix toolbox support in install_private_extension_xml()
#############################################################################

# %module
# % label: Maintains GRASS Addons extensions in local GRASS installation.
# % description: Downloads and installs extensions from GRASS Addons repository or other source into the local GRASS installation or removes installed extensions.
# % keyword: general
# % keyword: installation
# % keyword: extensions
# % keyword: addons
# % keyword: download
# %end

# %option
# % key: extension
# % type: string
# % key_desc: name
# % label: Name of extension to install or remove
# % description: Name of toolbox (set of extensions) when -t flag is given
# % required: yes
# %end
# %option
# % key: operation
# % type: string
# % description: Operation to be performed
# % required: yes
# % options: add,remove
# % answer: add
# %end
# %option
# % key: url
# % type: string
# % key_desc: url
# % label: URL or directory to get the extension from (supported only on Linux and Mac)
# % description: The official repository is used by default. User can specify a ZIP file, directory or a repository on common hosting services. If not identified, Subversion repository is assumed. See manual for all options.
# %end
# %option
# % key: prefix
# % type: string
# % key_desc: path
# % description: Prefix where to install extension (ignored when flag -s is given)
# % answer: $GRASS_ADDON_BASE
# % required: no
# %end
# %option
# % key: proxy
# % type: string
# % key_desc: proxy
# % description: Set the proxy with: "http=<value>,ftp=<value>"
# % required: no
# % multiple: yes
# %end
# %option
# % key: branch
# % type: string
# % key_desc: branch
# % description: Specific branch to fetch addon from (only used when fetching from git)
# % required: no
# % multiple: no
# %end

# %flag
# % key: l
# % description: List available extensions in the official GRASS GIS Addons repository
# % guisection: Print
# % suppress_required: yes
# %end
# %flag
# % key: c
# % description: List available extensions in the official GRASS GIS Addons repository including module description
# % guisection: Print
# % suppress_required: yes
# %end
# %flag
# % key: g
# % description: List available extensions in the official GRASS GIS Addons repository (shell script style)
# % guisection: Print
# % suppress_required: yes
# %end
# %flag
# % key: a
# % description: List locally installed extensions
# % guisection: Print
# % suppress_required: yes
# %end
# %flag
# % key: s
# % description: Install system-wide (may need system administrator rights)
# % guisection: Install
# %end
# %flag
# % key: d
# % description: Download source code and exit
# % guisection: Install
# %end
# %flag
# % key: i
# % description: Do not install new extension, just compile it
# % guisection: Install
# %end
# %flag
# % key: f
# % description: Force removal when uninstalling extension (operation=remove)
# % guisection: Remove
# %end
# %flag
# % key: t
# % description: Operate on toolboxes instead of single modules (experimental)
# % suppress_required: yes
# %end
# %flag
# % key: o
# % description: url refers to a fork of the official extension repository
# %end

# %rules
# % required: extension, -l, -c, -g, -a
# % exclusive: extension, -l, -c, -g
# % exclusive: extension, -l, -c, -a
# % requires: -o, url
# % requires: branch, url
# %end

# TODO: solve addon-extension(-module) confusion

import fileinput
import os
import codecs
import sys
import re
import atexit
import shutil
import zipfile
import tempfile
import json
import xml.etree.ElementTree as ET

if sys.version_info < (3, 8):
    from distutils.dir_util import copy_tree
else:
    from functools import partial

    copy_tree = partial(shutil.copytree, dirs_exist_ok=True)

from pathlib import Path
from subprocess import PIPE
from urllib import request as urlrequest
from urllib.error import HTTPError, URLError
from urllib.parse import urljoin, urlparse

# Get the XML parsing exceptions to catch. The behavior changed with Python 2.7
# and ElementTree 1.3.
from xml.parsers import expat  # TODO: works for any Python?

if hasattr(ET, "ParseError"):
    ETREE_EXCEPTIONS = (ET.ParseError, expat.ExpatError)
else:
    ETREE_EXCEPTIONS = expat.ExpatError

import grass.script as gs
from grass.script import task as gtask
from grass.script.utils import try_rmdir

# temp dir
REMOVE_TMPDIR = True
PROXIES = {}
HEADERS = {
    "User-Agent": "Mozilla/5.0",
}
GIT_URL = "https://github.com/OSGeo/grass-addons/"

# MAKE command
# GRASS Makefile are type of GNU Make and not BSD Make
# On FreeBSD (and other BSD and maybe unix) we have to
# use GNU Make program often "gmake" to distinct with the (bsd) "make"
if sys.platform.startswith("freebsd"):
    MAKE = "gmake"
else:
    MAKE = "make"


class GitAdapter:
    """
    Basic class for listing and downloading GRASS GIS AddOns using git

    """

    def __init__(
        self,
        addons=[],
        url="https://github.com/osgeo/grass-addons",
        git="git",
        working_directory=None,
        official_repository_structure=True,
        major_grass_version=None,
        branch=None,
        verbose=False,
        quiet=False,
    ):
        #: Attribute containing list of addons names
        self._addons = addons
        #: Attribute containing Git command name
        self._git = git
        #: Attribute containing the URL to the online repository
        self.url = url
        self.major_grass_version = major_grass_version
        #: Attribute flagging if the repository is structured like the official addons
        # repository
        self.official_repository_structure = official_repository_structure
        #: Attribute containing the path to the working directory where the repo is
        # cloned out to
        if working_directory:
            self.working_directory = Path(working_directory).absolute()
        else:
            self.working_directory = Path().absolute()

        # Check if working directory is writable
        self.__check_permissions()
        # Check if Git is installed
        self._is_git_installed()

        #: Attribute containing available branches
        self.branches = self._get_branch_list()
        #: Attribute containing the git version
        self.git_version = self._get_version()
        # Initialize the local copy
        self._initialize_clone()
        #: Attribute containing the default branch of the repository
        self.default_branch = self._get_default_branch()
        #: Attribute containing the branch used for checkout
        self.branch = self._set_branch(branch)
        #: Attribute containing list of addons in the repository with path to
        # directories
        self.addons = self._get_addons_list()

    def _get_version(self):
        """Get the installed git version"""
        git_version = gs.Popen([self._git, "--version"], stdout=PIPE, stderr=PIPE)
        git_version, stderr = git_version.communicate()
        if stderr:
            gs.fatal(
                _("Failed to get Git version.\n{}").format(
                    gs.decode(stderr),
                )
            )
        git_version = re.search(r"\d+.(\d+.\d+|\d+)", gs.decode(git_version))
        if not git_version:
            gs.fatal(_("Failed to get Git version."))
        git_version = git_version.group()
        if git_version.count(".") == 2:
            git_version = git_version.rsplit(".", 1)[0]
        return float(git_version)

    def _initialize_clone(self):
        """Get a minimal working copy of a git repository without content"""
        repo_directory = "grass_addons"
        if not self.working_directory.exists():
            self.working_directory.mkdir(exist_ok=True, parents=True)
        gs.call(
            [
                self._git,
                "clone",
                "-q",
                "--no-checkout",
                "--filter=blob:none",
                self.url,
                repo_directory,
            ],
            cwd=self.working_directory,
        )
        self.local_copy = self.working_directory / repo_directory

    def _is_git_installed(self):
        """Check if Git command is installed"""
        try:
            gs.call([self._git], stdout=PIPE)
        except OSError:
            gs.fatal(_("Could not found Git. Please install it."))

    def __check_permissions(self):
        """"""
        # Create working directory if it does not exist
        self.working_directory.mkdir(parents=True, exist_ok=True)
        # Check pemissions in case he workdir existed
        if not os.access(self.working_directory, os.W_OK):
            gs.fatal(
                _("Cannot write to working directory {}.").format(
                    self.working_directory
                )
            )

    def _get_branch_list(self):
        """Return commit hash reference and names for remote branches of
        a git repository

        :param url: URL to git repository, defaults to the official GRASS GIS
                    addon repository
        """
        branch_list = gs.Popen(
            [self._git, "ls-remote", "--heads", self.url],
            stdout=PIPE,
        )
        branch_list = gs.decode(branch_list.communicate()[0])
        return {
            branch.rsplit("/", 1)[-1]: branch.split("\t", 1)[0]
            for branch in branch_list.split("\n")
        }

    def _get_default_branch(self):
        """Return commit hash reference and names for remote branches of
        a git repository

        :param url: URL to git repository, defaults to the official GRASS GIS
                    addon repository
        """
        default_branch = gs.Popen(
            [self._git, "symbolic-ref", "refs/remotes/origin/HEAD"],
            cwd=self.local_copy,
            stdout=PIPE,
        )
        return gs.decode(default_branch.communicate()[0]).rstrip().rsplit("/", 1)[-1]

    def _get_version_branch(self):
        """Check if version branch for the current GRASS version exists,
        The method is only useful for repositories that follow the structure
        and concept of the official addon repository

        Returns None if no version branch is found."""
        version_branch = f"grass{self.major_grass_version}"

        return version_branch if version_branch in self.branches else None

    def _set_branch(self, branch_name):
        """Set the branch to check out to either:
        a) a user defined branch
        b) a version branch for repositories following the official addons repository
           structure
        c) the default branch of the repository
        """
        checkout_branch = None
        # Check user provided branch
        if branch_name:
            if branch_name not in self.branches:
                gs.fatal(
                    _("Branch <{branch}> not found in repository <{url}>").format(
                        branch=branch_name, url=self.url
                    )
                )
            else:
                checkout_branch = branch_name
        # Check version branch if relevant
        elif self.official_repository_structure:
            checkout_branch = self._get_version_branch()

        # Use default branch if none of the above are found
        return checkout_branch or self.default_branch

    def _get_addons_list(self):
        """Build a dictionary with addon name as key and path to directory with
        Makefile in repository"""
        file_list = gs.Popen(
            [self._git, "ls-tree", "--name-only", "-r", self.branch],
            cwd=self.local_copy,
            stdout=PIPE,
            stderr=PIPE,
        )
        file_list, stderr = file_list.communicate()
        if stderr:
            gs.fatal(
                _(
                    "Failed to get addons files list from the"
                    " Git repository <{repo_path}>.\n{error}"
                ).format(
                    repo_path=self.local_copy,
                    error=gs.decode(stderr),
                )
            )
        # Build addons dict
        addons_dict = {}
        addons = [f".*{addon}/Makefile\n" for addon in self._addons]
        addons_makefile_pattern = re.compile(
            rf"({'|'.join(addons)})",
        )
        addons_makefiles = re.findall(
            addons_makefile_pattern,
            gs.decode(file_list),
        )
        for addon in addons_makefiles:
            addon_dir = os.path.dirname(addon)
            addons_dict[os.path.basename(addon_dir)] = addon_dir.rstrip()
        return addons_dict

    def _addon_exists(self, addon_list):
        if not [self.addons[addon] for addon in addon_list if addon in self.addons]:
            gs.fatal(
                _("No addon named <{}> found in the repository.").format(
                    ", ".join(addon_list)
                )
            )

    def fetch_addons(self, addon_list, all_addons=False):
        self._addon_exists(addon_list)
        if addon_list:
            if self.git_version >= 2.25 and not all_addons:
                gs.call(
                    [self._git, "sparse-checkout", "init", "--cone"],
                    cwd=self.local_copy,
                )
                gs.call(
                    [
                        self._git,
                        "sparse-checkout",
                        "set",
                        *[self.addons[addon] for addon in addon_list],
                    ],
                    cwd=self.local_copy,
                )
        gs.call(
            [self._git, "checkout", self.branch],
            cwd=self.local_copy,
        )

    def get_addons_src_code_git_repo_url_path(self):
        """Get addons official GitHub repository source code URL path

        :return dict addons_url: dictionary of addons official GitHub
                                 repository source code URL path
        """
        addons_url = {}
        for addon in self.addons:
            addons_url[addon] = urljoin(
                self.url,
                urljoin(
                    "tree/",
                    urljoin(
                        f"{self.branch}/",
                        self.addons[addon],
                    ),
                ),
            )
        return addons_url


def replace_shebang_win(python_file):
    """
    Replaces "python" with "python3" in python files
    using UTF8 encoding on MS Windows
    """

    cur_dir = os.path.dirname(python_file)
    tmp_name = os.path.join(cur_dir, gs.tempname(12))

    with (
        codecs.open(python_file, "r", encoding="utf8") as in_file,
        codecs.open(tmp_name, "w", encoding="utf8") as out_file,
    ):
        for line in in_file:
            new_line = line.replace(
                "#!/usr/bin/env python\n", "#!/usr/bin/env python3\n"
            )
            out_file.write(new_line)

    os.remove(python_file)  # remove original
    os.rename(tmp_name, python_file)  # rename temp to original name


def urlretrieve(url, filename, *args, **kwargs):
    """Same function as 'urlretrieve', but with the ability to
    define headers.
    """
    request = urlrequest.Request(url, headers=HEADERS)
    response = urlrequest.urlopen(request, *args, **kwargs)
    Path(filename).write_bytes(response.read())


def urlopen(url, *args, **kwargs):
    """Wrapper around urlopen. Same function as 'urlopen', but with the
    ability to define headers.
    """
    request = urlrequest.Request(url, headers=HEADERS)
    return urlrequest.urlopen(request, *args, **kwargs)


def get_version_branch(major_version):
    """Check if version branch for the current GRASS version exists,
    if not, take branch for the previous version
    For the official repo we assume that at least one version branch is present"""
    version_branch = f"grass{major_version}"
    if sys.platform == "win32":
        return version_branch
    branch = gs.Popen(
        ["git", "ls-remote", "--heads", GIT_URL, f"refs/heads/{version_branch}"],
        stdout=PIPE,
        stderr=PIPE,
    )
    branch, stderr = branch.communicate()
    if stderr:
        gs.fatal(
            _(
                "Failed to get branch from the Git repository <{repo_path}>.\n{error}"
            ).format(
                repo_path=GIT_URL,
                error=gs.decode(stderr),
            )
        )
    branch = gs.decode(branch)
    if version_branch not in branch:
        version_branch = "grass{}".format(int(major_version) - 1)
    return version_branch


def get_default_branch(full_url):
    """Get default branch for repository in known hosting services
    (currently only implemented for github, gitlab and bitbucket API)
    In all other cases "main" is used as default"""
    # Parse URL
    url_parts = urlparse(full_url)
    # Get organization and repository component
    try:
        organization, repository = url_parts.path.split("/")[1:3]
    except URLError:
        gs.fatal(
            _("Cannot retrieve organization and repository from URL: <{}>.").format(
                full_url
            )
        )
    # Construct API call and retrieve default branch
    api_calls = {
        "github.com": f"https://api.github.com/repos/{organization}/{repository}",
        "gitlab.com": f"https://gitlab.com/api/v4/projects/{organization}%2F{repository}",  # noqa: E501
        "bitbucket.org": f"https://api.bitbucket.org/2.0/repositories/{organization}/{repository}/branching-model?",  # noqa: E501
    }
    # Try to get default branch via API. The API call is known to fail a) if the
    # full_url does not belong to an implemented hosting service or b) if the rate
    # limit of the API is exceeded
    try:
        req = urlrequest.urlopen(api_calls.get(url_parts.netloc))
        content = json.loads(req.read())
        # For github and gitlab
        default_branch = content.get("default_branch")
        # For bitbucket
        if not default_branch:
            default_branch = content.get("development").get("name")
    except URLError:
        default_branch = "main"
    return default_branch


def etree_fromfile(filename):
    """Create XML element tree from a given file name"""
    return ET.fromstring(Path(filename).read_text())


def etree_fromurl(url):
    """Create XML element tree from a given URL"""
    try:
        file_ = urlopen(url)
    except URLError:
        gs.fatal(
            _(
                "Download file from <{url}>,"
                " failed. File is not on the server or"
                " check your internet connection."
            ).format(url=url),
        )
    return ET.fromstring(file_.read())


def check_progs():
    """Check if the necessary programs are available"""
    # git to be tested once supported instead of `svn`
    for prog in (MAKE, "gcc", "git"):
        if not gs.find_program(prog, "--help"):
            gs.fatal(_("'%s' required. Please install '%s' first.") % (prog, prog))


# expand prefix to class name


def expand_module_class_name(class_letters):
    """Convert module class (family) letter or letters to class (family) name

    The letter or letters are used in module names, e.g. r.slope.aspect.
    The names are used in directories in Addons but also in the source code.

    >>> expand_module_class_name("r")
    'raster'
    >>> expand_module_class_name("v")
    'vector'
    """
    name = {
        "d": "display",
        "db": "db",
        "g": "general",
        "i": "imagery",
        "m": "misc",
        "ps": "postscript",
        "p": "paint",
        "r": "raster",
        "r3": "raster3d",
        "s": "sites",
        "t": "temporal",
        "v": "vector",
        "wx": "gui/wxpython",
    }

    return name.get(class_letters, class_letters)


def get_module_class_name(module_name):
    """Return class (family) name for a module

    The names are used in directories in Addons but also in the source code.

    >>> get_module_class_name("r.slope.aspect")
    'raster'
    >>> get_module_class_name("v.to.rast")
    'vector'
    """
    classchar = module_name.split(".", 1)[0]
    return expand_module_class_name(classchar)


def get_installed_extensions(force=False):
    """Get list of installed extensions or toolboxes (if -t is set)"""
    if flags["t"]:
        return get_installed_toolboxes(force)

    # TODO: extension != module
    return get_installed_modules(force)


def list_installed_extensions(toolboxes=False):
    """List installed extensions"""
    elist = get_installed_extensions()
    if elist:
        if toolboxes:
            gs.message(_("List of installed extensions (toolboxes):"))
        else:
            gs.message(_("List of installed extensions (modules):"))
        sys.stdout.write("\n".join(elist))
        sys.stdout.write("\n")
    elif toolboxes:
        gs.info(_("No extension (toolbox) installed"))
    else:
        gs.info(_("No extension (module) installed"))


def get_installed_toolboxes(force=False):
    """Get list of installed toolboxes

    Writes toolboxes file if it does not exist.
    Creates a new toolboxes file if it is not possible
    to read the current one.
    """
    xml_file = os.path.join(options["prefix"], "toolboxes.xml")
    if not os.path.exists(xml_file):
        write_xml_toolboxes(xml_file)
    # read XML file
    try:
        tree = etree_fromfile(xml_file)
    except ETREE_EXCEPTIONS + (OSError, IOError):
        os.remove(xml_file)
        write_xml_toolboxes(xml_file)
        return []
    ret = []
    for tnode in tree.findall("toolbox"):
        ret.append(tnode.get("code"))
    return ret


def get_installed_modules(force=False):
    """Get list of installed modules.

    Writes modules file if it does not exist and *force* is set to ``True``.
    Creates a new modules file if it is not possible
    to read the current one.
    """
    xml_file = os.path.join(options["prefix"], "modules.xml")
    if not os.path.exists(xml_file):
        if force:
            write_xml_modules(xml_file)
        else:
            gs.debug("No addons metadata file available", 1)
        return []
    # read XML file
    try:
        tree = etree_fromfile(xml_file)
    except ETREE_EXCEPTIONS + (OSError, IOError):
        os.remove(xml_file)
        write_xml_modules(xml_file)
        return []
    ret = []
    for tnode in tree.findall("task"):
        if flags["g"]:
            desc, keyw = get_optional_params(tnode)
            ret.extend(
                (
                    "name={0}".format(tnode.get("name").strip()),
                    "description={0}".format(desc),
                    "keywords={0}".format(keyw),
                    "executables={0}".format(",".join(get_module_executables(tnode))),
                )
            )
        else:
            ret.append(tnode.get("name").strip())

    return ret


# list extensions (read XML file from gs.osgeo.org/addons)


def list_available_extensions(url):
    """List available extensions/modules or toolboxes (if -t is given)

    For toolboxes it lists also all modules.
    """
    gs.debug("list_available_extensions(url={0})".format(url))
    if flags["t"]:
        gs.message(_("List of available extensions (toolboxes):"))
        tlist = get_available_toolboxes(url)
        tkeys = sorted(tlist.keys())
        for toolbox_code in tkeys:
            toolbox_data = tlist[toolbox_code]
            if flags["g"]:
                print("toolbox_name=" + toolbox_data["name"])
                print("toolbox_code=" + toolbox_code)
            else:
                print("%s (%s)" % (toolbox_data["name"], toolbox_code))
            if flags["c"] or flags["g"]:
                list_available_modules(url, toolbox_data["modules"])
            elif toolbox_data["modules"]:
                print(os.linesep.join(["* " + x for x in toolbox_data["modules"]]))
    else:
        gs.message(_("List of available extensions (modules):"))
        # TODO: extensions with several modules + lib
        list_available_modules(url)


def get_available_toolboxes(url):
    """Return toolboxes available in the repository"""
    tdict = {}
    url += "toolboxes.xml"
    try:
        tree = etree_fromurl(url)
        for tnode in tree.findall("toolbox"):
            mlist = []
            clist = []
            tdict[tnode.get("code")] = {
                "name": tnode.get("name"),
                "correlate": clist,
                "modules": mlist,
            }

            for cnode in tnode.findall("correlate"):
                clist.append(cnode.get("name"))

            for mnode in tnode.findall("task"):
                mlist.append(mnode.get("name"))
    except (HTTPError, OSError):
        gs.fatal(_("Unable to fetch addons metadata file"))

    return tdict


def get_toolbox_extensions(url, name):
    """Get extensions inside a toolbox in toolbox file at given URL

    :param url: URL of the directory (file name will be attached)
    :param name: toolbox name
    """
    # dictionary of extensions
    edict = {}

    url += "toolboxes.xml"

    try:
        tree = etree_fromurl(url)
        for tnode in tree.findall("toolbox"):
            if name == tnode.get("code"):
                for enode in tnode.findall("task"):
                    # extension name
                    ename = enode.get("name")
                    edict[ename] = {}
                    # list of modules installed by this extension
                    edict[ename]["mlist"] = []
                    # list of files installed by this extension
                    edict[ename]["flist"] = []
                break
    except (HTTPError, OSError):
        gs.fatal(_("Unable to fetch addons metadata file"))

    return edict


def get_module_files(mnode):
    """Return list of module files

    :param mnode: XML node for a module
    """
    flist = []
    if mnode.find("binary") is None:
        return flist
    for file_node in mnode.find("binary").findall("file"):
        filepath = file_node.text
        flist.append(filepath)

    return flist


def get_module_executables(mnode):
    """Return list of module executables

    :param mnode: XML node for a module
    """
    flist = []
    for filepath in get_module_files(mnode):
        if filepath.startswith(options["prefix"] + os.path.sep + "bin") or (
            sys.platform != "win32"
            and filepath.startswith(options["prefix"] + os.path.sep + "scripts")
        ):
            filename = os.path.basename(filepath)
            if sys.platform == "win32":
                filename = os.path.splitext(filename)[0]
            flist.append(filename)

    return flist


def get_optional_params(mnode):
    """Return description and keywords of a module as a tuple

    :param mnode: XML node for a module
    """
    try:
        desc = mnode.find("description").text
    except AttributeError:
        desc = ""
    if desc is None:
        desc = ""
    try:
        keyw = mnode.find("keywords").text
    except AttributeError:
        keyw = ""
    if keyw is None:
        keyw = ""

    return desc, keyw


def list_available_modules(url, mlist=None):
    """List modules available in the repository

    Tries to use XML metadata file first. Fallbacks to HTML page with a list.

    :param url: URL of the directory (file name will be attached)
    :param mlist: list only modules in this list
    """
    file_url = url + "modules.xml"
    gs.debug("url=%s" % file_url, 1)
    try:
        tree = etree_fromurl(file_url)
    except ETREE_EXCEPTIONS:
        gs.warning(
            _(
                "Unable to parse '{url}'. Trying to scan"
                " Git repository (may take some time)..."
            ).format(url=file_url)
        )
        list_available_extensions_svn(url)
        return
    except (HTTPError, URLError, OSError):
        list_available_extensions_svn(url)
        return

    for mnode in tree.findall("task"):
        name = mnode.get("name").strip()
        if mlist and name not in mlist:
            continue
        if flags["c"] or flags["g"]:
            desc, keyw = get_optional_params(mnode)

        if flags["g"]:
            print("name=" + name)
            print("description=" + desc)
            print("keywords=" + keyw)
        elif flags["c"]:
            if mlist:
                print("*", end="")
            print(name + " - " + desc)
        else:
            print(name)


# TODO: this is now broken/dead code, SVN is basically not used
# fallback for Trac should parse Trac HTML page
# this might be useful for potential SVN repos or anything
# which would list the extensions/addons as list
# TODO: fail when nothing is accessible
def list_available_extensions_svn(url):
    """List available extensions from HTML given by URL

    Filename is generated based on the module class/family.
    This works well for the structure which is in grass-addons repository.

    ``<li><a href=...`` is parsed to find module names.
    This works well for HTML page generated by Subversion.

    :param url: a directory URL (filename will be attached)
    """
    gs.debug("list_available_extensions_svn(url=%s)" % url, 2)
    gs.message(
        _(
            "Fetching list of extensions from"
            " GRASS-Addons SVN repository (be patient)..."
        )
    )
    pattern = re.compile(r'(<li><a href=".+">)(.+)(</a></li>)', re.IGNORECASE)

    if flags["c"]:
        gs.warning(_("Flag 'c' ignored, addons metadata file not available"))
    if flags["g"]:
        gs.warning(_("Flag 'g' ignored, addons metadata file not available"))

    prefixes = ["d", "db", "g", "i", "m", "ps", "p", "r", "r3", "s", "t", "v"]
    for prefix in prefixes:
        modclass = expand_module_class_name(prefix)
        gs.verbose(_("Checking for '%s' modules...") % modclass)

        # construct a full URL of a file
        file_url = "%s/%s" % (url, modclass)
        gs.debug("url = %s" % file_url, debug=2)
        try:
            file_ = urlopen(url)
        except (HTTPError, OSError):
            gs.debug(_("Unable to fetch '%s'") % file_url, debug=1)
            continue

        for line in file_.readlines():
            # list extensions
            sline = pattern.search(line)
            if not sline:
                continue
            name = sline.group(2).rstrip("/")
            if name.split(".", 1)[0] == prefix:
                print(name)

    # get_wxgui_extensions(url)


# TODO: this is a dead code, not clear why not used, but seems not needed
def get_wxgui_extensions(url):
    """Return list of extensions/addons in wxGUI directory at given URL

    :param url: a directory URL (filename will be attached)
    """
    mlist = []
    gs.debug(
        "Fetching list of wxGUI extensions from "
        "GRASS-Addons SVN repository (be patient)..."
    )
    pattern = re.compile(r'(<li><a href=".+">)(.+)(</a></li>)', re.IGNORECASE)
    gs.verbose(_("Checking for '%s' modules...") % "gui/wxpython")

    # construct a full URL of a file
    url = "%s/%s" % (url, "gui/wxpython")
    gs.debug("url = %s" % url, debug=2)
    file_ = urlopen(url)
    if not file_:
        gs.warning(_("Unable to fetch '%s'") % url)
        return None

    for line in file_.readlines():
        # list extensions
        sline = pattern.search(line)
        if not sline:
            continue
        name = sline.group(2).rstrip("/")
        if name not in {"..", "Makefile"}:
            mlist.append(name)

    return mlist


def cleanup():
    """Cleanup after the downloads and copilation"""
    if REMOVE_TMPDIR:
        try_rmdir(TMPDIR)


def write_xml_modules(name, tree=None):
    """Write element tree as a modules metadata file

    If the *tree* is not given, an empty file is created.

    :param name: file name
    :param tree: XML element tree
    """
    file_ = open(name, "w")
    file_.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    file_.write('<!DOCTYPE task SYSTEM "grass-addons.dtd">\n')
    file_.write(f'<addons version="{VERSION[0]}">\n')

    libgis_revison = gs.version()["libgis_revision"]
    if tree is not None:
        for tnode in tree.findall("task"):
            indent = 4
            file_.write('%s<task name="%s">\n' % (" " * indent, tnode.get("name")))
            indent += 4
            file_.write(
                "%s<description>%s</description>\n"
                % (" " * indent, tnode.find("description").text)
            )
            file_.write(
                "%s<keywords>%s</keywords>\n"
                % (" " * indent, tnode.find("keywords").text)
            )
            bnode = tnode.find("binary")
            if bnode is not None:
                file_.write("%s<binary>\n" % (" " * indent))
                indent += 4
                file_.writelines(
                    "%s<file>%s</file>\n"
                    % (" " * indent, os.path.join(options["prefix"], fnode.text))
                    for fnode in bnode.findall("file")
                )
                indent -= 4
                file_.write("%s</binary>\n" % (" " * indent))
            file_.write('%s<libgis revision="%s" />\n' % (" " * indent, libgis_revison))
            indent -= 4
            file_.write("%s</task>\n" % (" " * indent))

    file_.write("</addons>\n")
    file_.close()


def write_xml_extensions(name, tree=None):
    """Write element tree as a modules metadata file

    If the *tree* is not given, an empty file is created.

    :param name: file name
    :param tree: XML element tree
    """
    file_ = open(name, "w")
    file_.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    file_.write('<!DOCTYPE task SYSTEM "grass-addons.dtd">\n')
    file_.write(f'<addons version="{VERSION[0]}">\n')

    libgis_revison = gs.version()["libgis_revision"]
    if tree is not None:
        for tnode in tree.findall("task"):
            indent = 4
            # extension name
            file_.write('%s<task name="%s">\n' % (" " * indent, tnode.get("name")))
            indent += 4

            # file_.write(
            #     "%s<description>%s</description>\n"
            #     % (" " * indent, tnode.find("description").text)
            # )
            # file_.write(
            #     "%s<keywords>%s</keywords>\n"
            #     % (" " * indent, tnode.find("keywords").text)
            # )

            # extension files
            bnode = tnode.find("binary")
            if bnode is not None:
                file_.write("%s<binary>\n" % (" " * indent))
                indent += 4
                file_.writelines(
                    "%s<file>%s</file>\n"
                    % (" " * indent, os.path.join(options["prefix"], fnode.text))
                    for fnode in bnode.findall("file")
                )
                indent -= 4
                file_.write("%s</binary>\n" % (" " * indent))
            # extension modules
            mnode = tnode.find("modules")
            if mnode is not None:
                file_.write("%s<modules>\n" % (" " * indent))
                indent += 4
                file_.writelines(
                    "%s<module>%s</module>\n" % (" " * indent, fnode.text)
                    for fnode in mnode.findall("module")
                )
                indent -= 4
                file_.write("%s</modules>\n" % (" " * indent))

            file_.write('%s<libgis revision="%s" />\n' % (" " * indent, libgis_revison))
            indent -= 4
            file_.write("%s</task>\n" % (" " * indent))

    file_.write("</addons>\n")
    file_.close()


def write_xml_toolboxes(name, tree=None):
    """Write element tree as a toolboxes metadata file

    If the *tree* is not given, an empty file is created.

    :param name: file name
    :param tree: XML element tree
    """
    file_ = open(name, "w")
    file_.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    file_.write('<!DOCTYPE toolbox SYSTEM "grass-addons.dtd">\n')
    file_.write(f'<addons version="{VERSION[0]}">\n')
    if tree is not None:
        for tnode in tree.findall("toolbox"):
            indent = 4
            file_.write(
                '%s<toolbox name="%s" code="%s">\n'
                % (" " * indent, tnode.get("name"), tnode.get("code"))
            )
            indent += 4
            file_.writelines(
                '%s<correlate code="%s" />\n' % (" " * indent, tnode.get("code"))
                for cnode in tnode.findall("correlate")
            )
            file_.writelines(
                '%s<task name="%s" />\n' % (" " * indent, mnode.get("name"))
                for mnode in tnode.findall("task")
            )
            indent -= 4
            file_.write("%s</toolbox>\n" % (" " * indent))

    file_.write("</addons>\n")
    file_.close()


def install_extension(source=None, url=None, xmlurl=None, branch=None):
    """Install extension (e.g. one module) or a toolbox (list of modules)"""
    gisbase = os.getenv("GISBASE")
    if not gisbase:
        gs.fatal(_("$GISBASE not defined"))

    if options["extension"] in get_installed_extensions(force=True):
        gs.warning(
            _("Extension <%s> already installed. Re-installing...")
            % options["extension"]
        )

    # create a dictionary of extensions
    # for each extension
    #   - a list of modules installed by this extension
    #   - a list of files installed by this extension

    edict = None
    if flags["t"]:
        gs.message(_("Installing toolbox <%s>...") % options["extension"])
        edict = get_toolbox_extensions(xmlurl, options["extension"])
    else:
        edict = {}
        edict[options["extension"]] = {}
        # list of modules installed by this extension
        edict[options["extension"]]["mlist"] = []
        # list of files installed by this extension
        edict[options["extension"]]["flist"] = []
    if not edict:
        gs.warning(_("Nothing to install"))
        return

    ret = 0
    tmp_dir = None

    new_modules = []
    for extension in edict:
        ret1 = 0
        new_modules_ext = None
        if sys.platform == "win32":
            ret1, new_modules_ext, new_files_ext = install_extension_win(extension)
        else:
            (
                ret1,
                new_modules_ext,
                new_files_ext,
                tmp_dir,
            ) = install_extension_std_platforms(
                extension, source=source, url=url, branch=branch
            )
        if not flags["d"] and not flags["i"]:
            edict[extension]["mlist"].extend(new_modules_ext)
            edict[extension]["flist"].extend(new_files_ext)
            new_modules.extend(new_modules_ext)
            ret += ret1
            if len(edict) > 1:
                print("-" * 60)

    if flags["d"] or flags["i"]:
        return

    if ret != 0:
        gs.warning(_("Installation failed, sorry. Please check above error messages."))
    else:
        # update extensions metadata file
        gs.message(_("Updating extensions metadata file..."))
        install_extension_xml(edict)

        # update modules metadata file
        gs.message(_("Updating extension modules metadata file..."))
        install_module_xml(new_modules)

        for module in new_modules:
            update_manual_page(module)

        gs.message(
            _("Installation of <%s> successfully finished") % options["extension"]
        )

    if not os.getenv("GRASS_ADDON_BASE"):
        gs.warning(
            _(
                "This add-on module will not function until"
                " you set the GRASS_ADDON_BASE environment"
                ' variable (see "g.manual variables")'
            )
        )


def get_toolboxes_metadata(url):
    """Return metadata for all toolboxes from given URL

    :param url: URL of a modules metadata file
    :param mlist: list of modules to get metadata for
    :returns: tuple where first item is dictionary with module names as keys
        and dictionary with dest, keyw, files keys as value, the second item
        is list of 'binary' files (installation files)
    """
    data = {}
    try:
        tree = etree_fromurl(url)
        for tnode in tree.findall("toolbox"):
            clist = []
            for cnode in tnode.findall("correlate"):
                clist.append(cnode.get("code"))

            mlist = []
            for mnode in tnode.findall("task"):
                mlist.append(mnode.get("name"))

            code = tnode.get("code")
            data[code] = {
                "name": tnode.get("name"),
                "correlate": clist,
                "modules": mlist,
            }
    except (HTTPError, OSError):
        gs.error(_("Unable to read addons metadata file from the remote server"))
    return data


def install_toolbox_xml(url, name):
    """Update local toolboxes metadata file"""
    # read metadata from remote server (toolboxes)
    url += "toolboxes.xml"
    data = get_toolboxes_metadata(url)
    if not data:
        gs.warning(_("No addons metadata available"))
        return
    if name not in data:
        gs.warning(_("No addons metadata available for <%s>") % name)
        return

    xml_file = os.path.join(options["prefix"], "toolboxes.xml")
    # create an empty file if not exists
    if not os.path.exists(xml_file):
        write_xml_modules(xml_file)

    # read XML file
    tree = ET.fromstring(Path(xml_file).read_text())

    # update tree
    tnode = None
    for node in tree.findall("toolbox"):
        if node.get("code") == name:
            tnode = node
            break

    tdata = data[name]
    if tnode is not None:
        # update existing node
        for cnode in tnode.findall("correlate"):
            tnode.remove(cnode)
        for mnode in tnode.findall("task"):
            tnode.remove(mnode)
    else:
        # create new node for task
        tnode = ET.Element("toolbox", attrib={"name": tdata["name"], "code": name})
        tree.append(tnode)

    for cname in tdata["correlate"]:
        cnode = ET.Element("correlate", attrib={"code": cname})
        tnode.append(cnode)
    for tname in tdata["modules"]:
        mnode = ET.Element("task", attrib={"name": tname})
        tnode.append(mnode)

    write_xml_toolboxes(xml_file, tree)


def get_addons_metadata(url, mlist):
    """Return metadata for list of modules from given URL

    :param url: URL of a modules metadata file
    :param mlist: list of modules to get metadata for
    :returns: tuple where first item is dictionary with module names as keys
        and dictionary with dest, keyw, files keys as value, the second item
        is list of 'binary' files (installation files)
    """

    # TODO: extensions with multiple modules
    data = {}
    bin_list = []
    try:
        tree = etree_fromurl(url)
    except (HTTPError, URLError, OSError) as error:
        gs.error(
            _("Unable to read addons metadata file from the remote server: {0}").format(
                error
            )
        )
        return data, bin_list
    except ETREE_EXCEPTIONS as error:
        gs.warning(_("Unable to parse '%s': {0}").format(error) % url)
        return data, bin_list
    for mnode in tree.findall("task"):
        name = mnode.get("name")
        if name not in mlist:
            continue
        file_list = []
        bnode = mnode.find("binary")
        windows = sys.platform == "win32"
        if bnode is not None:
            for fnode in bnode.findall("file"):
                path = fnode.text.split("/")
                if path[0] == "bin":
                    bin_list.append(path[-1])
                    if windows:
                        path[-1] += ".exe"
                elif path[0] == "scripts":
                    bin_list.append(path[-1])
                    if windows:
                        path[-1] += ".py"
                file_list.append(os.path.sep.join(path))
        desc, keyw = get_optional_params(mnode)
        data[name] = {
            "desc": desc,
            "keyw": keyw,
            "files": file_list,
        }
    return data, bin_list


def install_extension_xml(edict):
    """Update XML files with metadata about installed modules and toolbox
    of an private addon

    """
    # TODO toolbox
    # if len(mlist) > 1:
    #     # read metadata from remote server (toolboxes)
    #     install_toolbox_xml(url, options['extension'])

    xml_file = os.path.join(options["prefix"], "extensions.xml")
    # create an empty file if not exists
    if not os.path.exists(xml_file):
        write_xml_extensions(xml_file)

    # read XML file
    tree = etree_fromfile(xml_file)

    # update tree
    for name in edict:
        # so far extensions do not have description or keywords
        # only modules have

        # try:
        #     desc = gtask.parse_interface(name).description
        #     # mname = gtask.parse_interface(name).name
        #     keywords = gtask.parse_interface(name).keywords
        # except Exception as e:
        #     gs.warning(
        #         _("No addons metadata available. Addons metadata file not updated.")
        #     )
        #     return []

        tnode = None
        for node in tree.findall("task"):
            if node.get("name") == name:
                tnode = node
                break

        if tnode is None:
            # create new node for task
            tnode = ET.Element("task", attrib={"name": name})

            # dnode = ET.Element("description")
            # dnode.text = desc
            # tnode.append(dnode)
            # knode = ET.Element("keywords")
            # knode.text = (",").join(keywords)
            # tnode.append(knode)

            # create binary
            bnode = ET.Element("binary")
            # list of all installed files for this extension
            for file_name in edict[name]["flist"]:
                fnode = ET.Element("file")
                fnode.text = file_name
                bnode.append(fnode)
            tnode.append(bnode)

            # create modules
            msnode = ET.Element("modules")
            # list of all installed modules for this extension
            for module_name in edict[name]["mlist"]:
                mnode = ET.Element("module")
                mnode.text = module_name
                msnode.append(mnode)
            tnode.append(msnode)
            tree.append(tnode)
        else:
            gs.verbose(
                "Extension already listed in metadata file; metadata not updated!"
            )
    write_xml_extensions(xml_file, tree)


def get_multi_addon_addons_which_install_only_html_man_page():
    """Get multi-addon addons which install only manual html page

    :return list addons: list of multi-addon addons which install
                         only manual html page
    """
    all_addon_dirs = []
    addon_paths = re.findall(
        rf".*{options['extension']}*.",
        get_addons_paths(gg_addons_base_dir=options["prefix"]),
    )
    addon_dir_paths = {os.path.dirname(i) for i in addon_paths}
    for addon_dir in addon_dir_paths:
        addon_src_files = list(
            re.finditer(rf"{addon_dir}/(.*py)|(.*c)\n", "\n".join(addon_paths)),
        )
        if not addon_src_files:
            all_addon_dirs.append(os.path.basename(addon_dir))
        else:
            for addon_src_file in addon_src_files:
                addon_paths.pop(addon_paths.index(addon_src_file.group(0)))
    return all_addon_dirs


def filter_multi_addon_addons(mlist):
    """Filter out list of multi-addon addons which contains
    and installs only *.html manual page, without source/binary
    executable module and doesn't need to check metadata.

    e.g. the i.sentinel multi-addon consists of several full i.sentinel.*
    addons along with a i.sentinel.html overview file.


    :param list mlist: list of multi-addons (groups of addons
                       with respective addon overview HTML pages)

    :return list mlist: list of individual multi-addons without respective
                        addon overview HTML pages
    """
    # Filters out add-ons that only contain the *.html man page,
    # e.g. multi-addon i.sentinel (root directory) contains only
    # the *.html manual page for installation, it does not need
    # to check if metadata is available if there is no executable module.
    for addon in get_multi_addon_addons_which_install_only_html_man_page():
        if addon in mlist:
            mlist.pop(mlist.index(addon))
    return mlist


def install_module_xml(mlist):
    """Update XML files with metadata about installed modules and toolbox
    of an private addon

    """

    xml_file = os.path.join(options["prefix"], "modules.xml")
    # create an empty file if not exists
    if not os.path.exists(xml_file):
        write_xml_modules(xml_file)

    # read XML file
    tree = etree_fromfile(xml_file)

    if sys.platform != "win32":
        # Filter multi-addon addons
        if len(mlist) > 1:
            mlist = filter_multi_addon_addons(
                mlist.copy()
            )  # mlist.copy() keep the original list of add-ons

    # update tree
    for name in mlist:
        try:
            desc = gtask.parse_interface(name).description
            # mname = gtask.parse_interface(name).name
            keywords = gtask.parse_interface(name).keywords
        except Exception as error:
            gs.warning(
                _("No metadata available for module '{name}': {error}").format(
                    name=name, error=error
                )
            )
            continue

        tnode = None
        for node in tree.findall("task"):
            if node.get("name") == name:
                tnode = node
                break

        if tnode is None:
            # create new node for task
            tnode = ET.Element("task", attrib={"name": name})
            dnode = ET.Element("description")
            dnode.text = desc
            tnode.append(dnode)
            knode = ET.Element("keywords")
            knode.text = (",").join(keywords)
            tnode.append(knode)

            # binary files installed with an extension are now
            # listed in extensions.xml

            # # create binary
            # bnode = etree.Element("binary")
            # list_of_binary_files = []
            # for file_name in os.listdir(url):
            #     file_type = os.path.splitext(file_name)[-1]
            #     file_n = os.path.splitext(file_name)[0]
            #     html_path = os.path.join(options["prefix"], "docs", "html")
            #     c_path = os.path.join(options["prefix"], "bin")
            #     py_path = os.path.join(options["prefix"], "scripts")
            #     # html or image file
            #     if file_type in [".html", ".jpg", ".png"] and file_n in os.listdir(
            #         html_path
            #     ):
            #         list_of_binary_files.append(os.path.join(html_path, file_name))
            #     # c file
            #     elif file_type in [".c"] and file_name in os.listdir(c_path):
            #         list_of_binary_files.append(os.path.join(c_path, file_n))
            #     # python file
            #     elif file_type in [".py"] and file_name in os.listdir(py_path):
            #         list_of_binary_files.append(os.path.join(py_path, file_n))
            # # man file
            # man_path = os.path.join(options["prefix"], "docs", "man", "man1")
            # if name + ".1" in os.listdir(man_path):
            #     list_of_binary_files.append(os.path.join(man_path, name + ".1"))
            # # add binaries to xml file
            # for binary_file_name in list_of_binary_files:
            #     fnode = etree.Element("file")
            #     fnode.text = binary_file_name
            #     bnode.append(fnode)
            # tnode.append(bnode)
            tree.append(tnode)
        else:
            gs.verbose(
                "Extension module already listed in metadata file; metadata not "
                "updated!"
            )
    write_xml_modules(xml_file, tree)

    return mlist


def install_extension_win(name):
    """Install extension on MS Windows"""
    gs.message(
        _("Downloading precompiled GRASS Addons <{}>...").format(options["extension"])
    )

    # build base URL
    base_url = (
        "http://wingrass.fsv.cvut.cz/"
        f"grass{VERSION[0]}{VERSION[1]}/addons/"
        f"grass-{VERSION[0]}.{VERSION[1]}.{VERSION[2]}"
    )

    # resolve ZIP URL
    source, url = resolve_source_code(url="{0}/{1}.zip".format(base_url, name))

    # to hide non-error messages from subprocesses
    outdev = open(os.devnull, "w") if gs.verbosity() <= 2 else sys.stdout

    # download Addons ZIP file
    os.chdir(TMPDIR)  # this is just to not leave something behind
    srcdir = os.path.join(TMPDIR, name)
    download_source_code(
        source=source,
        url=url,
        name=name,
        outdev=outdev,
        directory=srcdir,
        tmpdir=TMPDIR,
    )

    # collect module names and file names
    module_list = []
    module_name_pattern = re.compile(
        r"^([d,g,i,m,p,r,s,t,v]|^db|^ps|^r3|^wx)\..*[\.py,\.exe]$"
    )
    for r, d, f in os.walk(srcdir):
        for file in f:
            # Filter GRASS module name patterns
            if re.search(module_name_pattern, file):
                modulename = os.path.splitext(file)[0]
                module_list.append(modulename)
    # remove duplicates in case there are .exe wrappers for python scripts
    module_list = set(module_list)

    # change shebang from python to python3
    pyfiles = []
    for r, d, f in os.walk(srcdir):
        for file in f:
            if file.endswith(".py"):
                pyfiles.append(os.path.join(r, file))

    for filename in pyfiles:
        replace_shebang_win(filename)

    # collect old files
    old_file_list = []
    for r, d, f in os.walk(options["prefix"]):
        for filename in f:
            fullname = os.path.join(r, filename)
            old_file_list.append(fullname)

    # copy Addons copy tree to destination directory
    move_extracted_files(
        extract_dir=srcdir, target_dir=options["prefix"], files=os.listdir(srcdir)
    )

    # collect new files
    file_list = []
    for r, d, f in os.walk(options["prefix"]):
        for filename in f:
            fullname = os.path.join(r, filename)
            if fullname not in old_file_list:
                file_list.append(fullname)

    return 0, module_list, file_list


def download_source_code_svn(url, name, outdev, directory=None):
    """Download source code from a Subversion repository

    .. note:
        Stdout is passed to to *outdev* while stderr is will be just printed.

    :param url: URL of the repository
        (module class/family and name are attached)
    :param name: module name
    :param outdev: output divide for the standard output of the svn command
    :param directory: directory where the source code will be downloaded
        (default is the current directory with name attached)

    :returns: full path to the directory with the source code
        (useful when you not specify directory, if *directory* is specified
        the return value is equal to it)
    """
    if not gs.find_program("svn", "--help"):
        gs.fatal(_("svn not found but needed to fetch AddOns from an SVN repository"))
    if not directory:
        directory = os.path.join(os.getcwd, name)
    classchar = name.split(".", 1)[0]
    moduleclass = expand_module_class_name(classchar)
    url = url + "/" + moduleclass + "/" + name
    if gs.call(["svn", "checkout", url, directory], stdout=outdev) != 0:
        gs.fatal(_("GRASS Addons <%s> not found") % name)
    return directory


def download_source_code_official_github(url, name, branch, directory=None):
    """Download source code from a official GitHub repository

    .. note:
        Stdout is passed to to *outdev* while stderr is just printed.

    :param url: URL of the repository
        (module class/family and name are attached)
    :param name: module name
    :param branch: branch of the git repository to fetch from
    :param directory: directory where the source code will be downloaded
        (default is the current directory with name attached)

    :return str, str: full path to the directory with the source code
                      (useful when you not specify directory, if
                      *directory* is specified the return value is equal
                      to it),
                      addon official GitHub repository source code URL path
    """

    try:
        ga = GitAdapter(
            addons=[name],
            url=url,
            working_directory=directory,
            major_grass_version=int(VERSION[0]),
            branch=branch,
        )
    except RuntimeError:
        # if gs.call(["svn", "export", url, directory], stdout=outdev) != 0
        gs.fatal(_("GRASS Addons <%s> not found") % name)

    ga.fetch_addons([name])

    return (
        str(ga.local_copy / ga.addons[name]),
        ga.get_addons_src_code_git_repo_url_path()[name],
    )


def move_extracted_files(extract_dir, target_dir, files):
    """Fix state of extracted files by moving them to different directory

    When extracting, it is not clear what will be the root directory
    or if there will be one at all. So this function moves the files to
    a different directory in the way that if there was one directory extracted,
    the contained files are moved.
    """
    gs.debug("move_extracted_files({0})".format(locals()))
    if len(files) == 1:
        shutil.copytree(os.path.join(extract_dir, files[0]), target_dir)
    else:
        if not os.path.exists(target_dir):
            os.mkdir(target_dir)
        for file_name in files:
            actual_file = os.path.join(extract_dir, file_name)
            if os.path.isdir(actual_file):
                # shutil.copytree() replaced by copy_tree() because
                # shutil's copytree() fails when subdirectory exists
                copy_tree(actual_file, os.path.join(target_dir, file_name))
            else:
                shutil.copy(actual_file, os.path.join(target_dir, file_name))


# Original copyright and license of the original version of the CRLF function
# Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
# Python Software Foundation; All Rights Reserved
# Python Software Foundation License Version 2
# http://svn.python.org/projects/python/trunk/Tools/scripts/crlf.py
def fix_newlines(directory):
    """Replace CRLF with LF in all files in the directory

    Binary files are ignored. Recurses into subdirectories.
    """
    # skip binary files
    # see https://stackoverflow.com/a/7392391
    textchars = bytearray({7, 8, 9, 10, 12, 13, 27} | set(range(0x20, 0x100)) - {0x7F})

    def is_binary_string(bytes):
        return bool(bytes.translate(None, textchars))

    for root, unused, files in os.walk(directory):
        for name in files:
            filename = os.path.join(root, name)
            if is_binary_string(open(filename, "rb").read(1024)):
                continue  # ignore binary files

            # read content of text file
            data = Path(filename).read_bytes()

            # we don't expect there would be CRLF file by
            # purpose if we want to allow CRLF files we would
            # have to whitelite .py etc
            newdata = data.replace(b"\r\n", b"\n")
            if newdata != data:
                Path(filename).write_bytes(newdata)


def extract_zip(name, directory, tmpdir):
    """Extract a ZIP file into a directory"""
    gs.debug(
        "extract_zip(name={name}, directory={directory}, tmpdir={tmpdir})".format(
            name=name, directory=directory, tmpdir=tmpdir
        ),
        3,
    )
    try:
        zip_file = zipfile.ZipFile(name, mode="r")
        file_list = zip_file.namelist()
        # we suppose we can write to parent of the given dir
        # (supposing a tmp dir)
        extract_dir = os.path.join(tmpdir, "extract_dir")
        os.mkdir(extract_dir)
        for subfile in file_list:
            if "__pycache__" in subfile:
                continue
            zip_file.extract(subfile, extract_dir)
        files = os.listdir(extract_dir)
        move_extracted_files(extract_dir=extract_dir, target_dir=directory, files=files)
    except zipfile.BadZipfile as error:
        gs.fatal(_("ZIP file is unreadable: {0}").format(error))


# TODO: solve the other related formats
def extract_tar(name, directory, tmpdir):
    """Extract a TAR or a similar file into a directory"""
    gs.debug(
        "extract_tar(name={name}, directory={directory}, tmpdir={tmpdir})".format(
            name=name, directory=directory, tmpdir=tmpdir
        ),
        3,
    )
    import tarfile

    try:
        tar = tarfile.open(name)
        extract_dir = os.path.join(tmpdir, "extract_dir")
        os.mkdir(extract_dir)

        # Extraction filters were added in Python 3.12,
        # and backported to 3.8.17, 3.9.17, 3.10.12, and 3.11.4
        # See
        # https://docs.python.org/3.12/library/tarfile.html#tarfile-extraction-filter
        # and https://peps.python.org/pep-0706/
        # In Python 3.12, using `filter=None` triggers a DepreciationWarning,
        # and in Python 3.14, `filter='data'` will be the default
        if hasattr(tarfile, "data_filter"):
            tar.extractall(path=extract_dir, filter="data")
        else:
            # Remove this when no longer needed
            gs.warning(_("Extracting may be unsafe; consider updating Python"))
            tar.extractall(path=extract_dir)

        files = os.listdir(extract_dir)
        move_extracted_files(extract_dir=extract_dir, target_dir=directory, files=files)
    except tarfile.TarError as error:
        gs.fatal(_("Archive file is unreadable: {0}").format(error))

    del tarfile  # we don't need it anywhere else


extract_tar.supported_formats = ["tar.gz", "gz", "bz2", "tar", "gzip", "targz"]


def download_source_code(
    source, url, name, outdev, directory=None, tmpdir=None, branch=None
):
    """Get source code to a local directory for compilation

    :return dictionary, url: addon source code directory path,
                             addon official GitHub repository source code
                             URL path
    """
    gs.verbose(_("Type of source identified as '{source}'.").format(source=source))
    if source in {"official", "official_fork"}:
        gs.message(
            _("Fetching <{name}> from <{url}> (be patient)...").format(
                name=name, url=url
            )
        )
        directory, url = download_source_code_official_github(
            url, name, branch, directory=directory
        )
    elif source == "svn":
        gs.message(
            _("Fetching <{name}> from <{url}> (be patient)...").format(
                name=name, url=url
            )
        )
        download_source_code_svn(url, name, outdev, directory)
    elif source == "remote_zip":
        gs.message(
            _("Fetching <{name}> from <{url}> (be patient)...").format(
                name=name, url=url
            )
        )
        # we expect that the module.zip file is not by chance in the archive
        zip_name = os.path.join(tmpdir, "extension.zip")
        try:
            response = urlopen(url)
        except URLError:
            # Try download add-on from 'master' branch if default "main" fails
            if not branch:
                try:
                    url = url.replace("main", "master")
                    gs.message(
                        _(
                            "Expected default branch not found. "
                            "Trying again from <{url}>..."
                        ).format(url=url)
                    )
                    response = urlopen(url)
                except URLError:
                    gs.fatal(
                        _(
                            "Extension <{name}> not found. Please check "
                            "'url' and 'branch' options"
                        ).format(name=name)
                    )
            else:
                gs.fatal(_("Extension <{}> not found").format(name))

        with open(zip_name, "wb") as out_file:
            shutil.copyfileobj(response, out_file)
        extract_zip(name=zip_name, directory=directory, tmpdir=tmpdir)
        fix_newlines(directory)
    elif (
        source.startswith("remote_")
        and source.split("_")[1] in extract_tar.supported_formats
    ):
        # we expect that the module.tar.gz file is not by chance in the archive
        archive_name = os.path.join(tmpdir, "extension." + source.split("_")[1])
        urlretrieve(url, archive_name)
        extract_tar(name=archive_name, directory=directory, tmpdir=tmpdir)
        fix_newlines(directory)
    elif source == "zip":
        extract_zip(name=url, directory=directory, tmpdir=tmpdir)
        fix_newlines(directory)
    elif source in extract_tar.supported_formats:
        extract_tar(name=url, directory=directory, tmpdir=tmpdir)
        fix_newlines(directory)
    elif source == "dir":
        shutil.copytree(url, directory)
        fix_newlines(directory)
    else:
        # probably programmer error
        gs.fatal(
            _(
                "Unknown extension (addon) source type '{0}'."
                " Please report this to the grass-user mailing list."
            ).format(source)
        )
    assert os.path.isdir(directory)
    return directory, url


def create_md_if_missing(root_dir):
    """Recursively searches for HTML files in the specified directory.
    If an HTML file does not have a corresponding Markdown (.md) file,
    it creates one by copying the HTML file and renaming it.
    """
    for dirpath, _, filenames in os.walk(root_dir):
        html_files = [f for f in filenames if f.endswith(".html")]

        for html_file in html_files:
            md_file = os.path.splitext(html_file)[0] + ".md"
            md_path = os.path.join(dirpath, md_file)

            if not os.path.exists(md_path):
                html_path = os.path.join(dirpath, html_file)
                shutil.copy(html_path, md_path)


def install_extension_std_platforms(name, source, url, branch):
    """Install extension on standard platforms"""
    gisbase = os.getenv("GISBASE")
    path_to_src_code_message = _("Path to the source code:")

    # to hide non-error messages from subprocesses
    outdev = open(os.devnull, "w") if gs.verbosity() <= 2 else sys.stdout

    os.chdir(TMPDIR)  # this is just to not leave something behind
    srcdir = os.path.join(TMPDIR, name)
    srcdir, url = download_source_code(
        source,
        url,
        name,
        outdev,
        directory=srcdir,
        tmpdir=TMPDIR,
        branch=branch,
    )
    create_md_if_missing(srcdir)
    os.chdir(srcdir)

    pgm_not_found_message = _(
        "Module name not found. Check module Makefile syntax (PGM variable)."
    )
    # collect module names
    module_list = []
    for r, d, f in os.walk(srcdir):
        for filename in f:
            if filename == "Makefile":
                # get the module name: PGM = <module name>
                with open(os.path.join(r, "Makefile")) as fp:
                    for line in fp:
                        if re.match(r"PGM.*.=|PGM=", line):
                            try:
                                modulename = line.split("=")[1].strip()
                                if modulename:
                                    if modulename not in module_list:
                                        module_list.append(modulename)
                                else:
                                    gs.fatal(pgm_not_found_message)
                            except IndexError:
                                gs.fatal(pgm_not_found_message)

    # change shebang from python to python3
    pyfiles = []
    # r=root, d=directories, f = files
    for r, d, f in os.walk(srcdir):
        for file in f:
            if file.endswith(".py"):
                pyfiles.append(os.path.join(r, file))

    for filename in pyfiles:
        with fileinput.FileInput(filename, inplace=True) as file:
            for line in file:
                print(
                    line.replace("#!/usr/bin/env python\n", "#!/usr/bin/env python3\n"),
                    end="",
                )

    dirs = {
        "bin": os.path.join(srcdir, "bin"),
        "docs": os.path.join(srcdir, "docs"),
        "html": os.path.join(srcdir, "docs", "html"),
        "mkdocs": os.path.join(srcdir, "docs", "mkdocs"),
        "rest": os.path.join(srcdir, "docs", "rest"),
        "man": os.path.join(srcdir, "docs", "man"),
        "script": os.path.join(srcdir, "scripts"),
        # TODO: handle locales also for addons
        #             'string'  : os.path.join(srcdir, 'locale'),
        "string": srcdir,
        "etc": os.path.join(srcdir, "etc"),
    }

    make_cmd = [
        MAKE,
        "MODULE_TOPDIR=%s" % gisbase.replace(" ", r"\ "),
        "RUN_GISRC=%s" % os.environ["GISRC"],
        "BIN=%s" % dirs["bin"],
        "HTMLDIR=%s" % dirs["html"],
        "MDDIR=%s" % dirs["mkdocs"],
        "RESTDIR=%s" % dirs["rest"],
        "MANBASEDIR=%s" % dirs["man"],
        "SCRIPTDIR=%s" % dirs["script"],
        "STRINGDIR=%s" % dirs["string"],
        "ETC=%s" % os.path.join(dirs["etc"]),
        "SOURCE_URL=%s" % url,
    ]

    install_cmd = [
        MAKE,
        "MODULE_TOPDIR=%s" % gisbase,
        "ARCH_DISTDIR=%s" % srcdir,
        "INST_DIR=%s" % options["prefix"],
        "install",
    ]

    if flags["d"]:
        gs.message("\n%s\n" % _("To compile run:"))
        sys.stderr.write(" ".join(make_cmd) + "\n")
        gs.message("\n%s\n" % _("To install run:"))
        sys.stderr.write(" ".join(install_cmd) + "\n")
        gs.message(f"\n{path_to_src_code_message}\n")
        sys.stderr.write(f"{srcdir}\n")
        return 0, None, None, None

    os.chdir(srcdir)

    gs.message(_("Compiling..."))
    if not os.path.exists(os.path.join(gisbase, "include", "Make", "Module.make")):
        gs.fatal(_("Please install GRASS development package"))

    if gs.call(make_cmd, stdout=outdev) != 0:
        gs.fatal(_("Compilation failed, sorry. Please check above error messages."))

    if flags["i"]:
        gs.message(f"\n{path_to_src_code_message}\n")
        sys.stderr.write(f"{srcdir}\n")
        return 0, None, None, None

    # collect old files
    old_file_list = []
    for r, d, f in os.walk(options["prefix"]):
        for filename in f:
            fullname = os.path.join(r, filename)
            old_file_list.append(fullname)

    gs.message(_("Installing..."))
    ret = gs.call(install_cmd, stdout=outdev)

    # collect new files
    file_list = []
    for r, d, f in os.walk(options["prefix"]):
        for filename in f:
            fullname = os.path.join(r, filename)
            if fullname not in old_file_list:
                file_list.append(fullname)

    return ret, module_list, file_list, os.path.join(srcdir)


def remove_extension(force=False):
    """Remove existing extension
    extension or toolbox with extensions if -t is given)"""
    if flags["t"]:
        edict = get_toolbox_extensions(options["prefix"], options["extension"])
    else:
        edict = {}
        edict[options["extension"]] = {}
        # list of modules installed by this extension
        edict[options["extension"]]["mlist"] = []
        # list of files installed by this extension
        edict[options["extension"]]["flist"] = []

    # collect modules and files installed by these extensions
    mlist = []
    xml_file = os.path.join(options["prefix"], "extensions.xml")
    if os.path.exists(xml_file):
        # read XML file
        tree = None
        try:
            tree = etree_fromfile(xml_file)
        except ETREE_EXCEPTIONS + (OSError, IOError):
            os.remove(xml_file)
            write_xml_extensions(xml_file)

        if tree is not None:
            for tnode in tree.findall("task"):
                ename = tnode.get("name").strip()
                if ename in edict:
                    # modules installed by this extension
                    mnode = tnode.find("modules")
                    if mnode:
                        for fnode in mnode.findall("module"):
                            mname = fnode.text.strip()
                            edict[ename]["mlist"].append(mname)
                            mlist.append(mname)
                    # files installed by this extension
                    bnode = tnode.find("binary")
                    if bnode:
                        for fnode in bnode.findall("file"):
                            bname = fnode.text.strip()
                            edict[ename]["flist"].append(bname)
    else:
        if force:
            write_xml_extensions(xml_file)

        xml_file = os.path.join(options["prefix"], "modules.xml")
        if not os.path.exists(xml_file):
            if force:
                write_xml_modules(xml_file)
            else:
                gs.debug("No addons metadata file available", 1)

        # read XML file
        tree = None
        try:
            tree = etree_fromfile(xml_file)
        except ETREE_EXCEPTIONS + (OSError, IOError):
            os.remove(xml_file)
            write_xml_modules(xml_file)
            return []

        if tree is not None:
            for tnode in tree.findall("task"):
                ename = tnode.get("name").strip()
                if ename in edict:
                    # assume extension name == module name
                    edict[ename]["mlist"].append(ename)
                    mlist.append(ename)
                    # files installed by this extension
                    bnode = tnode.find("binary")
                    if bnode:
                        for fnode in bnode.findall("file"):
                            bname = fnode.text.strip()
                            edict[ename]["flist"].append(bname)

    if force:
        gs.verbose(_("List of removed files:"))
    else:
        gs.info(_("Files to be removed:"))

    eremoved = remove_extension_files(edict, force)

    if force:
        if len(eremoved) > 0:
            gs.message(_("Updating addons metadata file..."))
            remove_extension_xml(mlist, edict)
            for ename in edict:
                if ename in eremoved:
                    gs.message(_("Extension <%s> successfully uninstalled.") % ename)
    elif flags["t"]:
        gs.warning(
            _("Toolbox <%s> not removed. Re-run '%s' with '-f' flag to force removal")
            % (options["extension"], "g.extension")
        )
    else:
        gs.warning(
            _("Extension <%s> not removed. Re-run '%s' with '-f' flag to force removal")
            % (options["extension"], "g.extension")
        )


# remove existing extension(s) (reading XML file)


def remove_extension_files(edict, force=False):
    """Remove extensions specified in a dictionary

    Uses the file names from the file list of the dictionary
    Fallbacks to standard layout of files on prefix path on error.
    """
    # try to read XML metadata file first
    xml_file = os.path.join(options["prefix"], "extensions.xml")

    einstalled = []
    eremoved = []

    if os.path.exists(xml_file):
        tree = etree_fromfile(xml_file)
        if tree is not None:
            for task in tree.findall("task"):
                ename = task.get("name").strip()
                einstalled.append(ename)
    else:
        tree = None

    for name in edict:
        removed = True
        if len(edict[name]["flist"]) > 0:
            err = []
            for fpath in edict[name]["flist"]:
                gs.verbose(fpath)
                if force:
                    try:
                        os.remove(fpath)
                    except OSError:
                        msg = "Unable to remove file '%s'"
                        err.append(_(msg) % fpath)
                        removed = False
            if len(err) > 0:
                for error_line in err:
                    gs.error(error_line)
        else:
            if name not in einstalled:
                # try even if module does not seem to be available,
                # as the user may be trying to get rid of left over cruft
                gs.warning(_("Extension <%s> not found") % name)

            remove_extension_std(name, force)
            removed = False

        if removed is True:
            eremoved.append(name)

    return eremoved


def remove_extension_std(name, force=False):
    """Remove extension/module expecting the standard layout

    Any images for manuals or files installed in etc will not be
    removed
    """
    for fpath in [
        os.path.join(options["prefix"], "bin", name),
        os.path.join(options["prefix"], "scripts", name),
        os.path.join(options["prefix"], "docs", "html", name + ".html"),
        os.path.join(options["prefix"], "docs", "rest", name + ".txt"),
        os.path.join(options["prefix"], "docs", "man", "man1", name + ".1"),
    ]:
        if os.path.isfile(fpath):
            gs.verbose(fpath)
            if force:
                os.remove(fpath)

    # remove module libraries under GRASS_ADDONS/etc/{name}/*
    libpath = os.path.join(options["prefix"], "etc", name)
    if os.path.isdir(libpath):
        gs.verbose(libpath)
        if force:
            shutil.rmtree(libpath)


def remove_from_toolbox_xml(name):
    """Update local meta-file when removing existing toolbox"""
    xml_file = os.path.join(options["prefix"], "toolboxes.xml")
    if not os.path.exists(xml_file):
        return
    # read XML file
    tree = etree_fromfile(xml_file)
    for node in tree.findall("toolbox"):
        if node.get("code") != name:
            continue
        tree.remove(node)

    write_xml_toolboxes(xml_file, tree)


def remove_extension_xml(mlist, edict):
    """Update local meta-file when removing existing extension"""
    if len(edict) > 1:
        # update also toolboxes metadata
        remove_from_toolbox_xml(options["extension"])

    # modules
    xml_file = os.path.join(options["prefix"], "modules.xml")
    if os.path.exists(xml_file):
        # read XML file
        tree = etree_fromfile(xml_file)
        for name in mlist:
            for node in tree.findall("task"):
                if node.get("name") != name:
                    continue
                tree.remove(node)
        write_xml_modules(xml_file, tree)

    # extensions
    xml_file = os.path.join(options["prefix"], "extensions.xml")
    if os.path.exists(xml_file):
        # read XML file
        tree = etree_fromfile(xml_file)
        for name in edict:
            for node in tree.findall("task"):
                if node.get("name") != name:
                    continue
                tree.remove(node)
        write_xml_extensions(xml_file, tree)


# check links in CSS


def check_style_file(name):
    """Ensures that a specified HTML documentation support file exists

    If the file, e.g. a CSS file does not exist, the file is copied from
    the distribution.

    If the files are missing, a warning is issued.
    """
    dist_file = os.path.join(os.getenv("GISBASE"), "docs", "html", name)
    addons_file = os.path.join(options["prefix"], "docs", "html", name)

    try:
        shutil.copyfile(dist_file, addons_file)
    except shutil.SameFileError:
        pass
    except OSError as error:
        gs.warning(
            _(
                "Unable to create '{filename}': {error}."
                " Is the GRASS GIS documentation package installed?"
                " Installation continues,"
                " but documentation may not look right."
            ).format(filename=addons_file, error=error)
        )


def create_dir(path):
    """Creates the specified directory (with all dirs in between)

    NOOP for existing directory.
    """
    if os.path.isdir(path):
        return

    try:
        os.makedirs(path)
    except OSError as error:
        gs.fatal(_("Unable to create '%s': %s") % (path, error))

    gs.debug("'%s' created" % path)


def check_dirs():
    """Ensure that the necessary directories in prefix path exist"""
    create_dir(os.path.join(options["prefix"], "bin"))
    create_dir(os.path.join(options["prefix"], "docs", "html"))
    create_dir(os.path.join(options["prefix"], "docs", "rest"))
    check_style_file("grass_logo.png")
    check_style_file("hamburger_menu.svg")
    check_style_file("hamburger_menu_close.svg")
    check_style_file("grassdocs.css")
    create_dir(os.path.join(options["prefix"], "etc"))
    create_dir(os.path.join(options["prefix"], "docs", "man", "man1"))
    create_dir(os.path.join(options["prefix"], "scripts"))


# fix file URI in manual page


def update_manual_page(module):
    """Fix manual page for addons which are at different directory
    than core modules"""
    if module.split(".", 1)[0] == "wx":
        return  # skip for GUI modules

    gs.verbose(_("Manual page for <%s> updated") % module)
    # read original html file
    htmlfile = os.path.join(options["prefix"], "docs", "html", module + ".html")
    try:
        oldfile = open(htmlfile)
        shtml = oldfile.read()
    except OSError as error:
        gs.fatal(_("Unable to read manual page: %s") % error)
    else:
        oldfile.close()

    pos = []

    # fix logo URL
    pattern = r'''<a href="([^"]+)"><img src="grass_logo.png"'''
    for match in re.finditer(pattern, shtml):
        if match.group(1)[:4] == "http":
            continue
        pos.append(match.start(1))

    # find URIs
    pattern = r"""<a href="([^"]+)">([^>]+)</a>"""
    addons = get_installed_extensions(force=True)
    if sys.platform != "win32":
        # Multi-addon
        if len(addons) > 1:
            for a in get_multi_addon_addons_which_install_only_html_man_page():
                # Add multi-addon addons which install only manual html page
                addons.append(a)

    for match in re.finditer(pattern, shtml):
        if match.group(1)[:4] == "http":
            continue
        if match.group(1).replace(".html", "") in addons:
            continue
        pos.append(match.start(1))

    if not pos:
        return  # no match

    # replace file URIs
    prefix = "file://" + "/".join([os.getenv("GISBASE"), "docs", "html"])
    ohtml = shtml[: pos[0]]
    for i in range(1, len(pos)):
        ohtml += prefix + "/" + shtml[pos[i - 1] : pos[i]]
    ohtml += prefix + "/" + shtml[pos[-1] :]

    # write updated html file
    try:
        newfile = open(htmlfile, "w")
        newfile.write(ohtml)
    except OSError as error:
        gs.fatal(_("Unable for write manual page: %s") % error)
    else:
        newfile.close()


def resolve_install_prefix(path, to_system):
    """Determine and check the path for installation"""
    if to_system:
        path = os.environ["GISBASE"]
    if path == "$GRASS_ADDON_BASE":
        if not os.getenv("GRASS_ADDON_BASE"):
            from grass.app.runtime import get_grass_config_dir

            path = os.path.join(
                get_grass_config_dir(VERSION[0], VERSION[1], os.environ), "addons"
            )
            gs.warning(
                _("GRASS_ADDON_BASE is not defined, installing to {}").format(path)
            )
        else:
            path = os.environ["GRASS_ADDON_BASE"]
    if os.path.exists(path) and not os.access(path, os.W_OK):
        gs.fatal(
            _(
                "You don't have permission to install extension to <{0}>."
                " Try to run {1} with administrator rights"
                " (su or sudo)."
            ).format(path, "g.extension")
        )
    # ensure dir sep at the end for cases where path is used as URL and pasted
    # together with file names
    if not path.endswith(os.path.sep):
        path += os.path.sep
    os.environ["GRASS_PREFIX_ADDON_BASE"] = os.path.abspath(
        path
    )  # make likes absolute paths
    return os.environ["GRASS_PREFIX_ADDON_BASE"]


def resolve_xmlurl_prefix(url, source=None):
    """Determine and check the URL where the XML metadata files are stored

    It ensures that there is a single slash at the end of URL, so we can attach
     file name easily:

    >>> resolve_xmlurl_prefix("https://grass.osgeo.org/addons")
    'https://grass.osgeo.org/addons/'
    >>> resolve_xmlurl_prefix("https://grass.osgeo.org/addons/")
    'https://grass.osgeo.org/addons/'
    """
    gs.debug("resolve_xmlurl_prefix(url={0}, source={1})".format(url, source))
    if source in {"official", "official_fork"}:
        # use pregenerated modules XML file
        # Define branch to fetch from (latest or current version)
        version_branch = get_version_branch(VERSION[0])

        url = "https://grass.osgeo.org/addons/{}/".format(version_branch)
    # else try to get extensions XMl from SVN repository (provided URL)
    # the exact action depends on subsequent code (somewhere)

    if not url.endswith("/"):
        url += "/"
    return url


KNOWN_HOST_SERVICES_INFO = {
    "OSGeo Trac": {
        "domain": "trac.osgeo.org",
        "ignored_suffixes": ["format=zip"],
        "possible_starts": ["", "https://", "http://"],
        "url_start": "https://",
        "url_end": "?format=zip",
    },
    "GitHub": {
        "domain": "github.com",
        "ignored_suffixes": [".zip", ".tar.gz"],
        "possible_starts": ["", "https://", "http://"],
        "url_start": "https://",
        "url_end": "/archive/{branch}.zip",
    },
    "GitLab": {
        "domain": "gitlab.com",
        "ignored_suffixes": [".zip", ".tar.gz", ".tar.bz2", ".tar"],
        "possible_starts": ["", "https://", "http://"],
        "url_start": "https://",
        "url_end": "/-/archive/{branch}/{name}-{branch}.zip",
    },
    "Bitbucket": {
        "domain": "bitbucket.org",
        "ignored_suffixes": [".zip", ".tar.gz", ".gz", ".bz2"],
        "possible_starts": ["", "https://", "http://"],
        "url_start": "https://",
        "url_end": "/get/{branch}.zip",
    },
}

# TODO: support ZIP URLs which don't end with zip
# https://gitlab.com/user/reponame/repository/archive.zip?ref=b%C3%A9po


def resolve_known_host_service(url, name, branch):
    """Determine source type and full URL for known hosting service

    If the service is not determined from the provided URL, tuple with
    is two ``None`` values is returned.

    :param url: URL
    :param name: module name
    """
    match = None
    actual_start = None
    for key, value in KNOWN_HOST_SERVICES_INFO.items():
        for start in value["possible_starts"]:
            if url.startswith(start + value["domain"]):
                match = value
                actual_start = start
                gs.verbose(_("Identified {0} as known hosting service").format(key))
                for suffix in value["ignored_suffixes"]:
                    if url.endswith(suffix):
                        gs.verbose(
                            _(
                                "Not using {service} as known hosting service"
                                " because the URL ends with '{suffix}'"
                            ).format(service=key, suffix=suffix)
                        )
                        return None, None
    if match:
        actual_start = match["url_start"] if not actual_start else ""
        if "branch" in match["url_end"]:
            suffix = match["url_end"].format(
                name=name,
                branch=branch or get_default_branch(url),
            )
        else:
            suffix = match["url_end"].format(name=name)
        url = "{prefix}{base}{suffix}".format(
            prefix=actual_start, base=url.rstrip("/"), suffix=suffix
        )
        gs.verbose(_("Will use the following URL for download: {0}").format(url))
        return "remote_zip", url
    return None, None


def validate_url(url):
    if not os.path.exists(url):
        url_validated = False
        message = None
        if url.startswith("http"):
            try:
                open_url = urlopen(url)
                open_url.close()
                url_validated = True
            except URLError as error:
                message = error
        else:
            try:
                open_url = urlopen("http://" + url)
                open_url.close()
                url_validated = True
            except URLError as error:
                message = error
            try:
                open_url = urlopen("https://" + url)
                open_url.close()
                url_validated = True
            except URLError as error:
                message = error
        if not url_validated:
            gs.fatal(
                _("Cannot open URL <{url}>: {error}").format(url=url, error=message)
            )
    return True


# TODO: add also option to enforce the source type
# TODO: workaround, https://github.com/OSGeo/grass-addons/issues/528
def resolve_source_code(url=None, name=None, branch=None, fork=False):
    """Return type and URL or path of the source code

    Local paths are not presented as URLs to be usable in standard functions.
    Path is identified as local path if the directory of file exists which
    has the unfortunate consequence that the not existing files are evaluated
    as remote URLs. When path is not evaluated, Subversion is assumed for
    backwards compatibility. When GitHub repository is specified, ZIP file
    link is returned. The ZIP is for {branch} branch, not the default one because
    GitHub does not provide the default branch in the URL (July 2015).

    :returns: tuple with type of source and full URL or path

    Official repository:

    >>> resolve_source_code(name="g.example")  # doctest: +SKIP
    ('official', 'https://trac.osgeo.org/.../general/g.example')

    Subversion:

    >>> resolve_source_code("https://svn.osgeo.org/grass/grass-addons/grass7")
    ('svn', 'https://svn.osgeo.org/grass/grass-addons/grass7')

    ZIP files online:

    >>> resolve_source_code(
    ...     "https://trac.osgeo.org/.../r.modis?format=zip"
    ... )  # doctest: +SKIP
    ('remote_zip', 'https://trac.osgeo.org/.../r.modis?format=zip')

    Local directories and ZIP files:

    >>> resolve_source_code(os.path.expanduser("~"))  # doctest: +ELLIPSIS
    ('dir', '...')
    >>> resolve_source_code("/local/directory/downloaded.zip")  # doctest: +SKIP
    ('zip', '/local/directory/downloaded.zip')

    OSGeo Trac:

    >>> resolve_source_code("trac.osgeo.org/.../r.agent.aco")  # doctest: +SKIP
    ('remote_zip', 'https://trac.osgeo.org/.../r.agent.aco?format=zip')
    >>> resolve_source_code("https://trac.osgeo.org/.../r.agent.aco")  # doctest: +SKIP
    ('remote_zip', 'https://trac.osgeo.org/.../r.agent.aco?format=zip')

    GitHub:

    >>> resolve_source_code("github.com/user/g.example")  # doctest: +SKIP
    ('remote_zip', 'https://github.com/user/g.example/archive/master.zip')
    >>> resolve_source_code("github.com/user/g.example/")  # doctest: +SKIP
    ('remote_zip', 'https://github.com/user/g.example/archive/master.zip')
    >>> resolve_source_code("https://github.com/user/g.example")  # doctest: +SKIP
    ('remote_zip', 'https://github.com/user/g.example/archive/master.zip')
    >>> resolve_source_code("https://github.com/user/g.example/")  # doctest: +SKIP
    ('remote_zip', 'https://github.com/user/g.example/archive/master.zip')

    GitLab:

    >>> resolve_source_code("gitlab.com/JoeUser/GrassModule")  # doctest: +SKIP
    ('remote_zip', 'https://gitlab.com/JoeUser/GrassModule/-/archive/master/GrassModule-master.zip')
    >>> resolve_source_code("https://gitlab.com/JoeUser/GrassModule")  # doctest: +SKIP
    ('remote_zip', 'https://gitlab.com/JoeUser/GrassModule/-/archive/master/GrassModule-master.zip')

    Bitbucket:

    >>> resolve_source_code("bitbucket.org/joe-user/grass-module")  # doctest: +SKIP
    ('remote_zip', 'https://bitbucket.org/joe-user/grass-module/get/default.zip')
    >>> resolve_source_code(
    ...     "https://bitbucket.org/joe-user/grass-module"
    ... )  # doctest: +SKIP
    ('remote_zip', 'https://bitbucket.org/joe-user/grass-module/get/default.zip')
    """  # noqa: E501
    # Handle URL for the official repo
    if not url or url == GIT_URL:
        return "official", GIT_URL

    # Check if URL can be found
    # Catch corner case if local URL is given starting with file://
    url = url[6:] if url.startswith("file://") else url
    validate_url(url)

    # Return validated URL for official fork
    if fork:
        return "official_fork", url

    # Handle local URLs
    if os.path.isdir(url):
        return "dir", os.path.abspath(url)
    if os.path.exists(url):
        if url.endswith(".zip"):
            return "zip", os.path.abspath(url)
        for suffix in extract_tar.supported_formats:
            if url.endswith("." + suffix):
                return suffix, os.path.abspath(url)
    # Handle remote URLs
    else:
        source, resolved_url = resolve_known_host_service(url, name, branch)
        if source:
            return source, resolved_url
        # we allow URL to end with =zip or ?zip and not only .zip
        # unfortunately format=zip&version=89612 would require something else
        # special option to force the source type would solve it
        if url.endswith("zip"):
            return "remote_zip", url
        for suffix in extract_tar.supported_formats:
            if url.endswith(suffix):
                return "remote_" + suffix, url
        # fallback to the classic behavior
        return "svn", url


def get_addons_paths(gg_addons_base_dir):
    """Make or update list of the official addons source code paths
    prefix parameter plus /grass-addons directory using Git repository

    :param str gg_addons_base_dir: dir path where addons are installed

    :return str: list of all addons source code paths
    """
    addons_branch = get_version_branch(VERSION[0])
    grass_addons_dir = Path(gg_addons_base_dir) / "grass-addons"
    if grass_addons_dir.exists():
        try_rmdir(grass_addons_dir)
    gs.call(
        [
            "git",
            "clone",
            "-q",
            "--no-checkout",
            f"--branch={addons_branch}",
            "--filter=blob:none",
            GIT_URL,
        ],
        cwd=gg_addons_base_dir,
    )
    addons_file_list = gs.Popen(
        ["git", "ls-tree", "--name-only", "-r", addons_branch],
        cwd=grass_addons_dir,
        stdout=PIPE,
        stderr=PIPE,
    )
    addons_file_list, stderr = addons_file_list.communicate()
    if stderr:
        gs.fatal(
            _(
                "Failed to get addons files list from the"
                " Git repository <{repo_path}>.\n{error}"
            ).format(
                repo_path=grass_addons_dir,
                error=gs.decode(stderr),
            )
        )
    return gs.decode(addons_file_list)


def main():
    # check dependencies
    if not flags["a"] and sys.platform != "win32":
        check_progs()

    original_url = options["url"]
    branch = options["branch"]

    # manage proxies
    global PROXIES
    if options["proxy"]:
        PROXIES = {}
        for ptype, purl in (p.split("=") for p in options["proxy"].split(",")):
            PROXIES[ptype] = purl
        proxy = urlrequest.ProxyHandler(PROXIES)
        opener = urlrequest.build_opener(proxy)
        urlrequest.install_opener(opener)
        # Required for mkhtml.py script (get addon git commit from GitHub API server)
        os.environ["GRASS_PROXY"] = options["proxy"]

    # define path
    options["prefix"] = resolve_install_prefix(
        path=options["prefix"], to_system=flags["s"]
    )

    # list available extensions
    if flags["l"] or flags["c"] or (flags["g"] and not flags["a"]):
        # using dummy extension, we don't need any extension URL now,
        # but will work only as long as the function does not check
        # if the URL is actually valid or something
        source, url = resolve_source_code(
            name="dummy", url=original_url, branch=branch, fork=flags["o"]
        )
        xmlurl = resolve_xmlurl_prefix(original_url, source=source)
        list_available_extensions(xmlurl)
        return 0
    if flags["a"]:
        list_installed_extensions(toolboxes=flags["t"])
        return 0

    if flags["d"] or flags["i"]:
        flag = "d" if flags["d"] else "i"
        if options["operation"] != "add":
            gs.warning(
                _(
                    "Flag '{}' is relevant only to 'operation=add'. Ignoring this flag."
                ).format(flag)
            )
        else:
            global REMOVE_TMPDIR
            REMOVE_TMPDIR = False

    if options["operation"] == "add":
        check_dirs()
        if sys.platform == "win32":
            install_extension()
        else:
            if original_url == "" or flags["o"]:
                # Query GitHub API only if extension will be downloaded
                # from official GRASS GIS addons repository
                get_addons_paths(gg_addons_base_dir=options["prefix"])
            source, url = resolve_source_code(
                name=options["extension"],
                url=original_url,
                branch=branch,
                fork=flags["o"],
            )
            xmlurl = resolve_xmlurl_prefix(original_url, source=source)
            install_extension(source=source, url=url, xmlurl=xmlurl, branch=branch)
    else:  # remove
        remove_extension(force=flags["f"])

    return 0


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "--doctest":
        import doctest

        sys.exit(doctest.testmod().failed)
    options, flags = gs.parser()
    global TMPDIR
    TMPDIR = tempfile.mkdtemp()
    atexit.register(cleanup)

    grass_version = gs.version()
    VERSION = grass_version["version"].split(".")

    sys.exit(main())
