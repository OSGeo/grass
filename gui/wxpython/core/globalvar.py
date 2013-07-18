"""!
@package core.globalvar

@brief Global variables used by wxGUI

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import locale

if not os.getenv("GISBASE"):
    sys.exit("GRASS is not running. Exiting...")

# path to python scripts
ETCDIR = os.path.join(os.getenv("GISBASE"), "etc")
ETCICONDIR = os.path.join(os.getenv("GISBASE"), "etc", "gui", "icons")
ETCWXDIR = os.path.join(ETCDIR, "gui", "wxpython")
ETCIMGDIR = os.path.join(ETCDIR, "gui", "images")
ETCSYMBOLDIR = os.path.join(ETCDIR, "gui", "images", "symbols")

from core.debug import Debug

# cannot import from the core.utils module to avoid cross dependencies
try:
    # intended to be used also outside this module
    import gettext
    _ = gettext.translation('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale')).ugettext
except IOError:
    # using no translation silently
    def null_gettext(string):
        return string
    _ = null_gettext

if os.path.join(ETCDIR, "python") not in sys.path:
    sys.path.append(os.path.join(ETCDIR, "python"))

from grass.script.core import get_commands


def CheckWxVersion(version = [2, 8, 11, 0]):
    """!Check wx version"""
    ver = wx.version().split(' ')[0]
    if map(int, ver.split('.')) < version:
        return False

    return True

def CheckForWx():
    """!Try to import wx module and check its version"""
    if 'wx' in sys.modules.keys():
        return

    minVersion = [2, 8, 10, 1]
    try:
        try:
            import wxversion
        except ImportError, e:
            raise ImportError(e)
        # wxversion.select(str(minVersion[0]) + '.' + str(minVersion[1]))
        wxversion.ensureMinimal(str(minVersion[0]) + '.' + str(minVersion[1]))
        import wx
        version = wx.version().split(' ')[0]

        if map(int, version.split('.')) < minVersion:
            raise ValueError('Your wxPython version is %s.%s.%s.%s' % tuple(version.split('.')))

    except ImportError, e:
        print >> sys.stderr, 'ERROR: wxGUI requires wxPython. %s' % str(e)
        sys.exit(1)
    except (ValueError, wxversion.VersionError), e:
        print >> sys.stderr, 'ERROR: wxGUI requires wxPython >= %d.%d.%d.%d. ' % tuple(minVersion) + \
            '%s.' % (str(e))
        sys.exit(1)
    except locale.Error, e:
        print >> sys.stderr, "Unable to set locale:", e
        os.environ['LC_ALL'] = ''

if not os.getenv("GRASS_WXBUNDLED"):
    CheckForWx()
import wx
import wx.lib.flatnotebook as FN

"""
Query layer (generated for example by selecting item in the Attribute Table Manager)
Deleted automatically on re-render action
"""
# temporal query layer (removed on re-render action)
QUERYLAYER = 'qlayer'

"""!Style definition for FlatNotebook pages"""
FNPageStyle = FN.FNB_VC8 | \
    FN.FNB_BACKGROUND_GRADIENT | \
    FN.FNB_NODRAG | \
    FN.FNB_TABS_BORDER_SIMPLE

FNPageDStyle = FN.FNB_FANCY_TABS | \
    FN.FNB_BOTTOM | \
    FN.FNB_NO_NAV_BUTTONS | \
    FN.FNB_NO_X_BUTTON

FNPageColor = wx.Colour(125,200,175)

"""!Dialog widget dimension"""
DIALOG_SPIN_SIZE = (150, -1)
DIALOG_COMBOBOX_SIZE = (300, -1)
DIALOG_GSELECT_SIZE = (400, -1)
DIALOG_TEXTCTRL_SIZE = (400, -1)
DIALOG_LAYER_SIZE = (100, -1)
DIALOG_COLOR_SIZE = (30, 30)

MAP_WINDOW_SIZE = (800, 600)
GM_WINDOW_SIZE = (525, 600)

if sys.platform == 'win32':
    BIN_EXT = '.exe'
    SCT_EXT = '.py'
else:
    BIN_EXT = SCT_EXT = ''


def UpdateGRASSAddOnCommands(eList = None):
    """!Update list of available GRASS AddOns commands to use when
    parsing string from the command line

    @param eList list of AddOns commands to remove
    """
    global grassCmd, grassScripts

    # scan addons (path)
    addonPath = os.getenv('GRASS_ADDON_PATH', '')
    addonBase = os.getenv('GRASS_ADDON_BASE')
    if addonBase:
        addonPath += os.pathsep + os.path.join(addonBase, 'bin') + os.pathsep + \
            os.path.join(addonBase, 'scripts')

    # remove commands first
    if eList:
        for ext in eList:
            if ext in grassCmd:
                grassCmd.remove(ext)
        Debug.msg(1, "Number of removed AddOn commands: %d", len(eList))

    nCmd = 0
    for path in addonPath.split(os.pathsep):
        if not os.path.exists(path) or not os.path.isdir(path):
            continue
        for fname in os.listdir(path):
            if fname in ['docs', 'modules.xml']:
                continue
            if grassScripts: # win32
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

    Debug.msg(1, "Number of new AddOn commands: %d", nCmd)

"""@brief Collected GRASS-relared binaries/scripts"""
grassCmd, grassScripts = get_commands()
Debug.msg(1, "Number of GRASS commands: %d", len(grassCmd))
UpdateGRASSAddOnCommands()

"""@Toolbar icon size"""
toolbarSize = (24, 24)

"""@Is g.mlist available?"""
if 'g.mlist' in grassCmd:
    have_mlist = True
else:
    have_mlist = False

"""@Check version of wxPython, use agwStyle for 2.8.11+"""
hasAgw = CheckWxVersion()
