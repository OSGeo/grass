"""
New GRASS icon set
http://robert.szczepanek.pl/icons.php
https://svn.osgeo.org/osgeo/graphics/toolbar-icons/24x24/
"""

__author__ = "Robert Szczepanek"

import os
from pathlib import Path  # Import Path

from core import globalvar

iconPath = os.path.join(globalvar.ICONDIR, "grass")
iconPathObj = Path(iconPath)  # Create a Path object

iconSet = {}

for icon in iconPathObj.iterdir():  # Use iterdir()
    name = icon.stem  # Use Path.stem to get the name
    ext = icon.suffix  # Use Path.suffix to get the extension
    if ext != ".png":
        continue
    iconSet[name] = icon.name  # Store the full filename (name + ext)
