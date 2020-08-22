"""
@package core.globalvar

@brief Global variables used by wxGUI

(C) 2007-2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

from __future__ import print_function

import os
import sys
import locale

if not os.getenv("GISBASE"):
    sys.exit("GRASS is not running. Exiting...")

# path to python scripts
ETCDIR = os.path.join(os.getenv("GISBASE"), "etc")
GUIDIR = os.path.join(os.getenv("GISBASE"), "gui")
WXGUIDIR = os.path.join(GUIDIR, "wxpython")
ICONDIR = os.path.join(GUIDIR, "icons")
IMGDIR = os.path.join(GUIDIR, "images")
SYMBDIR = os.path.join(IMGDIR, "symbols")

# i18n is taken care of in the grass library code.
# So we need to import it before any of the GUI code.
from grass.script.core import get_commands

from core.debug import Debug

WXPY3_MIN_VERSION = [4, 0, 0, 0]


def parse_version_string(version):
    """Parse version number, return three numbers as list

    >>> parse_version_string("4.0.1")
    [4, 0, 1]
    >>> parse_version_string("4.0.0aX")
    [4, 0, 0]
    >>> parse_version_string("4.0.7.post2")
    [4, 0, 7]
    """
    try:
        # max: get only first three parts from wxPython 4.0.7.post2
        maxsplit = 2
        split_ver = version.split(".", maxsplit)
        parsed_version = list(map(int, split_ver))
    except ValueError:
        # remove last part of wxPython 4.0.0aX
        for i, c in enumerate(split_ver[-1]):
            if not c.isdigit():
                break
        parsed_version = list(map(int, split_ver[:-1])) + [int(split_ver[-1][:i])]
    return parsed_version


def version_as_string(version):
    """Return version list or tuple as text

    >>> version_as_string([1, 2, 3])
    '1.2.3'
    >>> version_as_string((1, 2, 3, 4))
    '1.2.3.4'
    """
    texts = [str(i) for i in version]
    return ".".join(texts)


def CheckWxPhoenix():
    if 'phoenix' in wx.version():
        return True
    return False


def CheckWxVersion(version):
    """Check wx version"""
    ver = wx.__version__
    parsed_version = parse_version_string(ver)

    if parsed_version < version:
        return False

    return True


def CheckForWx():
    """Try to import wx module"""
    if 'wx' in sys.modules.keys():
        return

    try:
        import wx
        version = parse_version_string(wx.__version__)
        if version < WXPY3_MIN_VERSION:
            raise ValueError(
                "Your wxPython version is {}".format(wx.__version__))
        return
    except ImportError as e:
        print('ERROR: wxGUI requires wxPython. {}'.format(e),
              file=sys.stderr)
        print('You can still use GRASS GIS modules in'
              ' the command line or in Python.', file=sys.stderr)
        sys.exit(1)
    except locale.Error as e:
        print("Unable to set locale:", e, file=sys.stderr)
        os.environ['LC_ALL'] = ''

if not os.getenv("GRASS_WXBUNDLED"):
    CheckForWx()
import wx

if CheckWxPhoenix():
    try:
        import agw.flatnotebook as FN
    except ImportError: # if it's not there locally, try the wxPython lib.
        import wx.lib.agw.flatnotebook as FN
else:
    import wx.lib.flatnotebook as FN



"""
Query layer (generated for example by selecting item in the Attribute Table Manager)
Deleted automatically on re-render action
"""
# temporal query layer (removed on re-render action)
QUERYLAYER = 'qlayer'

"""Style definition for FlatNotebook pages"""
FNPageStyle = FN.FNB_FF2 | \
    FN.FNB_BACKGROUND_GRADIENT | \
    FN.FNB_NODRAG | \
    FN.FNB_TABS_BORDER_SIMPLE

FNPageDStyle = FN.FNB_FANCY_TABS | \
    FN.FNB_BOTTOM | \
    FN.FNB_NODRAG | \
    FN.FNB_NO_NAV_BUTTONS | \
    FN.FNB_NO_X_BUTTON

FNPageColor = wx.Colour(125, 200, 175)

"""Dialog widget dimension"""
DIALOG_SPIN_SIZE = (150, -1)
DIALOG_COMBOBOX_SIZE = (300, -1)
DIALOG_GSELECT_SIZE = (400, -1)
DIALOG_TEXTCTRL_SIZE = (400, -1)
DIALOG_LAYER_SIZE = (100, -1)
DIALOG_COLOR_SIZE = (30, 30)

MAP_WINDOW_SIZE = (825, 600)

GM_WINDOW_MIN_SIZE = (525, 400)
# small for ms window which wraps the menu
# small for max os x which has the global menu
# small for ubuntu when menuproxy is defined
# not defined UBUNTU_MENUPROXY on linux means standard menu,
# so the probably problem
# UBUNTU_MENUPROXY= means ubuntu with disabled global menu [1]
# use UBUNTU_MENUPROXY=0 to disbale global menu on ubuntu but in the same time
# to get smaller lmgr
# [1] https://wiki.ubuntu.com/DesktopExperienceTeam/ApplicationMenu#Troubleshooting
if sys.platform in ('win32', 'darwin') or os.environ.get('UBUNTU_MENUPROXY'):
    GM_WINDOW_SIZE = (GM_WINDOW_MIN_SIZE[0], 600)
else:
    GM_WINDOW_SIZE = (625, 600)

if sys.platform == 'win32':
    BIN_EXT = '.exe'
    SCT_EXT = '.bat'
else:
    BIN_EXT = SCT_EXT = ''


def UpdateGRASSAddOnCommands(eList=None):
    """Update list of available GRASS AddOns commands to use when
    parsing string from the command line

    :param eList: list of AddOns commands to remove
    """
    global grassCmd, grassScripts

    # scan addons (path)
    addonPath = os.getenv('GRASS_ADDON_PATH', '')
    addonBase = os.getenv('GRASS_ADDON_BASE')
    if addonBase:
        addonPath += os.pathsep + os.path.join(addonBase, 'bin')
        if sys.platform != 'win32':
            addonPath += os.pathsep + os.path.join(addonBase, 'scripts')

    # remove commands first
    if eList:
        for ext in eList:
            if ext in grassCmd:
                grassCmd.remove(ext)
        Debug.msg(1, "Number of removed AddOn commands: %d", len(eList))

    nCmd = 0
    pathList = os.getenv('PATH', '').split(os.pathsep)
    for path in addonPath.split(os.pathsep):
        if not os.path.exists(path) or not os.path.isdir(path):
            continue

        # check if addon is in the path
        if pathList and path not in pathList:
            os.environ['PATH'] = path + os.pathsep + os.environ['PATH']

        for fname in os.listdir(path):
            if fname in ['docs', 'modules.xml']:
                continue
            if grassScripts:  # win32
                name, ext = os.path.splitext(fname)
                if name not in grassCmd:
                    if ext not in [BIN_EXT, SCT_EXT]:
                        continue
                    if name not in grassCmd:
                        grassCmd.add(name)
                        Debug.msg(3, "AddOn commands: %s", name)
                        nCmd += 1
                if ext == SCT_EXT and \
                        ext in grassScripts.keys() and \
                        name not in grassScripts[ext]:
                    grassScripts[ext].append(name)
            else:
                if fname not in grassCmd:
                    grassCmd.add(fname)
                    Debug.msg(3, "AddOn commands: %s", fname)
                    nCmd += 1

    Debug.msg(1, "Number of GRASS AddOn commands: %d", nCmd)

"""@brief Collected GRASS-relared binaries/scripts"""
grassCmd, grassScripts = get_commands()
Debug.msg(1, "Number of core GRASS commands: %d", len(grassCmd))
UpdateGRASSAddOnCommands()

"""@Toolbar icon size"""
toolbarSize = (24, 24)

"""@Check version of wxPython, use agwStyle for 2.8.11+"""
hasAgw = CheckWxVersion([2, 8, 11, 0])
wxPythonPhoenix = CheckWxPhoenix()

gtk3 = True if 'gtk3' in wx.PlatformInfo else False

"""@Add GUIDIR/scripts into path"""
os.environ['PATH'] = os.path.join(
    GUIDIR, 'scripts') + os.pathsep + os.environ['PATH']
