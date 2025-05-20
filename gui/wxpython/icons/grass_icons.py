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
iconPathObj = Path(iconPath)

iconSet = {}

for icon in iconPathObj.iterdir():
    name = icon.stem
    ext = icon.suffix
    if ext != ".png":
        continue
    iconSet[name] = icon.name
