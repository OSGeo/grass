"""
MODULE:    global.py

PURPOSE:   Global variables

           This module provide the space for global variables
           used in the code.

AUTHOR(S): GRASS Development Team
           Martin Landa <landa.martin gmail.com>

COPYRIGHT: (C) 2007 by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import sys
import locale

def CheckForWx():
    """Try to import wx module and check its version"""
    majorVersion = 2.8
    minorVersion = 1.1

    try:
        import wxversion
        wxversion.select(str(majorVersion))
        import wx
        version = wx.__version__
        if float(version[:3]) < majorVersion:
            raise ValueError('You are using wxPython version %s' % str(version))
        if float(version[:3]) == 2.8 and \
                float(version[4:]) < minorVersion:
            raise ValueError('You are using wxPython version %s' % str(version))

    except (ImportError, ValueError, wxversion.VersionError), e:
        print >> sys.stderr, 'ERROR: ' + str(e) + \
            '. wxPython >= %s.%s is required. Detailed information in README file.' % \
            (str(majorVersion), str(minorVersion))
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
