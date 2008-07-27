"""
@package global.py

@brief Global variables

This module provide the space for global variables
used in the code.

(C) 2007 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import locale

### i18N
import gettext
gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode=True)

def CheckForWx():
    """Try to import wx module and check its version"""
    minVersion = [2, 8, 1, 1]
    try:
        import wxversion
        wxversion.select(str(minVersion[0]) + '.' + str(minVersion[1]))
        import wx
        version = wx.version().split(' ')[0]
        if map(int, version.split('.')) < minVersion:
            raise ValueError('Your wxPython version is %s.%s.%s.%s' % tuple(version.split('.')))

    except (ImportError, ValueError, wxversion.VersionError), e:
        print >> sys.stderr, 'ERROR: wxGUI requires wxPython >= %d.%d.%d.%d. ' % tuple(minVersion) + \
            '%s. Detailed information in README file.' % (str(e))
        sys.exit(1)
    except locale.Error, e:
        print >> sys.stderr, "Unable to set locale:", e
        os.environ['LC_ALL'] = ''

CheckForWx()
import wx
import wx.lib.flatnotebook as FN

try:
    import subprocess
except:
    compatPath = os.path.join(globalvar.ETCWXDIR, "compat")
    sys.path.append(compatPath)
    import subprocess

"""
Query layer (generated for example by selecting item in the Attribute Table Manager)
Deleted automatically on re-render action
"""
# temporal query layer (removed on re-render action)
QUERYLAYER = 'qlayer'

# path to python scripts
ETCDIR = os.path.join(os.getenv("GISBASE"), "etc")
ETCWXDIR = os.path.join(ETCDIR, "wxpython")

"""Style definition for FlatNotebook pages"""
FNPageStyle = FN.FNB_VC8 | \
    FN.FNB_BACKGROUND_GRADIENT | \
    FN.FNB_NODRAG | \
    FN.FNB_TABS_BORDER_SIMPLE 
FNPageColor = wx.Colour(125,200,175)

"""Dialog widget dimension"""
DIALOG_SPIN_SIZE = (150, -1)
DIALOG_COMBOBOX_SIZE = (300, -1)
DIALOG_GSELECT_SIZE = (400, -1)
DIALOG_TEXTCTRL_SIZE = (400, -1)

MAP_WINDOW_SIZE = (680, 520)
HIST_WINDOW_SIZE = (500, 350)

MAP_DISPLAY_STATUSBAR_MODE = [_("Coordinates"),
                              _("Extent"),
                              _("Comp. region"),
                              _("Show comp. extent"),
                              _("Display mode"),
                              _("Display geometry"),
                              _("Map scale")]

"""File name extension binaries/scripts"""
if subprocess.mswindows:
    EXT_BIN = '.exe'
    EXT_SCT = '.bat'
else:
    EXT_BIN = ''
    EXT_SCT = ''

def GetGRASSCmds(bin=True, scripts=True, gui_scripts=True):
    """
    Create list of all available GRASS commands to use when
    parsing string from the command line
    """
    gisbase = os.environ['GISBASE']
    list = []
    if bin is True:
        list = os.listdir(os.path.join(gisbase, 'bin'))
    if scripts is True:
        list = list + os.listdir(os.path.join(gisbase, 'scripts')) 
    if gui_scripts is True:
        os.environ["PATH"] = os.getenv("PATH") + ':%s' % os.path.join(gisbase, 'etc', 'gui', 'scripts')
        list = list + os.listdir(os.path.join(gisbase, 'etc', 'gui', 'scripts'))
       
    if subprocess.mswindows:
        for idx in range(len(list)):
            list[idx] = list[idx].replace(EXT_BIN, '')
            list[idx] = list[idx].replace(EXT_SCT, '')

    return list

"""@brief Collected GRASS-relared binaries/scripts"""
grassCmd = {}
grassCmd['all'] = GetGRASSCmds()
grassCmd['script'] = GetGRASSCmds(bin=False)

"""@Toolbar icon size"""
toolbarSize = (24, 24)
