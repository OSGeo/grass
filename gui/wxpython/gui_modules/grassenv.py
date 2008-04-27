"""
MODULE:     grassenv

PURPOSE:    GRASS environment variable management

AUTHORS:    The GRASS Development Team
            Jachym Cepicky (Mendel University of Agriculture)
            Martin Landa <landa.martin gmail.com>

COPYRIGHT:  (C) 2006-2007 by the GRASS Development Team
            This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.

"""

import os
import sys

import globalvar

try:
    import subprocess
except:
    CompatPath = os.path.join(globalvar.ETCWXDIR, "compat")
    sys.path.append(CompatPath)
    import subprocess
    
# import gcmd

def GetGRASSVariable(var):
    """Return GRASS variable or '' if variable is not defined"""
    # gisEnv = gcmd.Command(['g.gisenv'])

    gisEnv = subprocess.Popen(['g.gisenv' + globalvar.EXT_BIN],
                              stdin=None,
                              stdout=subprocess.PIPE,
                              stderr=None)

    for item in gisEnv.stdout.readlines():
        if var in item:
            return item.split('=')[1].replace("'",'').replace(';','').strip()

    return None
