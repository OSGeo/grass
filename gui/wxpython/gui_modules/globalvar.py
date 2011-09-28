"""!
@package global.py

@brief Global variables

This module provide the space for global variables
used in the code.

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

sys.path.append(os.path.join(ETCDIR, "python"))
import grass.script as grass

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
    
    minVersion = [2, 8, 1, 1]
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

MAP_WINDOW_SIZE = (725, 600)
HIST_WINDOW_SIZE = (500, 350)
GM_WINDOW_SIZE = (500, 600)

MAP_DISPLAY_STATUSBAR_MODE = [_("Coordinates"),
                              _("Extent"),
                              _("Comp. region"),
                              _("Show comp. extent"),
                              _("Display mode"),
                              _("Display resolution"),
                              _("Display geometry"),
                              _("Map scale"),
                              _("Go to"),
                              _("Projection"),]

"""!File name extension binaries/scripts"""
if sys.platform == 'win32':
    EXT_BIN = '.exe'
    EXT_SCT = '.py'
else:
    EXT_BIN = ''
    EXT_SCT = ''

def GetGRASSCmds(bin = True, scripts = True, gui_scripts = True, addons = True):
    """!Create list of available GRASS commands to use when parsing
    string from the command line

    @param bin True to include executable into list
    @param scripts True to include scripts into list
    @param gui_scripts True to include GUI scripts into list
    """
    gisbase = os.environ['GISBASE']
    cmd = list()
    
    if bin:
        for executable in os.listdir(os.path.join(gisbase, 'bin')):
            ext = os.path.splitext(executable)[1]
            if not EXT_BIN or \
                    ext in (EXT_BIN, EXT_SCT):
                cmd.append(executable)
        
        # add special call for setting vector colors
        cmd.append('vcolors')
    
    if scripts:
        cmd += os.listdir(os.path.join(gisbase, 'scripts'))
    
    if gui_scripts:
        os.environ["PATH"] = os.getenv("PATH") + os.pathsep + os.path.join(gisbase, 'etc', 'gui', 'scripts')
        cmd = cmd + os.listdir(os.path.join(gisbase, 'etc', 'gui', 'scripts'))
    
    if addons and os.getenv('GRASS_ADDON_PATH'):
        path = os.getenv('GRASS_ADDON_PATH')
        for executable in os.listdir(os.path.join(path, 'bin')):
            ext = os.path.splitext(executable)[1]
            if not EXT_BIN or \
                    ext in (EXT_BIN, EXT_SCT):
                cmd.append(executable)
        cmd += os.listdir(os.path.join(path, 'scripts'))
    
    if sys.platform == 'win32':
        for idx in range(len(cmd)):
            name, ext = os.path.splitext(cmd[idx])
            if ext in (EXT_BIN, EXT_SCT):
                cmd[idx] = name
    
    return cmd

"""@brief Collected GRASS-relared binaries/scripts"""
grassCmd = {}
grassCmd['all']    = GetGRASSCmds()
grassCmd['script'] = GetGRASSCmds(bin = False, gui_scripts = False)

"""@Toolbar icon size"""
toolbarSize = (24, 24)

"""@Is g.mlist available?"""
if 'g.mlist' in grassCmd['all']:
    have_mlist = True
else:
    have_mlist = False

"""@Check version of wxPython, use agwStyle for 2.8.11+"""
hasAgw = CheckWxVersion()
