"""!
@package core.menudata

@brief Complex list for menu entries for wxGUI

Classes:
 - MenuData

Usage:
@code
python menudata.py [action] [manager|modeler]
@endcode

where <i>action</i>:
 - strings (default)
 - tree
 - commands
 - dump

(C) 2007-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton (Arizona State University)
@author Yann Chemin <yann.chemin gmail.com>
@author Martin Landa <landa.martin gmail.com>
@author Glynn Clements
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import sys
import pprint
try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

import wx
