"""
New GRASS icon set
http://robert.szczepanek.pl/icons.php
https://svn.osgeo.org/osgeo/graphics/toolbar-icons/24x24/
"""

__author__ = "Robert Szczepanek"

import os
from pathlib import Path
from core import globalvar

iconPath = os.path.join(globalvar.ICONDIR, "grass")

iconSet = {}

for icon in Path(iconPath).iterdir():
    name, ext = os.path.splitext(icon)
    if ext != ".png":
        continue
    iconSet[name] = icon
