"""
New GRASS icon set
http://robert.szczepanek.pl/icons.php
https://svn.osgeo.org/osgeo/graphics/toolbar-icons/24x24/
"""
__author__ = "Robert Szczepanek"

import os

from gui_modules import globalvar

iconPath = os.path.join(globalvar.ETCDIR, "gui", "icons", "grass")

iconSet = dict()

for icon in os.listdir(iconPath):
    name, ext = os.path.splitext(icon)
    if ext != '.png':
        continue
    iconSet[name] = icon
