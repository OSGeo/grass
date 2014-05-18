"""!
Default encoding for wxPython GRASS GUI.

(C) 2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Huidae Cho <grass4u gmail.com>
"""

import os
import sys

lang = os.environ["LC_CTYPE"].split(".")
if len(lang) == 2:
    sys.setdefaultencoding(lang[1].lower())
